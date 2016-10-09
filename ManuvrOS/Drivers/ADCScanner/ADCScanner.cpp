/*
File:   ADCScanner.cpp
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

*/


#include "ADCScanner.h"
#include <Platform/Platform.h>

// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_adc_scanner[] = {
  {  MANUVR_MSG_ADC_SCAN,   0x0000,  "ADC_SCAN",  ManuvrMsg::MSG_ARGS_NONE }, // It is time to scan the ADC channels.
};



ADCScanner::ADCScanner() : EventReceiver() {
  setReceiverName("ADCScanner");
  int mes_count = sizeof(message_defs_adc_scanner) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_adc_scanner, mes_count);
  for (int i = 0; i < 16; i++) {
    adc_list[i] = -1;
    last_sample[i] = 0;
    threshold[i] = 0;
  }
}


ADCScanner::~ADCScanner() {
  _periodic_check.enableSchedule(false);
  platform.kernel()->removeSchedule(&_periodic_check);
}


uint16_t ADCScanner::getSample(int8_t idx) {
  if ((idx >= 0) && (idx < 16)) {
    return last_sample[idx];
  }
  return 0;
}


void ADCScanner::addADCPin(int8_t pin_no) {
  if (pin_no >= 0) {
    for (int i = 0; i < 16; i++) {
      if (-1 == adc_list[i]) {
        adc_list[i] = pin_no;
        return;
      }
    }
  }
}


void ADCScanner::delADCPin(int8_t pin_no) {
  if (pin_no >= 0) {
    for (int i = 0; i < 16; i++) {
      if (pin_no == adc_list[i]) {
        adc_list[i] = -1;
        return;
      }
    }
  }
}



int8_t ADCScanner::scan() {
  int8_t return_value = 0;
  uint16_t current_sample = 0;
  for (int i = 0; i < 16; i++) {
    if (-1 != adc_list[i]) {
      current_sample = readPinAnalog(adc_list[i]);
      if (threshold[i] < current_sample) {
        return_value++;
      }
      last_sample[i] = current_sample;
    }
    else {
      last_sample[i] = 0;
    }
  }
  return return_value;
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
int8_t ADCScanner::attached() {
  EventReceiver::attached();

  // Build some pre-formed Events.
  _periodic_check.repurpose(MANUVR_MSG_ADC_SCAN, (EventReceiver*) this);
  _periodic_check.isManaged(true);
  _periodic_check.specific_target = (EventReceiver*) this;

  _periodic_check.alterScheduleRecurrence(-1);
  _periodic_check.alterSchedulePeriod(50);
  _periodic_check.autoClear(false);
  _periodic_check.enableSchedule(true);
  platform.kernel()->addSchedule(&_periodic_check);
  return 1;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ADCScanner::printDebug(StringBuilder *output) {
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
int8_t ADCScanner::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


int8_t ADCScanner::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_ADC_SCAN:
      if (scan()) {

      }
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
  return return_value;
}
