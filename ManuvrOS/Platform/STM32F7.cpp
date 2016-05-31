/*
File:   STM32F7.cpp
Author: J. Ian Lindsay
Date:   2015.12.02

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


This file is meant to contain a set of common functions that are typically platform-dependent.
  The goal is to make a class instance that is pre-processor-selectable for instantiation by the
  kernel, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)
*/

#include "Platform.h"
#include "STM32F7.h"

#include <unistd.h>


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/

#if defined(ENABLE_USB_VCP)
  #include "tm_stm32_usb_device.h"
  #include "tm_stm32_usb_device_cdc.h"
#endif

volatile static PlatformEXTIDef __ext_line_bindings[16];

/* Takes a pin and returns the IRQ channel. */
IRQn_Type _get_stm32f7_irq_channel(uint8_t _pin) {
  switch (_pin % 16) {
    case 0:  return EXTI0_IRQn;
    case 1:  return EXTI1_IRQn;
    case 2:  return EXTI2_IRQn;
    case 3:  return EXTI3_IRQn;
    case 4:  return EXTI4_IRQn;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:  return EXTI9_5_IRQn;
    default: return EXTI15_10_IRQn;
  }
}


void printEXTIDef(uint8_t _pin, StringBuilder* output) {
  uint16_t pin_idx   = (_pin % 16);
  const char* _msg_name = "<NONE>";
  if (NULL != __ext_line_bindings[pin_idx].event) {
    _msg_name = ManuvrMsg::getMsgTypeString(__ext_line_bindings[pin_idx].event->event_code);
  }
  output->concatf("\t---< EXTI Def %d >----------\n", pin_idx);
  output->concatf("\tPin         %d\n", __ext_line_bindings[pin_idx].pin);
  output->concatf("\tCondition   %d\n", getIRQConditionString(__ext_line_bindings[pin_idx].condition));
  output->concatf("\tFXN         0x%08x\n", (uint32_t) __ext_line_bindings[pin_idx].fxn);
  output->concatf("\tEvent       %s\n\n", _msg_name);
}


volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;
unsigned long     start_time_micros  = 0;


/*
* Trivial, since the period of systick is 1ms, and we are already
*   keeping track of how many times it has rolled over. Simply return
*   that value.
* Returns the number of milliseconds since reset.
*/
unsigned long millis(void) {  return millis_since_reset;  }


#define MCK 16000000
/**
* Written by Magnus Lundin.
* https://github.com/mlu/arduino-stm32/blob/master/hardware/cores/stm32/wiring.c
*
* @return  unsigned long  The number of microseconds since reset.
*/
unsigned long micros(void) {
  long v0 = SysTick->VAL;      // Glitch free clock
  long c0 = millis_since_reset;
  long v1 = SysTick->VAL;
  long c1 = millis_since_reset;
  if (v1 < v0) {             // Downcounting, no systick rollover
    return c0*8000-v1/(MCK/8000000UL);
    //return (unsigned long) millis_since_reset/1000000;
  }
  else { // systick rollover, use last count value
    return c1*8000-v1/(MCK/8000000UL);
    //return (unsigned long) millis_since_reset/1000000;
  }
  return 0;
}




/****************************************************************************************************
* Randomness                                                                                        *
****************************************************************************************************/
volatile uint32_t next_random_int[PLATFORM_RNG_CARRY_CAPACITY];

/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomInt() {
  while (!(RNG->SR & (RNG_SR_DRDY)));
  uint32_t return_value = RNG->DR;
  return return_value;
}

/**
* Called by the RNG ISR to provide new random numbers.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool provide_random_int(uint32_t nu_rnd) {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) {
    if (next_random_int[i] == 0) {
      next_random_int[i] = nu_rnd;
      return (i == PLATFORM_RNG_CARRY_CAPACITY-1) ? false : true;
    }
  }
  return false;
}

/*
* Init the RNG. Short and sweet.
*/
void init_RNG() {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) next_random_int[i] = 0;
  __HAL_RCC_RNG_CLK_ENABLE();
  RNG->CR |= RNG_CR_RNGEN;
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
int platformSerialNumberSize() { return 12; }

int getSerialNumber(uint8_t* buf) {
  *((uint32_t*)buf + 0) = *((uint32_t*) 0x1FF0F420);
  *((uint32_t*)buf + 4) = *((uint32_t*) 0x1FF0F424);
  *((uint32_t*)buf + 8) = *((uint32_t*) 0x1FF0F428);
  return 12;
}


/****************************************************************************************************
* Time and date                                                                                     *
****************************************************************************************************/
uint32_t rtc_startup_state = MANUVR_RTC_STARTUP_UNINITED;


/*
*
*/
bool initPlatformRTC() {
  return true;
}

/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool setTimeAndDate(char* nu_date_time) {
  return false;
}



/*
* Returns an integer representing the current datetime.
*/
uint32_t epochTime(void) {
  return 0;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != NULL) {
  }
}



/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/

/*
*
*/
volatile PlatformGPIODef gpio_pins[PLATFORM_GPIO_PIN_COUNT];


/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
  // Null-out all the pin definitions in preparation for assignment.
  for (uint8_t i = 0; i < PLATFORM_GPIO_PIN_COUNT; i++) {
    gpio_pins[i].event = 0;      // No event assigned.
    gpio_pins[i].fxn   = 0;      // No function pointer.
    gpio_pins[i].mode  = INPUT;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }
  for (uint8_t i = 0; i < 16; i++) {
    /* Zero all the EXTI definitions */
    __ext_line_bindings[i].event     = 0;
    __ext_line_bindings[i].fxn       = 0;
    __ext_line_bindings[i].condition = 0;
    __ext_line_bindings[i].pin       = 0;
  }

  /* GPIO Ports Clock Enable */
  //__GPIOE_CLK_ENABLE();
  //__GPIOB_CLK_ENABLE();
  //__GPIOG_CLK_ENABLE();
  //__GPIOD_CLK_ENABLE();
  //__GPIOC_CLK_ENABLE();
  //__GPIOA_CLK_ENABLE();
  //__GPIOI_CLK_ENABLE();
  //__GPIOH_CLK_ENABLE();
  //__GPIOF_CLK_ENABLE();
}



/**
* On STM32 parts, we construe an 8-bit pin number to be a linear reference to
*   successive 16-bit ports. See STM32F7.h for clarification.
*/
int8_t gpioDefine(uint8_t pin, int mode) {
  //if (pin < PLATFORM_GPIO_PIN_COUNT) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;
    GPIO_InitStruct.Mode   = GPIO_MODE_INPUT;

    switch (mode) {
      case INPUT_PULLUP:
        GPIO_InitStruct.Pull   = GPIO_PULLUP;
        break;
      case INPUT_PULLDOWN:
        GPIO_InitStruct.Pull   = GPIO_PULLDOWN;
        break;
      case OUTPUT:
        GPIO_InitStruct.Mode   = GPIO_MODE_OUTPUT_PP;
        break;
      case OUTPUT_OD:
        GPIO_InitStruct.Mode   = GPIO_MODE_OUTPUT_OD;
        break;
      case INPUT:
        // Nothing needs to happen. This is the default we set above.
        break;
      default:
        Kernel::log("Tried to define a GPIO pin with a mode that we don't handle.\n");
        GPIO_InitStruct.Mode   = GPIO_MODE_ANALOG;
        break;
    }

    HAL_GPIO_Init(_associated_port(pin), &GPIO_InitStruct);
  //}
  //else {
    //Kernel::log("Tried to define a GPIO pin that was out-of-range for this platform.\n");
  //}
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t _pin, uint8_t condition, ManuvrRunnable* isr_event) {
  uint16_t pin_idx   = (_pin % 16);
  GPIO_TypeDef* port = _associated_port(_pin);

  if (0 == __ext_line_bindings[pin_idx].condition) {
    /* There is not presently an interrupt condition on this pin.
       Proceed with impunity.  */
    __ext_line_bindings[pin_idx].pin       = _pin;
    __ext_line_bindings[pin_idx].condition = condition;
    __ext_line_bindings[pin_idx].event     = isr_event;

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(_pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;

    switch (condition) {
      case RISING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        break;
      case FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        break;
      case CHANGE:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
      case RISING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case FALLING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case CHANGE_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case RISING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case FALLING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case CHANGE_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      default:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    }
    HAL_GPIO_Init(_associated_port(_pin), &GPIO_InitStruct);
    IRQn_Type chan = _get_stm32f7_irq_channel(_pin);
    HAL_NVIC_SetPriority(chan, 0, pin_idx);  // IRQ channel, priority, pin idex.
    HAL_NVIC_EnableIRQ(chan);
  }
  else if (NULL == __ext_line_bindings[pin_idx].event) {
    /* No event already occupies the slot, but the def is active. Probably
         already has a FXN assigned. Ignore the supplied condition and add this
         event to the list of things to do. */
    __ext_line_bindings[pin_idx].event     = isr_event;
  }
  else {
    StringBuilder local_log("Tried to clobber an existing IRQ event association...\n");
    printEXTIDef(_pin, &local_log);
    Kernel::log(&local_log);
    return -1;
  }
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t _pin, uint8_t condition, FunctionPointer fxn) {
  uint16_t pin_idx   = (_pin % 16);
  GPIO_TypeDef* port = _associated_port(_pin);

  if (0 == __ext_line_bindings[pin_idx].condition) {
    /* There is not presently an interrupt condition on this pin.
       Proceed with impunity.  */
    __ext_line_bindings[pin_idx].pin       = _pin;
    __ext_line_bindings[pin_idx].condition = condition;
    __ext_line_bindings[pin_idx].fxn       = fxn;

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(_pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;

    switch (condition) {
      case RISING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        break;
      case FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        break;
      case CHANGE:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
      case RISING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case FALLING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case CHANGE_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case RISING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case FALLING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case CHANGE_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      default:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    }
    HAL_GPIO_Init(_associated_port(_pin), &GPIO_InitStruct);
    IRQn_Type chan = _get_stm32f7_irq_channel(_pin);
    HAL_NVIC_SetPriority(chan, 1, pin_idx);  // IRQ channel, priority, pin idex.
    HAL_NVIC_EnableIRQ(chan);
  }
  else if (NULL == __ext_line_bindings[pin_idx].fxn) {
    /* No fxn already occupies the slot, but the def is active. Probably
         already has an event assigned. Ignore the supplied condition and add this
         fxn to the list of things to do. */
    __ext_line_bindings[pin_idx].fxn = fxn;
  }
  else {
    StringBuilder local_log("Tried to clobber an existing IRQ event association...\n");
    printEXTIDef(_pin, &local_log);
    Kernel::log(&local_log);
    return -1;
  }
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  HAL_GPIO_WritePin(_associated_port(pin), _associated_pin(pin), (val?GPIO_PIN_SET:GPIO_PIN_RESET));
  return 0;
}


int8_t readPin(uint8_t pin) {
  return (GPIO_PIN_SET == HAL_GPIO_ReadPin(_associated_port(pin), _associated_pin(pin)));
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}


/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/




/****************************************************************************************************
* Interrupt-masking                                                                                 *
****************************************************************************************************/

void globalIRQEnable() {     asm volatile ("cpsid i");    }
void globalIRQDisable() {    asm volatile ("cpsie i");    }



/****************************************************************************************************
* Process control                                                                                   *
****************************************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
volatile void seppuku() {
  // This means "Halt" on a base-metal build.
  globalIRQDisable();
  HAL_RCC_DeInit();               // Switch to HSI, no PLL
  while(true);
}


/*
* Jump to the bootloader.
* Never returns.
*/
volatile void jumpToBootloader() {
  globalIRQDisable();
  TM_USBD_Stop(TM_USB_FS);    // DeInit() The USB device.
  __set_MSP(0x20001000);      // Set the main stack pointer...
  HAL_RCC_DeInit();               // Switch to HSI, no PLL

  // Per clive1's post, set some sort of key value just below the initial stack pointer.
  // We don't really care if we clobber something, because this fxn will reboot us. But
  // when the reset handler is executed, it will look for this value. If it finds it, it
  // will branch to the Bootloader code.
  *((unsigned long *)0x20001FF0) = 0xb00710ad;
  NVIC_SystemReset();
}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
volatile void hardwareShutdown() {
  globalIRQDisable();
  NVIC_SystemReset();
}

/*
* Causes immediate reboot.
* Never returns.
*/
volatile void reboot() {
  globalIRQDisable();
  TM_USBD_Stop(TM_USB_FS);    // DeInit() The USB device.
  __set_MSP(0x20001000);      // Set the main stack pointer...
  HAL_RCC_DeInit();               // Switch to HSI, no PLL

  // We do not want to enter the bootloader....
  *((unsigned long *)0x2000FFF0) = 0;
  NVIC_SystemReset();
}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
void platformPreInit() {
  gpioSetup();
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
void platformInit() {
  start_time_micros = micros();
  init_RNG();
  #if defined(ENABLE_USB_VCP)
    TM_USB_Init();    /* Init USB peripheral as VCP */
    TM_USBD_CDC_Init(TM_USB_FS);
    TM_USBD_Start(TM_USB_FS);
  #endif
}






/*
*
*/
void EXTI0_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR0)) {
    EXTI->PR = EXTI_PR_PR0;   // Clear service bit.
    //Kernel::log("EXTI 0\n");
    if (NULL != __ext_line_bindings[0].fxn) {
      __ext_line_bindings[0].fxn();
    }
    if (NULL != __ext_line_bindings[0].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[0].event);
    }
  }
}


/*
*
*/
void EXTI1_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR1)) {
    EXTI->PR = EXTI_PR_PR1;   // Clear service bit.
    //Kernel::log("EXTI 1\n");
    if (NULL != __ext_line_bindings[1].fxn) {
      __ext_line_bindings[1].fxn();
    }
    if (NULL != __ext_line_bindings[1].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[1].event);
    }
  }
}


/*
*
*/
void EXTI2_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR2)) {
    EXTI->PR = EXTI_PR_PR2;   // Clear service bit.
    Kernel::log("EXTI 2\n");
    if (NULL != __ext_line_bindings[2].fxn) {
      __ext_line_bindings[2].fxn();
    }
    if (NULL != __ext_line_bindings[2].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[2].event);
    }
  }
}


/*
*
*/
void EXTI3_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR3)) {
    EXTI->PR = EXTI_PR_PR3;   // Clear service bit.
    Kernel::log("EXTI 3\n");
    if (NULL != __ext_line_bindings[3].fxn) {
      __ext_line_bindings[3].fxn();
    }
    if (NULL != __ext_line_bindings[3].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[3].event);
    }
  }
}


/*
*
*/
void EXTI4_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR4)) {
    EXTI->PR = EXTI_PR_PR4;   // Clear service bit.
    Kernel::log("EXTI 4\n");
    if (NULL != __ext_line_bindings[4].fxn) {
      __ext_line_bindings[4].fxn();
    }
    if (NULL != __ext_line_bindings[4].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[4].event);
    }
  }
}


/*
*
*/
void EXTI9_5_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR5)) {
    EXTI->PR = EXTI_PR_PR5;   // Clear service bit.
    Kernel::log("EXTI 5\n");
    if (NULL != __ext_line_bindings[5].fxn) {
      __ext_line_bindings[5].fxn();
    }
    if (NULL != __ext_line_bindings[5].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[5].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR6)) {
    EXTI->PR = EXTI_PR_PR6;   // Clear service bit.
    Kernel::log("EXTI 6\n");
    if (NULL != __ext_line_bindings[6].fxn) {
      __ext_line_bindings[6].fxn();
    }
    if (NULL != __ext_line_bindings[6].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[6].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR7)) {
    EXTI->PR = EXTI_PR_PR7;   // Clear service bit.
    Kernel::log("EXTI 7\n");
    if (NULL != __ext_line_bindings[7].fxn) {
      __ext_line_bindings[7].fxn();
    }
    if (NULL != __ext_line_bindings[7].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[7].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR8)) {
    EXTI->PR = EXTI_PR_PR8;   // Clear service bit.
    Kernel::log("EXTI 8\n");
    if (NULL != __ext_line_bindings[8].fxn) {
      __ext_line_bindings[8].fxn();
    }
    if (NULL != __ext_line_bindings[8].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[8].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR9)) {
    EXTI->PR = EXTI_PR_PR9;   // Clear service bit.
    Kernel::log("EXTI 9\n");
    if (NULL != __ext_line_bindings[9].fxn) {
      __ext_line_bindings[9].fxn();
    }
    if (NULL != __ext_line_bindings[9].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[9].event);
    }
  }
}


/*
*
*/
void EXTI15_10_IRQHandler(void) {
  if (EXTI->PR & (EXTI_PR_PR11)) {
    EXTI->PR = EXTI_PR_PR11;   // Clear service bit.
    Kernel::log("EXTI 11\n");
    if (NULL != __ext_line_bindings[11].fxn) {
      __ext_line_bindings[11].fxn();
    }
    if (NULL != __ext_line_bindings[11].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[11].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR12)) {
    EXTI->PR = EXTI_PR_PR12;   // Clear service bit.
    Kernel::log("EXTI 12\n");
    if (NULL != __ext_line_bindings[12].fxn) {
      __ext_line_bindings[12].fxn();
    }
    if (NULL != __ext_line_bindings[12].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[12].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR13)) {
    EXTI->PR = EXTI_PR_PR13;   // Clear service bit.
    Kernel::log("EXTI 13\n");
    if (NULL != __ext_line_bindings[13].fxn) {
      __ext_line_bindings[13].fxn();
    }
    if (NULL != __ext_line_bindings[13].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[13].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR14)) {
    EXTI->PR = EXTI_PR_PR14;   // Clear service bit.
    Kernel::log("EXTI 14\n");
    if (NULL != __ext_line_bindings[14].fxn) {
      __ext_line_bindings[14].fxn();
    }
    if (NULL != __ext_line_bindings[14].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[14].event);
    }
  }
  if (EXTI->PR & (EXTI_PR_PR15)) {
    EXTI->PR = EXTI_PR_PR15;   // Clear service bit.
    Kernel::log("EXTI 15\n");
    if (NULL != __ext_line_bindings[15].fxn) {
      __ext_line_bindings[15].fxn();
    }
    if (NULL != __ext_line_bindings[15].event) {
      Kernel::isrRaiseEvent(__ext_line_bindings[15].event);
    }
  }
}
