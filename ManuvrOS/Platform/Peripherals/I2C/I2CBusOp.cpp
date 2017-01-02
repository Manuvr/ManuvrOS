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


#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(STM32F4XX)
  #include <stm32f4xx.h>
  #include <stm32f4xx_i2c.h>
  #include <stm32f4xx_gpio.h>
  #include "stm32f4xx_it.h"
#endif


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp() {
  xfer_state   = XferState::IDLE;
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, BusOpCallback* requester) : I2CBusOp() {
  opcode       = nu_op;
  callback     = requester;
  subaddr_sent((sub_addr >= 0) ? false : true);
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len) : I2CBusOp() {
  xfer_state   = XferState::IDLE;
  opcode       = nu_op;
  dev_addr     = dev_addr;
  sub_addr     = sub_addr;
  buf          = buf;
  buf_len      = len;
  subaddr_sent((sub_addr >= 0) ? false : true);
};


I2CBusOp::~I2CBusOp() {
}



/**
* Wipes this bus operation so it can be reused.
* Be careful not to blow away the flags that prevent us from being reaped.
*/
void I2CBusOp::wipe() {
  set_state(XferState::IDLE);
  // We need to preserve flags that deal with memory management.
  xfer_fault  = XferFault::NONE;
  opcode      = BusOpcode::UNDEF;
  buf_len     = 0;
  buf         = nullptr;
  callback    = nullptr;
}


/****************************************************************************************************
* These functions are for logging support.                                                          *
****************************************************************************************************/

/*
* Dump this item to the dev log.
*/
void I2CBusOp::printDebug() {
	StringBuilder temp;
	printDebug(&temp);
	Kernel::log(&temp);
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CBusOp::printDebug(StringBuilder* output) {
  BusOp::printBusOp("I2COp", this, output);
  output->concatf("\t device          0x%02x\n", dev_addr);

  if (sub_addr >= 0x00) {
    output->concatf("\t subaddress      0x%02x (%ssent)\n", sub_addr, (subaddr_sent() ? "" : "un"));
  }
  output->concat("\n\n");
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



#if defined(STM32F7XX) | defined(STM32F746xx)
  /*
  * Called from the ISR to advance this operation on the bus.
  * Still required for DMA because of subaddresses, START/STOP, etc...
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    StringBuilder output;
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 6) output.concatf("I2CBusOp::advance_operation(0x%08x): \t %s\t", status_reg, BusOp::getStateString(xfer_state));
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
        if (getVerbosity() > 5) {
          #ifdef __MANUVR_DEBUG
          output.concatf("\t--- Interrupt following job (0x%08x)\n", status_reg);
          #endif
          printDebug(&output);
        }
        break;

      case XferState::FAULT:     // Something asploded.
      default:
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", BusOp::getStateString(xfer_state));
        #endif
        abort();
        break;
    }

    if (getVerbosity() > 4) {
      if (getVerbosity() > 6) printDebug(&output);
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
    if (getVerbosity() > 6) output.concatf("I2CBusOp::advance_operation(0x%08x): \t %s\t", status_reg, BusOp::getStateString(xfer_state));
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
        if (getVerbosity() > 5) {
          #ifdef __MANUVR_DEBUG
          output.concatf("\t--- Interrupt following job (0x%08x)\n", status_reg);
          #endif
          printDebug(&output);
        }
        //markComplete();
        //Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, nullptr);   // Raise an event
        break;
      default:
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", BusOp::getStateString(xfer_state));
        #endif
        abort(I2C_ERR_CODE_DEF_CASE);
        break;
    }

    if (getVerbosity() > 4) {
      if (getVerbosity() > 6) printDebug(&output);
      #ifdef __MANUVR_DEBUG
      output.concatf("---> %s\n", BusOp::getStateString(xfer_state));
      #endif
    }

    if (output.length() > 0) Kernel::log(&output);
    return 0;
  }

#elif defined(ARDUiNO)

  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    return 0;
  }

#endif
