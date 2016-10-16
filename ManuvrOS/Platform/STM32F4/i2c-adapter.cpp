#include <Drivers/i2c-adapter/i2c-adapter.h>

#if defined(MANUVR_SUPPORT_I2C)
extern "C" {
  #include <stm32f4xx.h>
  #include <stm32f4xx_i2c.h>
  #include <stm32f4xx_gpio.h>
  #include "stm32f4xx_it.h"
  I2C_HandleTypeDef hi2c1;
}


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

#endif   // MANUVR_SUPPORT_I2C
