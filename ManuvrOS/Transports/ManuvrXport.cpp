#include "ManuvrXport.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/

uint16_t ManuvrXport::TRANSPORT_ID_POOL = 1;


ManuvrXport::ManuvrXport() {
  _xport_flags = 0;
  _xport_mtu   = PROTOCOL_MTU;
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

