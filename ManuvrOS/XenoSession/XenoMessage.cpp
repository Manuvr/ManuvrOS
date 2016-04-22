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


XenoMessage is the class that is the interface between ManuvrRunnables and
  XenoSessions.
     ---J. Ian Lindsay
*/

#include "XenoSession.h"
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
  provideEvent(existing_event);
}



/**
* Sometimes we might want to re-use this allocated object rather than free it.
* Do not change the unique_id. One common use-case for this fxn is to reply to a message.
*/
void XenoMessage::wipe() {
  session         = NULL;
  proc_state      = XENO_MSG_PROC_STATE_UNINITIALIZED;
  retries         = 0;     // How many times have we retried this packet?
  bytes_received  = 0;     // How many bytes of this command have we received? Meaningless for the sender.

  unique_id       = 0;
  bytes_total     = 0;     // How many bytes does this message occupy?
  message_code    = 0;     //

  if (NULL != event) {
    // TODO: Now we are worried about this.
    event = NULL;
  }

  millis_at_begin = 0;     // This is the milliseconds reading when we sent.
  time_created    = millis();
}


/**
* Calling this fxn will cause this Message to be populated with the given Event and unique_id.
* Calling this converts this XenoMessage into an outbound, and has the same general effect as
*   calling the constructor with an Event argument.
* TODO: For safety's sake, the Event is not retained. This has caused us some grief. Re-evaluate...
*
* @param   ManuvrRunnable* The Event that is to be communicated.
* @param   uint16_t          An explicitly-provided unique_id so that a dialog can be perpetuated.
*/
void XenoMessage::provideEvent(ManuvrRunnable *existing_event, uint16_t manual_id) {
  event = existing_event;
  unique_id = manual_id;
  message_code = event->event_code;                //
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;  // Implies we are sending.
}


/**
* We need to ACK certain messages. This converts this message to an ACK of the message
*   that it used to be. Then it can be simply fed back into the outbound queue.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::ack() {
  return 0;
}


/**
* Calling this sends a message to the counterparty asking them to retransmit the same message.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::retry() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY_RETRY);
  provideEvent(&temp_event, unique_id);
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  retries++;
  return 0;
}


/**
* Calling this will mark this message as critical fail and send a reply to the counterparty
*   telling them that we can't cope with it.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::fail() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY_FAIL);
  provideEvent(&temp_event, unique_id);
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  return 0;
}


/**
* Called by a session object to claim the message.
*
* @param   XenoSession* The session that is claiming us.
*/
void XenoMessage::claim(XenoSession* _ses) {
  proc_state = XENO_MSG_PROC_STATE_RECEIVING;
  session = _ses;
}



/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoMessage::printDebug(StringBuilder *output) {
  if (NULL == output) return;

  output->concatf("\t Message ID      0x%08x\n", (uint32_t) this);
  if (NULL != event) {
    output->concatf("\t Message type    %s\n", event->getMsgTypeString());
  }
  output->concatf("\t Message state   %s\n", getMessageStateString(proc_state));
  output->concatf("\t unique_id       0x%04x\n", unique_id);
  output->concatf("\t bytes_total     %d\n", bytes_total);
  output->concatf("\t bytes_received  %d\n", bytes_received);
  output->concatf("\t time_created    0x%08x\n", time_created);
  output->concatf("\t retries         %d\n\n", retries);
}
