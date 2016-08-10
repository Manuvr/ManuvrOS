/*
File:   XenoMessage.cpp
Author: J. Ian Lindsay
Date:   2014.11.20

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


XenoMessage is the class that unifies our counterparty's message format
  into a single internal type.
*/

#include "XenoMessage.h"
#include <Kernel.h>
#include <Platform/Platform.h>



/**
* @param   uint8_t The integer code that represents message state.
* @return  A pointer to a human-readable string indicating the message state.
*/
const char* XenoMessage::getMessageStateString(uint8_t _s_code) {
  switch (_s_code) {
    case XENO_MSG_PROC_STATE_UNINITIALIZED:         return "UNINITIALIZED";
    case XENO_MSG_PROC_STATE_RECEIVING:             return "RECEIVING";
    case XENO_MSG_PROC_STATE_AWAITING_PROC:         return "AWAITING_PROC";
    case XENO_MSG_PROC_STATE_PROCESSING_RUNNABLE:   return "PROCESSING";
    case XENO_MSG_PROC_STATE_AWAITING_SEND:         return "AWAITING_SEND";
    case XENO_MSG_PROC_STATE_AWAITING_REPLY:        return "AWAITING_REPLY";
    case XENO_MSG_PROC_STATE_SYNC_PACKET:           return "SYNC_PACKET";
    case XENO_MSG_PROC_STATE_AWAITING_REAP:         return "AWAITING_REAP";
    case (XENO_MSG_PROC_STATE_AWAITING_REAP | XENO_MSG_PROC_STATE_ERROR):
      return "SERIALIZING";
    default:                                        return "<UNKNOWN>";
  }
}




/****************************************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

XenoMessage::XenoMessage() {
  wipe();
}


XenoMessage::XenoMessage(ManuvrRunnable* existing_event) {
  wipe();
  // Should maybe set a flag in the event to indicate that we are now responsible
  //   for memory upkeep? Don't want it to get jerked out from under us and cause a crash.
}


/**
* Sometimes we might want to re-use this allocated object rather than free it.
*/
void XenoMessage::wipe() {
  proc_state      = XENO_MSG_PROC_STATE_UNINITIALIZED;
  bytes_received  = 0;     // How many bytes of this command have we received? Meaningless for the sender.
  bytes_total     = 0;     // How many bytes does this message occupy?

  if (NULL != event) {
    // TODO: Now we are worried about this.
    event = NULL;
  }
}


/**
* Calling this fxn will cause this Message to be populated with the given Event and unique_id.
* Calling this converts this XenoManuvrMessage into an outbound, and has the same general effect as
*   calling the constructor with an Event argument.
* TODO: For safety's sake, the Event is not retained. This has caused us some grief. Re-evaluate...
*
* @param   ManuvrRunnable* The Event that is to be communicated.
* @param   uint16_t          An explicitly-provided unique_id so that a dialog can be perpetuated.
*/
void XenoMessage::provideEvent(ManuvrRunnable *existing_event) {
  event = existing_event;  // TODO: Has memory implications...
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;  // Implies we are sending.
}



/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoMessage::printDebug(StringBuilder *output) {
  output->concatf("\t Message ID      0x%08x\n", (unsigned long) this);
  output->concatf("\t Message state   %s\n", getMessageStateString(proc_state));
  output->concatf("\t bytes_total     %d\n", bytes_total);
  output->concatf("\t bytes_received  %d\n", bytes_received);
  if (NULL != event) {
    output->concatf("\t Runnable type   %s\n", event->getMsgTypeString());
  }
}
