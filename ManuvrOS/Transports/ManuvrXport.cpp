#include "ManuvrXport.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"


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




/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/

uint16_t ManuvrXport::TRANSPORT_ID_POOL = 1;


ManuvrXport::ManuvrXport() {
  // No need to burden a client class with this.
  EventReceiver::__class_initializer();

  xport_id           = ManuvrXport::TRANSPORT_ID_POOL++;
  _xport_flags       = 0;
  _xport_mtu         = PROTOCOL_MTU;
  bytes_sent         = 0;
  bytes_received     = 0;
  session            = NULL;
  
  read_timeout_defer = false;

  #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
    _thread_id       = 0;
  #endif
}



/**
* Calling this function with another transport as an argument will cause them to be bound into a
*   bridge.
* Both transports must be non-session, and have no other transports bridged.
*/
int8_t ManuvrXport::bridge(ManuvrXport* _xport) {
  int8_t return_value = -1;
  if (NULL != _xport) {
    if (_xport != this) {
      if (!hasSession() && !_xport->hasSession()) {
        // TODO: At this point, we can create the bridge.
        nonSessionUsage(true);
        _xport->nonSessionUsage(true);
        _xport_flags = _xport_flags | MANUVR_XPORT_FLAG_IS_BRIDGED;
        return_value = 0;
      }
      else {
        Kernel::log("Cannot bridge. One or both transports have sessions.");
      }
    }
    else {
      Kernel::log("Cannot bridge to self.");
    }
  }
  else {
    Kernel::log("Cannot bridge to a NULL transport.");
  }
  return return_value;
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
}


void ManuvrXport::isDebugConsole(bool en) {
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_DEBUG_CONSOLE) : (_xport_flags & ~(MANUVR_XPORT_FLAG_DEBUG_CONSOLE));
  
  if (en) nonSessionUsage(true);    // If we are being used as a console, this most definately applies.
}



int8_t ManuvrXport::sendBuffer(StringBuilder* buf) {
  if (connected()) {
    write_port(buf->string(), buf->length());
  }
  else {
    Kernel::log("Tried to write to a transport that was not connected.");
  }
  return 0;
}




/**
* This is used to cleanup XenoSessions that were instantiated by this class.
* Typically, this would be called from  the session being passed in as the argument.
*   It might be the last thing to happen in the session object following hangup.
*
* This is a very basic implementation. An extension of this class might choose to
*   do something else here (disconnect the transport?). But this ensures memory safety.
*
* @param  The XenoSession to be reaped.
* @return zero on sucess. Non-zero on failure.
*/ 
int8_t ManuvrXport::reapXenoSession(XenoSession* ses) {
  if (NULL != ses) {
    ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_RETRACT_SRVC);
    event->addArg((EventReceiver*) ses);
    raiseEvent(event);

    // Might not do this, depending on transport.
    delete ses;  // TODO: should do this on the event callback.
    return 0;
  }
  
  return -1;
}


int8_t ManuvrXport::provide_session(XenoSession* ses) {
  if ((NULL != session) && (ses != session)) {
    // If we are about to clobber an existing session, we need to free it
    // first.
    __kernel->unsubscribe(session);

    // TODO: Might should warn someone at the other side? 
    //   Maybe we let XenoSession deal with it? At least we
    //   won't have a memory leak, lol.
    //     ---J. Ian Lindsay   Thu Dec 03 04:38:52 MST 2015
    delete session;
    session = NULL;
  }
  session = ses;
  //session->setVerbosity(verbosity);
  
  // This will warn us later to notify others of our removal, if necessary.
  set_xport_state(MANUVR_XPORT_FLAG_HAS_SESSION);
  return 0;
}



/*
* Mark this transport connected or disconnected.
* This method is virtual, and may be over-ridden if the specific transport has 
*   something more sophisticated in mind.
*/
void ManuvrXport::connected(bool en) {
  if (connected() == en) {
    // If we are already in the state specified, do nothing.
    // This will also be true if alwaysConnected().
    return;
  }
  
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_CONNECTED) : (_xport_flags & ~(MANUVR_XPORT_FLAG_CONNECTED));
  if (!nonSessionUsage()) {
    if (en) {
      // We are expected to instantiate a XenoSession, and broadcast its existance.
      // This will put it into the Event system so that auth and such can be handled cleanly.
      // Once the session sets up, it will broadcast itself as having done so.
      XenoSession* ses = new XenoSession(this);
      provide_session(ses);

      ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_ADVERTISE_SRVC);
      event->addArg((EventReceiver*) ses);
      raiseEvent(event);
      ses->sendSyncPacket();
    }
    else {
      // This is a disconnection event. We might want to cleanup all of our sessions
      // that are outstanding.
    }
  }
  createThread(&_thread_id, NULL, xport_read_handler, (void*) this);
}


/*
* Mark this transport connected or disconnected.
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



/*
* Mark this transport connected or disconnected.
* This method is virtual, and may be over-ridden if the specific transport has 
*   something more sophisticated in mind.
*/
void ManuvrXport::initialized(bool en) {
  if (initialized() == en) {
    // If we are already in the state specified, do nothing.
    return;
  }
  // TODO: Not strictly true. Unset connected? listening?
  // ---J. Ian Lindsay   Thu Dec 03 04:00:00 MST 2015
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_INITIALIZED) : (_xport_flags & ~(MANUVR_XPORT_FLAG_INITIALIZED));
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
  temp->concatf("Transport\n=======\n-- _xport_flags   0x%08x\n", _xport_flags);
  temp->concatf("-- xport_id        0x%04x\n", xport_id);
  temp->concatf("-- bytes sent      %u\n", bytes_sent);
  temp->concatf("-- bytes received  %u\n--\n", bytes_received);
  temp->concatf("-- initialized     %s\n", (initialized() ? "yes" : "no"));
  temp->concatf("-- connected       %s\n", (connected() ? "yes" : "no"));
  temp->concatf("-- listening       %s\n", (listening() ? "yes" : "no"));
  temp->concatf("-- has session     %s\n", (hasSession() ? "yes" : "no"));
}


int8_t ManuvrXport::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      break;

    case MANUVR_MSG_XPORT_SEND:
      if (NULL != session) {
        if (connected()) {
          StringBuilder* temp_sb;
          if (0 == active_event->getArgAs(&temp_sb)) {
            #ifdef __MANUVR_DEBUG
            if (verbosity > 3) local_log.concatf("We about to print %d bytes to the transport.\n", temp_sb->length());
            #endif
            write_port(temp_sb->string(), temp_sb->length());
          }
          
          //uint16_t xenomsg_id = session->nextMessage(&outbound_msg);
          //if (xenomsg_id) {
          //  if (write_port(outbound_msg.string(), outbound_msg.length()) ) {
          //    if (verbosity > 2) local_log.concatf("There was a problem writing to %s.\n", _addr);
          //  }
          //  return_value++;
          //}
          #ifdef __MANUVR_DEBUG
          else if (verbosity > 6) local_log.concat("Ignoring a broadcast that wasn't a StringBuilder.\n");
          #endif
        }
        else {
          #ifdef __MANUVR_DEBUG
          if (verbosity > 3) local_log.concat("Session is chatting, but we don't appear to have a connection.\n");
          #endif
        }
      }
      return_value++;
      break;
      
    case MANUVR_MSG_XPORT_RECEIVE:
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
      if (verbosity > 3) {
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
