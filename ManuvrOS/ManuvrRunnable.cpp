/*
File:   ManuvrRunnable.cpp
Author: J. Ian Lindsay
Date:   2015.12.18

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


This class began life as the merger of ScheduleItem and ManuvrRunnable.
*/

#include "Kernel.h"
#include "ManuvrRunnable.h"

const char* NUL_STR = "NULL";
const char* YES_STR = "Yes";
const char* NO_STR  = "No";


/**
* Vanilla constructor.
*/
ManuvrRunnable::ManuvrRunnable() : ManuvrMsg(0) {
  __class_initializer();
}


/**
* Constructor that specifies the message code, and a callback.
*
* @param code  The message id code.
* @param cb    A pointer to the EventReceiver that should be notified about completion of this event.
*/
ManuvrRunnable::ManuvrRunnable(uint16_t code, EventReceiver* cb) : ManuvrMsg(code) {
  __class_initializer();
  originator = cb;
}


/**
* Constructor that specifies the message code, but not a callback (which is set to NULL).
*
* @param code  The message id code.
*/
ManuvrRunnable::ManuvrRunnable(uint16_t code) : ManuvrMsg(code) {
  __class_initializer();
}


/**
* Constructor. Takes a void fxn(void) as a callback (Legacy).
*
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
*/
ManuvrRunnable::ManuvrRunnable(int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback) : ManuvrMsg(MANUVR_MSG_DEFERRED_FXN) {
  __class_initializer();
  threadEnabled(true);
  autoClear(ac);
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;

  schedule_callback   = sch_callback;    // This constructor uses the legacy callback.
}

/**
* Constructor. Takes an EventReceiver* as an originator.
* We need to deal with memory management of the Event. For a recurring schedule, we can't allow the
*   Kernel to reap the event. So it is very important to mark the event appropriately.
*
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
* @param ev           A pointer to an Event that we will periodically raise.
*/
ManuvrRunnable::ManuvrRunnable(int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* ori) {
  __class_initializer();
  threadEnabled(true);
  autoClear(ac);
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;

  originator          = ori;   // This constructor uses the EventReceiver callback...
}



/**
* Destructor.
*/
ManuvrRunnable::~ManuvrRunnable(void) {
  clearProfilingData();
  clearArgs();
}


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*
* The nature of this class makes this a better design, anyhow. Since the same object is often
*   repurposed many times, doing any sort of init in the constructor should probably be avoided.
*/
void ManuvrRunnable::__class_initializer() {
  _flags             = 0x00;
  originator         = nullptr;
  specific_target    = nullptr;
  prof_data          = nullptr;
  schedule_callback  = nullptr;
  priority           = EVENT_PRIORITY_DEFAULT;

  thread_recurs       = 0;
  thread_period       = 0;
  thread_time_to_wait = 0;
}



/**
* This is called when the event is being repurposed for another run through the event queue.
*
* @param code The new message id code.
* @return 0 on success, or appropriate failure code.
*/
int8_t ManuvrRunnable::repurpose(uint16_t code) {
  // These things have implications for memory management, which is why repurpose() doesn't touch them.
  uint8_t _persist_mask = MANUVR_RUNNABLE_FLAG_MEM_MANAGED | MANUVR_RUNNABLE_FLAG_PREALLOCD | MANUVR_RUNNABLE_FLAG_SCHEDULED;
  _flags              = _flags & _persist_mask;

  originator          = nullptr;
  specific_target     = nullptr;
  schedule_callback   = nullptr;
  priority            = EVENT_PRIORITY_DEFAULT;
  return ManuvrMsg::repurpose(code);
}


bool ManuvrRunnable::abort() {
  return Kernel::abortEvent(this);
}

/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrRunnable::printDebug(StringBuilder *output) {
  if (output == nullptr) return;
  StringBuilder msg_serial;
  ManuvrMsg::printDebug(output);
  int arg_count = serialize(&msg_serial);
  if (0 <= arg_count) {
  	  unsigned char* temp_buf = msg_serial.string();
  	  int temp_buf_len        = msg_serial.length();

  	  output->concatf("\t Preallocated          %s\n", (returnToPrealloc() ? YES_STR : NO_STR));
  	  output->concatf("\t Originator:           %s\n", (nullptr == originator ? NUL_STR : originator->getReceiverName()));
  	  output->concatf("\t specific_target:      %s\n", (nullptr == specific_target ? NUL_STR : specific_target->getReceiverName()));
  	  output->concatf("\t Argument count (ser): %d\n", arg_count);
  	  output->concatf("\t Bitstream length:     %d\n\t Buffer:  ", temp_buf_len);
  	  for (int i = 0; i < temp_buf_len; i++) {
  	    output->concatf("0x%02x ", *(temp_buf + i));
  	  }
  	  output->concat("\n\n");
  }
  //else {
  //  output->concatf("Failed to serialize message. Count was (%d).\n", arg_count);
  //}

  output->concatf("\t [%p] Schedule \n\t --------------------------------\n", this);

  output->concatf("\t Enabled       \t%s\n", (threadEnabled() ? YES_STR : NO_STR));
  output->concatf("\t Time-till-fire\t%u\n", thread_time_to_wait);
  output->concatf("\t Period        \t%u\n", thread_period);
  output->concatf("\t Recurs?       \t%d\n", thread_recurs);
  output->concatf("\t Exec pending: \t%s\n", (shouldFire() ? YES_STR : NO_STR));
  output->concatf("\t Autoclear     \t%s\n", (autoClear() ? YES_STR : NO_STR));
  output->concatf("\t Profiling?    \t%s\n", (profilingEnabled() ? YES_STR : NO_STR));

  if (nullptr != schedule_callback) {
    output->concat("\t Legacy callback\n");
  }
}


void ManuvrRunnable::printProfilerData(StringBuilder *output) {
  if (nullptr != prof_data) output->concatf("\t %p  %9u  %9u  %9u  %9u  %9u  %9u %s\n", this, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last, (threadEnabled() ? " " : "(INACTIVE)"));
}


/****************************************************************************************************
* Functions dealing with profiling this particular Runnable.                                        *
****************************************************************************************************/

/**
* Any schedule that has a TaskProfilerData object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply instance the appropriate struct into place.
*/
void ManuvrRunnable::profilingEnabled(bool enabled) {
  if (nullptr == prof_data) {
    // Profiler data does not exist. If enabled == false, do nothing.
    if (enabled) {
      prof_data = new TaskProfilerData();
      prof_data->profiling_active  = true;
    }
  }
  else {
    // Profiler data exists.
    prof_data->profiling_active  = enabled;
  }
}


/**
* Destroys whatever profiling data might be stored in this Runnable.
*/
void ManuvrRunnable::clearProfilingData() {
  if (nullptr != prof_data) {
    prof_data->profiling_active = false;
    delete prof_data;
    prof_data = nullptr;
  }
}


void ManuvrRunnable::noteExecutionTime(uint32_t profile_start_time, uint32_t profile_stop_time) {
  if (nullptr != prof_data) {
    profile_stop_time = micros();
    prof_data->run_time_last    = max(profile_start_time, profile_stop_time) - min(profile_start_time, profile_stop_time);  // Rollover invarient.
    prof_data->run_time_best    = min(prof_data->run_time_best,  prof_data->run_time_last);
    prof_data->run_time_worst   = max(prof_data->run_time_worst, prof_data->run_time_last);
    prof_data->run_time_total  += prof_data->run_time_last;
    prof_data->run_time_average = prof_data->run_time_total / ((prof_data->executions) ? prof_data->executions : 1);
    prof_data->executions++;
  }
}


/****************************************************************************************************
* Pertaining to deferred execution and scheduling....                                               *
****************************************************************************************************/

void ManuvrRunnable::fireNow(bool nu) {
  shouldFire(nu);
  thread_time_to_wait = thread_period;
}



bool ManuvrRunnable::alterSchedulePeriod(uint32_t nu_period) {
  bool return_value  = false;
  if (nu_period > 1) {
    thread_period       = nu_period;
    thread_time_to_wait = nu_period;
    return_value  = true;
  }
  return return_value;
}

bool ManuvrRunnable::alterScheduleRecurrence(int16_t recurrence) {
  fireNow(false);
  thread_recurs = recurrence;
  return true;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*/
bool ManuvrRunnable::alterSchedule(uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  bool return_value  = false;
  if (sch_period > 1) {
    if (sch_callback != nullptr) {
      fireNow(false);
      autoClear(ac);
      thread_recurs       = recurrence;
      thread_period       = sch_period;
      thread_time_to_wait = sch_period;
      schedule_callback   = sch_callback;
      return_value  = true;
    }
  }
  return return_value;
}


/**
* Returns true if...
* A) The schedule exists
*    AND
* B) The schedule is enabled, and has at least one more runtime before it *might* be auto-reaped.
*/
bool ManuvrRunnable::willRunAgain() {
  if (threadEnabled()) {
    if ((thread_recurs == -1) || (thread_recurs > 0)) {
      return true;
    }
  }
  return false;
}


/**
* Call to (en/dis)able a given schedule.
*  Will reset the time_to_wait so that if the schedule is re-enabled, it doesn't fire sooner than expected.
*  Returns true on success and false on failure.
*/
bool ManuvrRunnable::enableSchedule(bool en) {
  threadEnabled(en);
  fireNow(en);
  if (en) {
    thread_time_to_wait = thread_period;
  }
  return true;
}


/**
* Causes a given schedule's TTW (time-to-wait) to be set to the value we provide (this time only).
* If the schedule wasn't enabled before, it will be when we return.
*/
bool ManuvrRunnable::delaySchedule(uint32_t by_ms) {
  thread_time_to_wait = by_ms;
  threadEnabled(true);
  return true;
}



/****************************************************************************************************
* Actually execute this runnable.                                                                   *
****************************************************************************************************/
