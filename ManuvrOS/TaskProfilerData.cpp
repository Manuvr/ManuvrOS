/*
File:   TaskProfilerData.cpp
Author: J. Ian Lindsay
Date:   2016.03.11

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

*/


#include <Kernel.h>

#if defined(__MANUVR_EVENT_PROFILER)
  /**
  * Constructor
  */
  TaskProfilerData::TaskProfilerData() {
    msg_code         = 0;
    run_time_last    = 0;
    run_time_best    = 0xFFFFFFFF;   // Need __something__ to compare against...
    run_time_worst   = 0;
    run_time_average = 0;
    run_time_total   = 0;
    executions       = 0;   // How many times has this task been used?
    profiling_active = false;
  }

  /**
  * Destructor
  */
  TaskProfilerData::~TaskProfilerData() {
  }


  void TaskProfilerData::printDebug(StringBuilder *output) {
    output->concatf("%18s  %9u %9u %9u %9u %9u\n",
      ManuvrMsg::getMsgTypeString(msg_code),
      (unsigned long) run_time_total,
      (unsigned long) run_time_average,
      (unsigned long) run_time_worst,
      (unsigned long) run_time_best,
      (unsigned long) run_time_last
    );
  }


  void TaskProfilerData::printDebugHeader(StringBuilder *output) {
    output->concat("\n\t\t Execd \t\t Event \t\t total us   average     worst    best      last\n");
  }

#endif  //__MANUVR_EVENT_PROFILER
