/*
File:   Platform.cpp
Author: J. Ian Lindsay
Date:   2015.11.01


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "Platform.h"


#include <sys/time.h>
#include <unistd.h>


unsigned long millis() {
  timeval curTime;
  gettimeofday(&curTime, NULL);
  long milli = curTime.tv_usec / 1000;
  return milli;
}


unsigned long start_time_micros = 0;


/* Returns the current timestamp in microseconds. */
unsigned long micros() {
  timeval curTime;
  gettimeofday(&curTime, NULL);
  return (curTime.tv_usec - start_time_micros);
}




/****************************************************************************************************
* Scheduler callbacks. Please note that these are NOT part of StaticHub.                            *
****************************************************************************************************/
/*
* Scheduler callback
* This gets called periodically to moderate traffic volume across the USB link
*   until I figure out how to sink a hook into the DMA interrupt to take its place.
*/
void stdout_funnel() {
  if (StaticHub::log_buffer.count()) {
    if (!StaticHub::getInstance()->getVerbosity()) {
      StaticHub::log_buffer.clear();
    }
    else {
      printf("%s", StaticHub::log_buffer.position(0));
      StaticHub::log_buffer.drop_position(0);
    }
  }
  else {
    StaticHub::getInstance()->disableLogCallback();
  }
}



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/
volatile StaticHub* StaticHub::INSTANCE = NULL;
volatile uint32_t StaticHub::next_random_int[PLATFORM_RNG_CARRY_CAPACITY];
volatile uint32_t StaticHub::millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  StaticHub::watchdog_mark = 42;



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder StaticHub::log_buffer;

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void StaticHub::log(int severity, const char *str) {
  if (!INSTANCE->verbosity) return;
  //if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  //log_buffer.concat(str);
  if (str) printf("%s", str);
}

volatile void StaticHub::log(char *str) {
  if (!INSTANCE->verbosity) return;
  if (str) printf("%s", str);
}

volatile void StaticHub::log(const char *str) {
  if (!INSTANCE->verbosity) return;
  //if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  //log_buffer.concat(str);
  if (str) printf("%s", str);
}

volatile void StaticHub::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->verbosity) return;
  //log_buffer.concat("%d  %s:\t", severity, fxn_name);
  if (log_buffer.count() == 0) {
    if (NULL != INSTANCE) {
       if (INSTANCE->pid_log_moderator) {
         ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
       }
     }
  }
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;
  
  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
  printf("%s", log_buffer.string());
  log_buffer.clear();
}

volatile void StaticHub::log(StringBuilder *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concatHandoff(str);
  printf("%s", log_buffer.string());
  log_buffer.clear();
}


void StaticHub::disableLogCallback(){
  __scheduler.disableSchedule(pid_log_moderator);
}



/****************************************************************************************************
* Various small utility functions...                                                                *
****************************************************************************************************/

/**
* Sometimes we question the size of the stack.
*
* @return the stack pointer at call time.
*/
volatile uint32_t StaticHub::getStackPointer(void) {
  uint32_t test;
  test = (uint32_t) &test;  // Store the pointer.
  return test;
}


/**
* Called by the RNG ISR to provide new random numbers to StaticHub.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool StaticHub::provide_random_int(uint32_t nu_rnd) {
  return false;
}


/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t StaticHub::randomInt(void) {
  uint32_t return_value = rand();
  return return_value;
}


/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool StaticHub::setTimeAndDate(char* nu_date_time) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t StaticHub::currentTimestamp(void) {
  uint32_t return_value = 0;
  return return_value;
}

/*
* Same, but writes a string representation to the argument.
*/
void StaticHub::currentTimestamp(StringBuilder* target) {
}

/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void StaticHub::currentDateTime(StringBuilder* target) {
  if (target != NULL) {
  }
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀  ▀▀▀▀█░█▀▀▀▀ 
     ▐░▌     ▐░▌▐░▌    ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌ ▐░▌   ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌  ▐░▌  ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌   ▐░▌ ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌    ▐░▌▐░▌     ▐░▌          ▐░▌     
 ▄▄▄▄█░█▄▄▄▄ ▐░▌     ▐░▐░▌ ▄▄▄▄█░█▄▄▄▄      ▐░▌     
▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌     ▐░▌     
 ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀       ▀     

StaticHub bootstrap and setup fxns. This code is only ever called to initiallize this or that thing,
  and then becomes inert.
****************************************************************************************************/

/*
* Some peripherals and operations need a bit of time to complete. This function is called from a
*   one-shot schedule and performs all of the cleanup for latent consequences of bootstrap().
*/
int8_t StaticHub::bootComplete() {
  //EventReceiver::bootComplete()  <--- Note that we aren't going to do this. We already know about the scheduler.
  return 1;
}


int random_to_log(ManuvrEvent *active_event) {
  StringBuilder log_item("New random: ");
  log_item.concat((uint32_t) StaticHub::randomInt());
  StaticHub::log(&log_item);
  return 1;
}




/*
* When the system comes to life, we will want to setup some periodic schedules.
* Even for schedules that are meant to be one-shots, we will try to avoid a malloc()/free()
*   cycle if it's going to be a regular event. Just pre-build some schedules that we know
*   we will need and let them sit dormant until needed.
*/
void StaticHub::initSchedules(void) {
  pid_profiler_report = __scheduler.createSchedule(10000,  -1, false, &__scheduler, new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META));
  __scheduler.disableSchedule(pid_profiler_report);

  // This schedule marches the data into the USB VCP at a throttled rate.
  pid_log_moderator = __scheduler.createSchedule(2,  -1, false, stdout_funnel);
  __scheduler.delaySchedule(pid_log_moderator, 1000);
  
}


/*
* Init the RNG. Short and sweet.
*/
void StaticHub::init_RNG(void) volatile {
  srand(time(NULL));          // Seed the PRNG...
}


/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup 
*   in the class that deals with them. 
* Pending peripheral-level init of pins, we should just enable everything and let 
*   individual classes work out their own requirements.
*/
void StaticHub::gpioSetup() volatile {
}


int test_callback(ManuvrEvent *active_event) {
  StringBuilder *log_item;
  if (0 == active_event->getArgAs(&log_item)) {
    StaticHub::log(log_item);
  }
  return 1;
}


/*
* The primary init function. Calling this will bring the entire system online if everything is
*   working nominally.
*/
int8_t StaticHub::bootstrap() {
  log_buffer.concatf("\n\%s v%s    Build date: %s %s\nBootstrap completed.\n\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  
  event_manager.subscribe((EventReceiver*) &__scheduler);    // Subscribe the Scheduler.
  event_manager.subscribe((EventReceiver*) this);            // Subscribe StaticHub as top priority in EventManager.

  // Setup the first i2c adapter and Subscribe it to EventManager.
  i2c = new I2CAdapter(1);
  event_manager.subscribe((EventReceiver*) i2c);

  init_RNG();      // Fire up the RNG...

  initSchedules();   // We know we will need some schedules...

  // Instantiations complete. Run init() fxn's...
  
  event_manager.on(MANUVR_MSG_SYS_ISSUE_LOG_ITEM, test_callback, 0);


  EventManager::raiseEvent(MANUVR_MSG_SYS_BOOT_COMPLETED, this);  // Bootstrap complete
  return 0;
}


/*
* All external access to StaticHub's non-static members should obtain it's reference via this fxn...
*   Note that services that are dependant on SH during the bootstrap phase should have an SH reference
*   passed into their constructors, rather than forcing them to call this and risking an infinite 
*   recursion.
*/
StaticHub* StaticHub::getInstance() {
  if (StaticHub::INSTANCE == NULL) {
    // TODO: remove last traces of this pattern.
    // This should never happen, but if it does, it will crash us for sure.
    StaticHub::INSTANCE = new StaticHub();
    ((StaticHub*) StaticHub::INSTANCE)->bootstrap();
  }
  // And that is how the singleton do...
  return (StaticHub*) StaticHub::INSTANCE;
}


StaticHub::StaticHub() {
  StaticHub::INSTANCE = this;
  start_time_micros = micros();
}




/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄       ▄            ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄  
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌     ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀█░▌
     ▐░▌     ▐░▌          ▐░▌       ▐░▌     ▐░▌          ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░▌ ▐░▌   ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌▐░▌       ▐░▌
     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀█░█▀▀      ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌   ▐░▌ ▐░▌▐░▌       ▐░▌
     ▐░▌               ▐░▌▐░▌     ▐░▌       ▐░▌          ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌▐░▌       ▐░▌
 ▄▄▄▄█░█▄▄▄▄  ▄▄▄▄▄▄▄▄▄█░▌▐░▌      ▐░▌      ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌      ▐░░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀       ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀ 

Interrupt service routine support functions...                                                                    
A quick note is in order. These functions are static class members that are called directly from  
  the ISRs in stm32f4xx_it.c. They are not themselves ISRs. Keep them as short as possible.                                     

TODO: Keep them as short as possible.                                                             
****************************************************************************************************/


/*
* Called from the SysTick handler once per ms.
*/
volatile void StaticHub::advanceScheduler(void) {
  if (0 == (millis_since_reset++ % watchdog_mark)) {
    /* If it's time to punch the watchdog, do so. Doing it in the ISR will prevent a
         long-running operation from causing a reset. This way, the only thing that will
         cause a watchdog reset is if this ISR fails to fire. */
  }
  // TODO: advanceScheduler() can be made *much* faster with a bit of re-work in the Scheduler class.
  if (NULL != INSTANCE) ((StaticHub*) INSTANCE)->__scheduler.advanceScheduler();
}

