/*
File:   StopWatch.h
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
*/

#include <inttypes.h>
#include <stdint.h>
#include <Platform/Platform.h>


#ifndef __STOP_WATCH_H__
  #define __STOP_WATCH_H__

  class StringBuilder;

  class StopWatch {
    public:
      uint32_t  tag = 0;
      StopWatch();       // Constructor. Calls reset().
      ~StopWatch() {};   // Featureless destructor.

      inline uint32_t bestTime() {     return _run_time_best;      };
      inline uint32_t lastTime() {     return _run_time_last;      };
      inline uint32_t worstTime() {    return _run_time_worst;     };
      inline uint32_t meanTime() {     return _run_time_average;   };
      inline uint32_t totalTime() {    return _run_time_total;     };
      inline uint32_t executions() {   return _executions;         };
      inline void     markStart() {    _start_micros = micros();   };
      bool  markStop();
      void  reset();
      void printDebug(const char*, StringBuilder*);

      static void printDebugHeader(StringBuilder*);


    private:
      uint32_t _start_micros;
      uint32_t _run_time_last;
      uint32_t _run_time_best;
      uint32_t _run_time_worst;
      uint32_t _run_time_average;
      uint32_t _run_time_total;
      uint32_t _executions;
  };

#endif // __STOP_WATCH_H__
