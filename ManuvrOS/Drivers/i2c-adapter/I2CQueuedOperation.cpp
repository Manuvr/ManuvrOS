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


// Static initiallizer...
int I2CQueuedOperation::next_txn_id = 0;

/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CQueuedOperation::I2CQueuedOperation(uint8_t nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len) {
  this->initiated       = false;
  this->subaddr_sent    = (sub_addr >= 0) ? false : true;
  this->opcode          = nu_op;
  this->dev_addr        = dev_addr;
  this->sub_addr        = sub_addr;
  this->buf             = buf;
  this->len             = len;
  this->txn_id          = I2CQueuedOperation::next_txn_id++;
  this->err_code        = I2C_ERR_CODE_NO_ERROR;
  this->xfer_state      = I2C_XFER_STATE_INITIATE;
  this->device          = NULL;
  this->remaining_bytes = 0;
  this->requester       = NULL;
  this->reap_buffer     = false;
  this->subaddr_sent    = false;
  verbosity = 0;
};


I2CQueuedOperation::~I2CQueuedOperation(void) {
	if (reap_buffer) {
		if (buf != NULL) {
			free(buf);
		}
	}
}

      
/****************************************************************************************************
* These functions are for logging support.                                                          *
****************************************************************************************************/

const char* I2CQueuedOperation::getErrorString(int8_t code) {
  switch (code) {
    case I2C_ERR_CODE_NO_ERROR:     return "NO_ERROR";
    case I2C_ERR_CODE_DEF_CASE:     return "I2C_ERR_CODE_DEF_CASE";
    case I2C_ERR_CODE_NO_CASE:      return "NO_CASE";
    case I2C_ERR_CODE_NO_REASON:    return "NO_REASON";
    case I2C_ERR_CODE_BUS_FAULT:    return "BUS_FAULT";
    case I2C_ERR_CODE_NO_DEVICE:    return "NO_DEVICE";
    case I2C_ERR_CODE_ADDR_2TX:     return "ADDR_2TX";
    case I2C_ERR_CODE_BAD_OP:       return "Bad operation code";
    case I2C_ERR_CODE_TIMEOUT:      return "TIMEOUT";
    case I2C_ERR_CODE_CLASS_ABORT:  return "CLASS_ABORT";
    case I2C_ERR_CODE_BUS_BUSY:     return "ARBITRATION_LOST";
    default:                        return "<UNKNOWN>";
  }
}

const char* I2CQueuedOperation::getOpcodeString(uint8_t code) {
  switch (code) {
    case I2C_OPERATION_READ:        return "READ";
    case I2C_OPERATION_WRITE:       return "WRITE";
    case I2C_OPERATION_PING:        return "PING";
    default:                        return "<UNKNOWN>";
  }
}

const char* I2CQueuedOperation::getStateString(uint8_t code) {
  switch (code) {
    case I2C_XFER_STATE_INITIATE:   return "INITIATE";
    case I2C_XFER_STATE_START:      return "START";
    case I2C_XFER_STATE_ADDR:       return "ADDR";
    case I2C_XFER_STATE_SUBADDR:    return "SUBADDR";
    case I2C_XFER_STATE_BODY:       return "BODY";
    case I2C_XFER_STATE_DMA_WAIT:   return "DMA_WAIT";
    case I2C_XFER_STATE_STOP:       return "STOP";
    case I2C_XFER_STATE_COMPLETE:   return "COMPLETE";
    default:                        return "<UNKNOWN>";
  }
}


/*
* Dump this item to the dev log.
*/
void I2CQueuedOperation::printDebug(void) {
	StringBuilder temp;
	this->printDebug(&temp);
	Kernel::log(&temp);
}


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CQueuedOperation::printDebug(StringBuilder* temp) {
  if (temp != NULL) {
    temp->concatf("\n---[ %s I2CQueueOperation  0x%08x ]---\n", (completed() ? "Complete" : (initiated ? "Initiated" : "Uninitiated")), txn_id);
    temp->concatf("opcode:          %s\n", getOpcodeString(opcode));
    temp->concatf("device:          0x%02x\n", dev_addr);

    if (sub_addr >= 0x00) {
      temp->concatf("subaddress:      0x%02x (%ssent)\n", sub_addr, (subaddr_sent ? "" : "un"));
    }
    temp->concatf("xfer_state:      %s\n", getStateString(xfer_state));
    temp->concatf("err_code:        %s (%d)\n", getErrorString(err_code), err_code);
    temp->concatf("len:             %d\n", len);
    temp->concatf("remaining_bytes: %d\n", remaining_bytes);
    if (len > 0) {
      temp->concatf( "*(0x%08x):   ", (uint32_t) buf);
      for (uint8_t i = 0; i < len; i++) {
        temp->concatf("0x%02x ", (uint8_t) *(buf + i));
      }
    }
    temp->concat("\n");
  }
}
      




/****************************************************************************************************
* Job control functions. Calling these will affect control-flow during processing.                  *
****************************************************************************************************/

/* Call to mark something completed that may not be. Also sends a stop. */
int8_t I2CQueuedOperation::abort(void) {     return this->abort(I2C_ERR_CODE_NO_REASON);  }
int8_t I2CQueuedOperation::abort(int8_t er) {
  markComplete();
  err_code  = er;
  return 0;
}


/*
* 
*/
void I2CQueuedOperation::markComplete(void) {
	xfer_state = I2C_XFER_STATE_COMPLETE;
  initiated = true;  // Just so we don't accidentally get hung up thinking we need to start it.
	ManuvrEvent* q_rdy = Kernel::returnEvent(MANUVR_MSG_I2C_QUEUE_READY);
	q_rdy->specific_target = device;
  Kernel::isrRaiseEvent(q_rdy);   // Raise an event
}


/*
* This queue item can begin executing. This is where any bus access should be initiated.
* TODO: Needs to be doing bus-checks.
*/

int8_t I2CQueuedOperation::begin(void) {
  if (NULL == device) {
    abort(I2C_ERR_CODE_NO_DEVICE);
    return -1;
  }
  
  if ((NULL != requester) && !requester->operationCallahead(this)) {
    abort(I2C_ERR_CODE_CLASS_ABORT);
    return -1;
  }
  
  initiated = true;
  xfer_state = I2C_XFER_STATE_ADDR;
  init_dma();
  if (device->generateStart()) {
    // Failure to generate START condition.
    abort(I2C_ERR_CODE_NO_DEVICE);
    return -1;
  }
  return 0;
}

//http://tech.munts.com/MCU/Frameworks/ARM/stm32f4/libs/STM32F4xx_DSP_StdPeriph_Lib_V1.1.0/Project/STM32F4xx_StdPeriph_Examples/I2C/I2C_TwoBoards/I2C_DataExchangeDMA/main.c
// TODO: should check length > 0 before doing DMA init.
// TODO: should migrate this into i2c-adapter???
int8_t I2CQueuedOperation::init_dma() {
  int return_value = 0;

#if defined(__MK20DX256__) | defined(__MK20DX128__)
  device->dispatchOperation(this);
#elif defined(ARDUINO)

#elif defined(STM32F4XX)
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel            = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Enable;  // Required for differnt access-widths.
  DMA_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
  DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
  DMA_InitStructure.DMA_BufferSize         = (uint16_t) len;   // Why did clive1 have (len-1)??
  DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t) buf;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &I2C1->DR;

  if (opcode == I2C_OPERATION_READ) {
    DMA_Cmd(DMA1_Stream0, DISABLE);
    DMA_DeInit(DMA1_Stream0);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;   // Receive
    DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
    DMA_Init(DMA1_Stream0, &DMA_InitStructure);
    //if (verbosity > 5) local_log.concatf("init_dma():\ttxn_id: 0x%08x\tBuffer address: 0x%08x\n", txn_id, (uint32_t) buf);
    I2C_DMALastTransferCmd(I2C1, ENABLE);
  }
  else if (opcode == I2C_OPERATION_WRITE) {
    DMA_Cmd(DMA1_Stream7, DISABLE);
    DMA_DeInit(DMA1_Stream7);
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;   // Transmit
    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
    DMA_Init(DMA1_Stream7, &DMA_InitStructure);
    //if (verbosity > 5) local_log.concatf("init_dma():\ttxn_id: 0x%08x\tBuffer address: 0x%08x\n", txn_id, (uint32_t) buf);
    I2C_DMALastTransferCmd(I2C1, DISABLE);
  }
  else {
    return -1;
  }
  

#else   // Linux land...
  if (!device->switch_device(dev_addr)) return -1;
 
  if (opcode == I2C_OPERATION_READ) {
    uint8_t sa = (uint8_t) (sub_addr & 0x00FF);
    
    if (write(device->dev, &sa, 1) == 1) {
      return_value = read(device->dev, buf, len);
      if (return_value == len) {
        markComplete();
      }
      else return -1;
    }
    else {
      abort();
      return -1;
    }
  }
  
  else if (opcode == I2C_OPERATION_WRITE) {
    uint8_t buffer[len + 1];
    buffer[0] = (uint8_t) (sub_addr & 0x00FF);
    
    for (int i = 0; i < len; i++) buffer[i + 1] = *(buf + i);
    
    if (write(device->dev, &buffer, len+1) == len+1) {
      markComplete();
    }
    else {
      abort();
      return -1;
    }
  }
  else {
    abort();
    return -1;
  }

#endif

  //if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}


/*
* Called from the ISR to advance this operation on the bus.
* Still required for DMA because of subaddresses, START/STOP, etc...
*/
int8_t I2CQueuedOperation::advance_operation(uint32_t status_reg) {
  StringBuilder output;
#ifdef STM32F4XX
  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) output.concatf("I2CQueuedOperation::advance_operation(0x%08x): \t %s\t", status_reg, getStateString(xfer_state));
  #endif
  switch (xfer_state) {
    case I2C_XFER_STATE_START:     // We need to send a START condition.
        xfer_state = I2C_XFER_STATE_ADDR; // Need to send slave ADDR following a RESTART.
        device->generateStart();
        break;
      
    case I2C_XFER_STATE_ADDR:      // We need to send the 7-bit address.
      if (0x00000001 & status_reg) {
        // If we see a ping at this point, it means the ping succeeded. Mark it finished.
        if (opcode == I2C_OPERATION_PING) {
          markComplete();
          device->generateStop();
          if (output.length() > 0) Kernel::log(&output);
          return 0;
        }
        // We are ready to send the address...
        // If we need to send a subaddress, we will need to be in transmit mode. Even if our end-goal is to read.
        uint8_t temp_dir_code = ((opcode == I2C_OPERATION_WRITE || need_to_send_subaddr()) ? I2C_OPERATION_WRITE : I2C_OPERATION_READ);
        I2C_Send7bitAddress(I2C1, (dev_addr << 1), temp_dir_code);
        if (need_to_send_subaddr()) {                   
          xfer_state = I2C_XFER_STATE_SUBADDR;
        }
        else {
          xfer_state = I2C_XFER_STATE_BODY;
          I2C_DMACmd(I2C1, ENABLE);
        }
      }
      else if (0x00000002 & status_reg) {
        abort(I2C_ERR_CODE_ADDR_2TX);
      }
      break;
    case I2C_XFER_STATE_SUBADDR:   // We need to send a subaddress.
      if (0x00000002 & status_reg) {  // If the dev addr was sent...
        I2C_SendData(I2C1, (uint8_t) (sub_addr & 0x00FF));   // ...send subaddress.
        subaddr_sent = true;  // We've sent this already. Don't do it again.
        //xfer_state = I2C_XFER_STATE_ADDR; // Need to send slave ADDR following a RESTART.
        //I2C_GenerateSTART(I2C1, ENABLE);  // Start condition is proc'd after BUSY flag disasserts.
        xfer_state = I2C_XFER_STATE_START;
      }
      break;
    case I2C_XFER_STATE_BODY:      // We need to start the body of our transfer.
      xfer_state = I2C_XFER_STATE_DMA_WAIT;
      if (opcode == I2C_OPERATION_WRITE) {
        DMA_Cmd(DMA1_Stream7, ENABLE);
        I2C_DMACmd(I2C1, ENABLE);
      }
      else if (opcode == I2C_OPERATION_READ) {
        DMA_Cmd(DMA1_Stream0, ENABLE);
        I2C_DMACmd(I2C1, ENABLE);
      }
      else if (opcode == I2C_OPERATION_PING) {
        markComplete();
        device->generateStop();
      }
      break;
    case I2C_XFER_STATE_DMA_WAIT:
      break;
    case I2C_XFER_STATE_STOP:      // We need to send a STOP condition.
      if (opcode == I2C_OPERATION_WRITE) {
        if (0x00000004 & status_reg) {  // Byte transfer finished?
          device->generateStop();
          markComplete();
        }
      }
      else if (opcode == I2C_OPERATION_READ) {
        device->generateStop();
        markComplete();
      }
      else {
        abort();
      }
      break;
    case I2C_XFER_STATE_COMPLETE:  // This operation is comcluded.
      if (verbosity > 5) {
        #ifdef __MANUVR_DEBUG
        output.concatf("\t--- Interrupt following job (0x%08x)\n", status_reg);
        #endif
        printDebug(&output);
      }
      //markComplete();
      //Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, NULL);   // Raise an event
      break;
    default:
      #ifdef __MANUVR_DEBUG
      if (verbosity > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", getStateString(xfer_state));
      #endif
      abort(I2C_ERR_CODE_DEF_CASE);
      break;
  }
  
  if (verbosity > 4) {
    if (verbosity > 6) printDebug(&output);
    #ifdef __MANUVR_DEBUG
    output.concatf("---> %s\n", getStateString(xfer_state));
    #endif
  }

#elif defined(__MK20DX256__) | defined(__MK20DX128__)
  switch (status_reg) {
    case 1:
      subaddr_sent = true;
      break;
  }
  
#endif
  if (output.length() > 0) Kernel::log(&output);
  return 0;
}

