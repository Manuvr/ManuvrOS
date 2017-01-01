#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
extern "C" {
  #include <stm32f7xx_hal.h>
  #include <stm32f7xx_hal_gpio.h>
  #include <stm32f7xx_hal_i2c.h>
  I2C_HandleTypeDef hi2c1;
}


uint8_t _debug_scratch = 0;
static uint32_t val = 0;

bool _stm32f7_timing_reinit(I2C_HandleTypeDef *hi2c, uint32_t val) {
  hi2c->Init.Timing = val;
    //hi2c->Init.Timing           = 0x40912732;
    //hi2c1.Init.Timing           = 0x80621519;
    //hi2c1.Init.Timing           = 0x0030334E;
    //hi2c1.Init.Timing           = 0x0020010C;
    //hi2c1.Init.Timing           = 0x00100615;
    hi2c->Init.OwnAddress1      = 0;
    hi2c->Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.OwnAddress2      = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
  return (HAL_OK == HAL_I2C_Init(hi2c));
}


//I2CAdapter::I2CAdapter(uint8_t dev_id) : I2CAdapter(1, 22, 23) {}
int8_t I2CAdapter::bus_init() {
  switch (dev_id) {
    case 1:
      if (((23 == _bus_opts.sda_pin) || (25 == _bus_opts.sda_pin)) && ((22 == _bus_opts.scl_pin) || (24 == _bus_opts.scl_pin))) {
        // Valid pins:      SCL: B6, B8  (22, 24)     SDA: B7, B9  (23, 25)
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin       = (1 << (_bus_opts.sda_pin % 16)) | (1 << (_bus_opts.scl_pin % 16));
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        __HAL_RCC_I2C1_CLK_ENABLE();

        hi2c1.Instance = I2C1;
        if (_stm32f7_timing_reinit(&hi2c1, 0x7022E0E0)) {
          //HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_DISABLE);
          //busOnline(HAL_OK == HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 6));
          busOnline(true);
          HAL_NVIC_SetPriority(I2C1_EV_IRQn, 1, 0);
          HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
          HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
          HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
        }
        else {
          Kernel::log("I2CAdapter failed to init.\n");
        }
      }
      break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      Kernel::log("I2CAdapter unsupported.\n");
      break;
  }

  return (busOnline() ? 0 : -1);
}


int8_t I2CAdapter::bus_deinit() {
  __HAL_RCC_I2C1_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7|GPIO_PIN_6);
  HAL_I2C_DeInit(&hi2c1);
  return 0;
}



void I2CAdapter::printHardwareState(StringBuilder* output) {
  output->concatf("-- I2C%d (%sline) --------------------\n", dev, (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
  output->concatf("-- State       %u\n", hi2c1.State);
  output->concatf("-- ErrorCode   %u\n", hi2c1.ErrorCode);
  output->concatf("-- pBuffPtr    %p\n", hi2c1.pBuffPtr);
  output->concatf("-- XferSize    %d\n", hi2c1.XferSize);
  output->concatf("-- XferCount   %u\n", hi2c1.XferCount);
  output->concatf("-- CR1         0x%08x\n", I2C1->CR1);
  output->concatf("-- CR2         0x%08x\n", I2C1->CR2);
  output->concatf("-- TIMINGR     0x%08x\n", I2C1->TIMINGR);
  output->concatf("-- ISR         0x%08x\n", I2C1->ISR);
  output->concatf("-- RxDR        0x%08x\n\n", I2C1->RXDR);
}


int8_t I2CAdapter::generateStart() {
  //#ifdef __MANUVR_DEBUG
  //if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  //#endif
  return busOnline() ? 0 : -1;
}


int8_t I2CAdapter::generateStop() {
  //#ifdef __MANUVR_DEBUG
  //if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  //#endif
  return busOnline() ? 0 : -1;
}


int8_t I2CAdapter::dispatchOperation(I2CBusOp* op) {
  if (op->get_opcode() == BusOpcode::RX) {
    //*(op->buf) = i2cReadByte(op->dev_addr, (uint8_t)op->sub_addr);
    if (HAL_OK != HAL_I2C_Master_Receive_IT(&hi2c1, (uint16_t) op->dev_addr, op->buf, op->buf_len)) {
      op->abort(XferFault::BUS_FAULT);
    }
  }
  else if (op->get_opcode() == BusOpcode::TX) {
    //i2cSendByte(op->dev_addr, (uint8_t)op->sub_addr, *(op->buf));
    if (HAL_OK != HAL_I2C_Master_Transmit_IT(&hi2c1, (uint16_t) op->dev_addr, op->buf, op->buf_len)) {
      op->abort(XferFault::BUS_FAULT);
    }
  }
  else if (op->get_opcode() == BusOpcode::TX_CMD) {
    // Ping
    Kernel::log("I2CAdapter ping not yet supported. :-(\n");
  }
  else {
    op->abort(XferFault::BAD_PARAM);
  }
  return 0;
}

extern "C" {

  static void I2C_TransferConfig(I2C_HandleTypeDef *hi2c,  uint16_t DevAddress, uint8_t Size, uint32_t Mode, uint32_t Request) {
    uint32_t tmpreg = 0;
    /* Check the parameters */
    assert_param(IS_I2C_ALL_INSTANCE(hi2c->Instance));
    assert_param(IS_TRANSFER_MODE(Mode));
    assert_param(IS_TRANSFER_REQUEST(Request));
    /* Get the CR2 register value */
    tmpreg = hi2c->Instance->CR2;
    /* clear tmpreg specific bits */
    tmpreg &= (uint32_t)~((uint32_t)(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP));
    /* update tmpreg */
    tmpreg |= (uint32_t)(((uint32_t)DevAddress & I2C_CR2_SADD) | (((uint32_t)Size << 16 ) & I2C_CR2_NBYTES) | \
              (uint32_t)Mode | (uint32_t)Request);
    /* update CR2 register */
    hi2c->Instance->CR2 = tmpreg;
  }

  /* HAL ISR wrappers. */
  void I2C1_EV_IRQHandler(void) {
    StringBuilder debug_log;
    debug_log.concatf("I2C1_EV_IRQHandler(0x%08x, 0x%08x, 0x%08x)\tstatus: 0x%04x\n", I2C1->CR1, I2C1->CR2, I2C1->ISR, (unsigned long) hi2c1.State);

    /* I2C in mode Transmitter ---------------------------------------------------*/
    if (((__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TXIS) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_ADDR) == SET)) && (__HAL_I2C_GET_IT_SOURCE(&hi2c1, (I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_NACKI | I2C_IT_TXI | I2C_IT_ADDRI)) == SET)) {
      /* Slave mode selected */
      debug_log.concat("Slave mode selected0\n");
    }

    if (((__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TXIS) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET)) && (__HAL_I2C_GET_IT_SOURCE(&hi2c1, (I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_NACKI | I2C_IT_TXI)) == SET)) {
      debug_log.concat("Master mode ALIVE0\n");
      /* Master mode selected */
      if ((hi2c1.State == HAL_I2C_STATE_MASTER_BUSY_TX) || (hi2c1.State == HAL_I2C_STATE_MEM_BUSY_TX)) {
        uint16_t DevAddress;
        if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TXIS) == SET) {
          /* Write data to TXDR */
          hi2c1.Instance->TXDR = (*hi2c1.pBuffPtr++);
          hi2c1.XferSize--;
          hi2c1.XferCount--;
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) {
          if((hi2c1.XferSize == 0)&&(hi2c1.XferCount!=0)) {
            DevAddress = (hi2c1.Instance->CR2 & I2C_CR2_SADD);

            if(hi2c1.XferCount > 255) {
              I2C_TransferConfig(&hi2c1,DevAddress,255, I2C_RELOAD_MODE, I2C_NO_STARTSTOP);
              hi2c1.XferSize = 255;
            }
            else {
              I2C_TransferConfig(&hi2c1,DevAddress,hi2c1.XferCount, I2C_AUTOEND_MODE, I2C_NO_STARTSTOP);
              hi2c1.XferSize = hi2c1.XferCount;
            }
          }
          else {
            /* Wrong size Status regarding TCR flag event */
            hi2c1.ErrorCode |= HAL_I2C_ERROR_SIZE;
            HAL_I2C_ErrorCallback(&hi2c1);
          }
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) {
          if(hi2c1.XferCount == 0) {
            /* Generate Stop */
            hi2c1.Instance->CR2 |= I2C_CR2_STOP;
          }
          else {
            /* Wrong size Status regarding TCR flag event */
            hi2c1.ErrorCode |= HAL_I2C_ERROR_SIZE;
            HAL_I2C_ErrorCallback(&hi2c1);
          }
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) {
          /* Disable ERR, TC, STOP, NACK, TXI interrupt */
          __HAL_I2C_DISABLE_IT(&hi2c1,I2C_IT_ERRI | I2C_IT_TCI| I2C_IT_STOPI| I2C_IT_NACKI | I2C_IT_TXI );
          /* Clear STOP Flag */
          __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_STOPF);
          /* Clear Configuration Register 2 */
          I2C_RESET_CR2(&hi2c1);

          hi2c1.State = HAL_I2C_STATE_READY;

          if(hi2c1.State == HAL_I2C_STATE_MEM_BUSY_TX) {
            HAL_I2C_MemTxCpltCallback(&hi2c1);
          }
          else {
            HAL_I2C_MasterTxCpltCallback(&hi2c1);
          }
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET) {
          /* Clear NACK Flag */
          __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_AF);

          hi2c1.ErrorCode |= HAL_I2C_ERROR_AF;
          HAL_I2C_ErrorCallback(&hi2c1);
        }
      }
    }

    /* I2C in mode Receiver ----------------------------------------------------*/
    if (((__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_RXNE) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_ADDR) == SET)) && (__HAL_I2C_GET_IT_SOURCE(&hi2c1, (I2C_IT_TCI| I2C_IT_STOPI| I2C_IT_NACKI | I2C_IT_RXI | I2C_IT_ADDRI)) == SET)) {
      /* Slave mode selected */
      debug_log.concat("Slave mode selected1\n");
    }
    if (((__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_RXNE) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) || (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET)) && (__HAL_I2C_GET_IT_SOURCE(&hi2c1, (I2C_IT_TCI| I2C_IT_STOPI| I2C_IT_NACKI | I2C_IT_RXI)) == SET)) {
      debug_log.concat("Master mode ALIVE1\n");
      /* Master mode selected */
      if ((hi2c1.State == HAL_I2C_STATE_MASTER_BUSY_RX)) {
        uint16_t DevAddress;
        if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_RXNE) == SET) {
          /* Read data from RXDR */
          (*hi2c1.pBuffPtr++) = hi2c1.Instance->RXDR;
          debug_log.concatf("\t DAT REG = 0x%02x\n", (*hi2c1.pBuffPtr-1));
          hi2c1.XferSize--;
          hi2c1.XferCount--;
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TCR) == SET) {
          debug_log.concat("\t : TCR\n");
          if((hi2c1.XferSize == 0)&&(hi2c1.XferCount!=0)) {
            DevAddress = (hi2c1.Instance->CR2 & I2C_CR2_SADD);

            if(hi2c1.XferCount > 255) {
              debug_log.concat("\t : RELOAD\n");
              I2C_TransferConfig(&hi2c1,DevAddress,255, I2C_RELOAD_MODE, I2C_NO_STARTSTOP);
              hi2c1.XferSize = 255;
            }
            else {
              debug_log.concat("\t : AUTOEND\n");
              I2C_TransferConfig(&hi2c1,DevAddress,hi2c1.XferCount, I2C_AUTOEND_MODE, I2C_NO_STARTSTOP);
              hi2c1.XferSize = hi2c1.XferCount;
            }
          }
          else {
            debug_log.concat("\t : Size error TCR\n");
            /* Wrong size Status regarding TCR flag event */
            hi2c1.ErrorCode |= HAL_I2C_ERROR_SIZE;
            HAL_I2C_ErrorCallback(&hi2c1);
          }
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_TC) == SET) {
          if(hi2c1.XferCount == 0) {
            debug_log.concat("\t : Gen stop\n");
            /* Generate Stop */
            hi2c1.Instance->CR2 |= I2C_CR2_STOP;
          }
          else {
            debug_log.concat("\t : Size error\n");
            /* Wrong size Status regarding TCR flag event */
            hi2c1.ErrorCode |= HAL_I2C_ERROR_SIZE;
            HAL_I2C_ErrorCallback(&hi2c1);
          }
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_STOPF) == SET) {
          /* Disable ERR, TC, STOP, NACK, TXI interrupt */
          __HAL_I2C_DISABLE_IT(&hi2c1,I2C_IT_ERRI | I2C_IT_TCI| I2C_IT_STOPI| I2C_IT_NACKI | I2C_IT_RXI );
          /* Clear STOP Flag */
          __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_STOPF);
          /* Clear Configuration Register 2 */
          I2C_RESET_CR2(&hi2c1);

          hi2c1.State = HAL_I2C_STATE_READY;
          debug_log.concatf("\t : STOPF = 0x%02x\n", hi2c1.Instance->RXDR);

          HAL_I2C_MasterRxCpltCallback(&hi2c1);
        }
        else if(__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_AF) == SET) {
          /* Clear NACK Flag */
          __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_AF);

          hi2c1.ErrorCode |= HAL_I2C_ERROR_AF;
          HAL_I2C_ErrorCallback(&hi2c1);
        }
      }
      else {
        debug_log.concat("\t : UNKNOWN\n");
      }
    }
    Kernel::log(&debug_log);
  }






  void I2C1_ER_IRQHandler(void) {
    StringBuilder debug_log;
    debug_log.concatf("I2C1_ERROR    (0x%08x, 0x%08x, 0x%08x)\n\tstatus: 0x%04x\n", I2C1->CR1, I2C1->CR2, I2C1->ISR, (unsigned long) hi2c1.State);
    Kernel::log(&debug_log);
    HAL_I2C_ER_IRQHandler(&hi2c1);
  }

  /*
  * This is an ISR.
  */
  void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    StringBuilder debug_log;
    debug_log.concatf("HAL_I2C_MasterTxCpltCallback    (0x%08x, 0x%08x, 0x%08x)\n\tstatus: 0x%04x\n", I2C1->CR1, I2C1->CR2, I2C1->ISR, (unsigned long) hi2c1.State);
    if (i2c->current_job != NULL) {
      i2c->current_job->markComplete();
      //i2c->current_job->advance_operation(1);
    }
  }

  /*
  * This is an ISR.
  */
  void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    StringBuilder debug_log;
    debug_log.concatf("HAL_I2C_MasterRxCpltCallback    (0x%08x, 0x%08x, 0x%08x)\n\tstatus: 0x%04x\n", I2C1->CR1, I2C1->CR2, I2C1->ISR, (unsigned long) hi2c1.State);
    if (i2c->current_job != NULL) {
      i2c->current_job->markComplete();
      //i2c->current_job->advance_operation(1);
    }
    if (0x53 == *(i2c->current_job->buf)) {

    }
  }

  /*
  * This is an ISR.
  */
  void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    StringBuilder debug_log;
    debug_log.concatf("HAL_I2C_ErrorCallback    (0x%08x, 0x%08x, 0x%08x)\n\tstatus: 0x%04x\n", I2C1->CR1, I2C1->CR2, I2C1->ISR, (unsigned long) hi2c1.State);
    if (i2c->current_job != NULL) {
      i2c->current_job->abort(XferFault::HUNG_IRQ);
    }
  }
}   // extern "C"

#endif   // MANUVR_SUPPORT_I2C
