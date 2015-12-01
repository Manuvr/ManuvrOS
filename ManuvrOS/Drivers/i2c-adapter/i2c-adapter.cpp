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
#else
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
#endif


using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
volatile I2CAdapter* i2c = NULL;
#ifdef __cplusplus
}
#endif



/**************************************************************************
* Constructors / Destructors                                              *
**************************************************************************/

void I2CAdapter::__class_initializer() {
  EventReceiver::__class_initializer();

  current_queue_item = NULL;
  bus_error          = false;
  bus_online         = false;
  ping_run           = false;
  full_ping_running  = false;
  last_used_bus_addr = 0;

  for (uint16_t i = 0; i < 128; i++) ping_map[i] = 0;   // Zero the ping map.

  // Set a globalized refernece so we can hit the proper adapter from an ISR.
  i2c = this;
  
  // We need some internal events to allow communication back from the ISR.
  const MessageTypeDef i2c_message_defs[] = {
    { MANUVR_MSG_I2C_QUEUE_READY, MSG_FLAG_IDEMPOTENT,  "I2C_QUEUE_READY", ManuvrMsg::MSG_ARGS_NONE }  // The i2c queue is ready for attention.
  };
  
  int mes_count = sizeof(i2c_message_defs) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(i2c_message_defs, mes_count);
}


#ifdef STM32F4XX
I2CAdapter::I2CAdapter(uint8_t dev_id) {
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


// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! bus_online) return -1;
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, ENABLE);      //Enable EVT and ERR interrupts
  I2C_GenerateSTART(I2C1, ENABLE);   // Doing this will take us back to the ISR.
  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! bus_online) return -1;
  DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, DISABLE);
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, DISABLE);
  I2C_GenerateSTOP(I2C1, ENABLE);
  return 0;
}


#elif defined(__MK20DX256__) | defined(__MK20DX128__)


I2CAdapter::I2CAdapter(uint8_t dev_id) {
  __class_initializer();
  dev = dev_id;

  if (dev_id == 0) {
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR);
    Wire.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
    bus_online = true;
  }
  #if defined(__MK20DX256__)
  else if (dev_id == 1) {
    Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR);
    Wire1.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
    bus_online = true;
  }
  #endif
  else {
    // Unsupported
  }
}


I2CAdapter::~I2CAdapter() {
    bus_online = false;
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }
    
    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}



int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! bus_online) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.
  return 0;
}


int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! bus_online) return -1;
  return 0;
}



int8_t I2CAdapter::dispatchOperation(I2CQueuedOperation* op) {
  // TODO: This is awful. Need to ultimately have a direct ref to the class that *is* the adapter.
  if (0 == dev) {
    Wire.beginTransmission((uint8_t) (op->dev_addr & 0x00FF));
    if (op->need_to_send_subaddr()) {
      Wire.write((uint8_t) (op->sub_addr & 0x00FF));
      op->advance_operation(1);
    }
    
    if (op->opcode == I2C_OPERATION_READ) {
      Wire.endTransmission(I2C_NOSTOP);
      Wire.requestFrom(op->dev_addr, op->len, I2C_STOP, 10000);
      int i = 0;
      while(Wire.available()) {
        *(op->buf + i++) = (uint8_t) Wire.readByte();
      }
    }
    else if (op->opcode == I2C_OPERATION_WRITE) {
      for(int i = 0; i < op->len; i++) Wire.write(*(op->buf+i));
      Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }
    else if (op->opcode == I2C_OPERATION_PING) {
      Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }

    switch (Wire.status()) {
      case I2C_WAITING:
        op->markComplete();
        break;
      case I2C_ADDR_NAK:
        op->abort(I2C_ERR_SLAVE_NOT_FOUND);
        break;
      case I2C_DATA_NAK:
        op->abort(I2C_ERR_SLAVE_INVALID);
        break;
      case I2C_ARB_LOST:
        op->abort(I2C_ERR_CODE_BUS_BUSY);
        break;
      case I2C_TIMEOUT:
        op->abort(I2C_ERR_CODE_TIMEOUT);
        break;
    }
  }
#if defined(__MK20DX256__)
  else if (1 == dev) {
    Wire1.beginTransmission((uint8_t) (op->dev_addr & 0x00FF));
    if (op->need_to_send_subaddr()) {
      Wire1.write((uint8_t) (op->sub_addr & 0x00FF));
      op->advance_operation(1);
    }
    
    if (op->opcode == I2C_OPERATION_READ) {
      Wire1.endTransmission(I2C_NOSTOP);
      Wire1.requestFrom(op->dev_addr, op->len, I2C_STOP, 10000);
      int i = 0;
      while(Wire1.available()) {
        *(op->buf + i++) = (uint8_t) Wire1.readByte();
      }
    }
    else if (op->opcode == I2C_OPERATION_WRITE) {
      for(int i = 0; i < op->len; i++) Wire1.write(*(op->buf+i));
      Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }
    else if (op->opcode == I2C_OPERATION_PING) {
      Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }
    
    switch (Wire1.status()) {
      case I2C_WAITING:
        op->markComplete();
        break;
      case I2C_ADDR_NAK:
        op->abort(I2C_ERR_SLAVE_NOT_FOUND);
        break;
      case I2C_DATA_NAK:
        op->abort(I2C_ERR_SLAVE_INVALID);
        break;
      case I2C_ARB_LOST:
        op->abort(I2C_ERR_CODE_BUS_BUSY);
        break;
      case I2C_TIMEOUT:
        op->abort(I2C_ERR_CODE_TIMEOUT);
        break;
    }
  }
#endif
  else {
  }
  return 0;
}


#elif defined(_BOARD_FUBARINO_MINI_)    // Perhaps this is an arduino-style env?


I2CAdapter::I2CAdapter(uint8_t dev_id) {
  __class_initializer();
  dev = dev_id;

  if (dev_id == 0) {
    Wire.begin();
    bus_online = true;
  }
  else {
    // Unsupported.
  }
}


I2CAdapter::~I2CAdapter() {
    bus_online = false;
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }
    
    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}


int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! bus_online) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.
  
  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! bus_online) return -1;
  return 0;
}




#elif defined(ARDUINO)    // Perhaps this is an arduino-style env?

I2CAdapter::I2CAdapter(uint8_t dev_id) {
  __class_initializer();
  dev = dev_id;
  
  if (dev_id == 1) {
    //Wire.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400);
    bus_online = true;
  }
}


I2CAdapter::~I2CAdapter() {
    bus_online = false;
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
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! bus_online) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.
  
  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! bus_online) return -1;
  return 0;
}


#else  // Assuming a linux system...

I2CAdapter::I2CAdapter(uint8_t dev_id) {
  __class_initializer();
  dev = dev_id;

  char *filename = (char *) alloca(24);
  if (sprintf(filename, "/dev/i2c-%d", dev_id) > 0) {
    dev = open(filename, O_RDWR);
    if (dev < 0) {
      #ifdef __MANUVR_DEBUG
      Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to open the i2c bus represented by %s.\n", filename);
      #endif
    }
  }
  else {
    #ifdef __MANUVR_DEBUG
    Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Somehow we failed to sprintf and build a filename to open i2c bus %d.\n", dev_id);
    #endif
  }
}



I2CAdapter::~I2CAdapter() {
    if (dev >= 0) {
      #ifdef __MANUVR_DEBUG
      Kernel::log(__PRETTY_FUNCTION__, LOG_INFO, "Closing the open i2c bus...\n");
      #endif
      close(dev);
    }
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }
    
    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}



// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  return 0;
}


// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  return 0;
}

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

  if (dev >= 0) bus_online = true;
  if (bus_online) {
    advance_work_queue();
  }
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
int8_t I2CAdapter::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }
  
  return return_value;
}



int8_t I2CAdapter::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_INTERRUPTS_MASKED:
    case MANUVR_MSG_SYS_REBOOT:
    case MANUVR_MSG_SYS_BOOTLOADER:
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
  
  if (local_log.length() > 0) Kernel::log(&local_log);
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
		Kernel::log("Slave is invalid.");
		#endif
		return_value = I2C_ERR_SLAVE_INVALID;
	}
	if (dev_list.contains(slave)) {    // Check for pointer eqivillence.
	  #ifdef __MANUVR_DEBUG
		Kernel::log("Slave device exists.");
		#endif
		return_value = I2C_ERR_SLAVE_EXISTS;
	}
	else if (get_slave_dev_by_addr(slave->_dev_addr) == I2C_ERR_SLAVE_NOT_FOUND) {
		if (slave->assignBusInstance(this)) {
			int slave_index = dev_list.insert(slave);
			if (slave_index == -1) {
			  #ifdef __MANUVR_DEBUG
				Kernel::log("Failed to insert somehow. Disassigning...");
				#endif
				slave->disassignBusInstance();
				return_value = I2C_ERR_SLAVE_INSERTION;
			}
		}
		else {
		  #ifdef __MANUVR_DEBUG
			Kernel::log("Op would clobber bus instance.");
			#endif
			return_value = I2C_ERR_SLAVE_ASSIGN_CLOB;
		}
	}
	else {
	  #ifdef __MANUVR_DEBUG
		Kernel::log("Op would cause address collision with another slave device.");
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

  
#elif defined(MPCMZ)
// PIC32 MZ i2c support is broken at the time of this writing.

#else   // Assuming a linux environment.
  bool return_value = false;
  unsigned short timeout = 10000;
  if (nu_addr != last_used_bus_addr) {
    if (dev < 0) {
      // If the bus is either uninitiallized or not idle, decline
      // to switch the device. Return false;
      #ifdef __MANUVR_DEBUG
      Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "i2c bus is not online, so won't switch device. Failing....");
      #endif
      return return_value;
    }
    else {
      while (bus_error && (timeout > 0)) { timeout--; }
      if (bus_error) {
        #ifdef __MANUVR_DEBUG
        Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "i2c bus was held for too long. Failing....");
        #endif
        return return_value;
      }
      
      if (ioctl(dev, I2C_SLAVE, nu_addr) >= 0) {
        last_used_bus_addr = nu_addr;
        return_value = true;
      }
      else {
        #ifdef __MANUVR_DEBUG
        Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to acquire bus access and/or talk to slave at %d.", nu_addr);
        #endif
        bus_error = true;
      }
    }
  }
  else {
      return_value = true;
  }
#endif
  return return_value;
}



/*
* This is the function that should be called to queue-up a bus operation.
* It may or may not be started immediately.
*/
bool I2CAdapter::insert_work_item(I2CQueuedOperation *nu) {
  nu->verbosity = verbosity;
  nu->device = this;
	if (current_queue_item != NULL) {
		// Something is already going on with the bus. Queue...
		//Kernel::log(__PRETTY_FUNCTION__, 5, "Deferring i2c bus operation...");
		work_queue.insert(nu);
	}
	else {
		// Bus is idle. Put this work item in the active slot and start the bus operations...
		//Kernel::log(__PRETTY_FUNCTION__, 5, "Starting i2c operation now...");
		current_queue_item = nu;
		if ((dev >= 0) && (bus_online)) {
		  nu->begin();
		  if (verbosity > 6) {
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
		if (current_queue_item->completed()) {
			if (I2C_ERR_CODE_NO_ERROR == current_queue_item->err_code) {
			  //temp.concatf("Destroying successful job 0x%08x.\n", current_queue_item->txn_id);
			}
			else {
			  #ifdef __MANUVR_DEBUG
			  if (verbosity > 3) local_log.concatf("Destroying failed job 0x%08x.\n", current_queue_item->txn_id);
			  #endif
			  if (verbosity > 4) current_queue_item->printDebug(&local_log);
			}

			// Hand this completed operation off to the class that requested it. That class will
			//   take what it wants from the buffer and, when we return to execution here, we will
			//   be at liberty to clean the operation up.
			if (current_queue_item->requester != NULL) {
				// TODO: need some minor reorg to make this not so obtuse...
				current_queue_item->requester->operationCompleteCallback(current_queue_item);
			}
			else if (current_queue_item->opcode == I2C_OPERATION_PING) {
				if (current_queue_item->err_code == I2C_ERR_CODE_NO_ERROR) {
					ping_map[current_queue_item->dev_addr % 128] = 1;
				}
				else {
					ping_map[current_queue_item->dev_addr % 128] = -1;
				}
				
				if (full_ping_running) {
				  if ((current_queue_item->dev_addr & 0x00FF) < 127) {
				    ping_slave_addr(current_queue_item->dev_addr + 1);
				  }
				  else {
				    #ifdef __MANUVR_DEBUG
				    if (verbosity > 3) local_log.concat("Concluded i2c ping sweep.");
				    #endif
				    full_ping_running = false;
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
		if (!current_queue_item->initiated) {
			current_queue_item->begin();
		}
	}
	
	if (local_log.length() > 0) Kernel::log(&local_log);
}


/*
* Pass an i2c device, and this fxn will purge all of its queued work. Presumably, this is
*   because it is being detatched from the bux, but it may also be because one of it's operations
*   went bad.
*/
void I2CAdapter::purge_queued_work_by_dev(I2CDevice *dev) {
  I2CQueuedOperation* current = NULL;
  
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
  I2CQueuedOperation* current = NULL;
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
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_PING, addr, (int16_t) -1, NULL, 0);
    insert_work_item(nu);
    ping_run = true;
}


/*
* Debug fxn to print the ping map.
*/
void I2CAdapter::printPingMap(StringBuilder *temp) {
  if (temp != NULL) {
    if (!ping_run) {
      temp->concat("\n\nNo ping map to show.\n");
      return;
    }
    temp->concat("\n\n\tPing Map\n\t      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
    // TODO: This is needlessly-extravagent of memory. Do it this way instead...
    //char str_buf[];
    for (uint8_t i = 0; i < 128; i+=16) {
      temp->concatf("\t0x%02x: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
        i,
        ((ping_map[i + 0x00] == 0) ? " " : ((ping_map[i + 0x00] < 0) ? "-" : "*")),
        ((ping_map[i + 0x01] == 0) ? " " : ((ping_map[i + 0x01] < 0) ? "-" : "*")),
        ((ping_map[i + 0x02] == 0) ? " " : ((ping_map[i + 0x02] < 0) ? "-" : "*")),
        ((ping_map[i + 0x03] == 0) ? " " : ((ping_map[i + 0x03] < 0) ? "-" : "*")),
        ((ping_map[i + 0x04] == 0) ? " " : ((ping_map[i + 0x04] < 0) ? "-" : "*")),
        ((ping_map[i + 0x05] == 0) ? " " : ((ping_map[i + 0x05] < 0) ? "-" : "*")),
        ((ping_map[i + 0x06] == 0) ? " " : ((ping_map[i + 0x06] < 0) ? "-" : "*")),
        ((ping_map[i + 0x07] == 0) ? " " : ((ping_map[i + 0x07] < 0) ? "-" : "*")),
        ((ping_map[i + 0x08] == 0) ? " " : ((ping_map[i + 0x08] < 0) ? "-" : "*")),
        ((ping_map[i + 0x09] == 0) ? " " : ((ping_map[i + 0x09] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0A] == 0) ? " " : ((ping_map[i + 0x0A] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0B] == 0) ? " " : ((ping_map[i + 0x0B] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0C] == 0) ? " " : ((ping_map[i + 0x0C] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0D] == 0) ? " " : ((ping_map[i + 0x0D] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0E] == 0) ? " " : ((ping_map[i + 0x0E] < 0) ? "-" : "*")),
        ((ping_map[i + 0x0F] == 0) ? " " : ((ping_map[i + 0x0F] < 0) ? "-" : "*"))
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
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* I2CAdapter::getReceiverName() {  return "I2CAdapter";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CAdapter::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;
  
  EventReceiver::printDebug(temp);
  temp->concatf("--- bus_online             %s\n", (bus_online ? "yes" : "no"));
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
    for (int i = 0; i < min(work_queue.size(), I2CADAPTER_MAX_QUEUE_PRINT); i++) {
      work_queue.get(i)->printDebug(temp);
    }
    temp->concat("\n");
  }
  else {
    temp->concat("Empty queue.\n\n");
  }
}



void I2CAdapter::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  
  char c = *(str);
  uint8_t temp_byte = 0;        // Many commands here take a single integer argument.
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (c) {
    // i2c debugging cases....
    case 'i':
      if (temp_byte < dev_list.size()) {
        dev_list.get(temp_byte)->printDebug(&local_log);
      }
      else if (temp_byte == 255) {
        printDevs(&local_log);
      }
      else if (temp_byte == 253) {
        printPingMap(&local_log);
      }
      else {
        printDebug(&local_log);
      }
      break;
    case '1':
      gpioSetup();
      local_log.concat("i2c GPIO reset.\n");
      break;
    case '2':
      #ifdef STM32F4XX
        I2C_SoftwareResetCmd(I2C1, ENABLE);
        I2C_SoftwareResetCmd(I2C1, DISABLE);
      #endif
      local_log.concat("i2c software reset.\n");
      break;
    case '3':
      purge_queued_work();
      local_log.concat("i2c queue purged.\n");
      break;
    case '4':
      purge_stalled_job();
      local_log.concatf("Attempting to purge a stalled jorbe...\n");
      break;
#ifdef STM32F4XX
    case 'b':
      I2C1_EV_IRQHandler();
      break;
#endif
      
    case 'K':
      if (temp_byte) {
        local_log.concatf("ping i2c slave 0x%02x.\n", temp_byte);
        ping_slave_addr(temp_byte);
      }
      else if (!full_ping_running) {
        full_ping_running = true;
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

