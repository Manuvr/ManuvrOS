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


#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)

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

const char I2CAdapter::_ping_state_chr[4] = {' ', '.', '*', ' '};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

I2CAdapter::I2CAdapter(const I2CAdapterOptions* o) : EventReceiver("I2CAdapter"), BusAdapter(I2CADAPTER_MAX_QUEUE_DEPTH), _bus_opts(o) {
  // Some platforms (linux) will ignore pin-assignment values completely.

  _er_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _er_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);

  // Mark all of our preallocated SPI jobs as "No Reap" and pass them into the prealloc queue.
  for (uint8_t i = 0; i < I2CADAPTER_PREALLOC_COUNT; i++) {
    preallocated.insert(&__prealloc_pool[i]);
  }

  for (uint16_t i = 0; i < 32; i++) ping_map[i] = 0;   // Zero the ping map.
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
  uintptr_t pre_max  = pre_min + (sizeof(I2CBusOp) * I2CADAPTER_PREALLOC_COUNT);

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
* Slave device management functions...                                         *
*******************************************************************************/

/*
* Adds a new device to the bus.
*/
int8_t I2CAdapter::addSlaveDevice(I2CDevice* slave) {
	int8_t return_value = I2C_ERR_SLAVE_NO_ERROR;
	if (slave == nullptr) {
	  #if defined(MANUVR_DEBUG)
		Kernel::log("Slave is invalid.\n");
		#endif
		return_value = I2C_ERR_SLAVE_INVALID;
	}
	if (dev_list.contains(slave)) {    // Check for pointer eqivillence.
	  #if defined(MANUVR_DEBUG)
		Kernel::log("Slave device exists.\n");
		#endif
		return_value = I2C_ERR_SLAVE_EXISTS;
	}
	else if (get_slave_dev_by_addr(slave->_dev_addr) == I2C_ERR_SLAVE_NOT_FOUND) {
		if (slave->assignBusInstance((I2CAdapter*) this)) {
			int slave_index = dev_list.insert(slave);
			if (slave_index == -1) {
			  #if defined(MANUVR_DEBUG)
				Kernel::log("Failed to insert somehow. Disassigning...\n");
				#endif
				slave->disassignBusInstance();
				return_value = I2C_ERR_SLAVE_INSERTION;
			}
		}
		else {
		  #if defined(MANUVR_DEBUG)
			Kernel::log("Op would clobber bus instance.\n");
			#endif
			return_value = I2C_ERR_SLAVE_ASSIGN_CLOB;
		}
	}
	else {
	  #if defined(MANUVR_DEBUG)
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
		return_value = I2C_ERR_SLAVE_NO_ERROR;
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


/*
* Creates a specially-crafted WorkQueue object that will use the STM32F4 i2c peripheral
*   to discover if a device is active on the bus and addressable.
*/
void I2CAdapter::ping_slave_addr(uint8_t addr) {
  I2CBusOp* nu = new_op(BusOpcode::TX_CMD, this);
  nu->dev_addr = addr;
  nu->sub_addr = -1;
  nu->buf      = nullptr;
  nu->buf_len  = 0;

  queue_io_job(nu);
  _er_set_flag(I2C_BUS_FLAG_PING_RUN);
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t I2CAdapter::io_op_callahead(BusOp* _op) {
  // Bus adapters don't typically do anything here, other
  //   than permit the transfer.
  return 0;
}


/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return 0 on success, or appropriate error code.
*/
int8_t I2CAdapter::io_op_callback(BusOp* _op) {
  I2CBusOp* op = (I2CBusOp*) _op;
	if (op->get_opcode() == BusOpcode::TX_CMD) {
    // The only thing the i2c adapter uses this op-code for is pinging slaves.
    // We only support 7-bit addressing for now.
    set_ping_state_by_addr(op->dev_addr, op->hasFault() ? I2CPingState::NEG : I2CPingState::POS);

		if (_er_flag(I2C_BUS_FLAG_PINGING)) {
		  if ((op->dev_addr & 0x00FF) < 127) {  // TODO: yy???
		    ping_slave_addr(op->dev_addr + 1);
		  }
		  else {
		    _er_clear_flag(I2C_BUS_FLAG_PINGING);
		    #if defined(MANUVR_DEBUG)
		    if (getVerbosity() > 3) local_log.concat("Concluded i2c ping sweep.");
		    #endif
		  }
		}
	}

  flushLocalLog();
  return 0;
}


/**
* This is what is called when the class wants to conduct a transaction on the bus.
*
* @param  _op  The bus operation to execute.
* @return Zero on success, or appropriate error code.
*/
int8_t I2CAdapter::queue_io_job(BusOp* op) {
  I2CBusOp* nu = (I2CBusOp*) op;
  nu->setVerbosity(getVerbosity());
  nu->device = (I2CAdapter*)this;
	if (current_job) {
		// Something is already going on with the bus. Queue...
		work_queue.insert(nu);
	}
	else {
		// Bus is idle. Put this work item in the active slot and start the bus operations...
		current_job = nu;
		if ((getAdapterId() >= 0) && busOnline()) {
		  if (XferFault::NONE == nu->begin()) {
        #if defined(__BUILD_HAS_THREADS)
        if (_thread_id) wakeThread(_thread_id);
        #endif
      }
		  if (getVerbosity() > 6) {
		    nu->printDebug(&local_log);
		    Kernel::log(&local_log);
		  }
		}
		else {
		  Kernel::staticRaiseEvent(&_queue_ready);   // Raise an event
		}
	}
	return 0;
}



/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t I2CAdapter::advance_work_queue() {
  int8_t return_value = 0;
  bool recycle = busOnline();
  while (recycle) {
    return_value++;
    recycle = false;
  	if (current_job) {
      switch (current_job->get_state()) {
        // NOTE: Tread lightly. Omission of break; scattered throughout.
        /* These are start states. */

        /* These states are unstable and should decay into a "finish" state. */
        case XferState::IDLE:      // Bus op is allocated and waiting somewhere outside of the queue.
        case XferState::QUEUED:    // Bus op is idle and waiting for its turn. No bus control.
          if (!current_job->has_bus_control()) {
            current_job->begin();
          }
          break;
        case XferState::INITIATE:  // Waiting for initiation phase.
          current_job->advance();
          break;
        case XferState::ADDR:      // Addressing phase. Sending the address.
        case XferState::TX_WAIT:   // I/O operation in-progress.
        case XferState::RX_WAIT:   // I/O operation in-progress.
        case XferState::STOP:      // I/O operation in cleanup phase.
          //current_job->advance();
          break;

        case XferState::UNDEF:     // Freshly instanced (or wiped, if preallocated).
        default:
          current_job->abort(XferFault::ILLEGAL_STATE);
        /* These are finish states. */
        case XferState::FAULT:     // Fault condition.
        case XferState::COMPLETE:  // I/O op complete with no problems.
          _total_xfers++;
    			if (current_job->hasFault()) {
            _failed_xfers++;
    			  #if defined(MANUVR_DEBUG)
    			  if (getVerbosity() > 3) {
              local_log.concat("Destroying failed job.\n");
              current_job->printDebug(&local_log);
            }
    			  #endif
    			}
    			// Hand this completed operation off to the class that requested it. That class will
    			//   take what it wants from the buffer and, when we return to execution here, we will
    			//   be at liberty to clean the operation up.
          // Note that we forgo a separate callback queue, and are therefore unable to
          //   pipeline I/O and processing. They must happen sequentially.
          current_job->execCB();
    			reclaim_queue_item(current_job);   // Delete the queued work AND its buffer.
    			current_job = nullptr;
          break;
      }
  	}

    if (nullptr == current_job) {
  		// If there is nothing presently being serviced, we should promote an operation from the
  		//   queue into the active slot and initiate it in the block below.
  		current_job = work_queue.dequeue();
      if (current_job) {
        recycle = busOnline();
      }
  	}
  }

	flushLocalLog();
  return return_value;
}


/**
* Pass an i2c device, and this fxn will purge all of its queued work. Presumably, this is
*   because it is being detached from the bux, but it may also be because one of it's operations
*   went bad.
*
* @param  dev  The device pointer that owns jobs we wish purged.
*/
void I2CAdapter::purge_queued_work_by_dev(I2CDevice *dev) {
  I2CBusOp* current = nullptr;

  if (work_queue.size() > 0) {
    for (int i = 0; i < work_queue.size(); i++) {
      current = work_queue.get(i);
      if (current->dev_addr == dev->_dev_addr) {
        work_queue.remove(current);
        reclaim_queue_item(current);   // Delete the queued work AND its buffer.
      }
    }
  }

  // Check this last to head off any silliness with bus operations colliding with us.
  purge_stalled_job();

  // Lastly... initiate the next bus transfer if the bus is not sideways.
  advance_work_queue();
}


/**
* Purges only the work_queue. Leaves the currently-executing job.
*/
void I2CAdapter::purge_queued_work() {
  I2CBusOp* current = work_queue.dequeue();
  while (current) {
    current = work_queue.get();
    current->abort(XferFault::QUEUE_FLUSH);
    if (current->callback) {
      current->callback->io_op_callback(current);
    }
    reclaim_queue_item(current);   // Delete the queued work AND its buffer.
    current = work_queue.dequeue();
  }
}


void I2CAdapter::purge_stalled_job() {
  if (current_job) {
    current_job->abort(XferFault::QUEUE_FLUSH);
    if (current_job->callback) {
      current_job->callback->io_op_callback(current_job);
    }
    delete current_job;
    current_job = nullptr;
  }
}


I2CPingState I2CAdapter::get_ping_state_by_addr(uint8_t addr) {
  return (I2CPingState) ((ping_map[(addr & 0x7F) >> 2] >> ((addr & 0x03) << 1)) & 0x03);
}

void I2CAdapter::set_ping_state_by_addr(uint8_t addr, I2CPingState nu) {
  uint8_t m = (0x03 << ((addr & 0x03) << 1));
  uint8_t a = ping_map[(addr & 0x7F) >> 2] & ~m;
  ping_map[(addr & 0x7F) >> 2] = a | (((uint8_t) nu) << ((addr & 0x03) << 1));
}


/*
* Debug fxn to print the ping map.
*/
void I2CAdapter::printPingMap(StringBuilder *temp) {
  if (temp) {
    temp->concat("\n\n\tPing Map\n\t      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
    // TODO: This is needlessly-extravagent of memory. Do it this way instead...
    //char str_buf[];
    for (uint8_t i = 0; i < 128; i+=16) {
      temp->concatf("\t0x%02x: %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c\n",
        i,
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x00)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x01)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x02)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x03)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x04)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x05)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x06)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x07)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x08)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x09)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0A)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0B)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0C)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0D)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0E)],
        _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0F)]
      );
      //temp->concat(str_buf);
    }
  }
  temp->concat("\n");
}


void I2CAdapter::printDevs(StringBuilder *temp, uint8_t dev_num) {
  if (temp == nullptr) return;

  if (dev_list.get(dev_num) != nullptr) {
    temp->concat("\n\n");
    dev_list.get(dev_num)->printDebug(temp);
  }
  else {
    temp->concat("\n\nNot that many devices.\n");
  }
}


void I2CAdapter::printDevs(StringBuilder *temp) {
  if (temp == nullptr) return;

  EventReceiver::printDebug(temp);
  for (int i = 0; i < dev_list.size(); i++) {
    dev_list.get(i)->printDebug(temp);
  }
  temp->concat("\n");
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
    _queue_ready.repurpose(MANUVR_MSG_I2C_QUEUE_READY, (EventReceiver*) this);
    _queue_ready.incRefs();
    _queue_ready.specific_target = (EventReceiver*) this;
    _queue_ready.priority(1);
    bus_init();
    if (busOnline()) {
      advance_work_queue();
    }
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

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CAdapter::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
  printHardwareState(output);
  output->concatf("-- sda/scl             %u/%u\n", _bus_opts.sda_pin, _bus_opts.scl_pin);
  output->concatf("-- bus_error           %s\n", (busError()  ? "yes" : "no"));
  printAdapter(output);
  printWorkQueue(output, I2CADAPTER_MAX_QUEUE_PRINT);
}


#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i1", "Print hardware state" },
  { "i2", "Print ping map" },
  { "K", "Ping device(s)" },
  { "p", "Purge current job" },
  { "P", "Purge work-queue" },
  { "]", "Advance work-queue" },
  { "d", "Print debug for given device index." }
};


uint I2CAdapter::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void I2CAdapter::consoleCmdProc(StringBuilder* input) {
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

    case 'r':
      #ifdef STM32F4XX
        I2C_SoftwareResetCmd(I2C1, ENABLE);
        local_log.concat("i2c software reset.\n");
        I2C_SoftwareResetCmd(I2C1, DISABLE);
      #elif defined(STM32F7XX) || defined(STM32F746xx)
        I2C1->CR1 &= ~((uint32_t) I2C_CR1_PE);
        local_log.concat("i2c software reset.\n");
        while(I2C1->CR1 & I2C_CR1_PE) {}
        I2C1->CR1 |= I2C_CR1_PE;
      #else
        local_log.concat("i2c software reset unsupported.\n");
      #endif
      break;

    case 'P':
      purge_queued_work();
      local_log.concat("i2c queue purged.\n");
      break;
    case 'p':
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
      Kernel::staticRaiseEvent(&_queue_ready);   // Raise an event
      break;

    default:
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT
#endif  // MANUVR_SUPPORT_I2C
