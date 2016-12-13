/*
File:   I2CBusOp.cpp
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
#else
  // Unsupported platform
#endif


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len) {
  this->xfer_state      = XferState::IDLE;
  this->xfer_fault      = XferFault::NONE;
  this->opcode          = nu_op;
  this->subaddr_sent    = (sub_addr >= 0) ? false : true;
  this->dev_addr        = dev_addr;
  this->sub_addr        = sub_addr;
  this->buf             = buf;
  this->buf_len         = len;
  this->txn_id          = BusOp::next_txn_id++;
  this->device          = NULL;
  this->requester       = NULL;
  this->subaddr_sent    = false;
  verbosity = 0;
};


I2CBusOp::~I2CBusOp() {
}


/****************************************************************************************************
* These functions are for logging support.                                                          *
****************************************************************************************************/

/*
* Dump this item to the dev log.
*/
void I2CBusOp::printDebug(void) {
	StringBuilder temp;
	this->printDebug(&temp);
	Kernel::log(&temp);
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CBusOp::printDebug(StringBuilder* temp) {
  if (temp != NULL) {
    temp->concatf("\n---[ I2CQueueOperation  0x%08x ]---\n", txn_id);
    temp->concatf("\topcode:          %s\n", getOpcodeString());
    temp->concatf("\tdevice:          0x%02x\n", dev_addr);

    if (sub_addr >= 0x00) {
      temp->concatf("\tsubaddress:      0x%02x (%ssent)\n", sub_addr, (subaddr_sent ? "" : "un"));
    }
    temp->concatf("\txfer_state:      %s\n", getStateString());
    temp->concatf("\tFault:           %s\n", getErrorString());
    temp->concatf("\tbuf_len:         %d\n\t", buf_len);
    if (buf_len > 0) {
      temp->concatf( "*(0x%p):   ", buf);
      for (uint8_t i = 0; i < buf_len; i++) {
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
int8_t I2CBusOp::abort(XferFault er) {
  markComplete();
  xfer_fault = er;
  return 0;
}


/*
*
*/
void I2CBusOp::markComplete() {
	xfer_state = XferState::COMPLETE;
	ManuvrMsg* q_rdy = Kernel::returnEvent(MANUVR_MSG_I2C_QUEUE_READY);
	q_rdy->specific_target = device;
  Kernel::isrRaiseEvent(q_rdy);   // Raise an event
}


/*
* This queue item can begin executing. This is where any bus access should be initiated.
* TODO: Needs to be doing bus-checks.
*/

int8_t I2CBusOp::begin() {
  if (NULL == device) {
    abort(XferFault::DEV_NOT_FOUND);
    return -1;
  }

  if ((NULL != requester) && !requester->operationCallahead(this)) {
    abort(XferFault::IO_RECALL);
    return -1;
  }

  xfer_state = XferState::ADDR;
  init_dma();
  if (device->generateStart()) {
    // Failure to generate START condition.
    abort(XferFault::BUS_BUSY);
    return -1;
  }
  return 0;
}

//http://tech.munts.com/MCU/Frameworks/ARM/stm32f4/libs/STM32F4xx_DSP_StdPeriph_Lib_V1.1.0/Project/STM32F4xx_StdPeriph_Examples/I2C/I2C_TwoBoards/I2C_DataExchangeDMA/main.c
// TODO: should check length > 0 before doing DMA init.
// TODO: should migrate this into i2c-adapter???
int8_t I2CBusOp::init_dma() {
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
  DMA_InitStructure.DMA_BufferSize         = (uint16_t) buf_len;   // Why did clive1 have (len-1)??
  DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t) buf;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &I2C1->DR;

  if (opcode == BusOpcode::RX) {
    DMA_Cmd(DMA1_Stream0, DISABLE);
    DMA_DeInit(DMA1_Stream0);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;   // Receive
    DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
    DMA_Init(DMA1_Stream0, &DMA_InitStructure);
    //if (verbosity > 5) local_log.concatf("init_dma():\ttxn_id: 0x%08x\tBuffer address: 0x%08x\n", txn_id, (uint32_t) buf);
    I2C_DMALastTransferCmd(I2C1, ENABLE);
  }
  else if (opcode == BusOpcode::TX) {
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

#elif defined(STM32F7XX) | defined(STM32F746xx)
  device->dispatchOperation(this);

#elif defined(__MANUVR_LINUX)   // Linux land...
  if (!device->switch_device(dev_addr)) return -1;

  if (opcode == BusOpcode::RX) {
    uint8_t sa = (uint8_t) (sub_addr & 0x00FF);

    if (write(device->getDevId(), &sa, 1) == 1) {
      return_value = read(device->getDevId(), buf, buf_len);
      if (return_value == buf_len) {
        markComplete();
      }
      else return -1;
    }
    else {
      abort();
      return -1;
    }
  }
  else if (opcode == BusOpcode::TX) {
    uint8_t buffer[buf_len + 1];
    buffer[0] = (uint8_t) (sub_addr & 0x00FF);

    for (int i = 0; i < buf_len; i++) buffer[i + 1] = *(buf + i);

    if (write(device->getDevId(), &buffer, buf_len+1) == buf_len+1) {
      markComplete();
    }
    else {
      abort();
      return -1;
    }
  }
  else if (opcode == BusOpcode::TX_CMD) {
    uint8_t buffer[buf_len + 1];
    buffer[0] = (uint8_t) (sub_addr & 0x00FF);
    if (write(device->getDevId(), &buffer, 1) == 1) {
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

#else
  // No support
#endif

  //if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}


#if defined(STM32F7XX) | defined(STM32F746xx)
  /*
  * Called from the ISR to advance this operation on the bus.
  * Still required for DMA because of subaddresses, START/STOP, etc...
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    StringBuilder output;
    #ifdef __MANUVR_DEBUG
    if (verbosity > 6) output.concatf("I2CBusOp::advance_operation(0x%08x): \t %s\t", status_reg, BusOp::getStateString(xfer_state));
    #endif
    switch (xfer_state) {
      case XferState::QUEUED:     // These are states we should not be in at this point...
        break;

      case XferState::INITIATE:     // We need to send a START condition.
        break;

      case XferState::ADDR:      // We need to send the 7-bit address.
        markComplete();  // TODO: As long as we are using the HAL driver....
        break;

      case XferState::RX_WAIT:   // Main transfer.
      case XferState::TX_WAIT:   // Main transfer.
        break;

      case XferState::STOP:      // We need to send a STOP condition.
        break;

      case XferState::COMPLETE:  // This operation is comcluded.
        if (verbosity > 5) {
          #ifdef __MANUVR_DEBUG
          output.concatf("\t--- Interrupt following job (0x%08x)\n", status_reg);
          #endif
          printDebug(&output);
        }
        break;

      case XferState::FAULT:     // Something asploded.
      default:
        #ifdef __MANUVR_DEBUG
        if (verbosity > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", BusOp::getStateString(xfer_state));
        #endif
        abort();
        break;
    }

    if (verbosity > 4) {
      if (verbosity > 6) printDebug(&output);
      #ifdef __MANUVR_DEBUG
      output.concatf("---> %s\n", BusOp::getStateString(xfer_state));
      #endif
    }

    if (output.length() > 0) Kernel::log(&output);
    return 0;
  }


#elif defined(STM32F4XX)
  /*
  * Called from the ISR to advance this operation on the bus.
  * Still required for DMA because of subaddresses, START/STOP, etc...
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    StringBuilder output;
    #ifdef __MANUVR_DEBUG
    if (verbosity > 6) output.concatf("I2CBusOp::advance_operation(0x%08x): \t %s\t", status_reg, BusOp::getStateString(xfer_state));
    #endif
    switch (xfer_state) {
      case XferState::INITIATE:     // We need to send a START condition.
        xfer_state = XferState::ADDR; // Need to send slave ADDR following a RESTART.
        device->generateStart();
        break;

      case XferState::ADDR:      // We need to send the 7-bit address.
        if (0x00000001 & status_reg) {
          // If we see a ping at this point, it means the ping succeeded. Mark it finished.
          if (opcode == BusOpcode::TX_CMD) {
            markComplete();
            device->generateStop();
            if (output.length() > 0) Kernel::log(&output);
            return 0;
          }
          // We are ready to send the address...
          // If we need to send a subaddress, we will need to be in transmit mode. Even if our end-goal is to read.
          uint8_t temp_dir_code = ((opcode == BusOpcode::TX || need_to_send_subaddr()) ? BusOpcode::TX : BusOpcode::RX);
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
          //xfer_state = XferState::ADDR; // Need to send slave ADDR following a RESTART.
          //I2C_GenerateSTART(I2C1, ENABLE);  // Start condition is proc'd after BUSY flag disasserts.
          xfer_state = XferState::INITIATE;
        }
        break;
      case I2C_XFER_STATE_BODY:      // We need to start the body of our transfer.
        if (opcode == BusOpcode::TX) {
          xfer_state = XferState::TX_WAIT;
          DMA_Cmd(DMA1_Stream7, ENABLE);
          I2C_DMACmd(I2C1, ENABLE);
        }
        else if (opcode == BusOpcode::RX) {
          xfer_state = XferState::RX_WAIT;
          DMA_Cmd(DMA1_Stream0, ENABLE);
          I2C_DMACmd(I2C1, ENABLE);
        }
        else if (opcode == BusOpcode::TX_CMD) {
          xfer_state = XferState::TX_WAIT;
          // Pinging...
          markComplete();
          device->generateStop();
        }
        break;
      case XferState::RX_WAIT:
      case XferState::TX_WAIT:
        break;
      case XferState::STOP:      // We need to send a STOP condition.
        if (opcode == BusOpcode::TX) {
          if (0x00000004 & status_reg) {  // Byte transfer finished?
            device->generateStop();
            markComplete();
          }
        }
        else if (opcode == BusOpcode::RX) {
          device->generateStop();
          markComplete();
        }
        else {
          abort();
        }
        break;
      case XferState::COMPLETE:  // This operation is comcluded.
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
        if (verbosity > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", BusOp::getStateString(xfer_state));
        #endif
        abort(I2C_ERR_CODE_DEF_CASE);
        break;
    }

    if (verbosity > 4) {
      if (verbosity > 6) printDebug(&output);
      #ifdef __MANUVR_DEBUG
      output.concatf("---> %s\n", BusOp::getStateString(xfer_state));
      #endif
    }

    if (output.length() > 0) Kernel::log(&output);
    return 0;
  }

#elif defined(__MK20DX256__) | defined(__MK20DX128__)

  /*
  * Called from the ISR to advance this operation on the bus.
  * Still required for DMA because of subaddresses, START/STOP, etc...
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    switch (status_reg) {
      case 1:
        subaddr_sent = true;
        break;
    }
    return 0;
  }

#elif defined(__MANUVR_LINUX)

  /*
  * Linux doesn't have a concept of interrupt, but we might call this
  *   from an I/O thread.
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    return 0;
  }

#elif defined(ARDUiNO)

  /*
  * Linux doesn't have a concept of interrupt, but we might call this
  *   from an I/O thread.
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    return 0;
  }

#endif
