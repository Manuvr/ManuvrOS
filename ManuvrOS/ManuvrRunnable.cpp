/*
File:   ManuvrRunnable.cpp
Author: J. Ian Lindsay
Date:   2015.12.18

This class began life as the merger of ScheduleItem and ManuvrEvent.
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
  callback   = cb;
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
* @param nu_pid       This schedule's unique PID. This must be passed in from outside for now.
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
*/
ManuvrRunnable::ManuvrRunnable(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback) {
  __class_initializer();
  pid                 = nu_pid;
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;

  schedule_callback   = sch_callback;    // This constructor uses the legacy callback.
}

/**
* Constructor. Takes an EventReceiver* as a callback.
* We need to deal with memory management of the Event. For a recurring schedule, we can't allow the
*   Kernel to reap the event. So it is very important to mark the event appropriately.
*
* @param nu_pid       This schedule's unique PID. This must be passed in from outside for now.
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
* @param ev           A pointer to an Event that we will periodically raise.
*/
ManuvrRunnable::ManuvrRunnable(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* sch_callback) {
  __class_initializer();
  pid                 = nu_pid;
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;

  callback            = sch_callback;   // This constructor uses the EventReceiver callback...
  schedule_callback   = NULL;

  isScheduled(true);           // Needed so we don't reap the event.
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
  callback        = NULL;
  specific_target = NULL;
  priority        = EVENT_PRIORITY_DEFAULT;
  
  // These things have implications for memory management, which is why repurpose() doesn't touch them.
  mem_managed     = false;
  scheduled       = false;
  preallocated    = false;

  prof_data       = NULL;
}


/**
* This is called when the event is being repurposed for another run through the event queue.
*
* @param code The new message id code.
* @return 0 on success, or appropriate failure code.
*/
int8_t ManuvrRunnable::repurpose(uint16_t code) {
  flags           = 0x00;
  callback        = NULL;
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


bool ManuvrRunnable::isScheduled(bool nu) {
  scheduled = nu;
  return scheduled;   
}

bool ManuvrRunnable::isManaged(bool nu) {
  mem_managed = nu;
  return mem_managed;   
}


bool ManuvrRunnable::abort() {
  //TODO: Needs the Kernel refactor complete first....
  //return Kernel::abortEvent(this);
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
  	  output->concatf("\t Callback:             %s\n", (NULL == callback ? "NULL" : callback->getReceiverName()));
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

  output->concatf("\t [%10u] Schedule \n\t --------------------------------\n", pid);

  output->concatf("\t Enabled       \t%s\n", (thread_enabled ? "YES":"NO"));
  output->concatf("\t Time-till-fire\t%u\n", thread_time_to_wait);
  output->concatf("\t Period        \t%u\n", thread_period);
  output->concatf("\t Recurs?       \t%s\n", thread_recurs);
  output->concatf("\t Exec pending: \t%s\n", (thread_fire ? "YES":"NO")); 
  output->concatf("\t Autoclear     \t%s\n", (autoclear ? "YES":"NO"));
  output->concatf("\t Profiling?    \t%s\n", (isProfiling() ? "YES":"NO"));

  if (NULL != schedule_callback) {
    output->concat("\t Legacy callback\n");
  }
}





/****************************************************************************************************
* Functions dealing with profiling data.                                                            *
****************************************************************************************************/


/**
* Any schedule that has a TaskProfilerData object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply malloc() the appropriate struct into place and initialize it.
*/
void ManuvrRunnable::enableProfiling(bool enabled) {
  if (prof_data == NULL) {
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
  if (prof_data != NULL) {
    prof_data->profiling_active = false;
    delete prof_data;
    prof_data = NULL;
  }
}


