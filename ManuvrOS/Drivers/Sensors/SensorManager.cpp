/*
File:   SensorManager.cpp
Author: J. Ian Lindsay
Date:   2018.09.06


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


#include <Platform/Platform.h>
#include <EventReceiver.h>
#include "SensorWrapper.h"
#include <string.h>


#if defined(CONFIG_MANUVR_SENSOR_MGR)

const MessageTypeDef message_defs_sensors[] = {
  {  MANUVR_MSG_SENSOR_MGR_SVC,       MSG_FLAG_EXPORTABLE,   "SENSOR_MGR_SVC",    ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SENSOR_MGR_REPORT,    MSG_FLAG_EXPORTABLE,   "SENSOR_MGR_REPORT", ManuvrMsg::MSG_ARGS_NONE }  // Pass a group ID to free the channels it contains, or no args to ungroup everything.
};


/*
* Constructor.
*/
SensorManager::SensorManager() : EventReceiver("SensorManager") {
  int mes_count = sizeof(message_defs_sensors) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_sensors, mes_count);
}


/*
* Destructor.
*/
SensorManager::~SensorManager() {
}



/*******************************************************************************
* Methods for adding and removing sensors
*******************************************************************************/

/**
* @param  sensor  The SensorWrapper to add.
* @return 0 on success and -1 on failure.
*/
int8_t SensorManager::addSensor(SensorWrapper* sensor) {
  if (nullptr == sensor) return -1;

  if (0 <= _sensors.insertIfAbsent(sensor)) {
    sensor->setSensorManager(this);
  }
  return 0;
}


/**
* @param  sensor  The SensorWrapper to drop.
* @return 0 on success and -1 on failure.
*/
int8_t SensorManager::dropSensor(SensorWrapper* sensor) {
  if (nullptr == sensor) return -1;
  return (_sensors.remove(sensor) ? 0 : -1);
}


int SensorManager::_service_sensors() {
  Kernel::log("_service_sensors()\n");
  return 0;
}


int SensorManager::_report_sensors() {
  Kernel::log("_report_sensors()\n");
  return 0;
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t SensorManager::attached() {
  if (EventReceiver::attached()) {
    _sensor_report.repurpose(MANUVR_MSG_SENSOR_MGR_SVC, (EventReceiver*) this);
    _sensor_report.incRefs();
    _sensor_report.specific_target = (EventReceiver*) this;
    _sensor_report.alterScheduleRecurrence(0);
    _sensor_report.alterSchedulePeriod(5000);
    _sensor_report.autoClear(false);
    _sensor_report.enableSchedule(false);
    platform.kernel()->addSchedule(&_sensor_report);
    return 1;
  }
  return 0;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SensorManager::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
  output->concatf("-- Managing %d sensors:", _sensors.size());
  for (int i = 0; i < _sensors.size(); i++) {
    SensorWrapper* current = _sensors.get(i);
    output->concat("\n\t");
    current->printSensorSummary(output);
  }
  output->concat("\n\n");
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t SensorManager::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


int8_t SensorManager::notify(ManuvrMsg* event) {
  int8_t return_value = 0;

  switch (event->eventCode()) {
    case MANUVR_MSG_SENSOR_MGR_SVC:
      _service_sensors();
      return_value++;
      break;
    case MANUVR_MSG_SENSOR_MGR_REPORT:
      _report_sensors();
      return_value++;
      break;
    default:
      return_value += EventReceiver::notify(event);
      break;
  }

  flushLocalLog();
  return return_value;
}



#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i", "Info" },
  { "l", "List of sensors" }
};


uint SensorManager::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void SensorManager::consoleCmdProc(StringBuilder* input) {
  char* str = input->position(0);
  char c = *(str);
  int temp_int = ((*(str) != 0) ? atoi((char*) str+1) : 0);

  switch (c) {
    case 'i':
      printDebug(&local_log);
      break;

    default:
      break;
  }
  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT
#endif  //CONFIG_MANUVR_SENSOR_MGR
