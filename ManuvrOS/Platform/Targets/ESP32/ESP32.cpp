/*
File:   ESP32.cpp
Author: J. Ian Lindsay
Date:   2015.11.01

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


#include <CommonConstants.h>
#include <Platform/Platform.h>
#include "ESP32.h"

#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(MANUVR_STORAGE)
//#include <Platform/Targets/ESP32/ESP32Storage.h>
#endif

#if defined(MANUVR_CONSOLE_SUPPORT)
#include <Transports/StandardIO/StandardIO.h>
#include <XenoSession/Console/ManuvrConsole.h>
#endif

volatile PlatformGPIODef gpio_pins[PLATFORM_GPIO_PIN_COUNT];


/*******************************************************************************
* Watchdog                                                                     *
*******************************************************************************/


/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
volatile uint32_t     randomness_pool[PLATFORM_RNG_CARRY_CAPACITY];
volatile unsigned int _random_pool_r_ptr = 0;
volatile unsigned int _random_pool_w_ptr = 0;
long unsigned int rng_thread_id = 0;

/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (randomness_pool != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomInt() {
  return randomness_pool[_random_pool_r_ptr++ % PLATFORM_RNG_CARRY_CAPACITY];
}


/**
* This is a thread to keep the randomness pool flush.
*/
static void dev_urandom_reader(void* unused_param) {
  unsigned int rng_level    = 0;

  while (1) {
    rng_level = _random_pool_w_ptr - _random_pool_r_ptr;
    if (rng_level == PLATFORM_RNG_CARRY_CAPACITY) {
      // We have filled our entropy pool. Sleep.
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    else {
      randomness_pool[_random_pool_w_ptr++ % PLATFORM_RNG_CARRY_CAPACITY] = *((uint32_t*) 0x3FF75144);  // 32-bit RNG data register.
    }
  }
}



/*******************************************************************************
*  ___   _           _      ___
* (  _`\(_ )        ( )_  /'___)
* | |_) )| |    _ _ | ,_)| (__   _    _ __   ___ ___
* | ,__/'| |  /'_` )| |  | ,__)/'_`\ ( '__)/' _ ` _ `\
* | |    | | ( (_| || |_ | |  ( (_) )| |   | ( ) ( ) |
* (_)   (___)`\__,_)`\__)(_)  `\___/'(_)   (_) (_) (_)
* These are overrides and additions to the platform class.
*******************************************************************************/
void ESP32Platform::printDebug(StringBuilder* output) {
  output->concatf("==< ESP32 [%s] >==================================\n", getPlatformStateStr(platformState()));
  output->concatf("-- ESP-IDF version:    %s\n", esp_get_idf_version());
  ManuvrPlatform::printDebug(output);
}



/*******************************************************************************
* Identity and serial number                                                   *
*******************************************************************************/
/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int platformSerialNumberSize() {
  return 16;
}


/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  return 16;
}



/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/
/*
* Taken from:
* https://github.com/espressif/arduino-esp32
*/
unsigned long IRAM_ATTR millis() {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/*
* Taken from:
* https://github.com/espressif/arduino-esp32
*/
unsigned long IRAM_ATTR micros() {
  uint32_t ccount;
  __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
  return ccount / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
}


/*
*
*/
bool init_rtc() {
  return false;
}

/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool setTimeAndDateStr(char* nu_date_time) {
  return false;
}

/*
*/
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t epochTime() {
  return 0;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target) {
  }
}



/*******************************************************************************
* GPIO and change-notice                                                       *
*******************************************************************************/
void pin_isr_pitch_event() {
}


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
    gpio_pins[i].mode  = (int) GPIOMode::INPUT;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }
}


int8_t gpioDefine(uint8_t pin, GPIOMode mode) {
  gpio_config_t io_conf;
  if (!GPIO_IS_VALID_GPIO(pin)) {
    return -1;
  }

  // Handle the pull-up/down stuff first.
  switch (mode) {
    case GPIOMode::BIDIR_OD_PULLUP:
    case GPIOMode::INPUT_PULLUP:
      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
      break;
    case GPIOMode::INPUT_PULLDOWN:
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
      break;
    default:
      break;
  }

  switch (mode) {
    case GPIOMode::BIDIR_OD_PULLUP:
      if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
        return -1;
      }
      io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
      break;
    case GPIOMode::BIDIR_OD:
    case GPIOMode::OUTPUT_OD:
    case GPIOMode::OUTPUT:
      if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
        return -1;
      }
      io_conf.mode = (gpio_mode_t) mode;
      break;

    case GPIOMode::INPUT_PULLUP:
    case GPIOMode::INPUT_PULLDOWN:
    case GPIOMode::INPUT:
      io_conf.mode = GPIO_MODE_INPUT;
      break;

    default:
      Kernel::log("Unknown GPIO mode.\n");
      return -1;
  }

  // disable interrupt
  io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_DISABLE;
  io_conf.pin_bit_mask = (1 << pin);

  //configure GPIO with the given settings
  gpio_config(&io_conf);
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
  //detachInterrupt(pin);
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrMsg* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FxnPointer fxn) {
  //attachInterrupt(pin, fxn, condition);
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  return (int8_t) gpio_set_level((gpio_num_t) pin, val?1:0);
}


int8_t readPin(uint8_t pin) {
  return (int8_t) gpio_get_level((gpio_num_t) pin);
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return 0;
}


/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/
#if defined(MANUVR_STORAGE)
  // Called during boot to load configuration.
  int8_t ESP32Platform::_load_config() {
    if (_storage_device) {
      if (_storage_device->isMounted()) {
      }
    }
    return -1;
  }
#endif



/*******************************************************************************
* Interrupt-masking                                                            *
*******************************************************************************/

#if defined (__MANUVR_FREERTOS)
  void globalIRQEnable() {    } // taskENABLE_INTERRUPTS();    }
  void globalIRQDisable() {   } // taskDISABLE_INTERRUPTS();   }
#else
#endif


/*******************************************************************************
* Process control                                                              *
*******************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
void ESP32Platform::seppuku() {
  reboot();
}


/*
* Jump to the bootloader.
* Never returns.
*/
void ESP32Platform::jumpToBootloader() {
  esp_restart();
}


/*******************************************************************************
* Underlying system control.                                                   *
*******************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
void ESP32Platform::hardwareShutdown() {
  while(true) {
    sleep_millis(60000);
  }
}

/*
* Causes immediate reboot.
* Never returns.
*/
void ESP32Platform::reboot() {
  esp_restart();
}



/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_INNATE_DATETIME | \
              MANUVR_PLAT_FLAG_SERIALED | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t ESP32Platform::platformPreInit(Argument* root_config) {
  ManuvrPlatform::platformPreInit(root_config);
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) randomness_pool[i] = 0;
  _alter_flags(true, DEFAULT_PLATFORM_FLAGS);

  rng_thread_id = xTaskCreate(&dev_urandom_reader, "urandom_reader", 512, nullptr, 1, nullptr);
  if (rng_thread_id) {
    _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);
  }

  if (init_rtc()) {
    _alter_flags(true, MANUVR_PLAT_FLAG_RTC_SET);
  }
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);
  gpioSetup();

  if (root_config) {
  }
  #if defined(MANUVR_CONSOLE_SUPPORT)
    // TODO: This is a leak, for sure. Ref-count EventReceivers.
    StandardIO* _console_xport = new StandardIO();
    ManuvrConsole* _console = new ManuvrConsole((BufferPipe*) _console_xport);
    _kernel.subscribe((EventReceiver*) _console);
    _kernel.subscribe((EventReceiver*) _console_xport);
  #endif

  #if defined(MANUVR_STORAGE)
    //_storage_device = (Storage*) &_t_storage;
    //_kernel.subscribe((EventReceiver*) &_t_storage);
  #endif
  return 0;
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
int8_t ESP32Platform::platformPostInit() {
  #if defined (__MANUVR_FREERTOS)
  #else
  // No threads. We are responsible for pinging our own scheduler.
  // Turn on the periodic interrupts...
  #endif
  return 0;
}
