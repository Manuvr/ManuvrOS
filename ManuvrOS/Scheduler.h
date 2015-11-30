/*
File:   Scheduler.h
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

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <inttypes.h>

#include "FirmwareDefs.h"
#include "StringBuilder/StringBuilder.h"
#include <ManuvrOS/Platform/Platform.h>
#include "ManuvrOS/EventManager.h"



/* Type for schedule items... */
class ScheduleItem {
  public:
    uint32_t pid;                      // The process ID of this item. Zero is invalid.
    uint32_t thread_time_to_wait;      // How much longer until the schedule fires?
    uint32_t thread_period;            // How often does this schedule execute?
    FunctionPointer schedule_callback; // Pointers to the schedule service function.
    ManuvrEvent* event;                // If we have an event to fire, point to it here.

    TaskProfilerData* prof_data;       // If this schedule is being profiled, the ref will be here.
    EventReceiver*  callback_to_er;    // Optional callback to an EventReceiver.
    int16_t  thread_recurs;            // See Note 2.

    bool     thread_enabled;           // Is the schedule running?
    bool     thread_fire;              // Is the schedule to be executed?
    bool     autoclear;                // If true, this schedule will be removed after its last execution.


    ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback);
    ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver*  sch_callback, ManuvrEvent*);
    ~ScheduleItem();
    
    void enableProfiling(bool enabled);
    bool isProfiling();
    void clearProfilingData();           // Clears profiling data associated with the given schedule.
    
    void printDebug(StringBuilder*);


  private:
};



/**  Note 2:
* If this value is equal to -1, then the schedule recurs for as long as it remains enabled.
*  If the value is zero, the schedule is disabled upon execution.
*  If the value is anything else, the schedule remains enabled and this value is decremented.
*/




#define SCHEDULER_MAX_SKIP_BEFORE_RESET  10   // Skipping this many loops will cause us to reboot.

// This is the only version I've tested...
class Scheduler : public EventReceiver {
  public:
    Scheduler();   // Constructor
    ~Scheduler();  // Destructor

    /* Despite being public members, these values should not be written from outside the class.
       They are only public for the sake of convenience of reading them. Since they are profiling-related,
       no major class functionality (other than the profiler output) relies on them. */
    uint32_t next_pid;            // Next PID to assign.
    uint32_t currently_executing; // Hold PID of currently-executing Schedule. 0 if none.
    uint32_t productive_loops;    // Number of calls to serviceScheduledEvents() that actually called a schedule.
    uint32_t total_loops;         // Number of calls to serviceScheduledEvents().
    uint32_t overhead;            // The time in microseconds required to service the last empty schedule loop.
    bool     scheduler_ready;     // TODO: Convert to a uint8 and track states.

    int getTotalSchedules(void);   // How many total schedules are present?
    unsigned int getActiveSchedules(void);  // How many active schedules are present?
    uint32_t peekNextPID(void);         // Discover the next PID without actually incrementing it.
    
    bool scheduleBeingProfiled(uint32_t g_pid);
    void beginProfiling(uint32_t g_pid);
    void stopProfiling(uint32_t g_pid);
    void clearProfilingData(uint32_t g_pid);        // Clears profiling data associated with the given schedule.
    
    // Alters an existing schedule (if PID is found),
    bool alterSchedule(uint32_t schedule_index, uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
    bool alterSchedule(uint32_t schedule_index, bool auto_clear);
    bool alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback);
    bool alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence);
    bool alterSchedulePeriod(uint32_t schedule_index, uint32_t sch_period);
    
    /* Add a new schedule. Returns the PID. If zero is returned, function failed.
     * 
     * Parameters:
     * sch_period      The period of the schedule service routine.
     * recurrence      How many times should this schedule run?
     * auto_clear      When recurrence reaches 0, should the schedule be reaped?
     * sch_callback    The service function. Must be a pointer to a (void fxn(void)).
     */    
    uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
    uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, EventReceiver*  sch_callback, ManuvrEvent*);
    
    bool scheduleEnabled(uint32_t g_pid);   // Is the given schedule presently enabled?

    bool enableSchedule(uint32_t g_pid);   // Re-enable a previously-disabled schedule.
    bool disableSchedule(uint32_t g_pid);  // Turn a schedule off without removing it.
    bool removeSchedule(uint32_t g_pid);   // Clears all data relating to the given schedule.
    bool delaySchedule(uint32_t g_pid, uint32_t by_ms);  // Set the schedule's TTW to the given value this execution only.
    bool delaySchedule(uint32_t g_pid);                  // Reset the given schedule to its period and enable it.
    
    bool fireSchedule(uint32_t g_pid);                  // Fire the given schedule on the next idle loop.
    
    bool willRunAgain(uint32_t g_pid);                  // Returns true if the indicated schedule will fire again.

    int serviceScheduledEvents(void);        // Execute any schedules that have come due.
    void advanceScheduler(void);              // Push all enabled schedules forward by one tick.
    
    const char* getReceiverName();

    void printDebug(StringBuilder*);
    void printProfiler(StringBuilder*);
    void printSchedule(uint32_t g_pid, StringBuilder*);
    
    /* Overrides from EventReceiver */
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);

    // DEBUG FXNS
    void procDirectDebugInstruction(StringBuilder *);
    // DEBUG FXNS

  protected:
    int8_t bootComplete();

    
  private:
    PriorityQueue<ScheduleItem*> schedules;
    PriorityQueue<ScheduleItem*> execution_queue;
    uint32_t clicks_in_isr;
    uint32_t total_skipped_loops;
    uint32_t lagged_schedules;
    
    /* These members are concerned with reliability. */
    uint16_t skipped_loops;
    bool     bistable_skip_detect;  // Set in advanceScheduler(), cleared in serviceScheduledEvents().
    
    
    bool alterSchedule(ScheduleItem *obj, uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);

    void destroyAllScheduleItems(void);
    
    bool removeSchedule(ScheduleItem *obj);
    

    uint32_t get_valid_new_pid(void);    
    ScheduleItem* findNodeByPID(uint32_t g_pid);
    void destroyScheduleItem(ScheduleItem *r_node);

    bool delaySchedule(ScheduleItem *obj, uint32_t by_ms);
};

#endif