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
/* NOTE: There is magic here... We did a typedef dance in Platform.h to achieve
   a sort of "casting" of the stack-allocated platform instance.  */
Platform platform;

unsigned long ManuvrPlatform::_start_micros = 0;
unsigned long ManuvrPlatform::_boot_micros  = 0;

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
  _start_micros = micros();
  uint32_t default_flags = 0;

  #if defined(MANUVR_GPS_PIPE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_LOCATION;
  #endif

  #if defined(MANUVR_STORAGE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_STORAGE;
  #endif

  _alter_flags(true, default_flags);
  _discoverALUParams();

  #if defined(__BUILD_HAS_THREADS)
    platform.setIdleHook([]{ sleep_millis(20); });
  #endif

  #if defined(MANUVR_OPENINTERCONNECT)
    // Framework? Add it...
    ManuvrOIC* oic = new ManuvrOIC(root_config);
    _kernel.subscribe((EventReceiver*) oic);
  #endif
  return 0;
}


/**
* Prints details about this platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void ManuvrPlatform::printDebug(StringBuilder* output) {
  output->concatf("-- Ver/Build date:     %s   %s %s\n", VERSION_STRING, __DATE__, __TIME__);
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
  output->concatf("-- start_micros        0x%08x\n", _start_micros);
  output->concatf("-- boot_micros         0x%08x\n", _boot_micros);
  output->concatf("-- Timer resolution    %d ms\n", MANUVR_PLATFORM_TIMER_PERIOD_MS);
  output->concatf("-- Entropy pool size   %u bytes\n", PLATFORM_RNG_CARRY_CAPACITY * 4);
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
  #if defined(MANUVR_CONSOLE_SUPPORT)
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


/**
* Prints the platform's presently-loaded configuration.
*
* @param  StringBuilder* The buffer to output into.
*/
void ManuvrPlatform::printConfig(StringBuilder* output) {
  if (_config) {
    output->concat("--\n-- Loaded configuration:\n");
    _config->printDebug(output);
  }
}


/**
* Prints details about the cryptographic situation on the platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void ManuvrPlatform::printCryptoOverview(StringBuilder* out) {
  #if defined(__HAS_CRYPT_WRAPPER)
    out->concatf("-- Cryptographic support via %s.\n", __CRYPTO_BACKEND);
    int idx = 0;
    #if defined(WITH_MBEDTLS) & defined(MBEDTLS_SSL_TLS_C)
      out->concat("-- Supported TLS ciphersuites:");
      const int* cs_list = mbedtls_ssl_list_ciphersuites();
      while (0 != *(cs_list)) {
        if (0 == idx++ % 2) out->concat("\n--\t");
        out->concatf("\t%-40s", mbedtls_ssl_get_ciphersuite_name(*(cs_list++)));
      }
    #endif

    out->concat("\n-- Supported ciphers:");
    idx = 0;
    Cipher* list = list_supported_ciphers();
    while (Cipher::NONE != *(list)) {
      if (0 == idx++ % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", get_cipher_label((Cipher) *(list++)));
    }

    out->concat("\n-- Supported ECC curves:");
    CryptoKey* k_list = list_supported_curves();
    idx = 0;
    while (CryptoKey::NONE != *(k_list)) {
      if (0 == idx++ % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", get_pk_label((CryptoKey) *(k_list++)));
    }

    out->concat("\n-- Supported digests:");
    idx = 0;
    Hashes* h_list = list_supported_digests();
    while (Hashes::NONE != *(h_list)) {
      if (0 == idx++ % 6) out->concat("\n--\t");
      out->concatf("\t%-10s", get_digest_label((Hashes) *(h_list++)));
    }
  #else
  out->concat("No cryptographic support.\n");
  #endif  // WITH_MBEDTLS
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
  ManuvrMsg* boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  Kernel::staticRaiseEvent(boot_completed_ev);
  _set_init_state(MANUVR_INIT_STATE_KERNEL_BOOTING);
  uint8_t boot_passes = 100;
  while ((0 < _kernel.procIdleFlags()) && boot_passes) {
    boot_passes--;
  }
  #if defined(MANUVR_STORAGE)
    if (0 == _load_config()) {
      // If the config loaded, broadcast it.
      ManuvrMsg* conf_ev = Kernel::returnEvent(MANUVR_MSG_SYS_CONF_LOAD);
      // ???Necessary???  conf_ev->addArg(_config);
      Kernel::staticRaiseEvent(conf_ev);
    }
  #endif
  _set_init_state(MANUVR_INIT_STATE_POST_INIT);

  #if defined(__HAS_CRYPT_WRAPPER)
    // If we built-in cryptographic support, init the RNG.
    cryptographic_rng_init();
  #endif

  platformPostInit();    // Hook for platform-specific post-boot operations.

  if (nullptr == _self) {
    // If we have no other conception of "self", invent one.
    _self = new IdentityUUID(IDENTITY_STRING);
    if (_self) {
      _self->isSelf(true);
    }
  }
  _boot_micros = micros() - _start_micros;    // Note how long boot took.
  _set_init_state(MANUVR_INIT_STATE_NOMINAL); // Mark booted.
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
  if (_idle_hook) _idle_hook();
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


/**
 * Calling this function with a chain of Arguments will cause the store to be
 *   clobbered with the Argument contents.
 *
 * @param  nu The new configuration data to persist.
 * @return    0 on success. Non-zero otherwise.
 */
int8_t ManuvrPlatform::storeConf(Argument* nu) {
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


int wakeThread(unsigned long* _thread_id) {
  #if defined(__MANUVR_LINUX)
  #elif defined(__MANUVR_FREERTOS)
  vTaskResume(_thread_id);
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
  #if defined(__MANUVR_LINUX) | defined(__MANUVR_APPLE)
    struct timespec t = {(long) (millis / 1000), (long) ((millis % 1000) * 1000000UL)};
    nanosleep(&t, &t);
  #elif defined(__MANUVR_FREERTOS)
    if (platform.booted()) {
      vTaskDelay(millis / portTICK_PERIOD_MS);
    }
    else {
      // For some reason we need to delay prior to threads being operational.
      delay(millis);
    }
  #elif defined(ARDUINO)
    delay(millis);  // So wasteful...
  #endif
}


/**
* This is given as a convenience function. The programmer of an application could
*   call this as a last-act in the main function. This function never returns.
* NOTE: Unless inlined, this convenience will one additional stack-frame's worth of
*   memory. Probably not worth the convenience in most cases.  -Wl --gc-sections
*/
void ManuvrPlatform::forsakeMain() {
  while (1) {
    // Run forever.
    _kernel.procIdleFlags();
  }
}


/**
* Fills the given buffer with random bytes.
* Blocks if there is nothing random available.
*
* @param uint8_t* The buffer to fill.
* @param size_t The number of bytes to write to the buffer.
* @return 0, always.
*/
int8_t random_fill(uint8_t* buf, size_t len) {
  int written_len = 0;
  while (4 <= (len - written_len)) {
    // If we have slots for them, just up-cast and write 4-at-a-time.
    *((uint32_t*) (buf + written_len)) = randomInt();
    written_len += 4;
  }
  uint32_t slack = randomInt();
  while (0 < (len - written_len)) {
    *(buf + written_len) = (uint8_t) 0xFF & slack;
    slack = slack >> 8;
    written_len++;
  }
  return 0;
}

}  // extern "C"
