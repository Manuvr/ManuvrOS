/*
File:   I2CAdapter.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This class is supposed to be an i2c abstraction layer. The goal is to have
an object of this class that can be instantiated and used to communicate
with i2c devices (as a bus master) regardless of the platform.

This file is the tortured result of growing pains since the beginning of
  ManuvrOS. It has been refactored fifty-eleven times, suffered the brunt
  of all porting efforts, and has reached the point where it must be split
  apart into a more-portable platform-abstraction strategy.
*/


#include "I2CAdapter.h"
#include <Platform/Platform.h>


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

extern "C" {
  // TODO: This is hurting us, and is probably no longer required.
  volatile I2CAdapter* i2c = nullptr;
}

I2CBusOp I2CAdapter::__prealloc_pool[PREALLOCATED_I2C_JOBS];
//template<> PriorityQueue<I2CBusOp*> BusAdapter<I2CBusOp>::preallocated;

  // We need some internal events to allow communication back from the ISR.
const MessageTypeDef i2c_message_defs[] = {
  { MANUVR_MSG_I2C_DEBUG,       0x0000,  "I2C_DEBUG", ManuvrMsg::MSG_ARGS_NONE },  // TODO: This needs to go away.
  { MANUVR_MSG_I2C_QUEUE_READY, 0x0000,  "I2C_Q_RDY", ManuvrMsg::MSG_ARGS_NONE }  // The i2c queue is ready for attention.
};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

I2CAdapter::I2CAdapter(uint8_t dev_id) : I2CAdapter(dev_id, 255, 255) {
  // This should result in the platform-default for the given bus id.
  // Some platforms (linux) will ignore pin-assignment values completely.
}

I2CAdapter::I2CAdapter(uint8_t dev_id, uint8_t sda, uint8_t scl) : EventReceiver(), BusAdapter(12) {
  setReceiverName("I2CAdapter");
  dev     = dev_id;
  sda_pin = sda;
  scl_pin = scl;

  _er_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _er_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);

  for (uint16_t i = 0; i < 128; i++) ping_map[i] = 0;   // Zero the ping map.

  // Set a globalized refernece so we can hit the proper adapter from an ISR.
  i2c = this;   // TODO: Handcuffs. Kill it.

  int mes_count = sizeof(i2c_message_defs) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(i2c_message_defs, mes_count);

  _periodic_i2c_debug.repurpose(MANUVR_MSG_I2C_DEBUG, (EventReceiver*) this);
  _periodic_i2c_debug.incRefs();
  _periodic_i2c_debug.specific_target = (EventReceiver*) this;
  _periodic_i2c_debug.priority(1);
  _periodic_i2c_debug.alterSchedulePeriod(100);
  _periodic_i2c_debug.alterScheduleRecurrence(-1);
  _periodic_i2c_debug.autoClear(false);
  _periodic_i2c_debug.enableSchedule(false);
}


I2CAdapter::~I2CAdapter() {
  busOnline(false);
  while (dev_list.hasNext()) {
    dev_list.get()->disassignBusInstance();
    dev_list.remove();
  }

  /* TODO: The work_queue destructor will take care of its own cleanup, but
     We should abort any open transfers prior to deleting this list. */

  _er_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _er_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);

  _periodic_i2c_debug.enableSchedule(false);
  platform.kernel()->removeSchedule(&_periodic_i2c_debug);
  _periodic_i2c_debug.decRefs();
  bus_deinit();
}



/**
* Return a vacant I2CBusOp to the caller, allocating if necessary.
*
* @param  _op   The desired bus operation.
* @param  _req  The device pointer that is requesting the job.
* @return an I2CBusOp to be used. Only NULL if out-of-mem.
*/
I2CBusOp* I2CAdapter::new_op(BusOpcode _op, BusOpCallback* _req) {
  I2CBusOp* return_value = BusAdapter::new_op();
  return_value->set_opcode(_op);
  return_value->callback = (I2CDevice*) _req;
  return return_value;
}


/**
* This fxn will either free() the memory associated with the I2CBusOp object, or it
*   will return it to the preallocation queue.
*
* @param item The I2CBusOp to be reclaimed.
*/
void I2CAdapter::reclaim_queue_item(I2CBusOp* op) {
  uintptr_t obj_addr = ((uintptr_t) op);
  uintptr_t pre_min  = ((uintptr_t) __prealloc_pool);
  uintptr_t pre_max  = pre_min + (sizeof(I2CBusOp) * PREALLOCATED_I2C_JOBS);

  if (op->hasFault() && (getVerbosity() > 1)) {    // Print failures.
    op->printDebug(&local_log);
  }

  if ((obj_addr < pre_max) && (obj_addr >= pre_min)) {
    // If we are in this block, it means obj was preallocated. wipe and reclaim it.
    BusAdapter::return_op_to_pool(op);
  }
  else {
    // We were created because our prealloc was starved. we are therefore a transient heap object.
    //if (getVerbosity() > 6) local_log.concatf("I2CAdapter::reclaim_queue_item(): \t About to reap.\n");
    _heap_frees++;
    delete op;
  }

  flushLocalLog();
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t I2CAdapter::attached() {
  if (EventReceiver::attached()) {
    bus_init();
    if (busOnline()) {
      advance_work_queue();
    }
    platform.kernel()->addSchedule(&_periodic_i2c_debug);
    return 1;
  }
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t I2CAdapter::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_I2C_QUEUE_READY:
    default:
      break;
  }

  return return_value;
}



int8_t I2CAdapter::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_SYS_REBOOT:
    case MANUVR_MSG_SYS_BOOTLOADER:
      bus_deinit();
      return_value++;
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_I2C_QUEUE_READY:
      advance_work_queue();
      return_value++;
      break;

    #if defined(STM32F7XX) | defined(STM32F746xx)
    // TODO: This is a debugging aid while I sort out i2c on the STM32F7.
    case MANUVR_MSG_I2C_DEBUG:
      {
        //I2CBusOp* nu = new I2CBusOp(BusOpcode::RX, 0x27, (int16_t) 0, &_debug_scratch, 1);
        ////nu->callback = this;
        //queue_io_job(nu);
      }
      break;
    #endif

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}



/**************************************************************************
* Device management functions...                                          *
**************************************************************************/

/*
* Adds a new device to the bus.
*/
int8_t I2CAdapter::addSlaveDevice(I2CDevice* slave) {
	int8_t return_value = I2C_ERR_CODE_NO_ERROR;
	if (slave == NULL) {
	  #ifdef __MANUVR_DEBUG
		Kernel::log("Slave is invalid.\n");
		#endif
		return_value = I2C_ERR_SLAVE_INVALID;
	}
	if (dev_list.contains(slave)) {    // Check for pointer eqivillence.
	  #ifdef __MANUVR_DEBUG
		Kernel::log("Slave device exists.\n");
		#endif
		return_value = I2C_ERR_SLAVE_EXISTS;
	}
	else if (get_slave_dev_by_addr(slave->_dev_addr) == I2C_ERR_SLAVE_NOT_FOUND) {
		if (slave->assignBusInstance(this)) {
			int slave_index = dev_list.insert(slave);
			if (slave_index == -1) {
			  #ifdef __MANUVR_DEBUG
				Kernel::log("Failed to insert somehow. Disassigning...\n");
				#endif
				slave->disassignBusInstance();
				return_value = I2C_ERR_SLAVE_INSERTION;
			}
		}
		else {
		  #ifdef __MANUVR_DEBUG
			Kernel::log("Op would clobber bus instance.\n");
			#endif
			return_value = I2C_ERR_SLAVE_ASSIGN_CLOB;
		}
	}
	else {
	  #ifdef __MANUVR_DEBUG
		Kernel::log("Op would cause address collision with another slave device.\n");
		#endif
		return_value = I2C_ERR_SLAVE_COLLISION;
	}
	return return_value;
}


/*
* Removes a device from the bus.
*/
int8_t I2CAdapter::removeSlaveDevice(I2CDevice* slave) {
	int8_t return_value = I2C_ERR_SLAVE_NOT_FOUND;
	if (dev_list.remove(slave)) {
		slave->disassignBusInstance();
		purge_queued_work_by_dev(slave);
		return_value = I2C_ERR_CODE_NO_ERROR;
	}
	return return_value;
}


/*
* Searches this busses list of bound devices for the given address.
* Returns the posistion in the list, only because it is non-negative. This
*   is only called to prevent address collision. Not fetch a device handle.
*/
int I2CAdapter::get_slave_dev_by_addr(uint8_t search_addr) {
	for (int i = 0; i < dev_list.size(); i++) {
		if (search_addr == dev_list.get(i)->_dev_addr) {
			return i;
		}
	}
	return I2C_ERR_SLAVE_NOT_FOUND;
}


/**************************************************************************
* Workflow management functions...                                        *
**************************************************************************/
/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return SPI_CALLBACK_NOMINAL on success, or appropriate error code.
*/
int8_t I2CAdapter::io_op_callback(BusOp* _op) {
  I2CBusOp* op = (I2CBusOp*) _op;
  // There is zero chance this object will be a null pointer unless it was done on purpose.
  if (getVerbosity() > 2) {
    local_log.concatf("Probably shouldn't be in the default callback case...\n");
    op->printDebug(&local_log);
  }

  flushLocalLog();
  return 0;
}


/*
* This is the function that should be called to queue-up a bus operation.
* It may or may not be started immediately.
*/
int8_t I2CAdapter::queue_io_job(BusOp* op) {
  I2CBusOp* nu = (I2CBusOp*) op;
  nu->setVerbosity(getVerbosity());
  nu->device = this;
	if (current_queue_item) {
		// Something is already going on with the bus. Queue...
		work_queue.insert(nu);
	}
	else {
		// Bus is idle. Put this work item in the active slot and start the bus operations...
		current_queue_item = nu;
		if ((dev >= 0) && busOnline()) {
		  nu->begin();
		  if (getVerbosity() > 6) {
		    nu->printDebug(&local_log);
		    Kernel::log(&local_log);
		  }
		}
		else {
		  Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, nullptr);   // Raise an event
		}
	}
	return 0;
}


/*
* This function needs to be called to move the queue forward.
*/
int8_t I2CAdapter::advance_work_queue() {
	if (current_queue_item) {
		if (current_queue_item->isComplete()) {
			if (current_queue_item->hasFault()) {
			  #ifdef __MANUVR_DEBUG
			  if (getVerbosity() > 3) {
          local_log.concatf("Destroying failed job.\n");
          if (getVerbosity() > 4) current_queue_item->printDebug(&local_log);
        }
			  #endif
			}

			// Hand this completed operation off to the class that requested it. That class will
			//   take what it wants from the buffer and, when we return to execution here, we will
			//   be at liberty to clean the operation up.
			if (current_queue_item->callback) {
				// TODO: need some minor reorg to make this not so obtuse...
				current_queue_item->callback->io_op_callback(current_queue_item);
			}
			else if (current_queue_item->get_opcode() == BusOpcode::TX_CMD) {
				if (!current_queue_item->hasFault()) {
					ping_map[current_queue_item->dev_addr % 128] = 1;
				}
				else {
					ping_map[current_queue_item->dev_addr % 128] = -1;
				}

				if (_er_flag(I2C_BUS_FLAG_PINGING)) {
				  if ((current_queue_item->dev_addr & 0x00FF) < 127) {
				    ping_slave_addr(current_queue_item->dev_addr + 1);
				  }
				  else {
				    #ifdef __MANUVR_DEBUG
				    if (getVerbosity() > 3) local_log.concat("Concluded i2c ping sweep.");
				    #endif
				    _er_clear_flag(I2C_BUS_FLAG_PINGING);
				  }
				}
			}

			delete current_queue_item;
			current_queue_item = work_queue.dequeue();
		}
	}
	else {
		// If there is nothing presently being serviced, we should promote an operation from the
		//   queue into the active slot and initiate it in the block below.
		current_queue_item = work_queue.dequeue();
	}

	if (current_queue_item) {
		if (!current_queue_item->has_bus_control()) {
			current_queue_item->begin();
		}
	}

	flushLocalLog();
  return 0;
}


/*
* Pass an i2c device, and this fxn will purge all of its queued work. Presumably, this is
*   because it is being detached from the bux, but it may also be because one of it's operations
*   went bad.
*/
void I2CAdapter::purge_queued_work_by_dev(I2CDevice *dev) {
  I2CBusOp* current = NULL;

  if (work_queue.size() > 0) {
    for (int i = 0; i < work_queue.size(); i++) {
      current = work_queue.get(i);
      if (current->dev_addr == dev->_dev_addr) {
        work_queue.remove(current);
        delete current;   // Delete the queued work AND its buffer.
      }
    }
  }

  // Check this last to head off any silliness with bus operations colliding with us.
  purge_stalled_job();

  // Lastly... initiate the next bus transfer if the bus is not sideways.
  advance_work_queue();
}


/*
* Purges only the work_queue. Leaves the currently-executing job.
*/
void I2CAdapter::purge_queued_work() {
  I2CBusOp* current = work_queue.dequeue();
  while (current) {
    current = work_queue.get();
    current_queue_item->abort();
    if (current->callback) {
      current->callback->io_op_callback(current);
    }
    delete current;   // Delete the queued work AND its buffer.
    current = work_queue.dequeue();
  }
}


void I2CAdapter::purge_stalled_job() {
  if (current_queue_item != NULL) {
    current_queue_item->abort();
    if (NULL != current_queue_item->callback) current_queue_item->callback->io_op_callback(current_queue_item);
    delete current_queue_item;
    current_queue_item = NULL;
#ifdef STM32F4XX
    I2C_GenerateSTOP(I2C1, ENABLE);   // This may not be sufficient...
#endif
  }
}


/*
* Creates a specially-crafted WorkQueue object that will use the STM32F4 i2c peripheral
*   to discover if a device is active on the bus and addressable.
*/
void I2CAdapter::ping_slave_addr(uint8_t addr) {
    I2CBusOp* nu = new I2CBusOp(BusOpcode::TX_CMD, addr, (int16_t) -1, NULL, 0);
    queue_io_job(nu);
    _er_set_flag(I2C_BUS_FLAG_PING_RUN);
}


/*
* Debug fxn to print the ping map.
*/
void I2CAdapter::printPingMap(StringBuilder *temp) {
  if (temp != NULL) {
    temp->concat("\n\n\tPing Map\n\t      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
    // TODO: This is needlessly-extravagent of memory. Do it this way instead...
    //char str_buf[];
    for (uint8_t i = 0; i < 128; i+=16) {
      temp->concatf("\t0x%02x: %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c\n",
        i,
        ((ping_map[i + 0x00] == 0) ? ' ' : ((ping_map[i + 0x00] < 0) ? '-' : '*')),
        ((ping_map[i + 0x01] == 0) ? ' ' : ((ping_map[i + 0x01] < 0) ? '-' : '*')),
        ((ping_map[i + 0x02] == 0) ? ' ' : ((ping_map[i + 0x02] < 0) ? '-' : '*')),
        ((ping_map[i + 0x03] == 0) ? ' ' : ((ping_map[i + 0x03] < 0) ? '-' : '*')),
        ((ping_map[i + 0x04] == 0) ? ' ' : ((ping_map[i + 0x04] < 0) ? '-' : '*')),
        ((ping_map[i + 0x05] == 0) ? ' ' : ((ping_map[i + 0x05] < 0) ? '-' : '*')),
        ((ping_map[i + 0x06] == 0) ? ' ' : ((ping_map[i + 0x06] < 0) ? '-' : '*')),
        ((ping_map[i + 0x07] == 0) ? ' ' : ((ping_map[i + 0x07] < 0) ? '-' : '*')),
        ((ping_map[i + 0x08] == 0) ? ' ' : ((ping_map[i + 0x08] < 0) ? '-' : '*')),
        ((ping_map[i + 0x09] == 0) ? ' ' : ((ping_map[i + 0x09] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0A] == 0) ? ' ' : ((ping_map[i + 0x0A] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0B] == 0) ? ' ' : ((ping_map[i + 0x0B] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0C] == 0) ? ' ' : ((ping_map[i + 0x0C] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0D] == 0) ? ' ' : ((ping_map[i + 0x0D] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0E] == 0) ? ' ' : ((ping_map[i + 0x0E] < 0) ? '-' : '*')),
        ((ping_map[i + 0x0F] == 0) ? ' ' : ((ping_map[i + 0x0F] < 0) ? '-' : '*'))
      );
      //temp->concat(str_buf);
    }
  }
  temp->concat("\n");
}


void I2CAdapter::printDevs(StringBuilder *temp, uint8_t dev_num) {
  if (temp == NULL) return;

  if (dev_list.get(dev_num) != NULL) {
    temp->concat("\n\n");
    dev_list.get(dev_num)->printDebug(temp);
  }
  else {
    temp->concat("\n\nNot that many devices.\n");
  }
}

void I2CAdapter::printDevs(StringBuilder *temp) {
  if (temp == NULL) return;

  EventReceiver::printDebug(temp);
  for (int i = 0; i < dev_list.size(); i++) {
    dev_list.get(i)->printDebug(temp);
  }
  temp->concat("\n");
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CAdapter::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  BusAdapter::printAdapter((BusAdapter*)this, output);
  printHardwareState(output);
  output->concatf("-- sda/scl     %u/%u\n", sda_pin, scl_pin);
  output->concatf("-- bus_error   %s\n", (busError()  ? "yes" : "no"));

  if (current_queue_item) {
    output->concat("Currently being serviced:\n");
    current_queue_item->printDebug(output);
  }
  else {
    output->concat("Nothing being serviced.\n\n");
  }
  BusAdapter::printWorkQueue((BusAdapter*)this, output, I2CADAPTER_MAX_QUEUE_PRINT);
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void I2CAdapter::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  char c = *(str);
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(1);
  }
  else if (strlen(str) > 1) {
    // We allow a short-hand for the sake of short commands that involve a single int.
    temp_int = atoi(str + 1);
  }

  switch (c) {
    // i2c debugging cases....
    case 'i':
      switch (temp_int) {
        case 0:
          printDebug(&local_log);
          break;
        case 1:
          printHardwareState(&local_log);
          break;
        case 2:
          printDevs(&local_log);
          break;
        case 3:
          printPingMap(&local_log);
          break;
      }
      break;

    case 'd':
      dev_list.get(temp_int)->printDebug(&local_log);
      break;

    case '1':
      bus_init();
      local_log.concat("i2c GPIO reset.\n");
      break;

    #if defined(STM32F7XX) | defined(STM32F746xx)
    case '3':
      {
        //I2CBusOp* nu = new I2CBusOp(BusOpcode::RX, 0x27, (int16_t) 0, &_debug_scratch, 1);
        ////nu->callback = this;
        //queue_io_job(nu);
      }
      break;

    case 'x':
      _periodic_i2c_debug.fireNow();
      break;

    case 'Z':
    case 'z':
      if (temp_int) {
        _periodic_i2c_debug.alterSchedulePeriod(temp_int * 10);
      }
      _periodic_i2c_debug.enableSchedule(*(str) == 'Z');
      local_log.concatf("%s periodic reader.\n", (*(str) == 'Z' ? "Starting" : "Stopping"));
      break;
    #endif   // defined(STM32F7XX) | defined(STM32F746xx)

    case 'r':
      #ifdef STM32F4XX
        I2C_SoftwareResetCmd(I2C1, ENABLE);
        local_log.concat("i2c software reset.\n");
        I2C_SoftwareResetCmd(I2C1, DISABLE);
      #elif defined(STM32F7XX) | defined(STM32F746xx)
        I2C1->CR1 &= ~((uint32_t) I2C_CR1_PE);
        local_log.concat("i2c software reset.\n");
        while(I2C1->CR1 & I2C_CR1_PE) {}
        I2C1->CR1 |= I2C_CR1_PE;
      #else
        local_log.concat("i2c software reset unsupported.\n");
      #endif
      break;

    case 'p':
      purge_queued_work();
      local_log.concat("i2c queue purged.\n");
      break;
    case 'P':
      purge_stalled_job();
      local_log.concatf("Attempting to purge a stalled jorbe...\n");
      break;
    #ifdef STM32F4XX
      case 'b':
        I2C1_EV_IRQHandler();
        break;
    #endif

    case 'K':
      if (temp_int) {
        local_log.concatf("ping i2c slave 0x%02x.\n", temp_int);
        ping_slave_addr(temp_int);
      }
      else if (!_er_flag(I2C_BUS_FLAG_PINGING)) {
        _er_set_flag(I2C_BUS_FLAG_PINGING);
        ping_slave_addr(1);
      }
      break;
    case ']':
      local_log.concatf("Advanced i2c work queue.\n");
      Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, NULL);   // Raise an event
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT
