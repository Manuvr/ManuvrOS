/*
File:   ManuvrXport.cpp
Author: J. Ian Lindsay
Date:   2015.03.17

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


This driver is designed to give Manuvr platform-abstracted transports.

XenoSessions are optionally established by this class, and broadcast to the
  rest of the system (MANUVR_MSG_SESS_ESTABLISHED). By the time that has
  happened, the session ought to have become sync'd, self-identified,
  authenticated, etc. Therefore, session configuration must be done on a
  per-transport basis. This means that authentication (for instance) is
  transport-wide.

If sessions are enabled for a transport, the highest-level of the protocol
  touched by this class ought to be dealing with sync.

For non-session applications of this class, session-creation and management
  can be disabled. This would be appropriate in cases such as GPS, modems,
  and generally, anything that isn't manuvrable.

For debuggability, the transport has a special mode for acting as a debug
  console.
*/


#include "ManuvrXport.h"
#include <XenoSession/XenoSession.h>
#include <XenoSession/Manuvr/ManuvrSession.h>


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


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/

#if defined(__BUILD_HAS_FREERTOS)
  /*
  * In a threaded environment, we use threads to read ports.
  */
  void* xport_read_handler(void* active_xport) {
    if (active_xport) {
      // Wait until boot has ocurred...
      while (!((ManuvrXport*)active_xport)->erAttached()) taskYIELD();
      while (1) {
        if (0 == ((ManuvrXport*)active_xport)->read_port()) {
          taskYIELD();
        }
      }
    }
    return nullptr;
  }

#elif defined(__MANUVR_LINUX)
  /*
  * In a threaded environment, we use threads to read ports.
  */
  void* xport_read_handler(void* active_xport) {
    if (active_xport) {
      // Wait until boot has ocurred...
      while (!((ManuvrXport*)active_xport)->erAttached()) sleep_millis(50);
      while (1) {
        if (0 == ((ManuvrXport*)active_xport)->read_port()) {
          sleep_millis(20);
        }
      }
    }
    return nullptr;
  }

#endif


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Constructor.
*/
ManuvrXport::ManuvrXport(const char* nom) : EventReceiver(nom), BufferPipe() {
  // No need to burden a client class with this.
  _xport_mtu = MANUVR_PROTO_MTU;

  // Transports are all terminal.
  _bp_set_flag(BPIPE_FLAG_IS_TERMINUS, true);
}

/**
* Destructor.
*/
ManuvrXport::~ManuvrXport() {
  listening(false);

  // Cleanup any schedules...
  if (nullptr != _autoconnect_schedule) {
    _autoconnect_schedule->enableSchedule(false);
    platform.kernel()->removeSchedule(_autoconnect_schedule);
    _autoconnect_schedule = nullptr;
    delete _autoconnect_schedule;
  }
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* These functions can stop at transport.
*******************************************************************************/
const char* ManuvrXport::pipeName() { return getReceiverName(); }

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t ManuvrXport::toCounterparty(ManuvrPipeSignal _sig, void* _args) {
  if (getVerbosity() > 5) {
    local_log.concatf("%s --sig--> %s: %s\n", (haveNear() ? _near->pipeName() : "ORIG"), pipeName(), signalString(_sig));
    Kernel::log(&local_log);
  }
  switch (_sig) {
    case ManuvrPipeSignal::XPORT_CONNECT:
      if (!connected()) {
        connect();
      }
      return 0;
    case ManuvrPipeSignal::XPORT_DISCONNECT:
      if (connected()) {
        disconnect();
      }
      return 0;

    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }

  // If we are at this point, we need to pass to base-class.
  return BufferPipe::toCounterparty(_sig, _args);
}


/*******************************************************************************
* ___________                                                  __
* \__    ___/___________    ____   ____________   ____________/  |_
*   |    |  \_  __ \__  \  /    \ /  ___/\____ \ /  _ \_  __ \   __\
*   |    |   |  | \// __ \|   |  \\___ \ |  |_> >  <_> )  | \/|  |
*   |____|   |__|  (____  /___|  /____  >|   __/ \____/|__|   |__|
*                       \/     \/     \/ |__|
* Basal implementations.
*******************************************************************************/

int8_t ManuvrXport::disconnect() {
  connected(false);
  return 0;
}


/*
* Accessors for session behavior regarding connect/disconnect.
* This should be set to true in the case of a simple serial port if there is no
*   means of signalling to the software that a connection-related event has occured.
*   If a XenoSession is to be created by this transport, calling this will cause a
*   session to be created immediately with a configuration that periodically emits
*   sync packets for the sake of emulating the event.
*/
void ManuvrXport::alwaysConnected(bool en) {
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_ALWAYS_CONNECTED) : (_xport_flags & ~(MANUVR_XPORT_FLAG_ALWAYS_CONNECTED));
  if (nullptr != _autoconnect_schedule) {
    // If we have a reconnection schedule (we should not), free it.
    _autoconnect_schedule->enableSchedule(false);
    platform.kernel()->removeSchedule(_autoconnect_schedule);
    _autoconnect_schedule = nullptr;
    delete _autoconnect_schedule;
  }
}

/*
* By setting this transport to autoconnect, we will instance a schedule and enable it if the
*   transport is not already connected.
* If and when the transport connects, the schedule will be disabled and reaped.
* When disconnection occurs without explicit direction, the schedule will be re-enabled.
* We will NOT do any of this if the ALWAYS_CONNECTED flag is set.
*/
void ManuvrXport::autoConnect(bool en, uint32_t _ac_period) {
  if (en) {
    if (!alwaysConnected()) {
      // Autoconnection only makes sense if the transport is not always connected.
      _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_AUTO_CONNECT) : (_xport_flags & ~(MANUVR_XPORT_FLAG_AUTO_CONNECT));
      if (nullptr == _autoconnect_schedule) {
        // If we don't already have a ref to a schedule for this purpose.
        _autoconnect_schedule = new ManuvrMsg(MANUVR_MSG_XPORT_CONNECT, (EventReceiver*) this);
        _autoconnect_schedule->incRefs();
        _autoconnect_schedule->specific_target = (EventReceiver*) this;
        _autoconnect_schedule->alterScheduleRecurrence(-1);
        _autoconnect_schedule->alterSchedulePeriod(_ac_period);
        _autoconnect_schedule->autoClear(false);
        _autoconnect_schedule->enableSchedule(!connected());
        platform.kernel()->addSchedule(_autoconnect_schedule);
      }
    }
  }
  else {
    if (_autoconnect_schedule) {
      _autoconnect_schedule->enableSchedule(false);
      platform.kernel()->removeSchedule(_autoconnect_schedule);
      delete _autoconnect_schedule;
      _autoconnect_schedule = nullptr;
    }
  }
}


/*
* Mark this transport connected or disconnected.
*/
void ManuvrXport::connected(bool en) {
  if (connected() == en) {
    // If we are already in the state specified, do nothing.
    // This will also be true if alwaysConnected().
    return;
  }

  mark_connected(en);
  if (en) {
    // We are expected to instantiate a XenoSession, and broadcast its existance.
    // This will put it into the Event system so that auth and such can be handled cleanly.
    // Once the session sets up, it will broadcast itself as having done so.

    if (_autoconnect_schedule) _autoconnect_schedule->enableSchedule(false);
  }
  else {
    // This is a disconnection event. We might want to cleanup all of our sessions
    // that are outstanding.
    if (_autoconnect_schedule) _autoconnect_schedule->enableSchedule(true);
  }
  #if defined (__BUILD_HAS_FREERTOS) || defined (__MANUVR_LINUX)
    if (0 == _thread_id) {
      // If we are in a threaded environment, we will want a thread if there isn't one already.
      if (createThread(&_thread_id, nullptr, xport_read_handler, (void*) this)) {
        Kernel::log("Failed to create transport read thread.\n");
      }
    }
  #endif
  BufferPipe::fromCounterparty(en ? ManuvrPipeSignal::XPORT_CONNECT : ManuvrPipeSignal::XPORT_DISCONNECT, nullptr);
}


/*
* Mark this transport listening or not.
* This method is virtual, and may be over-ridden if the specific transport has
*   something more sophisticated in mind.
*/
void ManuvrXport::listening(bool en) {
  if (listening() == en) {
    // If we are already in the state specified, do nothing.
    return;
  }
  // TODO: Not strictly true. Unset connected? listening?
  // ---J. Ian Lindsay   Thu Dec 03 04:00:00 MST 2015
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_LISTENING) : (_xport_flags & ~(MANUVR_XPORT_FLAG_LISTENING));
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
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrXport::printDebug(StringBuilder *temp) {
  EventReceiver::printDebug(temp);
  BufferPipe::printDebug(temp);
  temp->concatf("--\n-- %s-oriented transport\n--\n", (streamOriented() ? "stream" : "message"));
  temp->concatf("-- _xport_flags:   0x%08x\n", _xport_flags);
  temp->concatf("-- bytes sent:     %u\n", bytes_sent);
  temp->concatf("-- bytes received: %u\n--\n", bytes_received);
  temp->concatf("-- initialized:    %s\n", (initialized() ? "yes" : "no"));
  temp->concatf("-- connected:      %s\n", (connected() ? "yes" : "no"));
  temp->concatf("-- listening:      %s\n", (listening() ? "yes" : "no"));
  temp->concatf("-- autoconnect:    %s\n", (autoConnect() ? "yes" : "no"));
}


int8_t ManuvrXport::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_XPORT_SEND:
      if (connected()) {
        StringBuilder* temp_sb;
        if (0 == active_event->getArgAs(&temp_sb)) {
          #ifdef MANUVR_DEBUG
          if (getVerbosity() > 3) local_log.concatf("We about to print %d bytes to the transport.\n", temp_sb->length());
          #endif
          toCounterparty(temp_sb, MEM_MGMT_RESPONSIBLE_CREATOR);
        }
        #ifdef MANUVR_DEBUG
        else if (getVerbosity() > 6) local_log.concat("Ignoring a broadcast that wasn't a StringBuilder.\n");
        #endif
      }
      else {
        #ifdef MANUVR_DEBUG
        if (getVerbosity() > 3) local_log.concat("Session is chatting, but we don't appear to have a connection.\n");
        #endif
      }
      return_value++;
      break;

    case MANUVR_MSG_XPORT_RECEIVE:
    case MANUVR_MSG_XPORT_QUEUE_RDY:
      break;

    case MANUVR_MSG_XPORT_RESERVED_0:
    case MANUVR_MSG_XPORT_RESERVED_1:
    case MANUVR_MSG_XPORT_INIT:
    case MANUVR_MSG_XPORT_RESET:
    case MANUVR_MSG_XPORT_ERROR:
    case MANUVR_MSG_XPORT_CB_QUEUE_RDY:
    case MANUVR_MSG_XPORT_IDENTITY:
      #ifdef MANUVR_DEBUG
      if (getVerbosity() > 3) {
        local_log.concatf("Transport %s received an event that was addressed to it, but is not yet handled.\n", getReceiverName());
        active_event->printDebug(&local_log);
      }
      #endif
      break;

    case MANUVR_MSG_XPORT_DEBUG:
      printDebug(&local_log);
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
/**
* This is a base-level debug function that takes direct input from a user.
*
* @param   input  A buffer containing the user's direct input.
*/
void ManuvrXport::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'C':
      local_log.concatf("%s: Connect...\n", getReceiverName());
      connect();
      break;
    case 'D':  // Force a state change with no underlying physical reason. Abuse test...
      local_log.concatf("%s: Disconnect...\n", getReceiverName());
      disconnect();
      break;
    case 'R':
      local_log.concatf("%s: Resetting...\n", getReceiverName());
      reset();
      break;
    default:
      break;
  }

  flushLocalLog();
}

#endif  // MANUVR_CONSOLE_SUPPORT
