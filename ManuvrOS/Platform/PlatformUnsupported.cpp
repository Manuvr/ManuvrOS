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

This file forms the catch-all for linux platforms that have no support.
*/

#include "Platform.h"
#include <Kernel.h>

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>



/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
volatile Kernel* __kernel = NULL;

struct itimerval _interval              = {0};
struct sigaction _signal_action_SIGALRM = {0};


bool set_linux_interval_timer() {
  _interval.it_value.tv_sec      = 0;
  _interval.it_value.tv_usec     = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;
  _interval.it_interval.tv_sec   = 0;
  _interval.it_interval.tv_usec  = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;

  int err = setitimer(ITIMER_VIRTUAL, &_interval, NULL);
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
      jumpToBootloader();
      break;
    case SIGKILL:
      Kernel::log("Received a SIGKILL signal. Something bad must have happened. Exiting hard....");
      jumpToBootloader();
      break;
    case SIGTERM:
      Kernel::log("Received a SIGTERM signal. Closing up shop...");
      jumpToBootloader();
      break;
    case SIGQUIT:
      Kernel::log("Received a SIGQUIT signal. Closing up shop...");
      jumpToBootloader();
      break;
    case SIGHUP:
      printf("Received a SIGHUP signal. Closing up shop...");
      jumpToBootloader();
      break;
    case SIGSTOP:
      Kernel::log("Received a SIGSTOP signal. Closing up shop...");
      jumpToBootloader();
      break;
    case SIGUSR1:
      break;
    case SIGUSR2:
      break;
    default:
      Kernel::log(__PRETTY_FUNCTION__, LOG_NOTICE, "Unhandled signal: %d", signo);
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
  if (sigaction(SIGVTALRM, &_signal_action_SIGALRM, NULL)) {
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
void init_RNG() {
  srand(time(NULL));          // Seed the PRNG...
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
  return 0;
}


/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  return 0;
}



/****************************************************************************************************
* Data persistence                                                                                  *
****************************************************************************************************/
// Returns true if this platform can store data locally.
bool persistCapable() {
  return false;
}


// Returns the number of bytes availible to store data.
unsigned long persistFree() {
  return -1L;
}



/****************************************************************************************************
* Time and date                                                                                     *
****************************************************************************************************/
uint32_t rtc_startup_state = MANUVR_RTC_STARTUP_UNINITED;


/*
*
*/
bool initPlatformRTC() {
  rtc_startup_state = MANUVR_RTC_STARTUP_GOOD_UNSET;
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
  if (target != NULL) {
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
/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
}


/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/



/****************************************************************************************************
* Interrupt-masking                                                                                 *
****************************************************************************************************/

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
* Process control                                                                                   *
****************************************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
*/
volatile void seppuku() {
  // Whatever the kernel cared to clean up, it better have done so by this point,
  //   as no other platforms return from this function.
  if (Kernel::log_buffer.length() > 0) {
    printf("\n\njumpToBootloader(): About to exit(). Remaining log follows...\n%s", Kernel::log_buffer.string());
  }
  printf("\n\n");
  exit(0);
}


/*
* On linux, we take this to mean: scheule a program restart with the OS,
*   and then terminate this one.
*/
volatile void jumpToBootloader() {
  // TODO: Schedule a program restart.
  seppuku();
}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/

volatile void hardwareShutdown() {
  // TODO: Actually shutdown the system.
}

volatile void reboot() {
  // TODO: Actually reboot the system.
}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
void platformPreInit() {
  __kernel = (volatile Kernel*) Kernel::getInstance();
  gpioSetup();
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
void platformInit() {
  start_time_micros = micros();
  init_RNG();
  initPlatformRTC();
  __kernel = (volatile Kernel*) Kernel::getInstance();
  initSigHandlers();
  set_linux_interval_timer();
}
