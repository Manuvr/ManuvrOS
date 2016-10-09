/*
File:   ManuvrableGPIO.h
Author: J. Ian Lindsay
Date:   2015.09.21

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


The idea here is not to provide any manner of abstraction for GPIO. Our
  only goal is to expose GPIO functionality to outside systems.
*/

#include "ManuvrableGPIO.h"


const MessageTypeDef gpio_message_defs[] = {
  /*
    For messages that have arguments, we have the option of defining inline lables for each parameter.
    This is advantageous for debugging and writing front-ends. We case-off here to make this choice at
    compile time.
  */
  {  MANUVR_MSG_GPIO_LEGEND       , MSG_FLAG_EXPORTABLE,  "GPIO_LEGEND",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIGITAL_READ      , MSG_FLAG_EXPORTABLE,  "DIGITAL_READ",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIGITAL_WRITE     , MSG_FLAG_EXPORTABLE,  "DIGITAL_WRITE",       ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_ANALOG_READ       , MSG_FLAG_EXPORTABLE,  "ANALOG_READ",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_ANALOG_WRITE      , MSG_FLAG_EXPORTABLE,  "ANALOG_WRITE",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_EVENT_ON_INTERRUPT, MSG_FLAG_EXPORTABLE,  "EVENT_ON_INTERRUPT",  ManuvrMsg::MSG_ARGS_NONE }, //
};



ManuvrableGPIO::ManuvrableGPIO() : EventReceiver() {
  setReceiverName("GPIO");
  _gpio_notice.isManaged(true);
  _gpio_notice.setOriginator((EventReceiver*) this);
  // Inform the Kernel of the codes we will be using...
  ManuvrMsg::registerMessages(gpio_message_defs, sizeof(gpio_message_defs) / sizeof(MessageTypeDef));
}

ManuvrableGPIO::~ManuvrableGPIO() {
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
int8_t ManuvrableGPIO::attached() {
  EventReceiver::attached();
  return 0;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrableGPIO::printDebug(StringBuilder *output) {
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
int8_t ManuvrableGPIO::callback_proc(ManuvrRunnable *event) {
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


int8_t ManuvrableGPIO::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;
  uint8_t pin   = 0;
  uint16_t _val = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_DIGITAL_READ:
      if (0 != active_event->getArgAs(0, &pin)) {
        if (0 != active_event->getArgAs(1, &_val)) {
          active_event->addArg(readPin(pin));
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_DIGITAL_WRITE:
      if (0 != active_event->getArgAs(0, &pin)) {
        if (0 != active_event->getArgAs(1, &_val)) {
          setPin(pin, (0 < _val));
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_ANALOG_READ:
      if (0 != active_event->getArgAs(0, &pin)) {
        if (0 != active_event->getArgAs(1, &_val)) {
          active_event->addArg((int32_t)readPinAnalog(pin));
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_ANALOG_WRITE:
      if (0 != active_event->getArgAs(0, &pin)) {
        if (0 != active_event->getArgAs(1, &_val)) {
          setPinAnalog(pin, _val);
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_EVENT_ON_INTERRUPT:
      //int8_t setPinEvent(uint8_t pin, ManuvrRunnable* isr_event);
      return_value++;
      break;

    case MANUVR_MSG_GPIO_LEGEND:
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  flushLocalLog();
  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void ManuvrableGPIO::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  uint8_t  _pin = (input->count() > 1) ? input->position_as_int(1) : 0;
  uint16_t _val = (input->count() > 2) ? input->position_as_int(2) : 0;

  switch (*(str)) {
    case 's':   // Read a pin
      _val = readPin(_pin);
      local_log.concatf("Pin %d state: %d\n", _pin, _val);
      break;
    case 'r':   // Set a pin mode.
    case 'w':   // Set a pin mode.
      local_log.concatf("Setting pin %d as %s.\n", _pin, ((*(str) == 'r')?"INPUT":"OUTPUT"));
      gpioDefine(_pin, (*(str) == 'r') ? INPUT : OUTPUT);
      break;
    case 'M':   // Set a pin mode.
    case 'm':   // Set a pin mode.
      local_log.concatf("Setting pin %d state: %d\n", _pin, ((*(str) == 'M')?1:0));
      setPin(_pin, ((*(str) == 'M')?true:false));
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT
