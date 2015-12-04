/*
File:   MsgProfiler.h
Author: J. Ian Lindsay
Date:   2015.12.01

The common message-profiling container.
*/

#ifndef __MANUVR_MSG_PROFILER_H__
  #define __MANUVR_MSG_PROFILER_H__

  #include <StringBuilder/StringBuilder.h>

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

      void printDebug(StringBuilder *);
      static void printDebugHeader(StringBuilder *);
  };

#endif

