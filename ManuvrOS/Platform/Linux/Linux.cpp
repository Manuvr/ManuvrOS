/*
File:   PlatformLinux.cpp
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

This file forms the catch-all for linux platforms that have no support.
*/

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#if defined(MANUVR_STORAGE)
#include <Platform/Linux/LinuxStorage.h>
#include <fcntl.h>      // Needed for integrity checks.
#include <sys/stat.h>   // Needed for integrity checks.
#endif

/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
volatile Kernel* __kernel = nullptr;

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
      Kernel::log("Received a SIGHUP signal. Closing up shop...");
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



/****************************************************************************************************
* Watchdog                                                                                          *
****************************************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;
unsigned long     start_time_micros  = 0;


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
  output->concatf("==< Linux [%s] >=================================\n", getPlatformStateStr(platformState()));
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
*/
unsigned long micros() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}


/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/
int8_t gpioDefine(uint8_t pin, int mode) {
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn) {
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  return 0;
}


int8_t readPin(uint8_t pin) {
  return 0;
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}



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



/*******************************************************************************
* Process control                                                              *
*******************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
*/
void ManuvrPlatform::seppuku() {
  #if defined(MANUVR_STORAGE)
    if (_identity && _identity->isDirty()) {
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

/*
* Function takes a path and a buffer as arguments. The binary is hashed and the ASCII representation is
*   placed in the buffer. The number of bytes read is returned on success. 0 is returned on failure.
*/
int hashFileByPath(char* path, uint8_t* h_buf) {
  int8_t return_value = -1;
  StringBuilder log;
  if (nullptr != path) {
    struct stat st;
    if(-1 != stat(path, &st)) {
      long self_size = st.st_size;
      int digest_size = 32;
      StringBuilder buf;

      int fd = open(path, O_RDONLY);
      if (fd >= 0) {
        int r_buf_len = 2 << 15;
        uint8_t* buffer = (uint8_t*) alloca(r_buf_len);
        int r_len = read(fd, buffer, r_buf_len);
        while (0 < r_len) {
          buf.concat(buffer, r_len);
          r_len = read(fd, buffer, r_buf_len);
        }
        close(fd);

        uint8_t* self_mass = buf.string();
        log.concatf("%s is %ld bytes.\n", path, self_size);

        // TODO: MBEDTLS_MD_SHA256 is too lib specific.
        int ret = manuvr_hash(self_mass, self_size, h_buf, digest_size, Hashes::SHA256);
        if (0 == ret) {
          StringBuilder hash(h_buf, digest_size);
          log.concat("This binary's SHA256 fingerprint is ");
          hash.printDebug(&log);
          return_value = digest_size;
        }
        else {
          log.concat("Failed to run the hash on the input path.\n");
        }
      }
      else {
        // Failure to read file.
        log.concat("Failed to read file.\n");
      }
    }
    else {
      // Failure to stat file.
      log.concat("Failed to stat file.\n");
    }
  }
  //Kernel::log(&log);
  printf((const char*)log.string());
  return return_value;
}


/*
* INTERNAL INTEGRITY-CHECKS
* The first task is to look in the mirror and find our executable's full path.
*   This will vary by OS, but for now we assume that we are on a linux system..
*/
int internal_integrity_check(uint8_t* test_buf, int test_len) {
    char *exe_path = (char *) alloca(512);   // 512 bytes ought to be enough for our path info...
    memset(exe_path, 0x00, 512);
    int exe_path_len = readlink("/proc/self/exe", exe_path, 512);
    if (!(exe_path_len > 0)) {
        printf("%s was unable to read its own path from /proc/self/exe. You may be running it on an unsupported operating system, or be running an old kernel. Please discover the cause and retry. Exiting...\n", program_name);
        return -1;
    }
    printf("This binary's path is %s\n", exe_path);
    int _len = (0 != test_len) ? test_len : 32;
    uint8_t* h_buf = (uint8_t*) alloca(_len);
    memset(h_buf, 0x00, _len);
    int ret = hashFileByPath(exe_path, h_buf);
    if (0 < ret) {
      if ((nullptr != test_buf) && (0 < test_len)) {
        for (int i = 0; i < test_len; i++) {
          if (*(h_buf+i) != *(test_buf+i)) {
            printf("Hashing %s yields a different value than expected. Exiting...\n", program_name);
            return -1;
          }
        }
      }
      else {
        // We have no idea what to expect. First boot?
      }
    }
    else {
      printf("Failed to hash file: %s\n", exe_path);
      return -1;
    }
    return 0;
}


/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_HAS_THREADS     | \
              MANUVR_PLAT_FLAG_INNATE_DATETIME | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t ManuvrPlatform::platformPreInit() {
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
  init_rng();
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);
  _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);

  // TODO: Evaluate consequences of choice.
  _kernel = Kernel::getInstance();
  __kernel = (volatile Kernel*) _kernel;
  // TODO: Evaluate consequences of choice.

  #if defined(MANUVR_STORAGE)
    _kernel->subscribe((EventReceiver*) sd);
  #endif

  platform.setIdleHook([]{ sleep_millis(20); });
  internal_integrity_check(nullptr, 0);

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
  if (nullptr == _identity) {
    _identity = new IdentityUUID(IDENTITY_STRING);
  }
  return 0;
}
