/*
File:   i2c-adapter.cpp
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


#include "i2c-adapter.h"

#if defined(__MK20DX256__) | defined(__MK20DX128__)
  #include <i2c_t3/i2c_t3.h>
#elif defined(STM32F4XX)
  #include <stm32f4xx.h>
  #include <stm32f4xx_i2c.h>
  #include <stm32f4xx_gpio.h>
  #include "stm32f4xx_it.h"

#elif defined(ARDUINO)
  #include <Wire/Wire.h>
#elif defined(__MANUVR_LINUX)
  // Unsupported platform? Try using the linux i2c library and cross fingers...
  #include <stdlib.h>
  #include <unistd.h>
  #include <linux/i2c-dev.h>
  #include <sys/types.h>
  #include <sys/ioctl.h>
  #include <sys/stat.h>
  #include <fstream>
  #include <iostream>
  #include <fcntl.h>
  #include <inttypes.h>
  #include <ctype.h>
#elif defined(STM32F7XX) | defined(STM32F746xx)
  // TODO: This is bad, and I know it. Need support ahead of a better abstraction strategy.
  extern "C" {
    #include <stm32f7xx_hal.h>
    #include <stm32f7xx_hal_gpio.h>
    #include <stm32f7xx_hal_i2c.h>
    I2C_HandleTypeDef hi2c1;
  }

#else
  // Unsupported. Much ugly case-off should not be in this file.
  // TODO: Migrate case-off code to platform directory.
#endif


extern "C" {
  volatile I2CAdapter* i2c = NULL;
}

  // We need some internal events to allow communication back from the ISR.
const MessageTypeDef i2c_message_defs[] = {
  { MANUVR_MSG_I2C_QUEUE_READY, MSG_FLAG_IDEMPOTENT,  "I2C_QUEUE_READY", ManuvrMsg::MSG_ARGS_NONE }  // The i2c queue is ready for attention.
};


/**************************************************************************
* Constructors / Destructors                                              *
**************************************************************************/

void I2CAdapter::__class_initializer() {
  setReceiverName("I2CAdapter");

  current_queue_item = NULL;
  last_used_bus_addr = 0;

  _er_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _er_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);

  for (uint16_t i = 0; i < 128; i++) ping_map[i] = 0;   // Zero the ping map.

  // Set a globalized refernece so we can hit the proper adapter from an ISR.
  i2c = this;

  int mes_count = sizeof(i2c_message_defs) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(i2c_message_defs, mes_count);

  _periodic_i2c_debug.repurpose(0x5051, (EventReceiver*) this);
  _periodic_i2c_debug.isManaged(true);
  _periodic_i2c_debug.specific_target = (EventReceiver*) this;
  _periodic_i2c_debug.priority        = 1;
  _periodic_i2c_debug.alterSchedulePeriod(100);
  _periodic_i2c_debug.alterScheduleRecurrence(-1);
  _periodic_i2c_debug.autoClear(false);
  _periodic_i2c_debug.enableSchedule(false);
}


#ifdef STM32F4XX
I2CAdapter::I2CAdapter(uint8_t dev_id) : EventReceiver() {
  __class_initializer();
  dev = dev_id;

  // This init() fxn was patterned after the STM32F4x7 library example.
  if (dev_id == 1) {
    //I2C_DeInit(I2C1);		//Deinit and reset the I2C to avoid it locking up

    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);    // enable APB1 peripheral clock for I2C1

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
    I2C_DeInit(I2C1);

    /* Reset I2Cx IP */

    /* Release reset signal of I2Cx IP */
    //RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

    /* setup SCL and SDA pins
     * You can connect the I2C1 functions to two different
     * pins:
     * 1. SCL on PB6
     * 2. SDA on PB7
     */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // we are going to use PB6 and PB7
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;           // set pins to alternate function
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      // set GPIO speed
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;         // set output to open drain --> the line has to be only pulled low, not driven high
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;       // enable pull up resistors
    GPIO_Init(GPIOB, &GPIO_InitStruct);                 // init GPIOB

    // Connect I2C1 pins to AF
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);    // SCL
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);    // SDA

    I2C_InitTypeDef I2C_InitStruct;

    // configure I2C1
    I2C_InitStruct.I2C_ClockSpeed = 400000;          // 400kHz
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;          // I2C mode
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;  // 50% duty cycle --> standard
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;           // own address, not relevant in master mode
    //I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;         // disable acknowledge when reading (can be changed later on)
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
    I2C_Init(I2C1, &I2C_InitStruct);                 // init I2C1
    I2C_Cmd(I2C1, ENABLE);       // enable I2C1
  }
}


I2CAdapter::~I2CAdapter() {
    I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, DISABLE);   // Shelve the interrupts.
    I2C_DeInit(I2C1);   // De-init
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }

    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}


int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! busOnline()) return -1;
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, ENABLE);      //Enable EVT and ERR interrupts
  I2C_GenerateSTART(I2C1, ENABLE);   // Doing this will take us back to the ISR.
  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! busOnline()) return -1;
  DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, DISABLE);
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, DISABLE);
  I2C_GenerateSTOP(I2C1, ENABLE);
  return 0;
}


#elif defined(STM32F7XX) | defined(STM32F746xx)
  // TODO: I know this is horrid. I'm sick of screwing with the build system today...
  #include <Platform/STM32F7/i2c-adapter.cpp>

#elif defined(__MK20DX256__) | defined(__MK20DX128__)
  // TODO: I know this is horrid. I'm sick of screwing with the build system today...
  #include <Platform/Teensy3/i2c-adapter.cpp>

#elif defined(_BOARD_FUBARINO_MINI_)    // Perhaps this is an arduino-style env?


I2CAdapter::I2CAdapter(uint8_t dev_id) : EventReceiver() {
  __class_initializer();
  dev = dev_id;

  if (dev_id == 0) {
    Wire.begin();
    busOnline(true);
  }
  else {
    // Unsupported.
  }
}


I2CAdapter::~I2CAdapter() {
    busOnline(false);
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }

    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}


int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! busOnline()) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.

  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! busOnline()) return -1;
  return 0;
}




#elif defined(ARDUINO)    // Perhaps this is an arduino-style env?

I2CAdapter::I2CAdapter(uint8_t dev_id) : EventReceiver() {
  __class_initializer();
  dev = dev_id;

  if (dev_id == 1) {
    //Wire.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400);
    busOnline(true);
  }
}


I2CAdapter::~I2CAdapter() {
    busOnline(false);
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }

    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}


// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! busOnline()) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.

  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! busOnline()) return -1;
  return 0;
}


#elif defined(__MANUVR_LINUX)  // Assuming a linux system...
  // TODO: I know this is horrid. I'm sick of screwing with the build system today...
  #include <Platform/Linux/i2c-adapter.cpp>

#else
  // No support.
#endif  // Platform case-offs



/**
* Setup GPIO pins and their bindings to on-chip peripherals, if required.
*/
void I2CAdapter::gpioSetup() {
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀
*
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/**
* There is a NULL-check performed upstream for the scheduler member. So no need
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t I2CAdapter::bootComplete() {
  EventReceiver::bootComplete();

  if (dev >= 0) busOnline(true);
  if (busOnline()) {
    advance_work_queue();
  }
  __kernel->addSchedule(&_periodic_i2c_debug);
  return 1;
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
int8_t I2CAdapter::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}



int8_t I2CAdapter::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_INTERRUPTS_MASKED:
    case MANUVR_MSG_SYS_REBOOT:
    case MANUVR_MSG_SYS_BOOTLOADER:
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_I2C_QUEUE_READY:
      advance_work_queue();
      return_value++;
      break;

    #if defined(STM32F7XX) | defined(STM32F746xx)
    // TODO: This is a debugging aid while I sort out i2c on the STM32F7.
    case 0x5051:
      {
        I2CBusOp* nu = new I2CBusOp(BusOpcode::RX, 0x27, (int16_t) 0, &_debug_scratch, 1);
        //nu->requester = this;
        insert_work_item(nu);
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

/*
* Private function that will switch the addressed i2c device via ioctl. This
*   function is meaningless on anything but a linux system, in which case it
*   will always return true;
* On a linux system, this will only return true if the ioctl call succeeded.
*/
bool I2CAdapter::switch_device(uint8_t nu_addr) {
#ifdef ARDUINO
  bool return_value = true;
#elif defined(STM32F4XX)
  bool return_value = true;

#elif defined(__MANUVR_LINUX)   // Assuming a linux environment.
  bool return_value = false;
  unsigned short timeout = 10000;
  if (nu_addr != last_used_bus_addr) {
    if (dev < 0) {
      // If the bus is either uninitiallized or not idle, decline
      // to switch the device. Return false;
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 1) {
        Kernel::log("i2c bus is not online, so won't switch device. Failing....\n");
      }
      #endif
      return return_value;
    }
    else {
      while (busError() && (timeout > 0)) { timeout--; }
      if (busError()) {
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) {
          Kernel::log("i2c bus was held for too long. Failing....\n");
        }
        #endif
        return return_value;
      }

      if (ioctl(dev, I2C_SLAVE, nu_addr) >= 0) {
        last_used_bus_addr = nu_addr;
        return_value = true;
      }
      else {
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) {
          local_log.concatf("Failed to acquire bus access and/or talk to slave at %d.\n", nu_addr);
          Kernel::log(&local_log);
        }
        #endif
        busError(true);
      }
    }
  }
  else {
      return_value = true;
  }
#else
  // No support
  bool return_value = false;
#endif
  return return_value;
}



/*
* This is the function that should be called to queue-up a bus operation.
* It may or may not be started immediately.
*/
bool I2CAdapter::insert_work_item(I2CBusOp *nu) {
  nu->verbosity = getVerbosity();
  nu->device = this;
	if (current_queue_item != NULL) {
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
		  Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, NULL);   // Raise an event
		}
	}
	return true;
}


/*
* This function needs to be called to move the queue forward.
*/
void I2CAdapter::advance_work_queue(void) {
	if (current_queue_item != NULL) {
		if (current_queue_item->isComplete()) {
			if (!current_queue_item->hasFault()) {
			  //temp.concatf("Destroying successful job 0x%08x.\n", current_queue_item->txn_id);
			}
			else {
			  #ifdef __MANUVR_DEBUG
			  if (getVerbosity() > 3) local_log.concatf("Destroying failed job 0x%08x.\n", current_queue_item->txn_id);
			  #endif
			  if (getVerbosity() > 6) current_queue_item->printDebug(&local_log);
			}

			// Hand this completed operation off to the class that requested it. That class will
			//   take what it wants from the buffer and, when we return to execution here, we will
			//   be at liberty to clean the operation up.
			if (current_queue_item->requester != NULL) {
				// TODO: need some minor reorg to make this not so obtuse...
				current_queue_item->requester->operationCompleteCallback(current_queue_item);
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
			current_queue_item = work_queue.get();
			if (current_queue_item != NULL) work_queue.remove();
		}
	}
	else {
		// If there is nothing presently being serviced, we should promote an operation from the
		//   queue into the active slot and initiate it in the block below.
		current_queue_item = work_queue.get();
		if (current_queue_item != NULL) work_queue.remove();
	}

	if (current_queue_item != NULL) {
		if (!current_queue_item->has_bus_control()) {
			current_queue_item->begin();
		}
	}

	flushLocalLog();
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
  I2CBusOp* current = NULL;
  while (work_queue.hasNext()) {
    current = work_queue.get();
    current_queue_item->abort();
    if (NULL != current->requester) current->requester->operationCompleteCallback(current);
    work_queue.remove();
    delete current;   // Delete the queued work AND its buffer.
  }
}


void I2CAdapter::purge_stalled_job() {
  if (current_queue_item != NULL) {
    current_queue_item->abort();
    if (NULL != current_queue_item->requester) current_queue_item->requester->operationCompleteCallback(current_queue_item);
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
    insert_work_item(nu);
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
void I2CAdapter::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;

  EventReceiver::printDebug(temp);
  temp->concatf("-- bus_online              %s\n", (busOnline() ? "yes" : "no"));
  #if defined(STM32F7XX) | defined(STM32F746xx)
    temp->concatf("-- State                   %u\n", hi2c1.State);
    temp->concatf("-- ErrorCode               %u\n", hi2c1.ErrorCode);
    temp->concatf("-- pBuffPtr                %p\n", hi2c1.pBuffPtr);
    temp->concatf("-- XferSize                %d\n", hi2c1.XferSize);
    temp->concatf("-- XferCount               %u\n", hi2c1.XferCount);
    temp->concatf("-- CR1                     0x%08x\n", I2C1->CR1);
    temp->concatf("-- CR2                     0x%08x\n", I2C1->CR2);
    temp->concatf("-- TIMINGR                 0x%08x\n", I2C1->TIMINGR);
    temp->concatf("-- ISR                     0x%08x\n", I2C1->ISR);
    temp->concatf("-- RxDR                    0x%08x\n", I2C1->RXDR);
  #endif
  printPingMap(temp);

  if (current_queue_item != NULL) {
    temp->concat("Currently being serviced:\n");
    current_queue_item->printDebug(temp);
  }
  else {
    temp->concat("Nothing being serviced.\n\n");
  }

  if (work_queue.size() > 0) {
    temp->concatf("\nQueue Listing (top 3 of %d total)\n", work_queue.size());
    for (int i = 0; i < std::min(work_queue.size(), I2CADAPTER_MAX_QUEUE_PRINT); i++) {
      work_queue.get(i)->printDebug(temp);
    }
    temp->concat("\n");
  }
  else {
    temp->concat("Empty queue.\n\n");
  }
}


#if defined(__MANUVR_CONSOLE_SUPPORT)
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
          printPingMap(&local_log);
          break;
        case 2:
          printDevs(&local_log);
          break;
      }
      break;

    case 'd':
      dev_list.get(temp_int)->printDebug(&local_log);
      break;

    case '1':
      gpioSetup();
      local_log.concat("i2c GPIO reset.\n");
      break;

    #if defined(STM32F7XX) | defined(STM32F746xx)
    case '3':
      {
        I2CBusOp* nu = new I2CBusOp(BusOpcode::RX, 0x27, (int16_t) 0, &_debug_scratch, 1);
        //nu->requester = this;
        insert_work_item(nu);
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

    case 't':
      I2C1->CR1 &= ~((uint32_t) I2C_CR1_PE);
      while(I2C1->CR1 & I2C_CR1_PE) {}
      busOnline(_stm32f7_timing_reinit(&hi2c1, temp_int));
      local_log.concat("i2c timing set.\n");
      break;
    #endif

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

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif  //__MANUVR_CONSOLE_SUPPORT
