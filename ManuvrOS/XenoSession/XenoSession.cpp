/*
File:   XenoSession.cpp
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


XenoSession is the class that manages dialog with other systems via some
  transport (IRDa, Bluetooth, USB VCP, etc).
     ---J. Ian Lindsay
*/

#include "XenoSession.h"
#include <Kernel.h>



/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* Logging support fxn.
*/
const char* XenoSession::sessionPhaseString(uint16_t state_code) {
  switch (state_code & 0x00FF) {
    case XENOSESSION_STATE_UNINITIALIZED:    return "UNINITIALIZED";
    case XENOSESSION_STATE_PENDING_CONN:     return "PENDING_CONN";
    case XENOSESSION_STATE_PENDING_SETUP:    return "PENDING_SETUP";
    case XENOSESSION_STATE_PENDING_AUTH:     return "PENDING_AUTH";
    case XENOSESSION_STATE_ESTABLISHED:      return "ESTABLISHED";
    case XENOSESSION_STATE_PENDING_HANGUP:   return "HANGUP IN PROGRESS";
    case XENOSESSION_STATE_HUNGUP:           return "HUNGUP";
    case XENOSESSION_STATE_DISCONNECTED:     return "DISCONNECTED";
    default:                                 return "<UNKNOWN SESSION STATE>";
  }
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
XenoSession::XenoSession(BufferPipe* _near_side) : EventReceiver(), BufferPipe() {
  setReceiverName("XenoSession");
  _bp_set_flag(BPIPE_FLAG_IS_TERMINUS, true);

  // TODO: Audit implications of this....
  // The link nearer to the transport should not free.
  if (_near_side) {
    setNear(_near_side);  // Our near-side is that passed-in transport.
    _near_side->setFar((BufferPipe*) this);
  }

  _session_service.repurpose(MANUVR_MSG_SESS_SERVICE, (EventReceiver*) this);
  _session_service.isManaged(true);
  _session_service.specific_target = (EventReceiver*) this;

  working            = nullptr;
  session_state      = XENOSESSION_STATE_UNINITIALIZED;
  session_last_state = XENOSESSION_STATE_UNINITIALIZED;

  mark_session_state(XENOSESSION_STATE_PENDING_SETUP);
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
XenoSession::~XenoSession() {
  purgeInbound();  // Need to do careful checks in here for open comm loops.
  purgeOutbound(); // Need to do careful checks in here for open comm loops.

  if (nullptr != working) {
    delete working;
    working = nullptr;
  }

  _relay_list.clear();
  _pending_exec.clear();
  _pending_reply.clear();

  Kernel::raiseEvent(MANUVR_MSG_SESS_HANGUP, nullptr);
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Session-base implementations.
*******************************************************************************/

const char* XenoSession::pipeName() { return getReceiverName(); }

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t XenoSession::fromCounterparty(ManuvrPipeSignal _sig, void* _args) {
  if (getVerbosity() > 5) {
    local_log.concatf("%s --sig--> %s: %s\n", (haveNear() ? _near->pipeName() : "ORIG"), pipeName(), signalString(_sig));
    Kernel::log(&local_log);
  }
  switch (_sig) {
    case ManuvrPipeSignal::XPORT_CONNECT:
    case ManuvrPipeSignal::XPORT_DISCONNECT:
      this->connection_callback(_sig == ManuvrPipeSignal::XPORT_CONNECT);
      return 1;

    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:   // The near side is detaching.
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }
  return BufferPipe::fromCounterparty(_sig, _args);
}



/**
* Tell the session to relay the given message code.
* We have a list of blacklisted codes that the session is not allowed to subscribe to.
*   The reason for this is to avoid an Event loop. The blacklist might be removed once
*   the EXPORT flag in the Message legend is being respected.
*
* @param   uint16_t The message type code to subscribe the session to.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::tapMessageType(uint16_t code) {
  switch (code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
    case MANUVR_MSG_SESS_SERVICE:
      break;

    default:
      {
        std::map<uint16_t, MessageTypeDef*>::iterator it = _relay_list.find(code);
        if (_relay_list.end() == it) {
          // If the relay list doesn't already have the message....
          _relay_list[code] = (MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(code);
          return 0;
        }
      }
      break;
  }

  return -1;
}


/**
* Tell the session not to relay the given message code.
*
* @param   uint16_t The message type code to unsubscribe the session from.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapMessageType(uint16_t code) {
  _relay_list.erase(code);
  return 0;
}


/**
* Unsubscribes the session from all optional messages.
* Whatever messages this session was watching for, it will not be after this call.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapAll() {
  _relay_list.clear();
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
int8_t XenoSession::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_SESS_HANGUP:
      // It is now safe to destroy this session. By triggering our owner's disconnection
      //   method, we indirectly invoke our own teardown.
      BufferPipe::toCounterparty(ManuvrPipeSignal::XPORT_DISCONNECT, NULL);
      mark_session_state(XENOSESSION_STATE_HUNGUP);
      break;
    default:
      break;
  }

  return return_value;
}


/*
* This is the override from EventReceiver, but there is a bit of a twist this time.
* Following the normal processing of the incoming event, this class compares it against
*   a list of events that it has been instructed to relay to the counterparty. If the event
*   meets the relay criteria, we serialize it and send it to the transport that we are bound to.
*/
int8_t XenoSession::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    /* General system events */
    case MANUVR_MSG_BT_CONNECTION_LOST:
      mark_session_state(XENOSESSION_STATE_DISCONNECTED);
      //_relay_list.clear();
      purgeInbound();
      purgeOutbound();
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 3) local_log.concatf("%p Session is now in state %s.\n", this, sessionPhaseString(getPhase()));
      #endif
      return_value++;
      break;

    case MANUVR_MSG_SESS_SERVICE:
      // If we ever see this, it means the class that extended us isn't reacting appropriately
      //   to its own requests-for-service. Pitch a warning.
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 1) {
        local_log.concatf("%p received SESS_SERVICE.\n", this);
        printDebug(&local_log);
      }
      #endif
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SESS_HANGUP:
      mark_session_state(XENOSESSION_STATE_PENDING_HANGUP);
      {
        #ifdef __MANUVR_DEBUG
        int out_purge = purgeOutbound();
        int in_purge  = purgeInbound();
        if (getVerbosity() > 5) local_log.concatf("%p Purged (%d) msgs from outbound and (%d) from inbound.\n", this, out_purge, in_purge);
        #else
        purgeOutbound();
        purgeInbound();
        #endif
      }
      return_value++;
      break;

    case MANUVR_MSG_XPORT_RECEIVE:
      {
        StringBuilder* buf;
        if (0 == active_event->getArgAs(&buf)) {
          fromCounterparty(buf, MEM_MGMT_RESPONSIBLE_BEARER);
        }
      }
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  /* We don't want to resonate... Don't react to Events that have us as the originator. */
  if (active_event->isOriginator((EventReceiver*) this)) {
    if ((XENO_SESSION_IGNORE_NON_EXPORTABLES) && (active_event->isExportable())) {
      /* This is the block that allows the counterparty to intercept events of its choosing. */
      std::map<uint16_t, MessageTypeDef*>::iterator it = _relay_list.find(active_event->eventCode());
      if (_relay_list.end() == it) {
        // If the relay list doesn't already have the message....
        // If we are in this block, it means we need to serialize the event and send it.
        sendEvent(active_event);
        return_value++;
      }
    }
  }

  flushLocalLog();
  return return_value;
}




/****************************************************************************************************
* Functions for managing dialogs and message queues.                                                *
****************************************************************************************************/

/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged.
*/
int XenoSession::purgeOutbound() {
  int return_value = _outbound_messages.size();
  XenoMessage* temp;
  while (_outbound_messages.hasNext()) {
    temp = _outbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 6) {
      local_log.concatf("\nSession %p Destroying outbound msg:\n", this);
      temp->printDebug(&local_log);
      Kernel::log(&local_log);
    }
    #endif
    _outbound_messages.remove(temp);
  }
  return return_value;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged.
*/
int XenoSession::purgeInbound() {
  int return_value = _inbound_messages.size();
  XenoMessage* temp;
  while (_inbound_messages.hasNext()) {
    temp = _inbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 6) {
      local_log.concatf("\nSession %p Destroying inbound msg:\n", this);
      temp->printDebug(&local_log);
      Kernel::log(&local_log);
    }
    #endif
    _inbound_messages.remove(temp);
  }

  return return_value;
}



/**
* Passing an Event into this fxn will cause the Event to be serialized and sent to our counter-party.
* This is the point at which choices are made about what happens to the event's life-cycle.
*/
int8_t XenoSession::sendEvent(ManuvrRunnable *active_event) {
  //XenoMessage* nu_outbound_msg = XenoMessage::fetchPreallocation(this);
  //nu_outbound_msg->provideEvent(active_event);

  //StringBuilder buf;
  //if (nu_outbound_msg->serialize(&buf) > 0) {
  //  toCounterparty(&buf);
  //}

  //if (nu_outbound_msg->expectsACK()) {
  //  _outbound_messages.insert(nu_outbound_msg);
  //}

  // We are about to pass a message across the transport.
  //ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_XPORT_SEND);
  //event->originator      = this;   // We want the callback and the only receiver of this
  //event->specific_target = owner;  //   event to be the transport that instantiated us.
  //raiseEvent(event);
  return 0;
}


int8_t XenoSession::connection_callback(bool _con) {
  mark_session_state(_con ? XENOSESSION_STATE_PENDING_SETUP : XENOSESSION_STATE_DISCONNECTED);
  return 0;
}


/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌
* ▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄
* ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
* ▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
* ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀
****************************************************************************************************/

/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoSession::printDebug(StringBuilder *output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
  BufferPipe::printDebug(output);

  output->concatf("-- Session ID           %p\n", this);
  output->concatf("-- Session phase        %s\n--\n", sessionPhaseString(getPhase()));

  std::map<uint16_t, MessageTypeDef*>::iterator it;
  output->concat("-- Msg codes to relay:\n");
  for (it = _relay_list.begin(); it != _relay_list.end(); it++) {
    output->concatf("\t%s\n", it->second->debug_label);
  }

  int x = _outbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Outbound Queue %d total, showing top %d ------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    int max_print = (x < XENO_SESSION_MAX_QUEUE_PRINT) ? x : XENO_SESSION_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
        _outbound_messages.get(i)->printDebug(output);
    }
  }

  x = _inbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Inbound Queue %d total, showing top %d -------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    int max_print = (x < XENO_SESSION_MAX_QUEUE_PRINT) ? x : XENO_SESSION_MAX_QUEUE_PRINT;
    for (int i = 0; i < max_print; i++) {
      _inbound_messages.get(i)->printDebug(output);
    }
  }

  if (NULL != working) {
    output->concat("\n-- XenoMessage in process  ----------------------------\n");
    working->printDebug(output);
  }
}


#ifdef MANUVR_CONSOLE_SUPPORT

void XenoSession::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'q':  // Manual message queue purge.
      purgeOutbound();
      purgeInbound();
      break;

    case 'T':   // Cause the session to untap everything.
      untapAll();
      local_log.concatf("Session untapped all the things.\n");
      break;

    case 't':   // Cause the session to tap an event.
      {
        const MessageTypeDef* temp_msg_def = ManuvrMsg::lookupMsgDefByLabel(str+1);
        if (MANUVR_MSG_UNDEFINED != temp_msg_def->msg_type_code) {
          tapMessageType(temp_msg_def->msg_type_code);
          local_log.concatf("Session tapped %s\n", temp_msg_def->debug_label);
        }
        else {
          local_log.concatf("Undefined message type. Session subscriptions are unaltered.\n");
        }
      }
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT
