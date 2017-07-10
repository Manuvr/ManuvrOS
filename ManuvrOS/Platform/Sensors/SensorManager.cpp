/*
File:   SensorManager.cpp
Author: J. Ian Lindsay
Date:   2016.08.13

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


This is an EventReceiver that is meant to ease the implementation burden of
  sensor I/O and management. There should only ever be one of these instanced,
  and sensors ought to be added to it.

Design goals:
  1) Fully async
*/

#include "SensorManager.h"

#ifdef CONFIG_WITH_SENSOR_AGGREGATOR

SensorManager* SensorManager::INSTANCE = nullptr;

const MessageTypeDef sensormgr_message_defs[] = {
  //{  MANUVR_MSG_ADP8866_IRQ,     0x0000,  "ADP8866_IRQ",  ManuvrMsg::MSG_ARGS_NONE },  //
  //{  MANUVR_MSG_ADP8866_CHAN_ENABLED, MSG_FLAG_EXPORTABLE,  "ADP8866_CHAN_EN", ManuvrMsg::MSG_ARGS_NONE }, //
  //{  MANUVR_MSG_ADP8866_CHAN_LEVEL,   MSG_FLAG_EXPORTABLE,  "ADP8866_CHAN_LVL",   ManuvrMsg::MSG_ARGS_NONE }, //
  //{  MANUVR_MSG_ADP8866_RESET,   0x0000,  "ADP8866_RESET",   ManuvrMsg::MSG_ARGS_NONE }, //
  //{  MANUVR_MSG_ADP8866_ASSIGN_BL,    MSG_FLAG_EXPORTABLE,  "ADP8866_ASSIGN_BL",    ManuvrMsg::MSG_ARGS_NONE }  //
};


SensorManager::SensorManager() : EventReceiver("SensorMgr") {
  if (nullptr == SensorManager::INSTANCE) {
    SensorManager::INSTANCE = this;
    int mes_count = sizeof(sensormgr_message_defs) / sizeof(MessageTypeDef);
    ManuvrMsg::registerMessages(sensormgr_message_defs, mes_count);
  }
}


SensorManager::~SensorManager() {
}



int SensorManager::registerSensor(SensorDriver* _sd) {
  return _s_list.insert(_sd);
}

int SensorManager::unregisterSensor(SensorDriver* _sd) {
  if (_s_list.remove(_sd)) {
    return -1;
  }
  return 0;
}


int SensorManager::registerDatum(SenseDatum* _sd) {
  return _d_list.insert(_sd);
}

int SensorManager::unregisterDatum(SenseDatum* _sd) {
  if (_d_list.remove(_sd)) {
    return -1;
  }
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
    return 1;
  }
  return 0;
}


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void SensorManager::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
  output->concatf("\tSensors tracked:   %d\n", _d_list.size());
  for (int i = 0; i < _d_list.size(); i++) {
    SensorDatum* sd = _d_list.get(i);
    output->concatf("\t  %d:\t%s\t", i, sd->desc);
    _d_list.get(i)->printValue(output);
    output->concat(sd->units);
    output->concat("\n");
  }
  output->concatf("\tDatums tracked:    %d\n", _d_list.size());
  for (int i = 0; i < _d_list.size(); i++) {
    SensorDatum* sd = _d_list.get(i);
    output->concatf("\t  %d:\t%s\t", i, sd->desc);
    _d_list.get(i)->printValue(output);
    output->concat(sd->units);
    output->concat("\n");
  }
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



/**
*/
int8_t SensorManager::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}



#ifdef MANUVR_CONSOLE_SUPPORT
void SensorManager::procDirectDebugInstruction(StringBuilder *input) {
  const char* str = (char *) input->position(0);
  char c    = *str;

  switch (c) {
    case 's':
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}

#endif  // CONFIG_WITH_SENSOR_AGGREGATOR
