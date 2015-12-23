/*
File:   ManuvrRunnable.cpp
Author: J. Ian Lindsay
Date:   2015.12.18

This class began life as the merger of ScheduleItem and ManuvrRunnable.
*/

#include "Kernel.h"
#include "ManuvrRunnable.h"


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
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;

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
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;

  originator          = ori;   // This constructor uses the EventReceiver callback...
  schedule_callback   = NULL;
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
  flags           = 0x00;  // TODO: Optimistic about collapsing the bools into this. Or make gcc do it.
  originator      = NULL;
  specific_target = NULL;
  prof_data       = NULL;
  priority        = EVENT_PRIORITY_DEFAULT;
  
  // These things have implications for memory management, which is why repurpose() doesn't touch them.
  mem_managed     = false;
  scheduled       = false;
  preallocated    = false;
}


/**
* This is called when the event is being repurposed for another run through the event queue.
*
* @param code The new message id code.
* @return 0 on success, or appropriate failure code.
*/
int8_t ManuvrRunnable::repurpose(uint16_t code) {
  flags           = 0x00;
  originator      = NULL;
  specific_target = NULL;
  priority        = EVENT_PRIORITY_DEFAULT;
  return ManuvrMsg::repurpose(code);
}



/**
* Was this event preallocated?
* Preallocation implies no reap.
*
* @return true if the Kernel ought to return this event to its preallocation queue.
*/
bool ManuvrRunnable::returnToPrealloc() {
  return preallocated;
}


/**
* Was this event preallocated?
* Preallocation implies no reap.
*
* @param  nu_val Pass true to cause this event to be marked as part of a preallocation pool.
* @return true if the Kernel ought to return this event to its preallocation queue.
*/
bool ManuvrRunnable::returnToPrealloc(bool nu_val) {
  preallocated = nu_val;
  //if (preallocated) mem_managed = true;
  return preallocated;
}


/**
* If the memory isn't managed explicitly by some other class, this will tell the Kernel to delete
*   the completed event.
* Preallocation implies no reap.
*
* @return true if the Kernel ought to free() this Event. False otherwise.
*/
bool ManuvrRunnable::eventManagerShouldReap() {
  if (mem_managed || preallocated || scheduled) {
    return false;
  }
  return true;
}


bool ManuvrRunnable::isManaged(bool nu) {
  mem_managed = nu;
  return mem_managed;   
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
  if (output == NULL) return;
  StringBuilder msg_serial;
  ManuvrMsg::printDebug(output);
  int arg_count = serialize(&msg_serial);
  if (0 <= arg_count) {
  	  unsigned char* temp_buf = msg_serial.string();
  	  int temp_buf_len        = msg_serial.length();
  	  
  	  output->concatf("\t Preallocated          %s\n", (preallocated ? "yes" : "no"));
  	  output->concatf("\t Originator:           %s\n", (NULL == originator ? "NULL" : originator->getReceiverName()));
  	  output->concatf("\t specific_target:      %s\n", (NULL == specific_target ? "NULL" : specific_target->getReceiverName()));
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

  output->concatf("\t [0x%08x] Schedule \n\t --------------------------------\n", (uint32_t) this);

  output->concatf("\t Enabled       \t%s\n", (thread_enabled ? "YES":"NO"));
  output->concatf("\t Time-till-fire\t%u\n", thread_time_to_wait);
  output->concatf("\t Period        \t%u\n", thread_period);
  output->concatf("\t Recurs?       \t%d\n", thread_recurs);
  output->concatf("\t Exec pending: \t%s\n", (thread_fire ? "YES":"NO")); 
  output->concatf("\t Autoclear     \t%s\n", (autoclear ? "YES":"NO"));
  output->concatf("\t Profiling?    \t%s\n", (profilingEnabled() ? "YES":"NO"));

  if (NULL != schedule_callback) {
    output->concat("\t Legacy callback\n");
  }
}


void ManuvrRunnable::printProfilerData(StringBuilder *output) {
  if (NULL != prof_data) output->concatf("\t 0x%08x  %9u  %9u  %9u  %9u  %9u  %9u %s\n", (uint32_t) this, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last, (threadEnabled() ? " " : "(INACTIVE)"));
}


/****************************************************************************************************
* Functions dealing with profiling this particular Runnable.                                        *
****************************************************************************************************/

/**
* Any schedule that has a TaskProfilerData object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply instance the appropriate struct into place.
*/
void ManuvrRunnable::profilingEnabled(bool enabled) {
  if (NULL == prof_data) {
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
  if (NULL != prof_data) {
    prof_data->profiling_active = false;
    delete prof_data;
    prof_data = NULL;
  }
}


void ManuvrRunnable::noteExecutionTime(uint32_t profile_start_time, uint32_t profile_stop_time) {
  if (NULL != prof_data) {
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
  thread_fire = nu;   
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
    if (sch_callback != NULL) {
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


