/*
File:   CoAP.h
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

The message class was derived from cantcoap. Pieces of the original header
  files are here until they are fully-digested and the useless WIN32 case-offs
  are removed.
*/

#ifndef __XENOSESSION_CoAP_H__
#define __XENOSESSION_CoAP_H__

#include <stdint.h>
#include <arpa/inet.h>  /* __BYTE_ORDER */  // TODO: Bad. Strip.
#include "../XenoSession.h"

#define MAX_PACKET_ID 65535

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define CoAP_SESS_FLAG_PING_WAIT  0x01    // Are we waiting on a ping reply?


#ifndef SYSDEP_H
#define SYSDEP_H

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    #define __LITTLE_ENDIAN__
  #elif __BYTE_ORDER == __BIG_ENDIAN
    #define __BIG_ENDIAN__
  #endif
#endif

#ifdef __LITTLE_ENDIAN__

#define endian_be16(x) ntohs(x)
#define endian_be32(x) ntohl(x)

#if defined(_byteswap_uint64) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#  define endian_be64(x) (_byteswap_uint64(x))
#elif defined(bswap_64)
#  define endian_be64(x) bswap_64(x)
#elif defined(__DARWIN_OSSwapInt64)
#  define endian_be64(x) __DARWIN_OSSwapInt64(x)
#else
#define endian_be64(x) \
    ( ((((uint64_t)x) << 56)                         ) | \
      ((((uint64_t)x) << 40) & 0x00ff000000000000ULL ) | \
      ((((uint64_t)x) << 24) & 0x0000ff0000000000ULL ) | \
      ((((uint64_t)x) <<  8) & 0x000000ff00000000ULL ) | \
      ((((uint64_t)x) >>  8) & 0x00000000ff000000ULL ) | \
      ((((uint64_t)x) >> 24) & 0x0000000000ff0000ULL ) | \
      ((((uint64_t)x) >> 40) & 0x000000000000ff00ULL ) | \
      ((((uint64_t)x) >> 56)                         ) )
#endif

#define endian_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)(from))[0]) << 8) | \
        (((uint16_t)((uint8_t*)(from))[1])     ) ))

#define endian_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)(from))[0]) << 24) | \
        (((uint32_t)((uint8_t*)(from))[1]) << 16) | \
        (((uint32_t)((uint8_t*)(from))[2]) <<  8) | \
        (((uint32_t)((uint8_t*)(from))[3])      ) ))

#define endian_load64(cast, from) ((cast)( \
        (((uint64_t)((uint8_t*)(from))[0]) << 56) | \
        (((uint64_t)((uint8_t*)(from))[1]) << 48) | \
        (((uint64_t)((uint8_t*)(from))[2]) << 40) | \
        (((uint64_t)((uint8_t*)(from))[3]) << 32) | \
        (((uint64_t)((uint8_t*)(from))[4]) << 24) | \
        (((uint64_t)((uint8_t*)(from))[5]) << 16) | \
        (((uint64_t)((uint8_t*)(from))[6]) << 8)  | \
        (((uint64_t)((uint8_t*)(from))[7])     )  ))

#else // __LITTLE_ENDIAN__

#define endian_be16(x) (x)
#define endian_be32(x) (x)
#define endian_be64(x) (x)

#define endian_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)(from))[0]) << 8) | \
        (((uint16_t)((uint8_t*)(from))[1])     ) ))

#define endian_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)(from))[0]) << 24) | \
        (((uint32_t)((uint8_t*)(from))[1]) << 16) | \
        (((uint32_t)((uint8_t*)(from))[2]) <<  8) | \
        (((uint32_t)((uint8_t*)(from))[3])      ) ))

#define endian_load64(cast, from) ((cast)( \
        (((uint64_t)((uint8_t*)(from))[0]) << 56) | \
        (((uint64_t)((uint8_t*)(from))[1]) << 48) | \
        (((uint64_t)((uint8_t*)(from))[2]) << 40) | \
        (((uint64_t)((uint8_t*)(from))[3]) << 32) | \
        (((uint64_t)((uint8_t*)(from))[4]) << 24) | \
        (((uint64_t)((uint8_t*)(from))[5]) << 16) | \
        (((uint64_t)((uint8_t*)(from))[6]) << 8)  | \
        (((uint64_t)((uint8_t*)(from))[7])     )  ))
#endif

#define endian_store16(to, num) \
    do { uint16_t val = endian_be16(num); memcpy(to, &val, 2); } while(0)
#define endian_store32(to, num) \
    do { uint32_t val = endian_be32(num); memcpy(to, &val, 4); } while(0)
#define endian_store64(to, num) \
    do { uint64_t val = endian_be64(num); memcpy(to, &val, 8); } while(0)

#endif // SYSDEP_H


/// Copyright (c) 2013, Ashley Mills.
#define COAP_HDR_SIZE 4
#define COAP_OPTION_HDR_BYTE 1

// CoAP PDU format
//   0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Ver| T |  TKL  |      Code     |          Message ID           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |   Token (if any, TKL bytes) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |   Options (if any) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |1 1 1 1 1 1 1 1|    Payload (if any) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


class CoAPMessage : public XenoMessage {
  public:
    char retained;
    char dup;
    uint16_t unique_id;
    char* topic;
    void *payload;

		CoAPMessage();
		CoAPMessage(uint8_t *pdu, int pduLength);
		CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength);
    ~CoAPMessage();

    virtual void printDebug(StringBuilder*);

    // Called to accumulate data into the class.
    // Returns -1 on failure, or the number of bytes consumed on success.
    int serialize(StringBuilder*);       // Returns the number of bytes resulting.
    int accumulate(unsigned char*, int);

    int decompose_publish();

    // TODO: These members were digested from cantcoap...
		/// CoAP message types. Note, values only work as enum.
		enum Type {
			COAP_CONFIRMABLE=0x00,
			COAP_NON_CONFIRMABLE=0x10,
			COAP_ACKNOWLEDGEMENT=0x20,
			COAP_RESET=0x30
		};

		// CoAP response codes.
		enum Code {
			COAP_EMPTY=0x00,
			COAP_GET,
			COAP_POST,
			COAP_PUT,
			COAP_DELETE,
			COAP_CREATED=0x41,
			COAP_DELETED,
			COAP_VALID,
			COAP_CHANGED,
			COAP_CONTENT,
			COAP_BAD_REQUEST=0x80,
			COAP_UNAUTHORIZED,
			COAP_BAD_OPTION,
			COAP_FORBIDDEN,
			COAP_NOT_FOUND,
			COAP_METHOD_NOT_ALLOWED,
			COAP_NOT_ACCEPTABLE,
			COAP_PRECONDITION_FAILED=0x8C,
			COAP_REQUEST_ENTITY_TOO_LARGE=0x8D,
			COAP_UNSUPPORTED_CONTENT_FORMAT=0x8F,
			COAP_INTERNAL_SERVER_ERROR=0xA0,
			COAP_NOT_IMPLEMENTED,
			COAP_BAD_GATEWAY,
			COAP_SERVICE_UNAVAILABLE,
			COAP_GATEWAY_TIMEOUT,
			COAP_PROXYING_NOT_SUPPORTED,
			COAP_UNDEFINED_CODE=0xFF
		};

		/// CoAP option numbers.
		enum Option {
			COAP_OPTION_IF_MATCH=1,
			COAP_OPTION_URI_HOST=3,
			COAP_OPTION_ETAG,
			COAP_OPTION_IF_NONE_MATCH,
			COAP_OPTION_OBSERVE,
			COAP_OPTION_URI_PORT,
			COAP_OPTION_LOCATION_PATH,
			COAP_OPTION_URI_PATH=11,
			COAP_OPTION_CONTENT_FORMAT,
			COAP_OPTION_MAX_AGE=14,
			COAP_OPTION_URI_QUERY,
			COAP_OPTION_ACCEPT=17,
			COAP_OPTION_LOCATION_QUERY=20,
			COAP_OPTION_BLOCK2=23,
			COAP_OPTION_BLOCK1=27,
			COAP_OPTION_SIZE2,
			COAP_OPTION_PROXY_URI=35,
			COAP_OPTION_PROXY_SCHEME=39,
			COAP_OPTION_SIZE1=60
		};

		/// CoAP content-formats.
		enum ContentFormat {
			COAP_CONTENT_FORMAT_TEXT_PLAIN = 0,
			COAP_CONTENT_FORMAT_APP_LINK  = 40,
			COAP_CONTENT_FORMAT_APP_XML,
			COAP_CONTENT_FORMAT_APP_OCTET,
			COAP_CONTENT_FORMAT_APP_EXI   = 47,
			COAP_CONTENT_FORMAT_APP_JSON  = 50
		};

		/// Sequence of these is returned by CoAPMessage::getOptions()
		struct CoapOption {
			uint16_t optionDelta;
			uint16_t optionNumber;
			uint16_t optionValueLength;
			int totalLength;
			uint8_t *optionPointer;
			uint8_t *optionValuePointer;
		};

		int reset();
		int validate();

		// version
		int setVersion(uint8_t version);
		uint8_t getVersion();

		// message type
		void setType(CoAPMessage::Type type);
		CoAPMessage::Type getType();

		// tokens
		int setTokenLength(uint8_t tokenLength);
		int getTokenLength();
		uint8_t* getTokenPointer();
		int setToken(uint8_t *token, uint8_t tokenLength);

		// message code
		void setCode(CoAPMessage::Code code);
		CoAPMessage::Code getCode();

		// message ID
		int setMessageID(uint16_t messageID);
		uint16_t getMessageID();

		// options
		int addOption(uint16_t optionNumber, uint16_t optionLength, uint8_t *optionValue);
		// gets a list of all options
		CoapOption* getOptions();

    /* Return the number of options in the message. */
		inline int getNumOptions() {      return _numOptions;    };

		// shorthand helpers
		int setURI(char *uri);
		int setURI(char *uri, int urilen);
		int getURI(char *dst, int dstlen, int *outLen);
		int addURIQuery(char *query);

		// content format helper
		int setContentFormat(CoAPMessage::ContentFormat format);

		// payload
		uint8_t* mallocPayload(int bytes);
		int setPayload(uint8_t *value, int len);
		uint8_t* getPayloadPointer();
		int getPayloadLength();
		uint8_t* getPayloadCopy();

		// pdu
		inline int getPDULength() {  return _pduLength;  };
		uint8_t* getPDUPointer();
		void setPDULength(int len);

		// debugging
		void print();
		void printBin();
		void printHex();
		void printOptionHuman(uint8_t *option);
		void printHuman();
		void printPDUAsCArray();

		static void printBinary(uint8_t b);
    static const char* optionNumToString(uint16_t);
    static const char* codeToString(CoAPMessage::Code);
		static CoAPMessage::Code httpStatusToCode(int httpStatus);


  private:
    // TODO: These members were digested from cantcoap...
		uint8_t* _pdu;
		uint8_t* _payloadPointer;
		int _pduLength;
		int _constructedFromBuffer;
		int _bufferLength;
		int _payloadLength;
		int _numOptions;
		uint16_t _maxAddedOptionNumber;

		void shiftPDUUp(int shiftOffset, int shiftAmount);
		void shiftPDUDown(int startLocation, int shiftOffset, int shiftAmount);
		uint8_t codeToValue(CoAPMessage::Code c);

		// option stuff
		int findInsertionPosition(uint16_t optionNumber, uint16_t *prevOptionNumber);
		int computeExtraBytes(uint16_t n);
		int insertOption(int insertionPosition, uint16_t optionDelta, uint16_t optionValueLength, uint8_t *optionValue);
		uint16_t getOptionDelta(uint8_t *option);
		void setOptionDelta(int optionPosition, uint16_t optionDelta);
		uint16_t getOptionValueLength(uint8_t *option);
};


struct CoAPOpts {
	char* clientid;
	int nodelimiter;
	char* delimiter;
	char* username;
	char* password;
	int showtopics;
};


class CoAPSession : public XenoSession {
  public:
    CoAPSession(BufferPipe*);
    ~CoAPSession();

    int8_t sendEvent(ManuvrRunnable*);

    /* Management of subscriptions... */
    int8_t subscribe(const char*, ManuvrRunnable*);  // Start getting broadcasts about a given message type.
    int8_t unsubscribe(const char*);                 // Stop getting broadcasts about a given message type.
    int8_t resubscribeAll();
    int8_t unsubscribeAll();

    int8_t connection_callback(bool connected);

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


  protected:
    int8_t bootComplete();


  private:
    CoAPMessage* working;

    PriorityQueue<CoAPMessage*> _pending_coap_messages;      // Valid CoAP messages that have arrived.
    ManuvrRunnable _ping_timer;    // Periodic KA ping.

    unsigned int _next_packetid;

    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

    inline int getNextPacketId() {
      return _next_packetid = (_next_packetid == MAX_PACKET_ID) ? 1 : _next_packetid + 1;
    };
};

#endif //__XENOSESSION_CoAP_H__
