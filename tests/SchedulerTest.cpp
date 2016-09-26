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
*/

#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <DataStructures/StringBuilder.h>

#include <Platform/Platform.h>


/*
*
*/
int SCHEDULER_INIT_STATE() {
  return -1;
}


/*
*
*/
int SCHEDULER_CREATE_SCHEDULES() {
  return -1;
}


/*
*
*/
int SCHEDULER_EXEC_SCHEDULES() {
  return -1;
}


/*
*
*/
int SCHEDULER_HANG() {
  return -1;
}


/*
*
*/
int SCHEDULER_COMPARE_AGAINST_RTC() {
  return -1;
}


/*
*
*/
int SCHEDULER_DESTROY_SCHEDULES() {
  return -1;
}


/*
*
*/
int SCHEDULER_COMPARE_RESULTS() {
  return -1;
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

  printf("EVENT_MANAGER_PREALLOC_COUNT     %d\n", EVENT_MANAGER_PREALLOC_COUNT);
  printf("MANUVR_PLATFORM_TIMER_PERIOD_MS  %d\n", MANUVR_PLATFORM_TIMER_PERIOD_MS);
  printf("MAXIMUM_SEQUENTIAL_SKIPS         %d\n", MAXIMUM_SEQUENTIAL_SKIPS);

  platform.platformPreInit();   // Our test fixture needs random numbers.

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
