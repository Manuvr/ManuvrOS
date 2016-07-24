/*
File:   MQTTSession.cpp
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

This is my C++ translation of the Paho Demo C library.
*/


#if defined (MANUVR_SUPPORT_MQTT)

#include "MQTTSession.h"


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

MQTTOpts opts = {
	(char*)"manuvr_client",
  0,
  (char*)"\n",
  QOS2,
  NULL,
  NULL,
  0
};


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
MQTTSession::MQTTSession(ManuvrXport* _xport) : XenoSession(_xport) {
	_ping_outstanding(false);
	working   = NULL;
	_next_packetid = 1;

  _ping_timer.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG);
  _ping_timer.isManaged(true);
  _ping_timer.originator      = (EventReceiver*) this;
  _ping_timer.specific_target = (EventReceiver*) this;
  _ping_timer.alterScheduleRecurrence(-1);
  _ping_timer.alterSchedulePeriod(4000);
  _ping_timer.autoClear(false);
  _ping_timer.enableSchedule(false);

  if (_xport->booted()) {
    bootComplete();   // Because we are instantiated well after boot, we call this on construction.
  }
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
MQTTSession::~MQTTSession() {
  _ping_timer.enableSchedule(false);
  __kernel->removeSchedule(&_ping_timer);

	if (NULL != working) {
		delete working;
		working = NULL;
	}
	unsubscribeAll();
	while (_pending_mqtt_messages.hasNext()) {
		delete _pending_mqtt_messages.dequeue();
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
* Inward toward the transport.
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t MQTTSession::toCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveNear()) {
        return _near->toCounterparty(buf, len, MEM_MGMT_RESPONSIBLE_CREATOR);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      // TODO: Freeing the buffer? Let UDP do it?
      if (haveNear()) {
        return _near->toCounterparty(buf, len, MEM_MGMT_RESPONSIBLE_BEARER);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}

/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t MQTTSession::fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  bin_stream_rx(buf, len);
  return MEM_MGMT_RESPONSIBLE_BEARER;
}



/****************************************************************************************************
* Subscription management.                                                                          *
****************************************************************************************************/
int8_t MQTTSession::subscribe(const char* topic, ManuvrRunnable* runnable) {
  std::map<const char*, ManuvrRunnable*>::iterator it = _subscriptions.find(topic);
  if (_subscriptions.end() == it) {
    // If the list doesn't already have the topic....
		runnable->originator  = (EventReceiver*) this;
    _subscriptions[topic] = runnable;
		return sendSub(topic, QOS1);   // TODO: make dynamic
  }
	return -1;
}


int8_t MQTTSession::unsubscribe(const char* topic) {
  std::map<const char*, ManuvrRunnable*>::iterator it = _subscriptions.find(topic);
  if (_subscriptions.end() != it) {
    // TODO: We need to clean up the runnable. For now, we'll assume it is handled elsewhere.
		// delete it->second;
		_subscriptions.erase(topic);
    return 0;
  }
	return -1;
}


int8_t MQTTSession::resubscribeAll() {
	if (isEstablished()) {
		std::map<const char*, ManuvrRunnable*>::iterator it;
  	for (it = _subscriptions.begin(); it != _subscriptions.end(); it++) {
			if (!sendSub(it->first, QOS1)) {   // TODO: Make QoS dynmaic.
				return -1;
			}
  	}
	}
	return 0;
}

int8_t MQTTSession::unsubscribeAll() {
	if (isEstablished()) {
		std::map<const char*, ManuvrRunnable*>::iterator it;
  	for (it = _subscriptions.begin(); it != _subscriptions.end(); it++) {
			if (sendUnsub(it->first)) {
				_subscriptions.erase(it->first);
			}
		}
  }
	else {
		_subscriptions.clear();
	}
	return 0;
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
int8_t MQTTSession::bin_stream_rx(unsigned char *buf, int len) {
  int8_t return_value = 0;
	if (len >= 1) {
		if (NULL == working) {
			working = new MQTTMessage();
		}

		int _eaten = working->accumulate(buf, len);
		if (-1 == _eaten) {
			delete working;
			working = NULL;
			return_value = -1;
		}
		else {
			// These are success cases.
			if (working->parseComplete()) {
				_pending_mqtt_messages.insert(working);
				requestService();   	// Pitch an event to deal with the message.
				working = NULL;

				if (len - _eaten > 0) {
					// We not only finished a message, but we are beginning another. Recurse.
					return bin_stream_rx(buf + _eaten, len - _eaten);
				}
			}
		}
	}
  return return_value;
}


int8_t MQTTSession::connection_callback(bool _con) {
	XenoSession::connection_callback(_con);
	StringBuilder output;
	output.concatf("\n\nSession%sconnected\n\n", (_con ? " " : " dis"));
	Kernel::log(&output);
	if (_con) {
		sendConnectPacket();
	}
  return 0;
}


/**
* Passing an Event into this fxn will cause the Event to be serialized and sent to our counter-party.
* This is the point at which choices are made about what happens to the event's life-cycle.
*/
int8_t MQTTSession::sendEvent(ManuvrRunnable *active_event) {
  //XenoMessage* nu_outbound_msg = XenoMessage::fetchPreallocation(this);
  //nu_outbound_msg->provideEvent(active_event);

  //StringBuilder buf;
  //if (nu_outbound_msg->serialize(&buf) > 0) {
  //  owner->sendBuffer(&buf);
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


/****************************************************************************************************
* Private functions that conduct the business of the session.                                       *
****************************************************************************************************/

bool MQTTSession::sendSub(const char* _topicStr, enum QoS qos) {
  if (isEstablished()) {
		printf("SESSION OPERATION sendSub() \n");
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)_topicStr;

    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_subscribe(buf, buf_size, 0, getNextPacketId(), 1, &topic, (int*)&qos);
    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return false;
}

bool MQTTSession::sendUnsub(const char* _topicStr) {
  if (isEstablished()) {
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)_topicStr;

    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_unsubscribe(buf, buf_size, 0, getNextPacketId(), 1, &topic);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return false;
}


bool MQTTSession::sendKeepAlive() {
  if (isEstablished()) {
		if (_ping_outstanding()) {
			#if defined (__MANUVR_DEBUG)
			if (getVerbosity() > 3) Kernel::log("MQTT session has an expired ping.\n");
			#endif
		}

    size_t buf_size     = 16;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_pingreq(buf, buf_size);

    if (len > 0) {
			_ping_outstanding(true);
      return (0 == sendPacket(buf, len));
    }
  }
	else {
		_ping_timer.enableSchedule(false);
	}
  return false;
}


bool MQTTSession::sendConnectPacket() {
  if (owner->connected()) {
		printf("SESSION OPERATION sendConnectPacket() \n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	  data.willFlag    = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = opts.clientid;
    data.username.cstring = opts.username;
    data.password.cstring = opts.password;
    data.keepAliveInterval = 10;
    data.cleansession = 1;

    size_t buf_size     = 96;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_connect(buf, buf_size, &data);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return false;
}


bool MQTTSession::sendDisconnectPacket() {
  if (isEstablished()) {
    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_disconnect(buf, buf_size);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return false;
}


bool MQTTSession::sendPublish(ManuvrRunnable* _msg) {
	if (isEstablished()) {
		enum QoS _qos = _msg->demandsACK() ? QOS1 : QOS0;
		int _msg_id = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)_msg->getMsgDef()->debug_label;

		switch (_qos) {
			case QOS0:
				break;
			case QOS1:
			case QOS2:
				_msg_id = getNextPacketId();
				break;
		}

    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);

		int len = MQTTSerialize_publish(
			buf, buf_size,
			0,
			_qos,
			0, //message->retained,
			_msg_id,
      topic,
			(unsigned char*) "TEST MESSAGE",         // message->payload,
			strlen("TEST MESSAGE")  //message->payloadlen
		);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
	}
	return false;
}


/*
*/
int MQTTSession::proc_publish(MQTTMessage* nu) {
	int return_value = nu->decompose_publish();

	if (return_value >= 0) {
  	std::map<const char*, ManuvrRunnable*>::iterator it = _subscriptions.find(nu->topic);
		for (it = _subscriptions.begin(); it != _subscriptions.end(); it++) {
			if (0 == strcmp((const char*) nu->topic, it->first)) {
				if (0 < nu->argumentBytes()) {
					it->second->inflateArgumentsFromBuffer((uint8_t*) nu->payload, nu->argumentBytes());
					//StringBuilder* _hack = new StringBuilder((uint8_t*) nu->payload, nu->argumentBytes());
					//_hack->concat('\0');  // Yuck... No native null-term.
					//_hack->string();  // Condense.
					//it->second->markArgForReap(it->second->addArg(_hack), true);
				}
				//nu->printDebug(&local_log);
				//local_log.concat("\n\n");
				//it->second->printDebug(&local_log);
				//Kernel::log(&local_log);

				raiseEvent(it->second);
				delete nu;  // TODO: Yuck. Nasty. No. Bad.
				return 0;
			}
		}

		if (getVerbosity() > 2) {
			local_log.concatf("%s got a PUBLISH on a topc (%s) it wasn't expecting.\n", getReceiverName(), nu->topic);
			Kernel::log(&local_log);
		}
	}
	return -1;
}


int MQTTSession::process_inbound() {
	if (0 == _pending_mqtt_messages.size()) {
		return -1;
	}
	MQTTMessage* nu = _pending_mqtt_messages.dequeue();

	unsigned short packet_type = nu->packetType();
	switch (packet_type) {
		case CONNACK:
			{
				printf("SESSION DEBUG BITS  0x%02x\n", *((uint8_t*)nu->payload + 1));
				switch (*((uint8_t*)nu->payload + 1)) {
					case 0:
						mark_session_state(XENOSESSION_STATE_ESTABLISHED);
						if(0 == (*((uint8_t*)nu->payload) & 0x01)) {
							// If we are in this block, it means we have a clean session.
							resubscribeAll();
						}
						_ping_timer.enableSchedule(true);
						break;
					default:
						mark_session_state(XENOSESSION_STATE_HUNGUP);
						_ping_timer.enableSchedule(false);
						break;
				}
			}
			delete nu;
			return 1;
    case PUBACK:
			// TODO: Mark outbound message as received.
      break;
    case SUBACK:
			// TODO: Only NOW should we insert into the subscription queue.
      break;
		case PUBLISH:
			proc_publish(nu);
      break;

    case PUBCOMP:
      break;
    case PINGRESP:
      _ping_outstanding(false);
      break;
		default:
			break;
	}


	return 0;
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
* Boot done finished-up.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t MQTTSession::bootComplete() {
  XenoSession::bootComplete();

  owner->getMTU();

  __kernel->addSchedule(&_ping_timer);

  if (owner->connected()) {
    // Are we connected right now?
    sendConnectPacket();
  }
  return 1;
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
int8_t MQTTSession::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
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
int8_t MQTTSession::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
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
      if (_pending_mqtt_messages.size() > 0) {
				process_inbound();
      	return_value++;
			}
      break;

    case MANUVR_MSG_SESS_HANGUP:
      //if (isEstablished()) {
				// If we have an existing session, we try to end it politely.
				sendDisconnectPacket();
			//}
			//else {
				XenoSession::notify(active_event);
			//}
      return_value++;
      break;

    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      sendKeepAlive();
      return_value++;
      break;

    default:
      return_value += XenoSession::notify(active_event);
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}



#if defined(__MANUVR_CONSOLE_SUPPORT)
void MQTTSession::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'q':  // Manual message queue purge.
      Kernel::raiseEvent(MANUVR_MSG_SESS_HANGUP, NULL);
      break;

		case 's':
			//if (subscribe("sillytest", &test_sub_event) == -1) {
				local_log.concat("Failed to subscribe to 'sillytest'.");
			//}
			//else {
			//	local_log.concat("Subscribed to 'sillytest'.");
			//}
			break;
    case 'w':  // Manual session poll.
      Kernel::raiseEvent(MANUVR_MSG_SESS_ORIGINATE_MSG, NULL);
      break;

		case 'c':
			if (sendConnectPacket() == -1) {
				local_log.concat("Failed to send MQTT connect packet.");
			}
			else {
				local_log.concat("Sent MQTT connect packet.");
			}
			break;

		case 'p':
			{
				ManuvrRunnable mr = ManuvrRunnable(MANUVR_MSG_SESS_ORIGINATE_MSG);
				if (sendPublish(&mr) == -1) {
					local_log.concat("Failed to send MQTT publish packet.");
				}
				else {
					local_log.concat("Sent MQTT publish packet.");
				}
			}
			break;

		case 'd':
			if (sendDisconnectPacket() == -1) {
				local_log.concat("Failed to send MQTT disconnect packet.");
			}
			else {
				local_log.concat("Sent MQTT disconnect packet.");
			}
			break;

    default:
      XenoSession::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif  // __MANUVR_CONSOLE_SUPPORT


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* MQTTSession::getReceiverName() {  return "MQTTSession";  }


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void MQTTSession::printDebug(StringBuilder *output) {
  XenoSession::printDebug(output);
  output->concatf("-- Next Packet ID       0x%08x\n", (uint32_t) _next_packetid);
  if (_ping_outstanding()) output->concat("-- EXPIRED PING\n");
  output->concat("-- Subscribed topics\n");

	std::map<const char*, ManuvrRunnable*>::iterator it;
  for (it = _subscriptions.begin(); it != _subscriptions.end(); it++) {
    output->concatf("--\t%s\t~~~~> %s\n", it->first, it->second->getMsgDef()->debug_label);
  }

	if (NULL != working) {
		output->concat("--\n-- Incomplete inbound message:\n");
		working->printDebug(output);
	}
}


#endif
