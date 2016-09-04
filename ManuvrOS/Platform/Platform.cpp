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


// TODO: I know this is horrid. I'm sick of screwing with the build system today...
#if defined(__MK20DX256__) | defined(__MK20DX128__)

#elif defined(STM32F4XX)
  #include "./STM32F4/STM32F4.cpp"
  ManuvrPlatform platform;
#elif defined(STM32F7XX) | defined(STM32F746xx)
  #include "./STM32F7/STM32F7.h"
  #include "./STM32F7/STM32F7.cpp"
  ManuvrPlatform platform;
#elif defined(ARDUINO)
  #include "./Arduino/Arduino.cpp"
  ManuvrPlatform platform;
#elif defined(__MANUVR_PHOTON)
  #include "./Particle/Photon.cpp"
  ManuvrPlatform platform;
#elif defined(__MANUVR_LINUX)
  ManuvrPlatform platform;
#else
  // Unsupportage.
  //#include "PlatformUnsupported.cpp"
#endif


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


// TODO: This is only here to ease the transition into polymorphic platform defs.
void ManuvrPlatform::printPlatformBasics(StringBuilder* output) {
  if (nullptr != platform._identity) {
    output->concatf("-- Identity:           %s\t", platform._identity->getHandle());
    platform._identity->toString(output);
    output->concat("\n");
  }
  else {
    output->concatf("-- Undifferentiated %s\n", IDENTITY_STRING);
  }
  output->concatf("-- Identity source:    %s\n", platform._check_flags(MANUVR_PLAT_FLAG_HAS_IDENTITY) ? "Generated at runtime" : "Loaded from storage");
  output->concatf("-- Ver/Build date:     %s\t%s %s\n", VERSION_STRING, __DATE__, __TIME__);
  output->concatf("-- %u-bit", platform.aluWidth());
  if (8 != platform.aluWidth()) {
    output->concatf(" %s-endian", platform.bigEndian() ? "Big" : "Little");
  }

  uintptr_t initial_sp = 0;
  uintptr_t final_sp   = 0;
  output->concatf("\n-- getStackPointer():  %p\n", &final_sp);
  output->concatf("-- stack grows %s\n--\n", (&final_sp > &initial_sp) ? "up" : "down");
  output->concatf("-- Timer resolution:   %d ms\n", MANUVR_PLATFORM_TIMER_PERIOD_MS);
  output->concatf("-- Hardware version:   %s\n", HW_VERSION_STRING);
  output->concatf("-- Entropy pool size:  %u bytes\n", PLATFORM_RNG_CARRY_CAPACITY * 4);
  if (platform.hasTimeAndDate()) {
    output->concatf("-- RTC State:          %s\n", platform.getRTCStateString());
  }
  output->concatf("-- millis()            0x%08x\n", millis());
  output->concatf("-- micros()            0x%08x\n", micros());

  output->concat("--\n-- Capabilities:\n");
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
  ManuvrRunnable *boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  Kernel::staticRaiseEvent(boot_completed_ev);
  _set_init_state(MANUVR_INIT_STATE_KERNEL_BOOTING);
  while (0 < _kernel.procIdleFlags()) {
    // TODO: Safety! Need to be able to diagnose infinte loops.
  }
  _set_init_state(MANUVR_INIT_STATE_POST_INIT);
  platformPostInit();
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
/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int ManuvrPlatform::platformSerialNumberSize() {
  if (nullptr != _identity) {
    return _identity->length();
  }
  return 0;
}

/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int ManuvrPlatform::getSerialNumber(uint8_t* buf) {
  if (nullptr != _identity) {
    return _identity->toBuffer(buf);
  }
  return 0;
}



/*******************************************************************************
* System hook-points                                                           *
*******************************************************************************/
void ManuvrPlatform::idleHook() {
  if (nullptr != _idle_hook) _idle_hook();
}

void ManuvrPlatform::setIdleHook(FunctionPointer nu) {
  _idle_hook = nu;
}

void ManuvrPlatform::wakeHook() {
  if (nullptr != _wake_hook) _wake_hook();
}

void ManuvrPlatform::setWakeHook(FunctionPointer nu) {
  _wake_hook = nu;
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
  #elif defined(__MANUVR_FREERTOS)
  //vTaskDelete(&_thread_id);
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
  #endif
}

}  // extern "C"
