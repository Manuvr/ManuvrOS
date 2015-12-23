#include <ManuvrOS/Kernel.h>




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
  output->concatf("%18s  %9u %9u %9u %9u %9u\n", ManuvrMsg::getMsgTypeString(msg_code), (unsigned long) run_time_total, (unsigned long) run_time_average, (unsigned long) run_time_worst, (unsigned long) run_time_best, (unsigned long) run_time_last);
}


void TaskProfilerData::printDebugHeader(StringBuilder *output) {
  output->concat("\n\t\t Execd \t\t Event \t\t total us   average     worst    best      last\n");
}

