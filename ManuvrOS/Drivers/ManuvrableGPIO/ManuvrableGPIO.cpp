/*
File:   ManuvrableGPIO.h
Author: J. Ian Lindsay
Date:   2015.09.21


Copyright (C) 2015 Manuvr
All rights reserved.

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

#include "ManuvrableGPIO.h"


const MessageTypeDef gpio_message_defs[] = {
  /* 
    For messages that have arguments, we have the option of defining inline lables for each parameter.
    This is advantageous for debugging and writing front-ends. We case-off here to make this choice at
    compile time.
  */
  #if defined (__ENABLE_MSG_SEMANTICS)
  {  MANUVR_MSG_GPIO_LEGEND       , MSG_FLAG_EXPORTABLE,  "GPIO_LEGEND",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIGITAL_READ      , MSG_FLAG_EXPORTABLE,  "DIGITAL_READ",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIGITAL_WRITE     , MSG_FLAG_EXPORTABLE,  "DIGITAL_WRITE",       ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_ANALOG_READ       , MSG_FLAG_EXPORTABLE,  "ANALOG_READ",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_ANALOG_WRITE      , MSG_FLAG_EXPORTABLE,  "ANALOG_WRITE",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_EVENT_ON_INTERRUPT, MSG_FLAG_EXPORTABLE,  "EVENT_ON_INTERRUPT",  ManuvrMsg::MSG_ARGS_NONE }, //
  #else
  {  MANUVR_MSG_GPIO_LEGEND       , MSG_FLAG_EXPORTABLE,  "GPIO_LEGEND",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_DIGITAL_READ      , MSG_FLAG_EXPORTABLE,  "DIGITAL_READ",        ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_DIGITAL_WRITE     , MSG_FLAG_EXPORTABLE,  "DIGITAL_WRITE",       ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_ANALOG_READ       , MSG_FLAG_EXPORTABLE,  "ANALOG_READ",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_ANALOG_WRITE      , MSG_FLAG_EXPORTABLE,  "ANALOG_WRITE",        ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_EVENT_ON_INTERRUPT, MSG_FLAG_EXPORTABLE,  "EVENT_ON_INTERRUPT",  ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  #endif
};



ManuvrableGPIO::ManuvrableGPIO() {

  // Inform the Kernel of the codes we will be using...
  ManuvrMsg::registerMessages(gpio_message_defs, sizeof(gpio_message_defs) / sizeof(MessageTypeDef));
}

ManuvrableGPIO::~ManuvrableGPIO() {
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 
* 
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrableGPIO::getReceiverName() {  return "ManuvrableGPIO";  }


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
* Some peripherals and operations need a bit of time to complete. This function is called from a
*   one-shot schedule and performs all of the cleanup for latent consequences of bootstrap().
*
* @return non-zero if action was taken. Zero otherwise.
*/
int8_t ManuvrableGPIO::bootComplete() {
  EventReceiver::bootComplete();

  return 0;
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
  switch (event->event_code) {
    default:
      break;
  }
  
  return return_value;
}


int8_t ManuvrableGPIO::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_DIGITAL_READ:
      //int8_t readPin(uint8_t pin);
      return_value++;
      break;

    case MANUVR_MSG_DIGITAL_WRITE:
      //int8_t setPin(uint8_t pin, bool high);
      return_value++;
      break;

    case MANUVR_MSG_ANALOG_READ:
      return_value++;
      break;

    case MANUVR_MSG_ANALOG_WRITE:
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
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
  return return_value;
}

