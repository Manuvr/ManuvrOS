/*
File:   ScheduleItem.cpp
Author: J. Ian Lindsay
Date:   2015.01.18

This class is meant to be a real-time task scheduler for small microcontrollers. It
should be driven by a periodic interrupt of some sort, but it may also be effectively
used with a reliable polling scheme (at the possible cost of timing accuracy).

A simple profiler is included which will allow the user of this class to determine
run-times and possibly even adjust task duty cycles accordingly.

Copyright (C) 2013 J. Ian Lindsay
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

#include "Scheduler.h"



/**
* Constructor. Takes a void fxn(void) as a callback (Legacy).
*
* @param nu_pid       This schedule's unique PID. This must be passed in from outside for now.
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
*/
ScheduleItem::ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback) {
  pid                 = nu_pid;
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;
  prof_data           = NULL;

  schedule_callback   = sch_callback;    // This constructor uses the legacy callback.
  event               = NULL;
  callback_to_er      = NULL;
}

/**
* Constructor. Takes an EventReceiver* as a callback.
* We need to deal with memory management of the Event. For a recurring schedule, we can't allow the
*   EventManager to reap the event. So it is very important to mark the event appropriately.
*
* @param nu_pid       This schedule's unique PID. This must be passed in from outside for now.
* @param recurrence   How many times should this schedule run?
* @param sch_period   How often should this schedule run (in milliseconds).
* @param ac           Should the scheduler autoclear this schedule when it finishes running?
* @param sch_callback A FunctionPointer to the callback. Useful for some general things.
* @param ev           A pointer to an Event that we will periodically raise.
*/
ScheduleItem::ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* sch_callback, ManuvrEvent* ev) {
  pid                 = nu_pid;
  thread_enabled      = true;
  thread_fire         = false;
  thread_recurs       = recurrence;
  thread_period       = sch_period;
  thread_time_to_wait = sch_period;
  autoclear           = ac;
  prof_data           = NULL;

  callback_to_er      = sch_callback;   // This constructor uses the EventReceiver callback...
  schedule_callback   = NULL;
  event               = ev;             // ...and mandates an event.
  
  event->isScheduled(true);           // Needed so we don't reap the event.
}


/**
* Destructor. Should make sure the scheduler isn't going to invoke this schedule again.
*/
ScheduleItem::~ScheduleItem() {
  clearProfilingData();
  if (NULL != event) {                    // If there was an Event tied to this schedule, we need to see if we ought to free it.
    if (!event->isManaged()) {            // But only if its mem is managed elsewhere....
      if (!event->returnToPrealloc()) {   // ...and only if it wasn't in the preallocation pool.
        // TODO: Seems sketchy... Needs to be re-thought. But should work for now.
        // Might crash if the event is fired, and this schedule is reaped prior to event execution.
        delete event;
        event = NULL;
      }
    }
  }
}




/****************************************************************************************************
* Functions dealing with profiling data.                                                            *
****************************************************************************************************/

/**
* Asks if this schedule is being profiled...
*  Returns true if so, and false if not.
*  Also returns false in the event that the ScheduleItem is NULL.
*/
bool ScheduleItem::isProfiling() {
  if (prof_data != NULL) {
    return prof_data->profiling_active;
  }
  return false;
}



/**
* Any schedule that has a TaskProfilerData object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply malloc() the appropriate struct into place and initialize it.
*/
void ScheduleItem::enableProfiling(bool enabled) {
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
* Destroys whatever profiling data might be stored in this schedule.
*/
void ScheduleItem::clearProfilingData() {
  if (prof_data != NULL) {
    prof_data->profiling_active = false;
    delete prof_data;
    prof_data = NULL;
  }
}


// TODO: Style explanation... short-circuit eval and NULL-checking.
void ScheduleItem::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  output->concatf("\t [%10u] Schedule \n\t --------------------------------\n", pid);
                  
  output->concatf("\t Enabled       \t%s\n", (thread_enabled ? "YES":"NO"));
  output->concatf("\t Time-till-fire\t%u\n", thread_time_to_wait);
  output->concatf("\t Period        \t%u\n", thread_period);
  output->concatf("\t Recurs?       \t%s\n", thread_recurs);
  output->concatf("\t Exec pending: \t%s\n", (thread_fire ? "YES":"NO")); 
  output->concatf("\t Autoclear     \t%s\n", (autoclear ? "YES":"NO"));
  output->concatf("\t Profiling?    \t%s\n", ((prof_data != NULL && prof_data->profiling_active) ? "YES":"NO"));

  if (NULL != callback_to_er) {
    output->concatf("\t EventReceiver:   %s\n", (char*)callback_to_er->getReceiverName());
    if (NULL != event) {
      event->printDebug(output);
    }
  }
  else if (NULL != schedule_callback) {
    output->concat("\t Legacy callback\n");
  }
  else {
    output->concat("\t No callback of any sort. This is a problem.\n");
  }
}



