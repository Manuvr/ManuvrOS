/*
File:   CoAPSession.cpp
Author: J. Ian Lindsay
Date:   2016.04.09

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

*/


#if defined (MANUVR_SUPPORT_COAP)

#include "CoAPSession.h"

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
* @param   BufferPipe* All sessions must have one (and only one) transport.
*/
CoAPSession::CoAPSession(BufferPipe* _near_side) : XenoSession(_near_side) {
	setReceiverName("CoAPSession");
	working   = nullptr;
	_next_packetid = 1;
  _bp_set_flag(BPIPE_FLAG_IS_BUFFERED, true);

	// If our base transport is packetized, we will implement CoAP as it is
	//   defined to run over UDP. Otherwise, we will assume TCP.
	if (_near_side->_bp_flag(BPIPE_FLAG_PIPE_PACKETIZED)) {
		_bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED, true);
	}

  _ping_timer.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG, (EventReceiver*) this);
  _ping_timer.incRefs();
  _ping_timer.specific_target = (EventReceiver*) this;
  _ping_timer.alterScheduleRecurrence(-1);
  _ping_timer.alterSchedulePeriod(4000);
  _ping_timer.autoClear(false);
  _ping_timer.enableSchedule(false);
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
CoAPSession::~CoAPSession() {
  _ping_timer.enableSchedule(false);
  platform.kernel()->removeSchedule(&_ping_timer);

	if (nullptr != working) {
		delete working;
		working = nullptr;
	}

	while (_pending_coap_messages.hasNext()) {
		delete _pending_coap_messages.dequeue();
	}
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t CoAPSession::fromCounterparty(StringBuilder* buf, int8_t mm) {
	if (bin_stream_rx(buf->string(), buf->length())) {
	}
	buf->clear();
  return MEM_MGMT_RESPONSIBLE_BEARER;
}



/****************************************************************************************************
* Functions for interacting with the transport driver.                                              *
****************************************************************************************************/

/**
* When we take bytes from the transport, and can't use them all right away,
*   we store them to prepend to the next group of bytes that come through.
*
* @param
* @param
* @return  int8_t  // TODO!!!
*/
int8_t CoAPSession::bin_stream_rx(unsigned char *buf, int len) {
	local_log.concatf("CoAPSession::bin_stream_rx(%p, %d): ", buf, len);
	CoAPMessage *recvPDU = new CoAPMessage(buf, len, len);
	if(recvPDU->validate()!=1) {
		local_log.concat("Malformed CoAP packet\n");
		return 0;
	}
	recvPDU->printDebug(&local_log);

	for (int x = 0; x < len; x++) local_log.concatf("%02x ", *(buf+x));

	local_log.concat("\n");
	Kernel::log(&local_log);
  return 1;
}


int8_t CoAPSession::connection_callback(bool _con) {
	local_log.concatf("\n\nSession%sconnected\n\n", (_con ? " " : " dis"));
	Kernel::log(&local_log);
	XenoSession::connection_callback(_con);
	if (_con) {
		//sendConnectPacket();
	}
  return 0;
}


/**
* Passing an Event into this fxn will cause the Event to be serialized and sent to our counter-party.
* This is the point at which choices are made about what happens to the event's life-cycle.
*/
int8_t CoAPSession::sendEvent(ManuvrMsg* active_event) {
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
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t CoAPSession::attached() {
  if (EventReceiver::attached()) {
  	platform.kernel()->addSchedule(&_ping_timer);
  	//if (owner->connected()) {
    	// Are we connected right now?
    	//sendConnectPacket();
  	//}
    return 1;
  }
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
int8_t CoAPSession::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 < event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
			event->clearArgs();
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
int8_t CoAPSession::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_XPORT_RECEIVE:
      {
        StringBuilder* buf;
        if (0 == active_event->getArgAs(&buf)) {
          bin_stream_rx(buf->string(), buf->length());
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_SESS_SERVICE:
      if (_pending_coap_messages.size() > 0) {
				//process_inbound();
      	return_value++;
			}
      break;

    case MANUVR_MSG_SESS_HANGUP:
      //if (isEstablished()) {
				// If we have an existing session, we try to end it politely.
				//sendDisconnectPacket();
			//}
			//else {
				XenoSession::notify(active_event);
			//}
      return_value++;
      break;

    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      //sendKeepAlive();
      return_value++;
      break;

    default:
      return_value += XenoSession::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}



#if defined(MANUVR_CONSOLE_SUPPORT)
void CoAPSession::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    default:
      XenoSession::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void CoAPSession::printDebug(StringBuilder *output) {
  XenoSession::printDebug(output);
  output->concatf("-- Next Packet ID       0x%08x\n", (uint32_t) _next_packetid);

	if (nullptr != working) {
		output->concat("--\n-- Incomplete inbound message:\n");
		working->printDebug(output);
	}
}

#endif
