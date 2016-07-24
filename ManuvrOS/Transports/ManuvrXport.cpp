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
#include "FirmwareDefs.h"
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
uint16_t ManuvrXport::TRANSPORT_ID_POOL = 1;


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/

#if defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)
  /*
  * In a threaded environment, we use threads to read ports.
  */
  void* xport_read_handler(void* active_xport) {
    if (NULL != active_xport) {
      ((ManuvrXport*)active_xport)->read_port();
    }
    return NULL;
  }

#else
  // Threads are unsupported here.
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
ManuvrXport::ManuvrXport() : BufferPipe() {
  // No need to burden a client class with this.
  EventReceiver::__class_initializer();

  xport_id              = ManuvrXport::TRANSPORT_ID_POOL++;
  _xport_flags          = 0;
  _xport_mtu            = PROTOCOL_MTU;
  _autoconnect_schedule = NULL;
  bytes_sent            = 0;
  bytes_received        = 0;
  session               = NULL;

  #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
    _thread_id       = 0;
  #endif
}

/**
* Destructor.
*/
ManuvrXport::~ManuvrXport() {
  #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
    // TODO: Tear down the thread.
  #endif

  if(_reap_session()) {
    delete session;
    session = NULL;
  }

  // Cleanup any schedules...
  if (NULL != _autoconnect_schedule) {
    _autoconnect_schedule->enableSchedule(false);
    __kernel->removeSchedule(_autoconnect_schedule);
    _autoconnect_schedule = NULL;
    delete _autoconnect_schedule;
  }
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
  // TODO: Might-should tear down the session?
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
  if (NULL != _autoconnect_schedule) {
    // If we have a reconnection schedule (we should not), free it.
    _autoconnect_schedule->enableSchedule(false);
    __kernel->removeSchedule(_autoconnect_schedule);
    _autoconnect_schedule = NULL;
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
      if (NULL == _autoconnect_schedule) {
        // If we don't already have a ref to a schedule for this purpose.
        _autoconnect_schedule = new ManuvrRunnable(MANUVR_MSG_XPORT_CONNECT);
        _autoconnect_schedule->isManaged(true);
        _autoconnect_schedule->originator      = (EventReceiver*) this;
        _autoconnect_schedule->specific_target = (EventReceiver*) this;
        _autoconnect_schedule->alterScheduleRecurrence(-1);
        _autoconnect_schedule->alterSchedulePeriod(_ac_period);
        _autoconnect_schedule->autoClear(false);
        _autoconnect_schedule->enableSchedule(!connected());
        __kernel->addSchedule(_autoconnect_schedule);
      }
    }
  }
  else {
    if (NULL != _autoconnect_schedule) {
      _autoconnect_schedule->enableSchedule(false);
      __kernel->removeSchedule(_autoconnect_schedule);
      delete _autoconnect_schedule;
      _autoconnect_schedule = NULL;
    }
  }
}


int8_t ManuvrXport::sendBuffer(StringBuilder* buf) {
  if (connected()) {
    toCounterparty(buf->string(), buf->length(), MEM_MGMT_RESPONSIBLE_CREATOR);
  }
  else {
    Kernel::log("Tried to write to a transport that was not connected.");
  }
  return 0;
}


int8_t ManuvrXport::provide_session(XenoSession* ses) {
  if ((NULL != session) && (ses != session)) {
    // If we are about to clobber an existing session, we need to free it first.
    __kernel->unsubscribe(session);
    //ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_RETRACT_SRVC);
    //event->addArg((EventReceiver*) ses);
    //raiseEvent(event);

    // TODO: Might should warn someone at the other side?
    //   Maybe we let XenoSession deal with it? At least we
    //   won't have a memory leak, lol.
    //     ---J. Ian Lindsay   Thu Dec 03 04:38:52 MST 2015
    delete session;
    session = NULL;
  }
  session = ses;
  return 0;
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
      if (NULL == session) {
        // We are expected to instantiate a XenoSession, and broadcast its existance.
        // This will put it into the Event system so that auth and such can be handled cleanly.
        // Once the session sets up, it will broadcast itself as having done so.
        // TODO: Session discovery should happen at this point.
        XenoSession* ses = (XenoSession*) new ManuvrSession(this);
        _reap_session(true);
        provide_session(ses);

        ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_ADVERTISE_SRVC);
        event->addArg((EventReceiver*) ses);
        raiseEvent(event);
      }

      if (NULL != _autoconnect_schedule) _autoconnect_schedule->enableSchedule(false);
      if (session) session->connection_callback(true);
    }
    else {
      // This is a disconnection event. We might want to cleanup all of our sessions
      // that are outstanding.
      if (NULL != _autoconnect_schedule) _autoconnect_schedule->enableSchedule(true);
      if (session) {
        session->connection_callback(false);
        if(_reap_session()) {
          delete session;
          session = NULL;
        }
      }
    }
  #if defined (__MANUVR_FREERTOS) | defined (__MANUVR_LINUX)
  if (_thread_id == 0) {
    // If we are in a threaded environment, we will want a thread if there isn't one already.
    createThread(&_thread_id, NULL, xport_read_handler, (void*) this);
  }
  #endif
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
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrXport::printDebug(StringBuilder *temp) {
  EventReceiver::printDebug(temp);
  temp->concatf("--\n-- %s-oriented transport\n--\n", (streamOriented() ? "stream" : "message"));
  temp->concatf("-- _xport_flags    0x%08x\n", _xport_flags);
  temp->concatf("-- xport_id        0x%04x\n", xport_id);
  temp->concatf("-- bytes sent      %u\n", bytes_sent);
  temp->concatf("-- bytes received  %u\n--\n", bytes_received);
  temp->concatf("-- initialized     %s\n", (initialized() ? "yes" : "no"));
  temp->concatf("-- connected       %s\n", (connected() ? "yes" : "no"));
  temp->concatf("-- listening       %s\n", (listening() ? "yes" : "no"));
  temp->concatf("-- Has session     %s\n--\n", (getSession() ? "yes" : "no"));
}


int8_t ManuvrXport::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    case MANUVR_MSG_XPORT_SEND:
      if (NULL != session) {
        if (connected()) {
          StringBuilder* temp_sb;
          if (0 == active_event->getArgAs(&temp_sb)) {
            #ifdef __MANUVR_DEBUG
            if (getVerbosity() > 3) local_log.concatf("We about to print %d bytes to the transport.\n", temp_sb->length());
            #endif
            toCounterparty(temp_sb->string(), temp_sb->length(), MEM_MGMT_RESPONSIBLE_CREATOR);
          }

          //uint16_t xenomsg_id = session->nextMessage(&outbound_msg);
          //if (xenomsg_id) {
          //  if (write_port(outbound_msg.string(), outbound_msg.length()) ) {
          //    if (getVerbosity() > 2) local_log.concatf("There was a problem writing to %s.\n", _addr);
          //  }
          //  return_value++;
          //}
          #ifdef __MANUVR_DEBUG
          else if (getVerbosity() > 6) local_log.concat("Ignoring a broadcast that wasn't a StringBuilder.\n");
          #endif
        }
        else {
          #ifdef __MANUVR_DEBUG
          if (getVerbosity() > 3) local_log.concat("Session is chatting, but we don't appear to have a connection.\n");
          #endif
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_XPORT_RECEIVE:
      break;

    case MANUVR_MSG_XPORT_RESERVED_0:
    case MANUVR_MSG_XPORT_RESERVED_1:
    case MANUVR_MSG_XPORT_SET_PARAM:
    case MANUVR_MSG_XPORT_GET_PARAM:
    case MANUVR_MSG_XPORT_INIT:
    case MANUVR_MSG_XPORT_RESET:
    case MANUVR_MSG_XPORT_CONNECT:
    case MANUVR_MSG_XPORT_DISCONNECT:
    case MANUVR_MSG_XPORT_ERROR:
    case MANUVR_MSG_XPORT_SESSION:
    case MANUVR_MSG_XPORT_QUEUE_RDY:
    case MANUVR_MSG_XPORT_CB_QUEUE_RDY:

    case MANUVR_MSG_XPORT_IDENTITY:
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 3) {
        local_log.concatf("TransportID %d received an event that was addressed to it, but is not yet handled.\n", xport_id);
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

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}


//int8_t ManuvrXport::bootComplete() {
//}
//int8_t ManuvrXport::callback_proc(ManuvrRunnable *) {
//}


#if defined(__MANUVR_CONSOLE_SUPPORT)
/**
* This is a base-level debug function that takes direct input from a user.
*
* @param   input  A buffer containing the user's direct input.
*/
void ManuvrXport::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'C':
      local_log.concatf("Transport 0x%04x: Connect...\n", xport_id);
      connect();
      break;
    case 'D':  // Force a state change with no underlying physical reason. Abuse test...
      local_log.concatf("Transport 0x%04x: Disconnect...\n", xport_id);
      disconnect();
      break;
    case 'R':
      local_log.concatf("Transport 0x%04x: Resetting...\n", xport_id);
      reset();
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}

#endif  // __MANUVR_CONSOLE_SUPPORT
