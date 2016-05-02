/*
File:   MQTTMessage.cpp
Author: J. Ian Lindsay
Date:   2016.04.28

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

MQTTMessage::MQTTMessage() : XenoMessage() {
	_parse_stage = 0;
	_header.byte = 0;
	_multiplier  = 1;
	payload      = NULL;
	qos          = QOS0;
	unique_id = 0;
	retained = 0;
	dup = 0;
}

MQTTMessage::~MQTTMessage() {
	if (NULL != payload) {
		free(payload);
		payload = NULL;
	}
}


/**
* This method is called to flatten this message (and its Event) into a string
*   so that the session can provide it to the transport.
*
* @return  The total size of the string that is meant for the transport,
*            or -1 if something went wrong.
*/
int MQTTMessage::serialize(StringBuilder* buffer) {
  if (NULL == event) {
    return 0;
  }

  awaitingSend(true);
  int arg_count  = event->serialize(buffer);
  if (arg_count >= 0) {
    bytes_total = (uint32_t) buffer->length();
  }

  return buffer->length();
}


/**
* This function should be called by the session to feed bytes to a message.
*
* @return  The number of bytes consumed, or a negative value on failure.
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
				bytes_total += (_tmp & 127) * _multiplier;
				if (0 == (_tmp & 128)) {
					if (bytes_total > 0) {
						payload = malloc(bytes_total);
						if (NULL == payload) {
							// Not enough memory.
							bytes_total = 0;
							return -1;
						}
					}
					else if (bytes_total == 0) {
						_parse_stage++;   // There will be no payload.
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
				if (bytesRemaining()) {
					//printf("PAYLOAD (%d): 0x%02x\n", bytes_received, *((uint8_t*)_buf + _r));
					*((uint8_t*) payload + bytes_received++) = *((uint8_t*)_buf + _r++);
					if (0 == bytesRemaining()) {
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


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void MQTTMessage::printDebug(StringBuilder *output) {
  XenoMessage::printDebug(output);
  output->concatf("\t unique_id       0x%04x\n", unique_id);
  output->concatf("\t Packet type     0x%02x\n", packetType());
	//for (int i = 0; i < bytes_total; i++) {
		//output->concatf("--  0x%02x\n", *((uint8_t*)payload + i));
	//}
}

#endif
