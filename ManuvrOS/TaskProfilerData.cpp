#include <ManuvrOS/Kernel.h>




/**
* Constructor
*/
TaskProfilerData::TaskProfilerData() {
  msg_code         = 0;
  run_time_last    = 0;
  run_time_best    = 10000000;
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
}


void TaskProfilerData::printDebugHeader(StringBuilder *output) {
}

