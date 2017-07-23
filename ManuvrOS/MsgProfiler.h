/*
File:   MsgProfiler.h
Author: J. Ian Lindsay
Date:   2015.12.01

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


The common message-profiling container.
*/

#ifndef __MANUVR_MSG_PROFILER_H__
  #define __MANUVR_MSG_PROFILER_H__

  class StringBuilder;

  class TaskProfilerData {
    public:
      TaskProfilerData();
      ~TaskProfilerData();

      uint32_t msg_code;
      uint32_t run_time_last;
      uint32_t run_time_best;
      uint32_t run_time_worst;
      uint32_t run_time_average;
      uint32_t run_time_total;
      uint32_t executions;       // How many times has this task been used?
      bool     profiling_active;

      void printDebug(StringBuilder*);
      static void printDebugHeader(StringBuilder*);
  };

#endif // __MANUVR_MSG_PROFILER_H__
