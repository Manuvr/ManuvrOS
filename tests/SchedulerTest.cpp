/*
File:   SchedulerTest.cpp
Author: J. Ian Lindsay
Date:   2016.09.25

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


This program runs tests on Scheduler reliability and resilience in the
  face of unlikely, but inevitable, problems..
We assume that the scheduler is being driven from a valid time source. Since we
  have to abuse that time source at run-time, this test must run on linux.
*/

#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <StringBuilder.h>
#include <Platform/Platform.h>



/*
* Globals.
*/
ManuvrMsg  schedule_0;
ManuvrMsg  schedule_1;
ManuvrMsg* schedule_2 = nullptr;
ManuvrMsg* schedule_3 = nullptr;

int count_0 = 0;

void sched_0_cb() {
  printf("\t sched_0_cb(): %d \t willRunAgain() = %s\n",
    count_0++,
    (schedule_0.willRunAgain() ? "true":"false")
  );
}

void sched_1_cb() {
}

void sched_2_cb() {
}

void sched_3_cb() {
}


/*
*
*/
int SCHEDULER_INIT_STATE() {
  printf("===< SCHEDULER_INIT_STATE >======================================\n");
  return 0;
}


/*
*
*/
int SCHEDULER_CREATE_SCHEDULES() {
  printf("===< SCHEDULER_CREATE_SCHEDULES >================================\n");
  schedule_0.repurpose(MANUVR_MSG_DEFERRED_FXN);
  schedule_1.repurpose(MANUVR_MSG_DEFERRED_FXN);
  schedule_0.incRefs();
  schedule_1.incRefs();

  if (schedule_0.alterSchedulePeriod(100)) {
    if (schedule_0.alterScheduleRecurrence(50)) {
      if (schedule_0.alterSchedule(sched_0_cb)) {
        if (platform.kernel()->addSchedule(&schedule_0)) {
          return 0;
        }
        else printf("Failed to add achedule.\n");
      }
      else printf("Failed to schedule_0.alterSchedule() with a callback.\n");
    }
    else printf("Failed to schedule_0.alterScheduleRecurrence()\n");
  }
  else printf("Failed to schedule_0.alterSchedulePeriod()\n");
  return -1;
}


/*
*
*/
int SCHEDULER_EXEC_SCHEDULES() {
  printf("===< SCHEDULER_EXEC_SCHEDULES >==================================\n");
  return 0;
}


/*
*
*/
int SCHEDULER_HANG() {
  printf("===< SCHEDULER_HANG >============================================\n");
  return 0;
}


/*
*
*/
int SCHEDULER_COMPARE_AGAINST_RTC() {
  printf("===< SCHEDULER_COMPARE_AGAINST_RTC >=============================\n");
  return 0;
}


/*
*
*/
int SCHEDULER_DESTROY_SCHEDULES() {
  printf("===< SCHEDULER_DESTROY_SCHEDULES >===============================\n");
  return 0;
}


/*
*
*/
int SCHEDULER_COMPARE_RESULTS() {
  printf("===< SCHEDULER_COMPARE_RESULTS >=================================\n");
  return 0;
}



void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.

  platform.platformPreInit();   // Our test fixture needs random numbers.
  platform.bootstrap();

  printf("EVENT_MANAGER_PREALLOC_COUNT     %d\n", EVENT_MANAGER_PREALLOC_COUNT);
  printf("MANUVR_PLATFORM_TIMER_PERIOD_MS  %d\n", MANUVR_PLATFORM_TIMER_PERIOD_MS);
  printf("MAXIMUM_SEQUENTIAL_SKIPS         %d\n", MAXIMUM_SEQUENTIAL_SKIPS);

  if (0 == SCHEDULER_INIT_STATE()) {
    if (0 == SCHEDULER_CREATE_SCHEDULES()) {
      if (0 == SCHEDULER_EXEC_SCHEDULES()) {
        if (0 == SCHEDULER_HANG()) {
          if (0 == SCHEDULER_COMPARE_AGAINST_RTC()) {
            if (0 == SCHEDULER_DESTROY_SCHEDULES()) {
              if (0 == SCHEDULER_COMPARE_RESULTS()) {
                printf("**********************************\n");
                printf("*  Scheduler tests all pass      *\n");
                printf("**********************************\n");
                exit_value = 0;
              }
              else printTestFailure("SCHEDULER_COMPARE_RESULTS");
            }
            else printTestFailure("SCHEDULER_DESTROY_SCHEDULES");
          }
          else printTestFailure("SCHEDULER_COMPARE_AGAINST_RTC");
        }
        else printTestFailure("SCHEDULER_HANG");
      }
      else printTestFailure("SCHEDULER_EXEC_SCHEDULES");
    }
    else printTestFailure("SCHEDULER_CREATE_SCHEDULES");
  }
  else printTestFailure("SCHEDULER_INIT_STATE");

  exit(exit_value);
}
