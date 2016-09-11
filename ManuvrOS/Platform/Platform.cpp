/*
File:   Platform.cpp
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

#include <Platform/Platform.h>
#include <Kernel.h>


#if defined(MANUVR_OPENINTERCONNECT)
#include <Frameworks/OIC/ManuvrOIC.h>
#endif


Platform platform;

/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
* Effectively, this entire class is static.
*******************************************************************************/

/**
* Issue a human-readable string representing the condition that causes an
*   IRQ to fire.
*
* @param  An IRQ condition code.
* @return A string constant.
*/
const char* ManuvrPlatform::getIRQConditionString(int con_code) {
  switch (con_code) {
    case RISING:   return "RISING";
    case FALLING:  return "FALLING";
    case CHANGE:   return "CHANGE";
    default:       return "<UNDEF>";
  }
}

/**
* Issue a human-readable string representing the platform state.
*
* @return A string constant.
*/
const char* ManuvrPlatform::getPlatformStateStr(int state) {
  switch (state) {
    default:
    case MANUVR_INIT_STATE_UNINITIALIZED:   return "UNINITIALIZED";
    case MANUVR_INIT_STATE_RESERVED_0:      return "RSRVD_0";
    case MANUVR_INIT_STATE_PREINIT:         return "PREINIT";
    case MANUVR_INIT_STATE_KERNEL_BOOTING:  return "BOOTING";
    case MANUVR_INIT_STATE_POST_INIT:       return "POST_INIT";
    case MANUVR_INIT_STATE_NOMINAL:         return "NOMINAL";
    case MANUVR_INIT_STATE_SHUTDOWN:        return "SHUTDOWN";
    case MANUVR_INIT_STATE_HALTED:          return "HALTED";
  }
}


/**
* Issue a human-readable string representing the state of the RTC
*
* @return A string constant.
*/
const char* ManuvrPlatform::getRTCStateString() {
  const uint32_t RTC_MASK = MANUVR_PLAT_FLAG_RTC_SET | MANUVR_PLAT_FLAG_RTC_READY | MANUVR_PLAT_FLAG_INNATE_DATETIME;
  switch (_pflags & RTC_MASK) {
    case MANUVR_PLAT_FLAG_INNATE_DATETIME:
      return "RTC_UNINIT";
    case MANUVR_PLAT_FLAG_RTC_READY | MANUVR_PLAT_FLAG_INNATE_DATETIME:
      return "RTC_RDY_UNSET";
    case RTC_MASK:
      return "RTC_RDY_SET";
    default:
      return "RTC_INVALID";
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
/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t ManuvrPlatform::platformPreInit(Argument* root_config) {
  // TODO: Should we really be setting capabilities this late?
  uint32_t default_flags = 0;

  #if defined(__MANUVR_MBEDTLS)
    default_flags |= MANUVR_PLAT_FLAG_HAS_CRYPTO;
  #endif

  #if defined(MANUVR_GPS_PIPE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_LOCATION;
  #endif

  #if defined(MANUVR_STORAGE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_STORAGE;
  #endif

  #if defined (__MANUVR_LINUX) || defined (__MANUVR_FREERTOS)
    default_flags |= MANUVR_PLAT_FLAG_HAS_THREADS;
    platform.setIdleHook([]{ sleep_millis(20); });
  #endif

  _alter_flags(true, default_flags);
  _discoverALUParams();

  #if defined(MANUVR_OPENINTERCONNECT)
    // Framework? Add it...
    ManuvrOIC* oic = new ManuvrOIC(root_config);
    _kernel.subscribe((EventReceiver*) oic);
  #endif
  return 0;
}


void ManuvrPlatform::printDebug(StringBuilder* output) {
  output->concatf("-- Ver/Build date:     %s   %s %s\n", VERSION_STRING, __DATE__, __TIME__);
  if (nullptr != platform._self) {
    output->concatf("-- Identity:           %s\t", platform._self->getHandle());
    platform._self->toString(output);
    output->concat("\n");
  }
  output->concatf("-- Identity source:    %s\n", platform._check_flags(MANUVR_PLAT_FLAG_HAS_IDENTITY) ? "Generated at runtime" : "Loaded from storage");
  output->concat("-- Hardware:\n");
  #if defined(HW_VERSION_STRING)
    output->concatf("--\t version:        %s\n", HW_VERSION_STRING);

  #endif
  #if defined(F_CPU)
    output->concatf("--\t CPU frequency:  %.2f MHz\n", (double) F_CPU/1000000.0);
  #endif
  output->concatf("--\t %u-bit", platform.aluWidth());
  if (8 != platform.aluWidth()) {
    output->concatf(" %s-endian", platform.bigEndian() ? "Big" : "Little");
  }

  uintptr_t initial_sp = 0;
  uintptr_t final_sp   = 0;
  output->concatf("\n-- getStackPointer():  %p\n", &final_sp);
  output->concatf("-- stack grows %s\n--\n", (&final_sp > &initial_sp) ? "up" : "down");
  output->concatf("-- millis()            0x%08x\n", millis());
  output->concatf("-- micros()            0x%08x\n", micros());
  output->concatf("-- Timer resolution:   %d ms\n", MANUVR_PLATFORM_TIMER_PERIOD_MS);
  output->concatf("-- Entropy pool size:  %u bytes\n", PLATFORM_RNG_CARRY_CAPACITY * 4);
  if (platform.hasTimeAndDate()) {
    output->concatf("-- RTC State:          %s\n", platform.getRTCStateString());
    output->concat("-- Current datetime:   ");
    currentDateTime(output);
    output->concat("\n");
  }

  output->concat("--\n-- Capabilities:\n");
  #if defined(__MANUVR_DEBUG)
    output->concat("--\t DEBUG build\n");
  #endif
  if (platform.hasThreads()) {
    output->concat("--\t Threads\n");
  }
  if (platform.hasCryptography()) {
    output->concat("--\t Crypt\n");
  }
  if (platform.hasStorage()) {
    output->concat("--\t Storage\n");
  }
  if (platform.hasLocation()) {
    output->concat("--\t Location\n");
  }

  output->concat("--\n-- Supported protocols: \n");
  #if defined(__MANUVR_CONSOLE_SUPPORT)
    output->concat("--\t Console\n");
  #endif
  #if defined(MANUVR_OVER_THE_WIRE)
    output->concat("--\t Manuvr\n");
  #endif
  #if defined(MANUVR_SUPPORT_COAP)
    output->concat("--\t CoAP\n");
  #endif
  #if defined(MANUVR_SUPPORT_MQTT)
    output->concat("--\t MQTT\n");
  #endif
  #if defined(MANUVR_SUPPORT_OSC)
    output->concat("--\t OSC\n");
  #endif
  platform.printConfig(output);
}


void ManuvrPlatform::printConfig(StringBuilder* output) {
  if (_config) {
    output->concat("--\n-- Loaded configuration:\n");
    _config->printDebug(output);
  }
}

void ManuvrPlatform::_discoverALUParams() {
  // We infer the ALU width by the size of pointers.
  // TODO: This will not work down to 8-bit because of paging schemes.
  uint32_t default_flags = 0;
  switch (sizeof(uintptr_t)) {
    // TODO: There is a way to do this with no conditionals. Figritout.
    default:
    case 1:
      // Default case is 8-bit. Do nothing.
      break;
    case 2:
      default_flags |= (uint32_t) (1 << 13);
      break;
    case 4:
      default_flags |= (uint32_t) (2 << 13);
      break;
    case 8:
      default_flags |= (uint32_t) (3 << 13);
      break;
  }
  _alter_flags(true, default_flags);

  // Now determine the endianess with a magic number and a pointer dance.
  if (8 != aluWidth()) {
    uint16_t test = 0xAA55;
    if (0xAA == *((uint8_t*) &test)) {
      _alter_flags(true, MANUVR_PLAT_FLAG_BIG_ENDIAN);
    }
  }
}



/**
* This is called by user code to initialize the platform.
*/
int8_t ManuvrPlatform::bootstrap() {
  /* Follow your shadow. */
  ManuvrRunnable* boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  Kernel::staticRaiseEvent(boot_completed_ev);
  _set_init_state(MANUVR_INIT_STATE_KERNEL_BOOTING);
  while (0 < _kernel.procIdleFlags()) {
    // TODO: Safety! Need to be able to diagnose infinte loops.
  }
  #if defined(MANUVR_STORAGE)
    if (0 == _load_config()) {
      // If the config loaded, broadcast it.
      ManuvrRunnable* conf_ev = Kernel::returnEvent(MANUVR_MSG_SYS_CONF_LOAD);
      // ???Necessary???  conf_ev->addArg(_config);
      Kernel::staticRaiseEvent(conf_ev);
    }
  #endif
  _set_init_state(MANUVR_INIT_STATE_POST_INIT);
  platformPostInit();
  if (nullptr == _self) {
    _self = new IdentityUUID(IDENTITY_STRING);
  }
  _set_init_state(MANUVR_INIT_STATE_NOMINAL);
  return 0;
}



/**
* Prints platform information without necessitating the caller
*   provide a buffer.
*/
void ManuvrPlatform::printDebug() {
  StringBuilder output;
  printDebug(&output);
  Kernel::log(&output);
}


/*******************************************************************************
* Identity and serial number                                                   *
*******************************************************************************/



/*******************************************************************************
* System hook-points                                                           *
*******************************************************************************/
void ManuvrPlatform::idleHook() {
  if (nullptr != _idle_hook) _idle_hook();
}

void ManuvrPlatform::setIdleHook(FxnPointer nu) {
  _idle_hook = nu;
}

void ManuvrPlatform::wakeHook() {
  if (nullptr != _wake_hook) _wake_hook();
}

void ManuvrPlatform::setWakeHook(FxnPointer nu) {
  _wake_hook = nu;
}


/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/
Argument* ManuvrPlatform::getConfKey(const char* key) {
  return (_config) ? _config->retrieveArgByKey(key) : nullptr;
}


int8_t ManuvrPlatform::setConf(Argument* nu) {
  #if defined(MANUVR_STORAGE)
  // Persist any open configuration...
  if ((nu) && (_storage_device)) {
    if (_storage_device->isMounted()) {
      // Persist config...
      StringBuilder shuttle;
      if (0 <= Argument::encodeToCBOR(nu, &shuttle)) {
        // TODO: Now would be a good time to persist any dirty identities.
        if (0 < _storage_device->persistentWrite(nullptr, shuttle.string(), shuttle.length(), 0)) {
          return 0;
        }
      }
    }
  }
  #endif
  return -1;
}


/*******************************************************************************
* TODO: Still digesting                                                        *
*******************************************************************************/

extern "C" {

/*
* Platform-abstracted function to enable or suspend interrupts. Whatever
*   that might mean on a given platform.
*/
void maskableInterrupts(bool enable) {
  if (enable) {
    globalIRQEnable();
  }
  else {
    globalIRQDisable();
  }
}



/*******************************************************************************
* Threading                                                                    *
*******************************************************************************/
/**
* On linux, we support pthreads. On microcontrollers, we support FreeRTOS.
* This is the wrapper to create a new thread.
*
* @return The thread's return value.
*/
int createThread(unsigned long* _thread_id, void* _something, ThreadFxnPtr _fxn, void* _args) {
  #if defined(__MANUVR_LINUX)
    return pthread_create(_thread_id, (const pthread_attr_t*) _something, _fxn, _args);
  #elif defined(__MANUVR_FREERTOS)
    // TODO: Make the task parameters 1-to-1 with pthreads.
    TaskHandle_t taskHandle;
    portBASE_TYPE ret = xTaskCreate((TaskFunction_t) _fxn, "_t", 2048, (void*)_args, 1, &taskHandle);
    if (pdPASS == ret) {
      *_thread_id = (unsigned long) taskHandle;
      return 0;
    }
  #endif
  //Kernel::log("Failed to create thread.\n");
  return -1;
}

int deleteThread(unsigned long* _thread_id) {
  #if defined(__MANUVR_LINUX)
  return pthread_cancel(*_thread_id);
  #elif defined(__MANUVR_FREERTOS)
  // TODO: Why didn't this work?
  //vTaskDelete(&_thread_id);
  return 0;
  #endif
  return -1;
}


/**
* Wrapper for causing threads to sleep. This is NOT intended to be used as a delay
*   mechanism, although that use-case will work. It is more for the sake of not
*   burning CPU needlessly in polling-loops where it might be better-used elsewhere.
*
* If you are interested in delaying without suspending the entire thread, you should
*   probably use interrupts instead.
*/
void sleep_millis(unsigned long millis) {
  #if defined(__MANUVR_LINUX)
    struct timespec t = {(long) (millis / 1000), (long) ((millis % 1000) * 1000000UL)};
    nanosleep(&t, &t);
  #elif defined(__MANUVR_FREERTOS)
    vTaskDelay(millis / portTICK_PERIOD_MS);
  #elif defined(ARDUINO)
    delay(millis);  // So wasteful...
  #endif
}

}  // extern "C"
