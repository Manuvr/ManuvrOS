#include "ManuvrXport.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"



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
  _pid               = 0;
  xport_state        = 0;
  bytes_sent         = 0;
  bytes_received     = 0;
  session            = NULL;

  read_timeout_defer = false;
  pid_read_abort     = 0;
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
    ManuvrEvent* event = Kernel::returnEvent(MANUVR_MSG_SYS_RETRACT_SRVC);
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
  set_xport_state(MANUVR_XPORT_STATE_HAS_SESSION);
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
      ses->markSessionConnected(true);
      
      // This will warn us later to notify others of our removal, if necessary.
      _xport_flags |= MANUVR_XPORT_FLAG_HAS_SESSION;
    
      ManuvrEvent* event = Kernel::returnEvent(MANUVR_MSG_SYS_ADVERTISE_SRVC);
      event->addArg((EventReceiver*) ses);
      raiseEvent(event);
    }
    else {
      // This is a disconnection event. We might want to cleanup all of our sessions
      // that are outstanding.
    }
  }
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
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_STATE_LISTENING) : (_xport_flags & ~(MANUVR_XPORT_STATE_LISTENING));
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
  _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_STATE_INITIALIZED) : (_xport_flags & ~(MANUVR_XPORT_STATE_INITIALIZED));
}



// Given a transport event, returns true if we need to act.
bool ManuvrXport::event_addresses_us(ManuvrEvent *event) {
  uint16_t temp_uint16;
  
  if (event->argCount()) {
    if (0 == event->getArgAs(&temp_uint16)) {
      if (temp_uint16 == xport_id) {
        // The first argument is our ID.
        return true;
      }
    }
    // Either not the correct arg form, or not our ID.
    return false;
  }
  
  // No arguments implies no first argument.
  // No first argument implies event is addressed to 'all transports'.
  // 'all transports' implies 'true'. We need to care.
  return true;
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
  temp->concatf("Transport\n=======\n-- xport_state    0x%02x\n", xport_state);
  temp->concatf("-- xport_id        0x%04x\n", xport_id);
  temp->concatf("-- bytes sent      %u\n", bytes_sent);
  temp->concatf("-- bytes received  %u\n\n", bytes_received);
  temp->concatf("-- connected       %s\n", (connected() ? "yes" : "no"));
  temp->concatf("-- has session     %s\n--\n", (hasSession() ? "yes" : "no"));
}




///**
//* There is a NULL-check performed upstream for the scheduler member. So no need 
//*   to do it again here.
//*
//* @return 0 on no action, 1 on action, -1 on failure.
//*/
//int8_t ManuvrXport::bootComplete() {
//}
//
//
//int8_t ManuvrXport::notify(ManuvrEvent*) {
//}
//
//
//int8_t ManuvrXport::callback_proc(ManuvrEvent *) {
//}

