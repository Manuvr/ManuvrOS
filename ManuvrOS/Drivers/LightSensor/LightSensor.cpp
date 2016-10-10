/*
File:   LightSensor.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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


This is a quick-and-dirty class to support reading a CdS cell from an analog
  pin, and then relaying the uncontrolled, unitless value out to the other
  modules in the system.
*/


#include "LightSensor.h"
#include <Platform/Platform.h>

uint16_t last_lux_read = 0;
uint8_t  last_lux_bin  = 0;

// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_light_sensor[] = {
  {  MANUVR_MSG_AMBIENT_LIGHT_LEVEL,  MSG_FLAG_EXPORTABLE,  "LIGHT_LEVEL",  ManuvrMsg::MSG_ARGS_NONE  }, // Unitless light-level report.
};



LightSensor::LightSensor(int analog_pin) : EventReceiver() {
  _analog_pin = analog_pin;
  setReceiverName("LightSensor");
  int mes_count = sizeof(message_defs_light_sensor) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_light_sensor, mes_count);
}


LightSensor::~LightSensor() {
  _periodic_check.enableSchedule(false);
  platform.kernel()->removeSchedule(&_periodic_check);
}


void LightSensor::light_check() {
  uint16_t current_lux_read = readPinAnalog(_analog_pin);
  uint8_t current_lux_bin = current_lux_read >> 2;

  uint8_t lux_delta = (last_lux_bin > current_lux_bin) ? (last_lux_bin - current_lux_bin) : (current_lux_bin - last_lux_bin);
  if (lux_delta > 3) {
    last_lux_bin = current_lux_bin;
    // This will cause broadcase of our timed event...
    _periodic_check.specific_target = nullptr;
  }
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
int8_t LightSensor::attached() {
  if (EventReceiver::attached()) {
    light_check();
    // Build some pre-formed Events.
    _periodic_check.repurpose(MANUVR_MSG_AMBIENT_LIGHT_LEVEL, (EventReceiver*) this);
    _periodic_check.isManaged(true);
    _periodic_check.specific_target = (EventReceiver*) this;
    _periodic_check.addArg((uint8_t*) &last_lux_bin);
    _periodic_check.alterScheduleRecurrence(-1);
    _periodic_check.alterSchedulePeriod(501);
    _periodic_check.autoClear(false);
    _periodic_check.enableSchedule(true);
    platform.kernel()->addSchedule(&_periodic_check);
    return 1;
  }
  return 0;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void LightSensor::printDebug(StringBuilder *output) {
  if (output == NULL) return;

  EventReceiver::printDebug(output);
  output->concat("\n");
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
int8_t LightSensor::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_AMBIENT_LIGHT_LEVEL:
      // Our event has come back to us after a full cycle. Refresh the reading.
      _periodic_check.specific_target = (EventReceiver*) this;
      light_check();
      break;
    default:
      break;
  }

  return return_value;
}


int8_t LightSensor::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_AMBIENT_LIGHT_LEVEL:
      // Our event has come back to us after a full cycle. Refresh the reading.
      return_value++;
      break;
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  flushLocalLog();
  return return_value;
}
