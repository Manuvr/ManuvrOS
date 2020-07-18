#include <I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
extern "C" {
  #include <stm32f4xx.h>
  #include <stm32f4xx_i2c.h>
  #include <stm32f4xx_gpio.h>
  #include "stm32f4xx_it.h"
  I2C_HandleTypeDef hi2c1;
}


int8_t I2CAdapter::bus_init() {
  // This init() fxn was patterned after the STM32F4x7 library example.
  if (dev == 1) {
    //I2C_DeInit(I2C1);    //Deinit and reset the I2C to avoid it locking up

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

  return (busOnline() ? 0 : -1);
}


//http://tech.munts.com/MCU/Frameworks/ARM/stm32f4/libs/STM32F4xx_DSP_StdPeriph_Lib_V1.1.0/Project/STM32F4xx_StdPeriph_Examples/I2C/I2C_TwoBoards/I2C_DataExchangeDMA/main.c
// TODO: should check length > 0 before doing DMA init.
// TODO: should migrate this into I2CAdapter???
int8_t I2CBusOp::init_dma() {
  int return_value = 0;

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
    //if (getVerbosity() > 5) local_log.concatf("init_dma():\ttxn_id: 0x%08x\tBuffer address: 0x%08x\n", txn_id, (uint32_t) buf);
    I2C_DMALastTransferCmd(I2C1, ENABLE);
  }
  else if (opcode == BusOpcode::TX) {
    DMA_Cmd(DMA1_Stream7, DISABLE);
    DMA_DeInit(DMA1_Stream7);
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;   // Transmit
    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
    DMA_Init(DMA1_Stream7, &DMA_InitStructure);
    //if (getVerbosity() > 5) local_log.concatf("init_dma():\ttxn_id: 0x%08x\tBuffer address: 0x%08x\n", txn_id, (uint32_t) buf);
    I2C_DMALastTransferCmd(I2C1, DISABLE);
  }
  else {
    return -1;
  }

  //if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}


int8_t I2CAdapter::bus_deinit() {
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, DISABLE);   // Shelve the interrupts.
  I2C_DeInit(I2C1);   // De-init
}


void I2CAdapter::printHardwareState(StringBuilder* output) {
  output->concatf("-- I2C%d (%sline) --------------------\n", dev, (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
}


int8_t I2CAdapter::generateStart() {
  #ifdef MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! busOnline()) return -1;
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, ENABLE);      //Enable EVT and ERR interrupts
  I2C_GenerateSTART(I2C1, ENABLE);   // Doing this will take us back to the ISR.
  return 0;
}

int8_t I2CAdapter::generateStop() {
  #ifdef MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! busOnline()) return -1;
  DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, DISABLE);
  I2C_ITConfig(I2C1, I2C_IT_EVT|I2C_IT_ERR, DISABLE);
  I2C_GenerateSTOP(I2C1, ENABLE);
  return 0;
}



  /*
  * Called from the ISR to advance this operation on the bus.
  * Still required for DMA because of subaddresses, START/STOP, etc...
  */
  int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
    StringBuilder output;
    #ifdef MANUVR_DEBUG
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
          #ifdef MANUVR_DEBUG
          output.concatf("\t--- Interrupt following job (0x%08x)\n", status_reg);
          #endif
          printDebug(&output);
        }
        //markComplete();
        //Kernel::raiseEvent(MANUVR_MSG_I2C_QUEUE_READY, nullptr);   // Raise an event
        break;
      default:
        #ifdef MANUVR_DEBUG
        if (getVerbosity() > 1) output.concatf("\t--- Something is bad wrong. Our xfer_state is %s\n", BusOp::getStateString(xfer_state));
        #endif
        abort(I2C_ERR_CODE_DEF_CASE);
        break;
    }

    if (getVerbosity() > 4) {
      if (getVerbosity() > 6) printDebug(&output);
      #ifdef MANUVR_DEBUG
      output.concatf("---> %s\n", BusOp::getStateString(xfer_state));
      #endif
    }

    if (output.length() > 0) Kernel::log(&output);
    return 0;
  }

#endif   // MANUVR_SUPPORT_I2C
