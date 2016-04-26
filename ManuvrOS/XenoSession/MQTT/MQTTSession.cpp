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


// TODO TODO TODO TODO TODO TODO TODO
// TODO TODO TODO TODO TODO TODO TODO

MQTTMessage::MQTTMessage() {
	_rem_len     = 0;
	_parse_stage = 0;
	_header.byte = 0;
	_multiplier  = 1;
	payload      = NULL;
	payloadlen   = 0;
	qos          = QOS0;
	id = 0;
	retained = 0;
	dup = 0;
}

MQTTMessage::~MQTTMessage() {
	if (NULL != payload) {
		free(payload);
		payload = NULL;
		payloadlen   = 0;
	}
}

/*
*
*/
int MQTTMessage::accumulate(unsigned char* _buf, int _len) {
	int _r = 0;
	uint8_t _tmp;
	while (_r < _len) {
		switch (_parse_stage) {
			case 0:
				// We haven't gotten any fields yet. Read the header byte.
				_header.byte = *((uint8_t*)_buf + _r++);
				_parse_stage++;
				break;
			case 1:
				// Read the remaining length, which is encoded as a string of 7-bit ints.
				_tmp = *((uint8_t*)_buf + _r++);
				payloadlen += (_tmp & 127) * _multiplier;
				if (0 == (_tmp & 128)) {
					if (payloadlen > 0) {
						payload = malloc(payloadlen);
						if (NULL == payload) {
							// Not enough memory.
							payloadlen = 0;
							return -1;
						}
					}
					_parse_stage++;   // Field completed.
				}
				else {
					_multiplier *= 128;
					if (_multiplier > 2097152) {
						// We have exceeded the 4-byte length-field. Failure...
						return -1;
					}
				}
				break;
			case 2:
				// Here, we just copy bytes into the payload field until we reach our
				//   length target.
				if (payloadlen > _rem_len) {
					*((uint8_t*) payload + _rem_len++) = *((uint8_t*)_buf + _r++);
					if (payloadlen == _rem_len) {
						_parse_stage++;   // Field completed.
					}
				}
				else {
					_parse_stage++;   // Field completed.
				}
				break;
			case 3:
				return _r;
				break;
			default:
				break;
		}
	}
	return _r;
}

// TODO TODO TODO TODO TODO TODO TODO
// TODO TODO TODO TODO TODO TODO TODO


/****************************************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here. Initializers first, functions second.
****************************************************************************************************/

MQTTOpts opts = {
	(char*)"manuvr_client",
  0,
  (char*)"\n",
  QOS2,
  NULL,
  NULL,
  0
};


/****************************************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
MQTTSession::MQTTSession(ManuvrXport* _xport) : XenoSession(_xport) {
	_ping_outstanding = 0;
	working   = NULL;
	_next_packetid = 1;

  for (int i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
    messageHandlers[i].topicFilter = 0;
  }

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
				acceptInbound(working);
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




int8_t MQTTSession::sendSub(const char* _topicStr, enum QoS qos) {
  if (isEstablished()) {
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)_topicStr;

    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_subscribe(buf, buf_size, 0, getNextPacketId(), 1, &topic, (int*)&qos);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return -1;
}

int8_t MQTTSession::sendUnsub(const char* _topicStr) {
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
  return -1;
}


int8_t MQTTSession::sendKeepAlive() {
  if (isEstablished()) {
    size_t buf_size     = 16;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_pingreq(buf, buf_size);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return -1;
}


int8_t MQTTSession::sendConnectPacket() {
  if (owner->connected()) {
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	  data.willFlag    = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = opts.clientid;
    data.username.cstring = opts.username;
    data.password.cstring = opts.password;
    data.keepAliveInterval = 10;
    data.cleansession = 1;

    size_t buf_size     = 160;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_connect(buf, buf_size, &data);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return -1;
}


int8_t MQTTSession::sendDisconnectPacket() {
  if (isEstablished()) {
    size_t buf_size     = 64;
    unsigned char* buf  = (unsigned char*) alloca(buf_size);
    int len = MQTTSerialize_disconnect(buf, buf_size);

    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return -1;
}

int8_t MQTTSession::sendPublish(ManuvrRunnable* _msg) {
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
	return -1;
}


int8_t MQTTSession::connection_callback(bool _con) {
	XenoSession::connection_callback(_con);
	if (_con) {
		sendConnectPacket();
	}
  return 0;
}


// These messages are arriving from the parser.
// Management of their memory is our responsibility.
int MQTTSession::acceptInbound(MQTTMessage* nu) {
	if (nu->parseComplete()) {
		unsigned short packet_type = nu->packetType();
		switch (packet_type) {
			case CONNACK:
				{
					if(0 == *((uint8_t*)nu->payload) & 0x01) {
						// If we are in this block, it means we have a clean session.
					}
					switch (*((uint8_t*)nu->payload + 1)) {
						case 0:
							mark_session_state(XENOSESSION_STATE_ESTABLISHED);
							//_ping_timer.enableSchedule(true);
							break;
						default:
							mark_session_state(XENOSESSION_STATE_HUNGUP);
							break;
					}
				}
				delete nu;
				return 1;
      case PUBACK:
      case SUBACK:
        break;
			case PUBLISH:
        break;

      case PUBCOMP:
        break;
      case PINGRESP:
        _ping_outstanding = 0;
        break;
			default:
				break;
		}
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

  _ping_timer.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG);
  _ping_timer.isManaged(true);
  _ping_timer.specific_target = (EventReceiver*) this;
  _ping_timer.alterScheduleRecurrence(-1);
  _ping_timer.alterSchedulePeriod(10000);
  _ping_timer.autoClear(false);
  _ping_timer.enableSchedule(false);

  __kernel->addSchedule(&_ping_timer);

  if (isConnected()) {
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
    case MANUVR_MSG_SELF_DESCRIBE:
      //sendEvent(event);
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

    case MANUVR_MSG_SESS_HANGUP:
      sendDisconnectPacket();
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


void MQTTSession::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'q':  // Manual message queue purge.
      purgeOutbound();
      purgeInbound();
      break;

		case 's':
			if (sendSub("sillytest", QOS2) == -1) {
				local_log.concat("Failed to subscribe to 'sillytest'.");
			}
			else {
				local_log.concat("Subscribed to 'sillytest'.");
			}
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
					local_log.concat("Failed to send MQTT connect packet.");
				}
				else {
					local_log.concat("Sent MQTT connect packet.");
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

	if (NULL != working) {
		output->concat("--\n-- Incomplete inbound message:\n");
		output->concatf("--     Packet id        0x%04x\n", working->id);
		output->concatf("--     Packet type      0x%02x\n", working->packetType());
		output->concatf("--     Payload length   %d\n", working->payloadlen);
		output->concatf("--     Parse complete   %s\n", working->parseComplete() ? "yes":"no");
		//for (int i = 0; i < working->payloadlen; i++) {
			//output->concatf("--  0x%02x\n", *((uint8_t*)working->payload + i));
		//}
	}
}


#endif
