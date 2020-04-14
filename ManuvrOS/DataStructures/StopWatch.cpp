/*
File:   StopWatch.cpp
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

#include "Utilities.h"
#include "StopWatch.h"
#include "StringBuilder.h"

/**
* Constructor
*/
StopWatch::StopWatch() {
  reset();
}


void StopWatch::reset() {
  _run_time_last    = 0;
  _run_time_best    = 0xFFFFFFFF;   // Need __something__ to compare against...
  _run_time_worst   = 0;
  _run_time_average = 0;
  _run_time_total   = 0;
  _executions       = 0;   // How many times has this task been used?
}


bool StopWatch::markStop() {
  uint32_t stop_micros = micros();
  bool ret = false;
  if (_start_micros > 0) {
    _executions++;
    _run_time_last    = wrap_accounted_delta(_start_micros, stop_micros);
    _run_time_best    = strict_min(_run_time_last, _run_time_best);
    _run_time_worst   = strict_max(_run_time_last, _run_time_worst);
    _run_time_total  += _run_time_last;
    _run_time_average = _run_time_total / _executions;
    _start_micros    = 0;
    ret = true;
  }
  return ret;
}


void StopWatch::printDebug(const char* label, StringBuilder *output) {
  if (_executions) {
    output->concatf("%14s %10u %10u %10u %10u %10u %10u\n",
      label,
      _executions,
      (unsigned long) _run_time_total,
      (unsigned long) _run_time_average,
      (unsigned long) _run_time_worst,
      (unsigned long) _run_time_best,
      (unsigned long) _run_time_last
    );
  }
  else {
    output->concatf("%14s <NO DATA>\n", label);
  }
}


void StopWatch::printDebugHeader(StringBuilder *output) {
  output->concat("          Name      Execd   total us    average      worst       best       last\n");
  output->concat("--------------------------------------------------------------------------------\n");
}
