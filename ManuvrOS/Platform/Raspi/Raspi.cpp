/*
File:   PlatformRaspi.cpp
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


Bits and pieces of low-level GPIO access and discovery code was taken from this library:
http://abyz.co.uk/rpi/pigpio/
*/



#include "Platform.h"
#include <Kernel.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>

#if defined(MANUVR_STORAGE)
#include <Platform/Linux/LinuxStorage.h>
#endif

/* GPIO access macros... */
#define PI_BANK (pin >> 5)
#define PI_BIT  (1 << (pin & 0x1F))

#define GPSET0 7
#define GPSET1 8
#define GPCLR0 10
#define GPCLR1 11
#define GPLEV0 13
#define GPLEV1 14


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
volatile Kernel* __kernel = nullptr;

static volatile unsigned rev = 0;
static volatile uint32_t piModel      = 0;
static volatile uint32_t piPeriphBase = 0;
static volatile uint32_t piBusAddr    = 0;

#define GPIO_LEN  0xB4
#define SYST_LEN  0x1C

static volatile uint32_t  *gpioReg = (volatile uint32_t*) MAP_FAILED;
static volatile uint32_t  *systReg = (volatile uint32_t*) MAP_FAILED;

struct itimerval _interval              = {0};
struct sigaction _signal_action_SIGALRM = {0};


bool set_linux_interval_timer() {
  _interval.it_value.tv_sec      = 0;
  _interval.it_value.tv_usec     = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;
  _interval.it_interval.tv_sec   = 0;
  _interval.it_interval.tv_usec  = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;

  int err = setitimer(ITIMER_VIRTUAL, &_interval, nullptr);
  if (err) {
    Kernel::log("Failed to enable interval timer.");
  }
  return (0 == err);
}

bool unset_linux_interval_timer() {
  // TODO: We ultimately need to be retaining the values in the struct, as well as
  //   the current system time to calculate the delta in case we get re-enabled.
  _interval.it_value.tv_sec      = 0;
  _interval.it_value.tv_usec     = 0;
  _interval.it_interval.tv_sec   = 0;
  _interval.it_interval.tv_usec  = 0;
  return true;
}


void sig_handler(int signo) {
  switch (signo) {
    case SIGINT:
      Kernel::log("Received a SIGINT signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGKILL:
      Kernel::log("Received a SIGKILL signal. Something bad must have happened. Exiting hard....");
      platform.seppuku();
      break;
    case SIGTERM:
      Kernel::log("Received a SIGTERM signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGQUIT:
      Kernel::log("Received a SIGQUIT signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGHUP:
      printf("Received a SIGHUP signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGSTOP:
      Kernel::log("Received a SIGSTOP signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGUSR1:
      break;
    case SIGUSR2:
      break;
    default:
      #ifdef __MANUVR_DEBUG
      {
        StringBuilder _log;
        _log.concatf("Unhandled signal: %d", signo);
        Kernel::log(&_log);
      }
      #endif
      break;
  }

  // Echo whatever signals we receive to the child proc (if we are the parent).
  //if ((looper_pid > 0) && (signo != SIGALRM) && (signo != SIGUSR2)) {
  //  kill(looper_pid, signo);
  //}
}


void linux_timer_handler(int sig_num) {
  ((Kernel*)__kernel)->advanceScheduler(MANUVR_PLATFORM_TIMER_PERIOD_MS);
}


// The parent process should call this function to set the callback address to its signal handlers.
//     Returns 1 on success, 0 on failure.
// TODO: Convert all other signals over to sigaction().
int initSigHandlers() {
  int return_value    = 1;
  // Try to open a binding to listen for signals from the OS...
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGINT to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGQUIT to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGHUP, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGHUP to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGTERM to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGUSR1 to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGUSR2 to the signal system. Failing...");
    return_value = 0;
  }

  _signal_action_SIGALRM.sa_handler   = &linux_timer_handler;
  if (sigaction(SIGVTALRM, &_signal_action_SIGALRM, nullptr)) {
    Kernel::log("Failed to bind to SIGVTALRM.");
    return_value = 0;
  }

  return return_value;
}


/**
 * Import an absolute register address so we can access it as-if we were on
 *   a microcontroller...
 *
 * Taken from...
 * http://abyz.co.uk/rpi/pigpio/
 */
static uint32_t* initMapMem(int fd, uint32_t addr, uint32_t len) {
  return (uint32_t *) mmap(0, len,
    PROT_READ|PROT_WRITE|PROT_EXEC,
    MAP_SHARED|MAP_LOCKED,
    fd, addr);
}


/**
 * Makes a guess at the version of the RasPi we are running on.
 *
 * Taken from...
 * http://abyz.co.uk/rpi/pigpio/
 * ...and adapted to fit this file.
 */
unsigned gpioHardwareRevision() {
  if (rev) return rev;

  FILE * filp = fopen("/proc/cpuinfo", "r");
  if (nullptr != filp) {
    char buf[512];
    char term;
    int chars = 4; /* number of chars in revision string */

    while (fgets(buf, sizeof(buf), filp) != nullptr) {
      if (0 == piModel) {
        if (!strncasecmp("model name", buf, 10)) {
          if (strstr (buf, "ARMv6") != nullptr) {
            piModel = 1;
            chars = 4;
            piPeriphBase = 0x20000000;
            piBusAddr = 0x40000000;
            Kernel::log("Found a Raspberry Pi v1.\n");
          }
          else if (strstr (buf, "ARMv7") != nullptr) {
            piModel = 2;
            chars = 6;
            piPeriphBase = 0x3F000000;
            piBusAddr = 0xC0000000;
            Kernel::log("Found a Raspberry Pi v2.\n");
          }
        }
      }
      if (!strncasecmp("revision", buf, 8)) {
        if (sscanf(buf+strlen(buf)-(chars+1), "%x%c", &rev, &term) == 2) {
          if (term != '\n') {
            rev = 0;
          }
        }
      }
    }
    fclose(filp);
  }
  else {
    Kernel::log("Failed to open /proc/cpuinfo.\n");
  }
  return rev;
}


int gpioInitialise() {
  /* sets piModel, needed for peripherals address */
  if (0 < gpioHardwareRevision()) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC) ;
    if (fd < 0) {
      Kernel::log("Cannot access /dev/mem. No GPIO functions available.\n");
      return -1;
    }

    gpioReg  = initMapMem(fd, (piPeriphBase + 0x200000),  GPIO_LEN);
    systReg  = initMapMem(fd, (piPeriphBase + 0x003000),  SYST_LEN);

    close(fd);

    if ((gpioReg == MAP_FAILED) || (systReg == MAP_FAILED)) {
      Kernel::log("mmap failed. No GPIO functions available.\n");
      return -1;
    }
  }
  else {
    Kernel::log("Could not determine raspi hardware revision.\n");
    return -1;
  }
  return 0;
}



/****************************************************************************************************
* Watchdog                                                                                          *
****************************************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;
unsigned long     start_time_micros  = 0;


/*******************************************************************************
* Interrupt-masking                                                            *
*******************************************************************************/

// Ze interrupts! Zhey do nuhsing!
// TODO: Perhaps raise the nice value?
// At minimum, turn off the periodic timer, since this is what would happen on
//   other platforms.
void globalIRQEnable() {
  // TODO: Need to stack the time remaining.
  //set_linux_interval_timer();
}

void globalIRQDisable() {
  // TODO: Need to unstack the time remaining and fire any schedules.
  //unset_linux_interval_timer();
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
  uint32_t return_value = rand();
  return return_value;
}

/**
* Called by the RNG ISR to provide new random numbers.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool provide_random_int(uint32_t nu_rnd) {
  return false;
}

/*
* Init the RNG. Short and sweet.
*/
void init_rng() {
  srand(time(nullptr));          // Seed the PRNG...
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
void ManuvrPlatform::printDebug(StringBuilder* output) {
  output->concatf("==< Linux on Raspi [%s] >===========================\n", getPlatformStateStr(platformState()));
  printPlatformBasics(output);
}

/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/
/*
*/
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s) {
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
* Returns an integer representing the current datetime.
*/
uint32_t currentTimestamp(void) {
  uint32_t return_value = (uint32_t) time(0);
  return return_value;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != nullptr) {
    time_t t = time(0);
    struct tm  tstruct;
    char       buf[64];
    tstruct = *localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    target->concat(buf);
  }
}


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}


/*
* Not provided elsewhere on a linux platform.
* Returns the number of microseconds after system boot. Wraps around
*   after 1 hour 11 minutes 35 seconds.
*
* Taken from:
* http://abyz.co.uk/rpi/pigpio/
*/
unsigned long micros() {
  return systReg[1];
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int platformSerialNumberSize() {
  return 8;
}


/**
* Writes the serial number to the indicated buffer.
* Thanks, Narishma!
* https://www.raspberrypi.org/forums/viewtopic.php?f=31&t=18936
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (!f) return 0;

  char line[256];
  long serial = 0;
  while (fgets(line, 256, f)) {
    if (strncmp(line, "Serial", 6) == 0) {
      char serial_string[16 + 1];
      serial = atoi(strcpy(serial_string, strchr(line, ':') + 2));
      fclose(f);
      return serial;
    }
  }

  fclose(f);
  return 0;
}



/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/
/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
  gpioInitialise();
}

int8_t gpioDefine(uint8_t pin, int mode) {
  if (piModel) {
    int reg   = pin / 10;
    int shift = (pin % 10) * 3;

    gpioReg[reg] = (gpioReg[reg] & ~(7<<shift));

    switch (mode) {
      case INPUT:
        break;
      case INPUT_PULLUP:
        break;
      case OUTPUT:
        gpioReg[reg] = (gpioReg[reg] | (1<<shift));
        break;
      default:
        break;
    }
    return 0;
  }
  return -1;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FxnPointer fxn) {
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  if (piModel) {
    *(gpioReg + (val ? GPSET0 : GPCLR0)) = 1 << pin;
    return 0;
  }
  return -1;
}


int8_t readPin(uint8_t pin) {
  if (piModel) {
    return (((*(gpioReg + GPLEV0 + PI_BANK) & PI_BIT) != 0) ? 1 : 0);
  }
  return -1;
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return -1;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}



/*******************************************************************************
* Process control                                                              *
*******************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
*/
void ManuvrPlatform::seppuku() {
  #if defined(MANUVR_STORAGE)
    if (_self && _self->isDirty()) {
      // Save the dirty identity.
      // TODO: int8_t persistentWrite(const char*, uint8_t*, int, uint16_t);
    }
  #endif
  // Whatever the kernel cared to clean up, it better have done so by this point,
  //   as no other platforms return from this function.
  printf("\nseppuku(): About to exit().\n\n");
  exit(0);
}


/*
* On linux, we take this to mean: scheule a program restart with the OS,
*   and then terminate this one.
*/
void ManuvrPlatform::jumpToBootloader() {
  // TODO: Pull binary from a location of firmware's choice.
  // TODO: Install firmware after validation.
  // TODO: Schedule a program restart.
  printf("\njumpToBootloader(): About to exit().\n\n");
  exit(0);
}


/*******************************************************************************
* Underlying system control.                                                   *
*******************************************************************************/

void ManuvrPlatform::hardwareShutdown() {
  // TODO: Actually shutdown the system.
  printf("\nhardwareShutdown(): About to exit().\n\n");
  exit(0);
}


void ManuvrPlatform::reboot() {
  // TODO: Actually reboot the system.
  printf("\nreboot(): About to exit().\n\n");
  exit(0);
}



extern char* program_name;// TODO: Yuck



/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_HAS_THREADS     | \
              MANUVR_PLAT_FLAG_SERIALED | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)


/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t ManuvrPlatform::platformPreInit(Argument* root_config) {
  // TODO: Should we really be setting capabilities this late?
  uint32_t default_flags = DEFAULT_PLATFORM_FLAGS;

  #if defined(MANUVR_STORAGE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_STORAGE;
    Argument opts("./manuvr.dat");
    opts.setKey("filepath");
    LinuxStorage* sd = new LinuxStorage(&opts);
    _storage_device = (Storage*) sd;
  #endif
  #if defined(__MANUVR_MBEDTLS)
    default_flags |= MANUVR_PLAT_FLAG_HAS_CRYPTO;
  #endif

  #if defined(MANUVR_GPS_PIPE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_LOCATION;
  #endif

  _alter_flags(true, default_flags);
  _discoverALUParams();

  start_time_micros = micros();
  gpioSetup();
  init_rng();
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);
  _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);

  // TODO: Evaluate consequences of choice.
  __kernel = (volatile Kernel*) &_kernel;
  // TODO: Evaluate consequences of choice.

  #if defined(MANUVR_STORAGE)
    _kernel.subscribe((EventReceiver*) sd);
  #endif

  platform.setIdleHook([]{ sleep_millis(20); });
  //internal_integrity_check(nullptr, 0);  // TODO: soon

  initSigHandlers();
  set_linux_interval_timer();
  return 0;
}


/*
* Called before kernel instantiation. So do the minimum required to ensure
*   internal system sanity.
*/
int8_t ManuvrPlatform::platformPostInit() {
  #if defined(MANUVR_STORAGE)
    if (_storage_device->isMounted()) {
      // TODO: Read configuration.
      //int8_t persistentRead(const char*, uint8_t*, int, uint16_t);
    }
  #endif
  if (nullptr == _self) {
    _self = new IdentityUUID(IDENTITY_STRING);
  }
  return 0;
}
