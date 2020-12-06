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
#include "soc/efuse_reg.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp32/rom/ets_sys.h"


#if defined(CONFIG_MANUVR_STORAGE)
  #include "ESP32Storage.h"
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
volatile static uint32_t     randomness_pool[PLATFORM_RNG_CARRY_CAPACITY];
volatile static unsigned int _random_pool_r_ptr = 0;
volatile static unsigned int _random_pool_w_ptr = 0;
static long unsigned int rng_thread_id = 0;
static bool using_wifi_peripheral = false;
static bool using_adc1            = false;
static bool using_adc2            = false;

/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (randomness_pool != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t IRAM_ATTR randomUInt32() {
  return randomness_pool[_random_pool_r_ptr++ % PLATFORM_RNG_CARRY_CAPACITY];
}


/**
* This is a thread to keep the randomness pool flush.
*/
static void IRAM_ATTR dev_urandom_reader(void* unused_param) {
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
  output->concatf("-- Heap Free/Minimum:  %u/%u\n", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
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
  return 6;
}


/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  esp_efuse_mac_get_default(buf);
  return 6;
}



/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/
/*
* Taken from:
* https://github.com/espressif/arduino-esp32
*/
long unsigned IRAM_ATTR millis() {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/*
* Taken from:
* https://github.com/espressif/arduino-esp32
*/
long unsigned IRAM_ATTR micros() {
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
uint64_t epochTime() {
  struct timeval tv;
  return (0 == gettimeofday(&tv, nullptr)) ? 0 : tv.tv_sec;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != nullptr) {
    time_t now;
    struct tm tstruct;
    time(&now);
    localtime_r(&now, &tstruct);
    char buf[64];
    memset(buf, 0, 64);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    target->concat(buf);
  }
}


/**
* Delay execution for such and so many microseconds.
*/
void sleep_us(uint32_t udelay) {
  ets_delay_us(udelay);
}



/*******************************************************************************
* GPIO and change-notice                                                       *
*******************************************************************************/
static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static void gpio_task_handler(void* arg) {
  uint32_t io_num;
  for(;;) {
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      if (GPIO_IS_VALID_GPIO(io_num)) {
        if (nullptr != gpio_pins[io_num].fxn) {
          gpio_pins[io_num].fxn();
        }
        if (nullptr != gpio_pins[io_num].event) {
          Kernel::isrRaiseEvent(gpio_pins[io_num].event);
        }
      }
    }
  }
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
    gpio_pins[i].mode  = GPIOMode::INPUT;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(gpio_task_handler, "gpiotsk", 2048, NULL, 10, NULL);
  gpio_install_isr_service(0);
}




adc1_channel_t _gpio_analog_get_chan1_from_pin(uint8_t pin) {
  switch (pin) {
    case 32:    return ADC1_CHANNEL_4;
    case 33:    return ADC1_CHANNEL_5;
    case 34:    return ADC1_CHANNEL_6;
    case 35:    return ADC1_CHANNEL_7;
    case 36:    return ADC1_CHANNEL_0;
    case 37:    return ADC1_CHANNEL_1;
    case 38:    return ADC1_CHANNEL_2;
    case 39:    return ADC1_CHANNEL_3;
    default:    return ADC1_CHANNEL_MAX;   // Invalid analog input pin.
  }
}

adc2_channel_t _gpio_analog_get_chan2_from_pin(uint8_t pin) {
  switch (pin) {
    case 0:     return ADC2_CHANNEL_1;
    case 2:     return ADC2_CHANNEL_2;
    case 4:     return ADC2_CHANNEL_0;
    case 12:    return ADC2_CHANNEL_5;
    case 13:    return ADC2_CHANNEL_4;
    case 14:    return ADC2_CHANNEL_6;
    case 15:    return ADC2_CHANNEL_3;
    case 25:    return ADC2_CHANNEL_8;
    case 26:    return ADC2_CHANNEL_9;
    case 27:    return ADC2_CHANNEL_7;
    default:    return ADC2_CHANNEL_MAX;   // Invalid analog input pin.
  }
}



int8_t _gpio_analog_in_pin_setup(uint8_t pin) {
  switch (pin) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
      {
        adc1_channel_t chan = _gpio_analog_get_chan1_from_pin(pin);
        if (!using_adc1) {
          using_adc1 = (ESP_OK == adc1_config_width(ADC_WIDTH_BIT_12));
        }
        adc1_config_channel_atten(chan, ADC_ATTEN_DB_11);
        gpio_pins[pin].mode  = GPIOMode::ANALOG_IN;
      }
      return 0;

    case 0:
    case 2:
    case 4:
    case 12:
    case 13:
    case 14:
    case 15:
    case 25:
    case 26:
    case 27:
      if (!using_wifi_peripheral) {
        adc2_channel_t chan = _gpio_analog_get_chan2_from_pin(pin);
        if (!using_adc2) {
          using_adc2 = true;
        }
        adc2_config_channel_atten(chan, ADC_ATTEN_DB_11);
        gpio_pins[pin].mode  = GPIOMode::ANALOG_IN;
        return 0;
      }
      return -1;   // Can't use ADC while wifi peripheral is using it.

    default:    return -1;   // Invalid analog input pin.
  }
}



int8_t pinMode(uint8_t pin, GPIOMode mode) {
  gpio_config_t io_conf;
  if (!GPIO_IS_VALID_GPIO(pin)) {
    return -1;
  }

  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  io_conf.intr_type    = (gpio_int_type_t) GPIO_PIN_INTR_DISABLE;
  io_conf.pin_bit_mask = (uint64_t) ((uint64_t)1 << pin);

  // Handle the pull-up/down stuff first.
  switch (mode) {
    case GPIOMode::BIDIR_OD_PULLUP:
    case GPIOMode::INPUT_PULLUP:
      io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
      break;
    case GPIOMode::INPUT_PULLDOWN:
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      break;
    default:
      break;
  }

  switch (mode) {
    case GPIOMode::ANALOG_IN:
      return _gpio_analog_in_pin_setup(pin);

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

  gpio_config(&io_conf);
  gpio_pins[pin].mode = mode;
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
  if (GPIO_IS_VALID_GPIO(pin)) {
    gpio_isr_handler_remove((gpio_num_t) pin);
    gpio_pins[pin].event = 0;      // No event assigned.
    gpio_pins[pin].fxn   = 0;      // No function pointer.
  }
}



int8_t setPinEvent(uint8_t pin, IRQCondition condition, ManuvrMsg* isr_event) {
  if (0 == setPinFxn(pin, condition, gpio_pins[pin].fxn)) {
    gpio_pins[pin].event = isr_event;
    return 0;
  }
  return -1;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, IRQCondition condition, FxnPointer fxn) {
  if (!GPIO_IS_VALID_GPIO(pin)) {
    ESP_LOGW("ESP32Platform", "GPIO %u is invalid\n", pin);
    return -1;
  }
  gpio_config_t io_conf;
  io_conf.pin_bit_mask = (1 << pin);
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  switch (gpio_pins[pin].mode) {
    case GPIOMode::INPUT_PULLUP:
      io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
      break;
    case GPIOMode::INPUT_PULLDOWN:
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      break;
    case GPIOMode::INPUT:
      break;
    default:
      ESP_LOGE("ESP32Platform", "GPIO %u is not an input.\n", pin);
      return -1;
  }

  switch (condition) {
    case IRQCondition::CHANGE:
      io_conf.intr_type = (gpio_int_type_t) GPIO_INTR_ANYEDGE;
      break;
    case IRQCondition::RISING:
      io_conf.intr_type = (gpio_int_type_t) GPIO_INTR_POSEDGE;
      break;
    case IRQCondition::FALLING:
      io_conf.intr_type = (gpio_int_type_t) GPIO_INTR_NEGEDGE;
      break;
    default:
      ESP_LOGE("ESP32Platform", "IRQCondition is invalid for pin %u\n", pin);
      return -1;
  }

  gpio_pins[pin].fxn = fxn;
  gpio_config(&io_conf);
  gpio_set_intr_type((gpio_num_t) pin, GPIO_INTR_ANYEDGE);
  gpio_isr_handler_add((gpio_num_t) pin, gpio_isr_handler, (void*) (0L + pin));
  return 0;
}


int8_t IRAM_ATTR setPin(uint8_t pin, bool val) {
  return (int8_t) gpio_set_level((gpio_num_t) pin, val?1:0);
}


int8_t IRAM_ATTR readPin(uint8_t pin) {
  return (int8_t) gpio_get_level((gpio_num_t) pin);
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}


int readPinAnalog(uint8_t pin) {
  int val = 0;
  switch (pin) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
      if (using_adc1) {
        adc1_channel_t chan = _gpio_analog_get_chan1_from_pin(pin);
        val = adc1_get_raw(chan);
      }
      break;

    case 0:
    case 2:
    case 4:
    case 12:
    case 13:
    case 14:
    case 15:
    case 25:
    case 26:
    case 27:
      if (using_adc2) {
        adc2_channel_t chan = _gpio_analog_get_chan2_from_pin(pin);
        adc2_get_raw(chan, ADC_WIDTH_BIT_12, &val);
      }
      break;

    default:
      break;
  }
  return val;
}



/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/
#if defined(CONFIG_MANUVR_STORAGE)
  ESP32Storage _esp_storage(nullptr);

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

#if defined (__BUILD_HAS_FREERTOS)
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
    sleep_ms(60000);
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

  rng_thread_id = xTaskCreate(&dev_urandom_reader, "rnd_rdr", 580, nullptr, 1, nullptr);
  if (rng_thread_id) {
    _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);
  }

  if (init_rtc()) {
    _alter_flags(true, MANUVR_PLAT_FLAG_RTC_SET);
  }
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);
  gpioSetup();

  #if defined(CONFIG_MANUVR_STORAGE)
    _storage_device = (Storage*) &_esp_storage;
    _kernel.subscribe((EventReceiver*) &_esp_storage);
    _alter_flags(true, MANUVR_PLAT_FLAG_HAS_STORAGE);
  #endif

  if (root_config) {
    char* tz_string = nullptr;
    if (root_config->getValueAs("tz", &tz_string)) {
      setenv("TZ", tz_string, 1);
      tzset();
    }
  }
  #if defined(MANUVR_CONSOLE_SUPPORT)
    // TODO: This is a leak, for sure. Ref-count EventReceivers.
    StandardIO* _console_xport = new StandardIO();
    ManuvrConsole* _console = new ManuvrConsole((BufferPipe*) _console_xport);
    _kernel.subscribe((EventReceiver*) _console);
    _kernel.subscribe((EventReceiver*) _console_xport);
  #endif

  #if defined(MANUVR_SUPPORT_TCPSOCKET) || defined(MANUVR_SUPPORT_UDPSOCKET)
    tcpip_adapter_init();
  #endif
  return 0;
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
int8_t ESP32Platform::platformPostInit() {
  #if defined (__BUILD_HAS_FREERTOS)
  #else
  // No threads. We are responsible for pinging our own scheduler.
  // Turn on the periodic interrupts...
  uint64_t current = micros();
  esp_sleep_enable_timer_wakeup(current + 10000);
  #endif
  return 0;
}
