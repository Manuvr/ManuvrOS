/*
File:   Scheduler.cpp
Author: J. Ian Lindsay
Date:   2013.07.10

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
#include "StaticHub/StaticHub.h"



/****************************************************************************************************
* Class-management functions...                                                                     *
****************************************************************************************************/

/**
*  Called once on setup to bring the timer system into a known state.
*/
Scheduler::Scheduler() {
  __class_initializer();
  next_pid             = 1;      // Next PID to assign.
  currently_executing  = 0;      // Hold PID of currently-executing Schedule. 0 if none.
  productive_loops     = 0;      // Number of calls to serviceScheduledEvents() that actually called a schedule.
  total_loops          = 0;      // Number of calls to serviceScheduledEvents().
  overhead             = 0;      // The time in microseconds required to service the last empty schedule loop.
  scheduler_ready      = false;  // TODO: Convert to a uint8 and track states.
  skipped_loops        = 0;
  total_skipped_loops  = 0;
  lagged_schedules     = 0;
  bistable_skip_detect = false;  // Set in advanceScheduler(), cleared in serviceScheduledEvents().
  clicks_in_isr = 0;
}


/**
* Destructor.
*/
Scheduler::~Scheduler() {
  destroyAllScheduleItems();
}





/****************************************************************************************************
* Functions dealing with profiling data.                                                            *
****************************************************************************************************/

/**
*  Given the schedule PID, begin profiling.
*/
void Scheduler::beginProfiling(uint32_t g_pid) {
  ScheduleItem *current = findNodeByPID(g_pid);
  current->enableProfiling(true);
}


/**
*  Given the schedule PID, stop profiling.
* Stops profiling without destroying the collected data.
* Note: If profiling is ever re-started on this schedule, the profiling data
*  that this function preserves will be wiped.
*/
void Scheduler::stopProfiling(uint32_t g_pid) {
  ScheduleItem *current = findNodeByPID(g_pid);
  current->enableProfiling(false);
}


/**
*  Given the schedule PID, reset the profiling data.
*  Typically, we'd do this when the profiler is being turned on.
*/
void Scheduler::clearProfilingData(uint32_t g_pid) {
  ScheduleItem *current = findNodeByPID(g_pid);
  if (NULL != current) current->clearProfilingData();
}




/****************************************************************************************************
* Linked-list helper functions...                                                                   *
****************************************************************************************************/

// Fire the given schedule on the next idle loop.
bool Scheduler::fireSchedule(uint32_t g_pid) {
  ScheduleItem *current = this->findNodeByPID(g_pid);
  if (NULL != current) {
    current->thread_fire = true;
    current->thread_time_to_wait = current->thread_period;
    execution_queue.insert(current);
    return true;
  }
  return false;
}


/**
* Returns the number of schedules presently defined.
*/
int Scheduler::getTotalSchedules() {
  return schedules.size();
}


/**
* Returns the number of schedules presently active.
*/
unsigned int Scheduler::getActiveSchedules() {
  unsigned int return_value = 0;
  ScheduleItem *current;
  for (int i = 0; i < schedules.size(); i++) {
    current = schedules.get(i);
    if (current->thread_enabled) {
      return_value++;
    }
  }
  return return_value;
}



/**
* Destroy everything in the list. Should only be called by the destructor, but no harm
*  in calling it for other reasons. Will stop and wipe all schedules. 
*/
void Scheduler::destroyAllScheduleItems() {
  execution_queue.clear();
  while (schedules.hasNext()) {  delete schedules.dequeue();   }
}


/**
* Traverses the linked list and returns a pointer to the node that has the given PID.
* Returns NULL if a node is not found that meets this criteria.
*/
ScheduleItem* Scheduler::findNodeByPID(uint32_t g_pid) {
  ScheduleItem *current = NULL;
  for (int i = 0; i < schedules.size(); i++) {
    current = schedules.get(i);
    if (current->pid == g_pid) {
      return current;
    }
  }
  return NULL;
}


/**
*  When we assign a new PID, call this function to get one. Since we don't want
*    to collide with one that already exists, or get the zero value. 
*/
uint32_t Scheduler::get_valid_new_pid() {
    uint32_t return_value = next_pid++;
    if (return_value == 0) {
        return_value = get_valid_new_pid();  // Recurse...
    }
    // Takes too long, but represents a potential bug.
    //else if (this->findNodeByPID(return_value) == NULL) {
    //	return_value = this->get_valid_new_pid();  // Recurse...
    //}

	return return_value;
}


uint32_t Scheduler::peekNextPID() {  return next_pid;  }


/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
uint32_t Scheduler::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  uint32_t return_value  = 0;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      ScheduleItem *nu_sched = new ScheduleItem(get_valid_new_pid(), recurrence, sch_period, ac, sch_callback);
      if (nu_sched != NULL) {  // Did we actually malloc() successfully?
        return_value  = nu_sched->pid;
        schedules.insert(nu_sched);
        nu_sched->enableProfiling(return_value);
      }
    }
  }
  return return_value;
}


/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
uint32_t Scheduler::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, EventReceiver* sch_callback, ManuvrEvent* event) {
  uint32_t return_value  = 0;
  if (sch_period > 1) {
    ScheduleItem *nu_sched = new ScheduleItem(get_valid_new_pid(), recurrence, sch_period, ac, sch_callback, event);
    if (nu_sched != NULL) {  // Did we actually malloc() successfully?
      return_value  = nu_sched->pid;
      schedules.insert(nu_sched);
      nu_sched->enableProfiling(return_value);
    }
  }
  return return_value;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*/
bool Scheduler::alterSchedule(ScheduleItem *obj, uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  bool return_value  = false;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      if (obj != NULL) {
        obj->thread_fire         = false;
        obj->thread_recurs       = recurrence;
        obj->thread_period       = sch_period;
        obj->thread_time_to_wait = sch_period;
        obj->autoclear           = ac;
        obj->schedule_callback   = sch_callback;
        return_value  = true;
      }
    }
  }
  return return_value;
}

bool Scheduler::alterSchedule(uint32_t g_pid, uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  return alterSchedule(findNodeByPID(g_pid), sch_period, recurrence, ac, sch_callback);
}

bool Scheduler::alterSchedule(uint32_t schedule_index, bool ac) {
  bool return_value  = false;
  ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->autoclear = ac;
    return_value  = true;
  }
  return return_value;
}

bool Scheduler::alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback) {
  bool return_value  = false;
  if (sch_callback != NULL) {
    ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->schedule_callback   = sch_callback;
      return_value  = true;
    }
  }
  return return_value;
}

bool Scheduler::alterSchedulePeriod(uint32_t schedule_index, uint32_t sch_period) {
  bool return_value  = false;
  if (sch_period > 1) {
    ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->thread_fire         = false;
      nu_sched->thread_period       = sch_period;
      nu_sched->thread_time_to_wait = sch_period;
      return_value  = true;
    }
  }
  return return_value;
}

bool Scheduler::alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence) {
  bool return_value  = false;
  ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->thread_fire         = false;
    nu_sched->thread_recurs       = recurrence;
    return_value  = true;
  }
  return return_value;
}


/**
* Returns true if...
* A) The schedule exists
*    AND
* B) The schedule is enabled, and has at least one more runtime before it *might* be auto-reaped.
*/
bool Scheduler::willRunAgain(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    if (nu_sched->thread_enabled) {
      if ((nu_sched->thread_recurs == -1) || (nu_sched->thread_recurs > 0)) {
        return true;
      }
    }
  }
  return false;
}


bool Scheduler::scheduleEnabled(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    return nu_sched->thread_enabled;
  }
  return false;
}


/**
* Enable a previously disabled schedule.
*  Returns true on success and false on failure.
*/
bool Scheduler::enableSchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    nu_sched->thread_enabled = true;
    return true;
  }
  return false;
}


bool Scheduler::delaySchedule(ScheduleItem *obj, uint32_t by_ms) {
  if (obj != NULL) {
    obj->thread_time_to_wait = by_ms;
    obj->thread_enabled = true;
    return true;
  }
  return false;
}

/**
* Causes a given schedule's TTW (time-to-wait) to be set to the value we provide (this time only).
* If the schedule wasn't enabled before, it will be when we return.
*/
bool Scheduler::delaySchedule(uint32_t g_pid, uint32_t by_ms) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  return delaySchedule(nu_sched, by_ms);
}

/**
* Causes a given schedule's TTW (time-to-wait) to be reset to its period.
* If the schedule wasn't enabled before, it will be when we return.
*/
bool Scheduler::delaySchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched = findNodeByPID(g_pid);
  return delaySchedule(nu_sched, nu_sched->thread_period);
}



/**
* Call this function to push the schedules forward.
* We can be assured of exclusive access to the scheules queue as long as we
*   are in an ISR.
*/
void Scheduler::advanceScheduler() {
  clicks_in_isr++;
  
  //int x = schedules.size();
  //ScheduleItem *current;
  //
  //for (int i = 0; i < x; i++) {
  //  current = schedules.recycle();
  //  if (current->thread_enabled) {
  //    if (current->thread_time_to_wait > 0) {
  //      current->thread_time_to_wait--;
  //    }
  //    else {
  //      current->thread_fire = true;
  //      current->thread_time_to_wait = current->thread_period;
  //      execution_queue.insert(current);
  //    }
  //  }
  //}
  if (bistable_skip_detect) {
    // Failsafe block
    if (skipped_loops > 5000) {
      // TODO: We are hung in a way that we probably cannot recover from. Reboot...
#ifdef STM32F4XX
      jumpToBootloader();
#endif
    }
    else if (skipped_loops == 2000) {
      printf("Hung scheduler...\n");
    }
    else if (skipped_loops == 2040) {
      /* Doing all this String manipulation in an ISR would normally be an awful idea.
         But we don't care here because we're hung anyhow, and we need to know why. */
      StringBuilder output;
      StaticHub::getInstance()->fetchEventManager()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 2200) {
      StringBuilder output;
      printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 3500) {
      StringBuilder output;
      StaticHub::getInstance()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    skipped_loops++;
  }
  else {
    bistable_skip_detect = true;
  }
}


/**
* Call to disable a given schedule.
*  Will reset the time_to_wait so that if the schedule is re-enabled, it doesn't fire sooner than expected.
*  Returns true on success and false on failure.
*/
bool Scheduler::disableSchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
      nu_sched->thread_enabled = false;
      nu_sched->thread_fire    = false;
      nu_sched->thread_time_to_wait = nu_sched->thread_period;
      return true;
  }
  return false;
}


/**
* Will remove the indicated schedule and wipe its profiling data.
* In case this gets called from the schedule's service function (IE,
*   if the schedule tries to delete itself), let it expire this run
*   rather than ripping the rug out from under ourselves.
*
* @param  A pointer to the schedule item to be removed. 
* @return true on success and false on failure.
*/
bool Scheduler::removeSchedule(ScheduleItem *obj) {
  if (obj != NULL) {
    if (obj->pid != this->currently_executing) {
      schedules.remove(obj);
      execution_queue.remove(obj);
      delete obj;
    }
    else { 
      obj->autoclear = true;
      obj->thread_recurs = 0;
    }
    return true;
  }
  return false;
}

bool Scheduler::removeSchedule(uint32_t g_pid) {
  ScheduleItem *obj  = findNodeByPID(g_pid);
  return removeSchedule(obj);
}


/**
* This is the function that is called from the main loop to offload big
*  tasks into idle CPU time. If many scheduled items have fired, function
*  will churn through all of them. The presumption is that they are 
*  latency-sensitive.
*/
int Scheduler::serviceScheduledEvents() {
  if (!scheduler_ready) return -1;
  int return_value = 0;
  uint32_t origin_time = micros();


  uint32_t temp_clicks = clicks_in_isr;
  clicks_in_isr = 0;
  if (0 == temp_clicks) {
    return return_value;
  }
  
  int x = schedules.size();
  ScheduleItem *current;
  
  for (int i = 0; i < x; i++) {
    current = schedules.recycle();
    if (current->thread_enabled) {
      if (current->thread_time_to_wait > temp_clicks) {
        current->thread_time_to_wait -= temp_clicks;
      }
      else {
        current->thread_fire = true;
        uint32_t adjusted_ttw = (temp_clicks - current->thread_time_to_wait);
        if (adjusted_ttw <= current->thread_period) {
          current->thread_time_to_wait = current->thread_period - adjusted_ttw;
        }
        else {
          // TODO: Possible error-case? Too many clicks passed. We have schedule jitter...
          // For now, we'll just throw away the difference.
          current->thread_time_to_wait = current->thread_period;
          lagged_schedules++;
        }
        execution_queue.insert(current);
      }
    }
  }
  
  uint32_t profile_start_time = 0;
  uint32_t profile_stop_time  = 0;

  current = execution_queue.dequeue();
  while (NULL != current) {
    if (current->thread_fire) {
      currently_executing = current->pid;
      TaskProfilerData *prof_data = NULL;

      if (current->isProfiling()) {
        prof_data = current->prof_data;
        profile_start_time = micros();
      }

      if (NULL != current->event) {  // This is an event-based schedule.
        //if (NULL != current->event->specific_target) {
        //  // TODO: Note: If we do this, we are outside of the profiler's scope.
        //  current->event->specific_target->notify(current->event);   // Pitch the schedule's event.
        //  
        //  if (NULL != current->event->callback) {
        //    current->event->callback->callback_proc(current->event);
        //  }
        //}
        //else {
          EventManager::staticRaiseEvent(current->event);
        //}
      }
      else if (NULL != current->schedule_callback) {
        ((void (*)(void)) current->schedule_callback)();   // Call the schedule's service function.
      }
      
      if (NULL != prof_data) {
        profile_stop_time = micros();
        prof_data->run_time_last    = max(profile_start_time, profile_stop_time) - min(profile_start_time, profile_stop_time);  // Rollover invarient.
        prof_data->run_time_best    = min(prof_data->run_time_best,  prof_data->run_time_last);
        prof_data->run_time_worst   = max(prof_data->run_time_worst, prof_data->run_time_last);
        prof_data->run_time_total  += prof_data->run_time_last;
        prof_data->run_time_average = prof_data->run_time_total / ((prof_data->executions) ? prof_data->executions : 1);
        prof_data->executions++;
      }            
      current->thread_fire = false;
      currently_executing = 0;
         
      switch (current->thread_recurs) {
        case -1:           // Do nothing. Schedule runs indefinitely.
          break;
        case 0:            // Disable (and remove?) the schedule.
          if (current->autoclear) {
            removeSchedule(current);
          }
          else {
            current->thread_enabled = false;  // Disable the schedule...
            current->thread_fire    = false;  // ...mark it as serviced.
            current->thread_time_to_wait = current->thread_period;  // ...and reset the timer.
          }
          break;
        default:           // Decrement the run count.
          current->thread_recurs--;
          break;
      }
      return_value++;
    }
    current = execution_queue.dequeue();
  }
  overhead = micros() - origin_time;
  total_loops++;
  if (return_value > 0) productive_loops++;
  
  // We just ran a loop. Punch the bistable swtich and clear the skip count.
  bistable_skip_detect = false;
  total_skipped_loops += skipped_loops;
  skipped_loops        = 0;
  
  return return_value;
}



void Scheduler::printProfiler(StringBuilder* output) {
  if (NULL == output) return;
  
  bool header_unprinted = true;
  if (schedules.size() > 0) {
    ScheduleItem *current;

    StringBuilder active_str;
    StringBuilder secondary_str;
    for (int i = 0; i < schedules.size(); i++) {
      current = schedules.get(i);
      if (current->isProfiling()) {
        TaskProfilerData *prof_data = current->prof_data;
        if (header_unprinted) {
          header_unprinted = false;
          output->concat("\t PID         Execd      total us   average    worst      best       last\n\t -----------------------------------------------------------------------------\n");
        }
        if (current->thread_enabled) {
          active_str.concatf("\t %10u  %9d  %9d  %9d  %9d  %9d  %9d\n", current->pid, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last);
        }
        else {
          secondary_str.concatf("\t %10u  %9d  %9d  %9d  %9d  %9d  %9d  (INACTIVE)\n", current->pid, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last);
        }
        
        output->concatHandoff(&active_str);
        output->concatHandoff(&secondary_str);
      }
    }
  }
  
  if (header_unprinted) {
    output->concat("Nothing being profiled.\n");
  }
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* Scheduler::getReceiverName() {  return "Scheduler";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void Scheduler::printDebug(StringBuilder *output) {
	if (NULL == output) return;
	EventReceiver::printDebug(output);
  output->concatf("--- Schedules location:  0x%08x\n", &schedules);
  output->concatf("--- Total loops:      %u\n--- Productive loops: %u\n---Skipped loops: %u\n---Lagged schedules %u\n", (unsigned long) total_loops, (unsigned long) productive_loops, (unsigned long) total_skipped_loops, (unsigned long) lagged_schedules);
  if (total_loops) output->concatf("--- Duty cycle:       %2.4f%\n--- Overhead:         %d microseconds\n", ((double)((double) productive_loops / (double) total_loops) * 100), overhead);
  output->concatf("--- Next PID:         %u\n--- Total schedules:  %d\n--- Active schedules: %d\n\n", peekNextPID(), getTotalSchedules(), getActiveSchedules());
  
  printProfiler(output);
}



void Scheduler::printSchedule(uint32_t g_pid, StringBuilder* output) {
  if (NULL == output) return;

  ScheduleItem *nu_sched;
  nu_sched  = findNodeByPID(g_pid);
  if (NULL != nu_sched) nu_sched->printDebug(output);
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
 ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 

These are overrides from EventReceiver interface...
****************************************************************************************************/

/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t Scheduler::bootComplete() {
  scheduler = this;
  scheduler_ready = true;
  boot_completed = true;
  return 1;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t Scheduler::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }
  
  return return_value;
}




int8_t Scheduler::notify(ManuvrEvent *active_event) {
  uint32_t temp_uint32 = 0;
  int8_t return_value = 0;
  switch (active_event->event_code) {
    /* General system events */
    //case MANUVR_MSG_SYS_BOOT_COMPLETED:
    //  //scheduler_ready = true;
    //  break;
    case MANUVR_MSG_INTERRUPTS_MASKED:
      break;
    case MANUVR_MSG_SYS_REBOOT:
      break;
    case MANUVR_MSG_SYS_BOOTLOADER:
      break;
      
    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SCHED_ENABLE_BY_PID:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        if (enableSchedule(temp_uint32)) {
          return_value++;
        }
      }
      break;
    case MANUVR_MSG_SCHED_DISABLE_BY_PID:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        if (disableSchedule(temp_uint32)) {
          return_value++;
        }
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_START:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        beginProfiling(temp_uint32);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_STOP:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        stopProfiling(temp_uint32);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_DUMP:
      if (active_event->args.size() > 0) {
        while (0 == active_event->consumeArgAs(&temp_uint32)) {
          printProfiler(&local_log);
          return_value++;
        }
      }
      else {
        printProfiler(&local_log);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_DUMP_META:
      printDebug(&local_log);
      return_value++;
      break;
    case MANUVR_MSG_SCHED_DUMP_SCHEDULES:
      if (active_event->args.size() > 0) {
        ScheduleItem *nu_sched;
        while (0 == active_event->consumeArgAs(&temp_uint32)) {
          nu_sched  = findNodeByPID(temp_uint32);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
        }
      }
      else {
        ScheduleItem *nu_sched;
        for (int i = 0; i < schedules.size(); i++) {
          nu_sched  = schedules.get(i);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
          return_value++;
        }
      }
      return_value++;
      break;
    case MANUVR_MSG_SCHED_WIPE_PROFILER:
      break;
    case MANUVR_MSG_SCHED_DEFERRED_EVENT:
      {
        //uint32_t period = 1000;
        //int16_t recurrence = 1;
        ////sch_callback
        //if (createSchedule(period, recurrence, true, FunctionPointer sch_callback)) {
        //  return_value++;
        //}
      }
      break;
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}




void Scheduler::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'i':
      printDebug(&local_log);
      break;
    case 'S':
      if (temp_byte) {
        ScheduleItem *nu_sched;
        nu_sched  = findNodeByPID(temp_byte);
        if (NULL != nu_sched) nu_sched->printDebug(&local_log);
      }
      else {
        ScheduleItem *nu_sched;
        for (int i = 0; i < schedules.size(); i++) {
          nu_sched  = schedules.get(i);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
        }
      }
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
#endif
}


