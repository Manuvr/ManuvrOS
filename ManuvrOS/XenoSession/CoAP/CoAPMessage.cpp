/*
File:   CoAPMessage.cpp
Author: J. Ian Lindsay
Date:   2016.08.03

This file is an adaption of cantcoap to Manuvr. I would have preferred to
  build it as a library as is done with MQTT, but C++ linkage and
  platform-specific ties made this necessary to achieve IP abstraction.
Cantcoap is simply a packet parser/packer, so all notions of stateful
  session are concentrated in CoAPSession.
Original license preserved below.


Copyright (c) 2013, Ashley Mills.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// version, 2 bits
// type, 2 bits
  // 00 Confirmable
  // 01 Non-confirmable
  // 10 Acknowledgement
  // 11 Reset

// token length, 4 bits
// length of token in bytes (only 0 to 8 bytes allowed)
#include "CoAPSession.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>


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

const char* CoAPMessage::optionNumToString(uint16_t code) {
  switch(code) {
    case COAP_OPTION_IF_MATCH:       return "IF_MATCH";
    case COAP_OPTION_URI_HOST:       return "URI_HOST";
    case COAP_OPTION_ETAG:           return "ETAG";
    case COAP_OPTION_IF_NONE_MATCH:  return "IF_NONE_MATCH";
    case COAP_OPTION_OBSERVE:        return "OBSERVE";
    case COAP_OPTION_URI_PORT:       return "URI_PORT";
    case COAP_OPTION_LOCATION_PATH:  return "LOCATION_PATH";
    case COAP_OPTION_URI_PATH:       return "URI_PATH";
    case COAP_OPTION_CONTENT_FORMAT: return "CONTENT_FORMAT";
    case COAP_OPTION_MAX_AGE:        return "MAX_AGE";
    case COAP_OPTION_URI_QUERY:      return "URI_QUERY";
    case COAP_OPTION_ACCEPT:         return "ACCEPT";
    case COAP_OPTION_LOCATION_QUERY: return "LOCATION_QUERY";
    case COAP_OPTION_PROXY_URI:      return "PROXY_URI";
    case COAP_OPTION_PROXY_SCHEME:   return "PROXY_SCHEME";
    case COAP_OPTION_BLOCK1:         return "BLOCK1";
    case COAP_OPTION_BLOCK2:         return "BLOCK2";
    case COAP_OPTION_SIZE1:          return "SIZE1";
    case COAP_OPTION_SIZE2:          return "SIZE2";
    default:                         return "<UNDEF>";
  }
}

const char* CoAPMessage::codeToString(CoAPMessage::Code code) {
  switch(code) {
    case COAP_EMPTY:                      return "0.00 Empty";
    case COAP_GET:                        return "0.01 GET";
    case COAP_POST:                       return "0.02 POST";
    case COAP_PUT:                        return "0.03 PUT";
    case COAP_DELETE:                     return "0.04 DELETE";
    case COAP_CREATED:                    return "2.01 Created";
    case COAP_DELETED:                    return "2.02 Deleted";
    case COAP_VALID:                      return "2.03 Valid";
    case COAP_CHANGED:                    return "2.04 Changed";
    case COAP_CONTENT:                    return "2.05 Content";
    case COAP_BAD_REQUEST:                return "4.00 Bad Request";
    case COAP_UNAUTHORIZED:               return "4.01 Unauthorized";
    case COAP_BAD_OPTION:                 return "4.02 Bad Option";
    case COAP_FORBIDDEN:                  return "4.03 Forbidden";
    case COAP_NOT_FOUND:                  return "4.04 Not Found";
    case COAP_METHOD_NOT_ALLOWED:         return "4.05 Method Not Allowef";
    case COAP_NOT_ACCEPTABLE:             return "4.06 Not Acceptable";
    case COAP_PRECONDITION_FAILED:        return "4.12 Precondition Failed";
    case COAP_REQUEST_ENTITY_TOO_LARGE:   return "4.13 Request Entity Too Large";
    case COAP_UNSUPPORTED_CONTENT_FORMAT: return "4.15 Unsupported Content-Format";
    case COAP_INTERNAL_SERVER_ERROR:      return "5.00 Internal Server Error";
    case COAP_NOT_IMPLEMENTED:            return "5.01 Not Implemented";
    case COAP_BAD_GATEWAY:                return "5.02 Bad Gateway";
    case COAP_SERVICE_UNAVAILABLE:        return "5.03 Service Unavailable";
    case COAP_GATEWAY_TIMEOUT:            return "5.04 Gateway Timeout";
    case COAP_PROXYING_NOT_SUPPORTED:     return "5.05 Proxying Not Supported";
    case COAP_UNDEFINED_CODE:
    default:                              return "<UNDEF>";
  }
}

const char* CoAPMessage::typeToString(CoAPMessage::Type type) {
  switch(type) {
    case COAP_CONFIRMABLE:      return "CONFIRMABLE";
    case COAP_NON_CONFIRMABLE:  return "NON-CONFIRMABLE";
    case COAP_ACKNOWLEDGEMENT:  return "ACK";
    case COAP_RESET:            return "RESET";
  }
}

/**
* Converts a http status code as an integer, to a CoAP code.
*
* @param httpStatus the HTTP status code as an integer (e.g 200)
* @return The correct corresponding CoAPMessage::Code on success,
*           CoAPMessage::COAP_UNDEFINED_CODE on failure.
*/
CoAPMessage::Code CoAPMessage::httpStatusToCode(int httpStatus) {
  switch(httpStatus) {
    case 1:    return CoAPMessage::COAP_GET;
    case 2:    return CoAPMessage::COAP_POST;
    case 3:    return CoAPMessage::COAP_PUT;
    case 4:    return CoAPMessage::COAP_DELETE;
    case 201:  return CoAPMessage::COAP_CREATED;
    case 202:  return CoAPMessage::COAP_DELETED;
    case 203:  return CoAPMessage::COAP_VALID;
    case 204:  return CoAPMessage::COAP_CHANGED;
    case 205:  return CoAPMessage::COAP_CONTENT;
    case 400:  return CoAPMessage::COAP_BAD_REQUEST;
    case 401:  return CoAPMessage::COAP_UNAUTHORIZED;
    case 402:  return CoAPMessage::COAP_BAD_OPTION;
    case 403:  return CoAPMessage::COAP_FORBIDDEN;
    case 404:  return CoAPMessage::COAP_NOT_FOUND;
    case 405:  return CoAPMessage::COAP_METHOD_NOT_ALLOWED;
    case 406:  return CoAPMessage::COAP_NOT_ACCEPTABLE;
    case 412:  return CoAPMessage::COAP_PRECONDITION_FAILED;
    case 413:  return CoAPMessage::COAP_REQUEST_ENTITY_TOO_LARGE;
    case 415:  return CoAPMessage::COAP_UNSUPPORTED_CONTENT_FORMAT;
    case 500:  return CoAPMessage::COAP_INTERNAL_SERVER_ERROR;
    case 501:  return CoAPMessage::COAP_NOT_IMPLEMENTED;
    case 502:  return CoAPMessage::COAP_BAD_GATEWAY;
    case 503:  return CoAPMessage::COAP_SERVICE_UNAVAILABLE;
    case 504:  return CoAPMessage::COAP_GATEWAY_TIMEOUT;
    case 505:  return CoAPMessage::COAP_PROXYING_NOT_SUPPORTED;
    default:   return CoAPMessage::COAP_UNDEFINED_CODE;
  }
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/


/// Memory-managed constructor. Buffer for PDU is dynamically sized and allocated by the object.
/**
 * When using this constructor, the CoAPMessage class will allocate space for the PDU.
 * Contrast this with the parameterized constructors, which allow the use of an external buffer.
 *
 * Note, the PDU container and space can be reused by issuing a CoAPMessage::reset(). If the new PDU exceeds the
 * space of the previously allocated memory, then further memory will be dynamically allocated.
 *
 * Deleting the object will free the Object container and all dynamically allocated memory.
 *
 * \note It would have been nice to use something like UDP_CORK or MSG_MORE, to allow separate buffers
 * for token, options, and payload but these FLAGS aren't implemented for UDP in LwIP so stuck with one buffer for now.
 *
 * CoAP version defaults to 1.
 *
 * \sa CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength), CoAPMessage::CoAPMessage::(uint8_t *buffer, int bufferLength, int pduLength),
 * CoAPMessage:CoAPMessage()~
 *
 */
CoAPMessage::CoAPMessage() {
  // pdu
  _pdu = (uint8_t*)calloc(4,sizeof(uint8_t));
  _pduLength = 4;
  _bufferLength = _pduLength;

  //options
  _numOptions = 0;
  _maxAddedOptionNumber = 0;

  // payload
  _payloadPointer = nullptr;
  _payloadLength = 0;

  _constructedFromBuffer = 0;

  setVersion(1);
}

/// Construct a PDU using an external buffer. No copy of the buffer is made.
/**
 * This constructor is normally used where a PDU has been received over the network, and it's length is known.
 * In this case the CoAPMessage object is probably going to be used as a temporary container to access member values.
 *
 * It is assumed that \b pduLength is the length of the actual CoAP PDU, and consequently the buffer will also be this size,
 * contrast this with CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength) which allows the buffer to
 * be larger than the PDU.
 *
 * A PDU constructed in this manner must be validated with CoAPMessage::validate() before the member variables will be accessible.
 *
 * \warning The validation call parses the PDU structure to set some internal parameters. If you do
 * not validate the PDU, then the behaviour of member access functions will be undefined.
 *
 * The buffer can be reused by issuing a CoAPMessage::reset() but the class will not change the size of the buffer. If the
 * newly constructed PDU exceeds the size of the buffer, the function called (for example CoAPMessage::addOption) will fail.
 *
 * Deleting this object will only delete the Object container and will not delete the PDU buffer.
 *
 * @param pdu A pointer to an array of bytes which comprise the CoAP PDU
 * @param pduLength The length of the CoAP PDU pointed to by \b pdu

 * \sa CoAPMessage::CoAPMessage(), CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength)
 */
CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength) : CoAPMessage(pdu,pduLength,pduLength) {
  // delegated to CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength)
}

/// Construct object from external buffer that may be larger than actual PDU.
/**
 * This differs from CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength) in that the buffer may be larger
 * than the actual CoAP PDU contained int the buffer. This is typically used when a large buffer is reused
 * multiple times. Note that \b pduLength can be 0.
 *
 * If an actual CoAP PDU is passed in the buffer, \b pduLength should match its length. CoAPMessage::validate() must
 * be called to initiate the object before member functions can be used.
 *
 * A PDU constructed in this manner must be validated with CoAPMessage::validate() before the member variables will be accessible.
 *
 * \warning The validation call parses the PDU structure to set some internal parameters. If you do
 * not validate the PDU, then the behaviour of member access functions will be undefined.
 *
 * The buffer can be reused by issuing a CoAPMessage::reset() but the class will not change the size of the buffer. If the
 * newly constructed PDU exceeds the size of the buffer, the function called (for example CoAPMessage::addOption) will fail.
 *
 * Deleting this object will only delete the Object container and will not delete the PDU buffer.
 *
 * \param buffer A buffer which either contains a CoAP PDU or is intended to be used to construct one.
 * \param bufferLength The length of the buffer
 * \param pduLength If the buffer contains a CoAP PDU, this specifies the length of the PDU within the buffer.
 *
 * \sa CoAPMessage::CoAPMessage(), CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength)
 */
CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength) {
  // sanity
  if(pduLength<4&&pduLength!=0) {
    coap_log.concat("PDU cannot have a length less than 4\n");
    Kernel::log(&coap_log);
  }

  // pdu
  _pdu = buffer;
  _bufferLength = bufferLength;
  if(pduLength==0) {
    // this is actually a fresh pdu, header always exists
    _pduLength = 4;
    // make sure header is zeroed
    _pdu[0] = 0x00; _pdu[1] = 0x00; _pdu[2] = 0x00; _pdu[3] = 0x00;
    setVersion(1);
  } else {
    _pduLength = pduLength;
  }

  _constructedFromBuffer = 1;

  // options
  _numOptions = 0;
  _maxAddedOptionNumber = 0;

  // payload
  _payloadPointer = nullptr;
  _payloadLength = 0;
}

/// Reset CoAPMessage container so it can be reused to build a new PDU.
/**
 * This resets the CoAPMessage container, setting the pdu length, option count, etc back to zero. The
 * PDU can then be populated as if it were newly constructed.
 *
 * Note that the space available will depend on how the CoAPMessage was originally constructed:
 * -# CoAPMessage::CoAPMessage()
 *
 *   Available space initially be \b _pduLength. But further space will be allocated as needed on demand,
 *    limited only by the OS/environment.
 *
 * -# CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength)
 *
 *    Space is limited by the variable \b pduLength. The PDU cannot exceed \b pduLength bytes.
 *
 * -# CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength)
 *
 *    Space is limited by the variable \b bufferLength. The PDU cannot exceed \b bufferLength bytes.
 *
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::reset() {
  // pdu
  memset(_pdu,0x00,_bufferLength);
  // packet always has at least a header
  _pduLength = 4;

  // options
  _numOptions = 0;
  _maxAddedOptionNumber = 0;
  // payload
  _payloadPointer = nullptr;
  _payloadLength = 0;
  return 0;
}

/// Validates a PDU constructed using an external buffer.
/**
 * When a CoAPMessage is constructed using an external buffer, the programmer must call this function to
 * check that the received PDU is a valid CoAP PDU.
 *
 * \warning The validation call parses the PDU structure to set some internal parameters. If you do
 * not validate the PDU, then the behaviour of member access functions will be undefined.
 *
 * \return 1 if the PDU validates correctly, 0 if not. XXX maybe add some error codes
 */
int CoAPMessage::validate() {
  if(_pduLength<4) {
    coap_log.concatf("PDU has to be a minimum of 4 bytes. This: %d bytes\n",_pduLength);
    Kernel::log(&coap_log);
    return 0;
  }

  // check header
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

  // version must be 1
  int version = getVersion();
  if (version != 1) {
    coap_log.concatf("Invalid version: %d\n", version);
    Kernel::log(&coap_log);
    return 0;
  }
  coap_log.concatf("Version: %d\n", version);
  coap_log.concatf("Type: %d\n", getType());

  // token length must be between 0 and 8
  int tokenLength = getTokenLength();
  if(tokenLength<0||tokenLength>8) {
    coap_log.concatf("Invalid token length: %d\n",tokenLength);
    Kernel::log(&coap_log);
    return 0;
  }
  coap_log.concatf("Token length: %d\n",tokenLength);
  // check total length
  if((COAP_HDR_SIZE+tokenLength)>_pduLength) {
    coap_log.concat("Token length would make pdu longer than actual length.\n");
    Kernel::log(&coap_log);
    return 0;
  }

  // check that code is valid
  CoAPMessage::Code code = getCode();
  if(code<COAP_EMPTY ||
    (code>COAP_DELETE&&code<COAP_CREATED) ||
    (code>COAP_CONTENT&&code<COAP_BAD_REQUEST) ||
    (code>COAP_NOT_ACCEPTABLE&&code<COAP_PRECONDITION_FAILED) ||
    (code==0x8E) ||
    (code>COAP_UNSUPPORTED_CONTENT_FORMAT&&code<COAP_INTERNAL_SERVER_ERROR) ||
    (code>COAP_PROXYING_NOT_SUPPORTED) ) {
    coap_log.concatf("Invalid CoAP code: %d\n",code);
    return 0;
  }
  coap_log.concatf("CoAP code: %d\n",code);

  // token can be anything so nothing to check

  // check that options all make sense
  uint16_t optionDelta =0, optionNumber = 0, optionValueLength = 0;
  int totalLength = 0;

  // first option occurs after token
  int optionPos = COAP_HDR_SIZE + getTokenLength();

  // may be 0 options
  if(optionPos==_pduLength) {
    coap_log.concat("No options. No payload.\n");
    Kernel::log(&coap_log);
    _numOptions = 0;
    _payloadLength = 0;
    return 1;
  }

  int bytesRemaining = _pduLength-optionPos;
  int numOptions = 0;
  uint8_t upperNibble = 0x00, lowerNibble = 0x00;

  // walk over options and record information
  while(1) {
    // check for payload marker
    if(bytesRemaining>0) {
      uint8_t optionHeader = _pdu[optionPos];
      if(optionHeader==0xFF) {
        // payload
        if(bytesRemaining>1) {
          _payloadPointer = &_pdu[optionPos+1];
          _payloadLength = (bytesRemaining-1);
          _numOptions = numOptions;
          coap_log.concatf("Payload found, length: %d\n",_payloadLength);
          Kernel::log(&coap_log);
          return 1;
        }
        // payload marker but no payload
        _payloadPointer = nullptr;
        _payloadLength = 0;
        coap_log.concat("Payload marker but no payload.\n");
        Kernel::log(&coap_log);
        return 0;
      }

      // check that option delta and option length are valid values
      upperNibble = (optionHeader & 0xF0) >> 4;
      lowerNibble = (optionHeader & 0x0F);
      if(upperNibble==0x0F||lowerNibble==0x0F) {
        coap_log.concatf("Expected option header or payload marker, got: 0x%x%x\n",upperNibble,lowerNibble);
        Kernel::log(&coap_log);
        return 0;
      }
      coap_log.concatf("Option header byte appears sane: 0x%x%x\n",upperNibble,lowerNibble);
    }
    else {
      coap_log.concat("No more data. No payload.\n");
      Kernel::log(&coap_log);
      _payloadPointer = nullptr;
      _payloadLength = 0;
      _numOptions = numOptions;
      return 1;
    }

    // skip over option header byte
    bytesRemaining--;

    // check that there is enough space for the extended delta and length bytes (if any)
    int headerBytesNeeded = computeExtraBytes(upperNibble);
    coap_log.concatf("%d extra bytes needed for extended delta\n",headerBytesNeeded);
    if(headerBytesNeeded>bytesRemaining) {
      coap_log.concatf("Not enough space for extended option delta, needed %d, have %d.\n",headerBytesNeeded,bytesRemaining);
      Kernel::log(&coap_log);
      return 0;
    }
    headerBytesNeeded += computeExtraBytes(lowerNibble);
    if(headerBytesNeeded>bytesRemaining) {
      coap_log.concatf("Not enough space for extended option length, needed %d, have %d.\n",
        (headerBytesNeeded-computeExtraBytes(upperNibble)),bytesRemaining);
      Kernel::log(&coap_log);
      return 0;
    }
    coap_log.concatf("Enough space for extended delta and length: %d, continuing.\n",headerBytesNeeded);

    // extract option details
    optionDelta = getOptionDelta(&_pdu[optionPos]);
    optionNumber += optionDelta;
    optionValueLength = getOptionValueLength(&_pdu[optionPos]);
    coap_log.concatf("Got option: %d with length %d\n",optionNumber,optionValueLength);
    // compute total length
    totalLength = 1; // mandatory header
    totalLength += computeExtraBytes(optionDelta);
    totalLength += computeExtraBytes(optionValueLength);
    totalLength += optionValueLength;
    // check there is enough space
    if(optionPos+totalLength>_pduLength) {
      coap_log.concatf("Not enough space for option payload, needed %d, have %d.\n", (totalLength-headerBytesNeeded-1),_pduLength-optionPos);
      Kernel::log(&coap_log);
      return 0;
    }
    coap_log.concatf("Enough space for option payload: %d %d\n",optionValueLength,(totalLength-headerBytesNeeded-1));

    // recompute bytesRemaining
    bytesRemaining -= totalLength;
    bytesRemaining++; // correct for previous --

    // move to next option
    optionPos += totalLength;

    // inc number of options XXX
    numOptions++;
  }

  Kernel::log(&coap_log);
  return 1;
}

/// Destructor. Does not free buffer if constructor passed an external buffer.
/**
 * The destructor acts differently, depending on how the object was initially constructed (from buffer or not):
 *
 * -# CoAPMessage::CoAPMessage()
 *
 *   Complete object is destroyed.
 *
 * -# CoAPMessage::CoAPMessage(uint8_t *pdu, int pduLength)
 *
 *    Only object container is destroyed. \b pdu is left intact.
 *
 * -# CoAPMessage::CoAPMessage(uint8_t *buffer, int bufferLength, int pduLength)
 *
 *    Only object container is destroyed. \b pdu is left intact.
 *
 */
CoAPMessage::~CoAPMessage() {
  if(!_constructedFromBuffer) {
    free(_pdu);
  }
}

/// Shorthand function for setting a resource URI.
/**
 * This will parse the supplied \b uri and construct enough URI_PATH and URI_QUERY options to encode it.
 * The options are added to the PDU.
 *
 * At present only simple URI formatting is handled, only '/','?', and '&' separators, and no port or protocol specificaiton.
 *
 * The function will split on '/' and create URI_PATH elements until it either reaches the end of the string
 * in which case it will stop or if it reaches '?' it will start splitting on '&' and create URI_QUERY elements
 * until it reaches the end of the string.
 *
 * Here is an example:
 *
 * /a/b/c/d?x=1&y=2&z=3
 *
 * Will be broken into four URI_PATH elements "a", "b", "c", "d", and three URI_QUERY elements "x=1", "y=2", "z=3"
 *
 * TODO: Add protocol extraction, port extraction, and some malformity checking.
 *
 * \param uri The uri to parse.
 * \param urilen The length of the uri to parse.
 *
 * \return 1 on success, 0 on failure.
 */
int CoAPMessage::setURI(char *uri, int urilen) {
  // only '/', '?', '&' and ascii chars allowed

  // sanitation
  if(urilen<=0||uri==nullptr) {
    coap_log.concat("Null or zero-length uri passed.\n");
    Kernel::log(&coap_log);
    return 1;
  }

  // single character URI path (including '/' case)
  if(urilen==1) {
    addOption(COAP_OPTION_URI_PATH,1,(uint8_t*)uri);
    return 0;
  }

  // TODO, queries
  // extract ? to mark where to stop processing path components
  // and then process the query params

  // local vars
  char *startP=uri,*endP=nullptr;
  int oLen = 0;
  char splitChar = '/';
  int queryStageTriggered = 0;
  uint16_t optionType = COAP_OPTION_URI_PATH;
  while(1) {
    // stop at end of string or query
    if(*startP==0x00||*(startP+1)==0x00) {
      break;
    }

    // ignore leading slash
    if(*startP==splitChar) {
      coap_log.concat("Skipping leading slash\n");
      Kernel::log(&coap_log);
      startP++;
    }

    // find next split point
    endP = strchr(startP,splitChar);

    // might not be another slash
    if(endP==nullptr) {
      coap_log.concat("Ending out of slash\n");
      // check if there is a ?
      endP = strchr(startP,'?');
      // done if no queries
      if(endP==nullptr) {
        endP = uri+urilen;
      }
      else {
        queryStageTriggered = 1;
      }
    }

    // get length of segment
    oLen = endP-startP;

    #ifdef DEBUG
    char *b = (char*)malloc(oLen+1);
    memcpy(b,startP,oLen);
    b[oLen] = 0x00;
    coap_log.concatf("Adding URI_PATH %s\n",b);
    Kernel::log(&coap_log);
    free(b);
    #endif

    // add option
    if(addOption(optionType,oLen,(uint8_t*)startP)!=0) {
      coap_log.concat("Error adding option\n");
      Kernel::log(&coap_log);
      return 1;
    }
    startP = endP;

    if(queryStageTriggered) {
      splitChar = '&';
      optionType = COAP_OPTION_URI_QUERY;
      startP++;
      queryStageTriggered = false;
    }
  }

  return 0;
}

/// Concatenates any URI_PATH elements and URI_QUERY elements into a single string.
/**
 * Parses the PDU options and extracts all URI_PATH and URI_QUERY elements,
 * concatenating them into a single string with slash and amphersand separators accordingly.
 *
 * The produced string will be NULL terminated.
 *
 * \param dst Buffer into which to copy the concatenated path elements.
 * \param dstlen Length of buffer.
 * \param outLen Pointer to integer, into which URI length will be placed.
 *
 * \return 0 on success, 1 on failure. \b outLen will contain the length of the concatenated elements.
 */
int CoAPMessage::getURI(char *dst, int dstlen, int *outLen) {
  if(outLen==nullptr) {
    coap_log.concat("Output length pointer is NULL\n");
    Kernel::log(&coap_log);
    return 1;
  }

  if(dst==nullptr) {
    coap_log.concat("NULL destination buffer\n");
    Kernel::log(&coap_log);
    *outLen = 0;
    return 1;
  }

  // check destination space
  if(dstlen<=0) {
    *dst = 0x00;
    *outLen = 0;
    coap_log.concat("Destination buffer too small (0)!\n");
    Kernel::log(&coap_log);
    return 1;
  }
  // check option count
  if(_numOptions==0) {
    *dst = 0x00;
    *outLen = 0;
    return 0;
  }
  // get options
  CoAPMessage::CoapOption *options = getOptions();
  if(options==nullptr) {
    *dst = 0x00;
    *outLen = 0;
    return 0;
  }
  // iterate over options to construct URI
  CoapOption *o = nullptr;
  int bytesLeft = dstlen-1; // space for 0x00
  int oLen = 0;
  // add slash at beggining
  if(bytesLeft>=1) {
    *dst = '/';
    dst++;
    bytesLeft--;
  }
  else {
    coap_log.concatf("No space for initial slash needed 1, got %d\n",bytesLeft);
    Kernel::log(&coap_log);
    free(options);
    return 1;
  }

  char separator = '/';
  int firstQuery = 1;

  for(int i=0; i<_numOptions; i++) {
    o = &options[i];
    oLen = o->optionValueLength;
    if(o->optionNumber==COAP_OPTION_URI_PATH||o->optionNumber==COAP_OPTION_URI_QUERY) {
      // if the option is a query, change the separator to &
      if(o->optionNumber==COAP_OPTION_URI_QUERY) {
        if(firstQuery) {
          // change previous '/' to a '?'
          *(dst-1) = '?';
          firstQuery = 0;
        }
        separator = '&';
      }

      // check space
      if(oLen>bytesLeft) {
        coap_log.concatf("Destination buffer too small, needed %d, got %d\n",oLen,bytesLeft);
        Kernel::log(&coap_log);
        free(options);
        return 1;
      }

      // case where single '/' exists
      if(oLen==1&&o->optionValuePointer[0]=='/') {
        *dst = 0x00;
        *outLen = 1;
        free(options);
        return 0;
      }

      // copy URI path or query component
      memcpy(dst,o->optionValuePointer,oLen);

      // adjust counters
      dst += oLen;
      bytesLeft -= oLen;

      // add separator following (don't know at this point if another option is coming)
      if(bytesLeft>=1) {
        *dst = separator;
        dst++;
        bytesLeft--;
      }
      else {
        coap_log.concat("Ran out of space after processing option\n");
        Kernel::log(&coap_log);
        free(options);
        return 1;
      }
    }
  }

  // remove terminating separator
  dst--;
  bytesLeft++;
  // add null terminating byte (always space since reserved)
  *dst = 0x00;
  *outLen = (dstlen-1)-bytesLeft;
  free(options);
  return 0;
}

/// Sets the CoAP version.
/**
 * \param version CoAP version between 0 and 3.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setVersion(uint8_t version) {
  if(version>3) {
    return 0;
  }

  _pdu[0] &= 0x3F;
  _pdu[0] |= (version << 6);
  return 1;
}

/**
 * Gets the CoAP Version.
 * @return The CoAP version between 0 and 3.
 */
uint8_t CoAPMessage::getVersion() {  return (_pdu[0]&0xC0)>>6; }

/**
 * Sets the type of this CoAP PDU.
 * \param mt The type, one of:
 * - COAP_CONFIRMABLE
 * - COAP_NON_CONFIRMABLE
 * - COAP_ACKNOWLEDGEMENT
 * - COAP_RESET.
 */
void CoAPMessage::setType(CoAPMessage::Type mt) {
  _pdu[0] &= 0xCF;
  _pdu[0] |= mt;
}

/// Returns the type of the PDU.
CoAPMessage::Type CoAPMessage::getType() {
  return (CoAPMessage::Type)(_pdu[0]&0x30);
}


/// Set the token length.
/**
 * \param tokenLength The length of the token in bytes, between 0 and 8.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setTokenLength(uint8_t tokenLength) {
  if(tokenLength>8)
    return 1;

  _pdu[0] &= 0xF0;
  _pdu[0] |= tokenLength;
  return 0;
}

/// Returns the token length.
int CoAPMessage::getTokenLength() {
  return _pdu[0] & 0x0F;
}

/// Returns a pointer to the PDU token.
uint8_t* CoAPMessage::getTokenPointer() {
  if(getTokenLength()==0) {
    return nullptr;
  }
  return &_pdu[4];
}

/// Set the PDU token to the supplied byte sequence.
/**
 * This sets the PDU token to \b token and sets the token length to \b tokenLength.
 * \param token A sequence of bytes representing the token.
 * \param tokenLength The length of the byte sequence.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setToken(uint8_t *token, uint8_t tokenLength) {
  coap_log.concat("Setting token\n");
  Kernel::log(&coap_log);
  if(token==nullptr) {
    coap_log.concat("NULL pointer passed as token reference\n");
    Kernel::log(&coap_log);
    return 1;
  }

  if(tokenLength==0) {
    coap_log.concat("Token has zero length\n");
    Kernel::log(&coap_log);
    return 1;
  }

  // if tokenLength has not changed, just copy the new value
  uint8_t oldTokenLength = getTokenLength();
  if(tokenLength==oldTokenLength) {
    memcpy((void*)&_pdu[4],token,tokenLength);
    return 0;
  }

  // otherwise compute new length of PDU
  uint8_t oldPDULength = _pduLength;
  _pduLength -= oldTokenLength;
  _pduLength += tokenLength;

  // now, have to shift old memory around, but shift direction depends
  // whether pdu is now bigger or smaller
  if(_pduLength>oldPDULength) {
    // new PDU is bigger, need to allocate space for new PDU
    if(!_constructedFromBuffer) {
      uint8_t *newMemory = (uint8_t*)realloc(_pdu,_pduLength);
      if(newMemory==nullptr) {
        // malloc failed
        coap_log.concat("Failed to allocate memory for token\n");
        Kernel::log(&coap_log);
        _pduLength = oldPDULength;
        return 1;
      }
      _pdu = newMemory;
      _bufferLength = _pduLength;
    }
    else {
      // constructed from buffer, check space
      if(_pduLength>_bufferLength) {
        coap_log.concatf("Buffer too small to contain token, needed %d, got %d.\n",_pduLength-oldPDULength,_bufferLength-oldPDULength);
        Kernel::log(&coap_log);
        _pduLength = oldPDULength;
        return 1;
      }
    }

    // and then shift everything after token up to end of new PDU
    // memory overlaps so do this manually so to avoid additional mallocs
    int shiftOffset = _pduLength-oldPDULength;
    int shiftAmount = _pduLength-tokenLength-COAP_HDR_SIZE; // everything after token
    shiftPDUUp(shiftOffset,shiftAmount);

    // now copy the token into the new space and set official token length
    memcpy((void*)&_pdu[4],token,tokenLength);
    setTokenLength(tokenLength);

    // and return success
    return 0;
  }

  // new PDU is smaller, copy the new token value over the old one
  memcpy((void*)&_pdu[4],token,tokenLength);
  // and shift everything after the new token down
  int startLocation = COAP_HDR_SIZE+tokenLength;
  int shiftOffset = oldPDULength-_pduLength;
  int shiftAmount = oldPDULength-oldTokenLength-COAP_HDR_SIZE;
  shiftPDUDown(startLocation,shiftOffset,shiftAmount);
  // then reduce size of buffer
  if(!_constructedFromBuffer) {
    uint8_t *newMemory = (uint8_t*)realloc(_pdu,_pduLength);
    if(newMemory==nullptr) {
      // malloc failed, PDU in inconsistent state
      coap_log.concat("Failed to shrink PDU for new token. PDU probably broken\n");
      Kernel::log(&coap_log);
      return 1;
    }
    _pdu = newMemory;
    _bufferLength = _pduLength;
  }

  // and officially set the new tokenLength
  setTokenLength(tokenLength);
  return 0;
}

/// Sets the CoAP response code
void CoAPMessage::setCode(CoAPMessage::Code code) {
  _pdu[1] = code;
  // there is a limited set of response codes
}

/// Gets the CoAP response code
CoAPMessage::Code CoAPMessage::getCode() {
  return (CoAPMessage::Code)_pdu[1];
}


/// Set messageID to the supplied value.
/**
 * \param messageID A 16bit message id.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setMessageID(uint16_t messageID) {
  // message ID is stored in network byte order
  uint8_t *to = &_pdu[2];
  endian_store16(to, messageID);
  return 0;
}

/// Returns the 16 bit message ID of the PDU.
uint16_t CoAPMessage::getMessageID() {
  // mesasge ID is stored in network byteorder
  uint8_t *from = &_pdu[2];
  uint16_t messageID = endian_load16(uint16_t, from);
  return messageID;
}


/**
 * This returns the options as a sequence of structs.
 */
CoAPMessage::CoapOption* CoAPMessage::getOptions() {
  coap_log.concatf("getOptions() called, %d options.\n",_numOptions);
  Kernel::log(&coap_log);

  uint16_t optionDelta =0, optionNumber = 0, optionValueLength = 0;
  int totalLength = 0;

  if(_numOptions==0) {
    return nullptr;
  }

  // malloc space for options
  CoapOption *options = (CoapOption*)malloc(_numOptions*sizeof(CoapOption));
  if(nullptr == options) {
    coap_log.concat("Failed to allocate memory for options.\n");
    Kernel::log(&coap_log);
    return nullptr;
  }

  // first option occurs after token
  int optionPos = COAP_HDR_SIZE + getTokenLength();

  // walk over options and record information
  for(int i=0; i<_numOptions; i++) {
    // extract option details
    optionDelta = getOptionDelta(&_pdu[optionPos]);
    optionNumber += optionDelta;
    optionValueLength = getOptionValueLength(&_pdu[optionPos]);
    // compute total length
    totalLength = 1; // mandatory header
    totalLength += computeExtraBytes(optionDelta);
    totalLength += computeExtraBytes(optionValueLength);
    totalLength += optionValueLength;
    // record option details
    options[i].optionNumber = optionNumber;
    options[i].optionDelta = optionDelta;
    options[i].optionValueLength = optionValueLength;
    options[i].totalLength = totalLength;
    options[i].optionPointer = &_pdu[optionPos];
    options[i].optionValuePointer = &_pdu[optionPos+totalLength-optionValueLength];
    // move to next option
    optionPos += totalLength;
  }

  return options;
}

/// Add an option to the PDU.
/**
 * Unlike other implementations, options can be added in any order, and in-memory manipulation will be
 * performed to ensure the correct ordering of options (they use a delta encoding of option numbers).
 * Re-ordering memory like this incurs a small performance cost, so if you care about this, then you
 * might want to add options in ascending order of option number.
 * \param optionNumber The number of the option, see the enum CoAPMessage::Option for shorthand notations.
 * \param optionLength The length of the option payload in bytes.
 * \param optionValue A pointer to the byte sequence that is the option payload (bytes will be copied).
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::addOption(uint16_t insertedOptionNumber, uint16_t optionValueLength, uint8_t *optionValue) {
  // this inserts the option in memory, and re-computes the deltas accordingly
  // prevOption <-- insertionPosition
  // nextOption

  // find insertion location and previous option number
  uint16_t prevOptionNumber = 0; // option number of option before insertion point
  int insertionPosition = findInsertionPosition(insertedOptionNumber,&prevOptionNumber);
  coap_log.concatf("inserting option at position %d, after option with number: %hu\n",insertionPosition,prevOptionNumber);
  Kernel::log(&coap_log);

  // compute option delta length
  uint16_t optionDelta = insertedOptionNumber-prevOptionNumber;
  uint8_t extraDeltaBytes = computeExtraBytes(optionDelta);

  // compute option length length
  uint16_t extraLengthBytes = computeExtraBytes(optionValueLength);

  // compute total length of option
  uint16_t optionLength = COAP_OPTION_HDR_BYTE + extraDeltaBytes + extraLengthBytes + optionValueLength;

  // if this is at the end of the PDU, job is done, just malloc and insert
  if(insertionPosition==_pduLength) {
    coap_log.concat("Inserting at end of PDU\n");
    Kernel::log(&coap_log);

    // optionNumber must be biggest added
    _maxAddedOptionNumber = insertedOptionNumber;

    // set new PDU length and allocate space for extra option
    int oldPDULength = _pduLength;
    _pduLength += optionLength;
    if(!_constructedFromBuffer) {
      uint8_t *newMemory = (uint8_t*)realloc(_pdu,_pduLength);
      if(newMemory==nullptr) {
        coap_log.concat("Failed to allocate memory for option.\n");
        Kernel::log(&coap_log);
        _pduLength = oldPDULength;
        // malloc failed
        return 1;
      }
      _pdu = newMemory;
      _bufferLength = _pduLength;
    }
    else {
      // constructed from buffer, check space
      if(_pduLength>_bufferLength) {
        coap_log.concatf("Buffer too small for new option: needed %d, got %d.\n",_pduLength-oldPDULength,_bufferLength-oldPDULength);
        Kernel::log(&coap_log);
        _pduLength = oldPDULength;
        return 1;
      }
    }

    // insert option at position
    insertOption(insertionPosition,optionDelta,optionValueLength,optionValue);
    _numOptions++;
    return 0;
  }
  // XXX could do 0xFF pdu payload case for changing of dynamically allocated application space SDUs < yeah, if you're insane

  // the next option might (probably) needs it's delta changing
  // I want to take this into account when allocating space for the new
  // option, to avoid having to do two mallocs, first get info about this option
  int nextOptionDelta = getOptionDelta(&_pdu[insertionPosition]);
  int nextOptionNumber = prevOptionNumber + nextOptionDelta;
  int nextOptionDeltaBytes = computeExtraBytes(nextOptionDelta);
  // recompute option delta, relative to inserted option
  int newNextOptionDelta = nextOptionNumber-insertedOptionNumber;
  int newNextOptionDeltaBytes = computeExtraBytes(newNextOptionDelta);
  // determine adjustment
  int optionDeltaAdjustment = newNextOptionDeltaBytes-nextOptionDeltaBytes;

  // create space for new option, including adjustment space for option delta
  //DBG_PDU();
  coap_log.concat("Creating space\n");
  int mallocLength = optionLength+optionDeltaAdjustment;
  int oldPDULength = _pduLength;
  _pduLength += mallocLength;

  if(!_constructedFromBuffer) {
    uint8_t *newMemory = (uint8_t*)realloc(_pdu,_pduLength);
    if(newMemory==nullptr) {
      coap_log.concat("Failed to allocate memory for option\n");
      Kernel::log(&coap_log);
      _pduLength = oldPDULength;
      return 1;
    }
    _pdu = newMemory;
    _bufferLength = _pduLength;
  } else {
    // constructed from buffer, check space
    if(_pduLength>_bufferLength) {
      coap_log.concatf("Buffer too small to contain option, needed %d, got %d.\n",_pduLength-oldPDULength,_bufferLength-oldPDULength);
      Kernel::log(&coap_log);
      _pduLength = oldPDULength;
      return 1;
    }
  }

  // move remainder of PDU data up to create hole for new option
  //DBG_PDU();
  coap_log.concat("Shifting PDU.\n");
  shiftPDUUp(mallocLength,_pduLength-(insertionPosition+mallocLength));
  //DBG_PDU();

  // adjust option delta bytes of following option
  // move the option header to the correct position
  int nextHeaderPos = insertionPosition+mallocLength;
  _pdu[nextHeaderPos-optionDeltaAdjustment] = _pdu[nextHeaderPos];
  nextHeaderPos -= optionDeltaAdjustment;
  // and set the new value
  setOptionDelta(nextHeaderPos, newNextOptionDelta);

  // new option shorter
  // p p n n x x x x x
  // p p n n x x x x x -
  // p p - n n x x x x x
  // p p - - n x x x x x
  // p p o o n x x x x x

  // new option longer
  // p p n n x x x x x
  // p p n n x x x x x - - -
  // p p - - - n n x x x x x
  // p p - - n n n x x x x x
  // p p o o n n n x x x x x

  // note, it can only ever be shorter or the same since if an option was inserted the delta got smaller
  // but I'll leave that little comment in, just to show that it would work even if the delta got bigger

  // now insert the new option into the gap
  coap_log.concat("Inserting new option... ");
  insertOption(insertionPosition,optionDelta,optionValueLength,optionValue);
  coap_log.concat("done\n");

  // done, mark it with B!
  Kernel::log(&coap_log);
  return 0;
}

/// Allocate space for a payload.
/**
 * For dynamically constructed PDUs, this will allocate space for a payload in the object
 * and return a pointer to it. If the PDU was constructed from a buffer, this doesn't
 * malloc anything, it just changes the _pduLength and returns the payload pointer.
 *
 * \note The pointer returned points into the PDU buffer.
 * \param len The length of the payload buffer to allocate.
 * \return Either a pointer to the payload buffer, or NULL if there wasn't enough space / allocation failed.
 */
uint8_t* CoAPMessage::mallocPayload(int len) {
  coap_log.concat("Entering mallocPayload\n");
  // sanity checks
  if(len==0) {
    coap_log.concat("Cannot allocate a zero length payload\n");
    Kernel::log(&coap_log);
    return nullptr;
  }

  // further sanity
  if(len==_payloadLength) {
    coap_log.concat("Space for payload of specified length already exists\n");
    if(_payloadPointer==nullptr) {
      coap_log.concatf("Garbage PDU. Payload length is %d, but existing _payloadPointer NULL",_payloadLength);
      Kernel::log(&coap_log);
      return nullptr;
    }
    return _payloadPointer;
  }

  coap_log.concatf("_bufferLength: %d, _pduLength: %d, _payloadLength: %d\n",_bufferLength,_pduLength,_payloadLength);

  // might be making payload bigger (including bigger than 0) or smaller
  int markerSpace = 1;
  int payloadSpace = len;
  // is this a resizing?
  if(_payloadLength!=0) {
    // marker already exists
    markerSpace = 0;
    // compute new payload length (can be negative if shrinking payload)
    payloadSpace = len-_payloadLength;
  }

  // make space for payload (and payload marker if necessary)
  int newLen = _pduLength+payloadSpace+markerSpace;
  if(!_constructedFromBuffer) {
    uint8_t* newPDU = (uint8_t*)realloc(_pdu,newLen);
    if(newPDU==nullptr) {
      coap_log.concat("Cannot allocate (or shrink) space for payload\n");
      Kernel::log(&coap_log);
      return nullptr;
    }
    _pdu = newPDU;
    _bufferLength = newLen;
  } else {
    // constructed from buffer, check space
    coap_log.concatf("newLen: %d, _bufferLength: %d\n",newLen,_bufferLength);
    if(newLen>_bufferLength) {
      coap_log.concatf("Buffer too small to contain desired payload, needed %d, got %d.\n",newLen-_pduLength,_bufferLength-_pduLength);
      Kernel::log(&coap_log);
      return nullptr;
    }
  }

  // deal with fresh allocation case separately
  if(_payloadPointer==nullptr) {
    // set payload marker
    _pdu[_pduLength] = 0xFF;
    // payload at end of old PDU
    _payloadPointer = &_pdu[_pduLength+1];
    _pduLength = newLen;
    _payloadLength = len;
    return _payloadPointer;
  }

  // otherwise, just adjust length of PDU
  _pduLength = newLen;
  _payloadLength = len;
  coap_log.concat("Leaving mallocPayload\n");
  Kernel::log(&coap_log);
  return _payloadPointer;
}

/// Set the payload to the byte sequence specified. Allocates memory in dynamic PDU if necessary.
/**
 * This will set the payload to \b payload. It will allocate memory in the case where the PDU was
 * constructed without an external buffer.
 *
 * This will fail either if the fixed buffer isn't big enough, or if memory could not be allocated
 * in the non-external-buffer case.
 *
 * \param payload Pointer to payload byte sequence.
 * \param len Length of payload byte sequence.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setPayload(uint8_t *payload, int len) {
  if(payload==nullptr) {
    coap_log.concat("NULL payload pointer.\n");
    Kernel::log(&coap_log);
    return 1;
  }

  uint8_t *payloadPointer = mallocPayload(len);
  if(payloadPointer==nullptr) {
    coap_log.concat("Allocation of payload failed\n");
    Kernel::log(&coap_log);
    return 1;
  }

  // copy payload contents
  memcpy(payloadPointer,payload,len);

  return 0;
}

/// Returns a pointer to a buffer which is a copy of the payload buffer (dynamically allocated).
uint8_t* CoAPMessage::getPayloadCopy() {
  if(_payloadLength==0) {
    return nullptr;
  }

  // malloc space for copy
  uint8_t *payload = (uint8_t*)malloc(_payloadLength);
  if(payload==nullptr) {
    coap_log.concat("Unable to allocate memory for payload\n");
    Kernel::log(&coap_log);
    return nullptr;
  }

  // copy and return
  memcpy(payload,_payloadPointer,_payloadLength);
  return payload;
}

/// Shorthand for setting the content-format option.
/**
 * Sets the content-format to the specified value (adds an option).
 * \param format The content format, one of:
 *
 * - COAP_CONTENT_FORMAT_TEXT_PLAIN
 * - COAP_CONTENT_FORMAT_APP_LINK
 * - COAP_CONTENT_FORMAT_APP_XML
 * - COAP_CONTENT_FORMAT_APP_OCTET
 * - COAP_CONTENT_FORMAT_APP_EXI
 * - COAP_CONTENT_FORMAT_APP_JSON
 *
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::setContentFormat(CoAPMessage::ContentFormat format) {
  if(format==0) {
    // minimal representation means null option value
    if(addOption(CoAPMessage::COAP_OPTION_CONTENT_FORMAT,0,nullptr)!=0) {
      coap_log.concat("Error setting content format\n");
      Kernel::log(&coap_log);
      return 1;
    }
    return 0;
  }

  uint8_t c[2];

  // just use 1 byte if can do it
  if(format<256) {
    c[0] = format;
    if(addOption(CoAPMessage::COAP_OPTION_CONTENT_FORMAT,1,c)!=0) {
      coap_log.concat("Error setting content format\n");
      Kernel::log(&coap_log);
      return 1;
    }
    return 0;
  }

  uint8_t *to = c;
  endian_store16(to, format);
  if(addOption(CoAPMessage::COAP_OPTION_CONTENT_FORMAT,2,c)!=0) {
    coap_log.concat("Error setting content format\n");
    Kernel::log(&coap_log);
    return 1;
  }
  return 0;
}

/// PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE
/// PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE

/// Moves a block of bytes to end of PDU from given offset.
/**
 * This moves the block of bytes _pdu[_pduLength-1-shiftOffset-shiftAmount] ... _pdu[_pduLength-1-shiftOffset]
 * to the end of the PDU.
 * \param shiftOffset End of block to move, relative to end of PDU (-1).
 * \param shiftAmount Length of block to move.
 */
void CoAPMessage::shiftPDUUp(int shiftOffset, int shiftAmount) {
  coap_log.concatf("shiftOffset: %d, shiftAmount: %d",shiftOffset,shiftAmount);
  int destPointer = _pduLength-1;
  int srcPointer  = destPointer-shiftOffset;
  while(shiftAmount--) {
    _pdu[destPointer] = _pdu[srcPointer];
    destPointer--;
    srcPointer--;
  }
  Kernel::log(&coap_log);
}

/// Moves a block of bytes down a specified number of steps.
/**
 * Moves the block of bytes _pdu[startLocation+shiftOffset] ... _pdu[startLocation+shiftOffset+shiftAmount]
 * down to \b startLocation.
 * \param startLocation Index where to shift the block to.
 * \param shiftOffset Where the block starts, relative to start index.
 * \param shiftAmount Length of block to shift.
 */
void CoAPMessage::shiftPDUDown(int startLocation, int shiftOffset, int shiftAmount) {
  coap_log.concatf("startLocation: %d, shiftOffset: %d, shiftAmount: %d\n",startLocation,shiftOffset,shiftAmount);
  int srcPointer = startLocation+shiftOffset;
  while(shiftAmount--) {
    _pdu[startLocation] = _pdu[srcPointer];
    startLocation++;
    srcPointer++;
  }
  Kernel::log(&coap_log);
}

/// Gets the payload length of an option.
/**
 * \param option Pointer to location of option in PDU.
 * \return The 16 bit option-payload length.
 */
uint16_t CoAPMessage::getOptionValueLength(uint8_t *option) {
  uint16_t delta = (option[0] & 0xF0) >> 4;
  uint16_t length = (option[0] & 0x0F);
  // no extra bytes
  if(length<13) {
    return length;
  }

  // extra bytes skip header
  int offset = 1;
  // skip extra option delta bytes
  if(delta==13) {
    offset++;
  }
  else if(delta==14) {
    offset+=2;
  }

  // process length
  if(length == 13) {
    return (option[offset]+13);
  }
  else {
    uint8_t *from = &option[offset];
    uint16_t value = endian_load16(uint16_t, from);
    return value+269;
  }
}


/// Gets the delta of an option.
/**
 * \param option Pointer to location of option in PDU.
 * \return The 16 bit delta.
 */
uint16_t CoAPMessage::getOptionDelta(uint8_t *option) {
  uint16_t delta = (option[0] & 0xF0) >> 4;
  switch(delta) {
    case 13:    // single byte option delta
      return (option[1]+13);
    case 14:
      uint8_t *from = &option[1];
      uint16_t value = endian_load16(uint16_t, from);
      return value+269;
  }
  return delta;
}


/// Finds the insertion position in the current list of options for the specified option.
/**
 * \param optionNumber The option's number.
 * \param prevOptionNumber A pointer to a uint16_t which will store the option number of the option previous
 * to the insertion point.
 * \return 0 on success, 1 on failure. \b prevOptionNumber will contain the option number of the option
 * before the insertion position (for example 0 if no options have been inserted).
 */
int CoAPMessage::findInsertionPosition(uint16_t optionNumber, uint16_t *prevOptionNumber) {
  // zero this for safety
  *prevOptionNumber = 0x00;

  coap_log.concatf("_pduLength: %d\n",_pduLength);
  Kernel::log(&coap_log);

  // if option is bigger than any currently stored, it goes at the end
  // this includes the case that no option has yet been added
  if( (optionNumber >= _maxAddedOptionNumber) || (_pduLength == (COAP_HDR_SIZE+getTokenLength())) ) {
    *prevOptionNumber = _maxAddedOptionNumber;
    return _pduLength;
  }

  // otherwise walk over the options
  int optionPos = COAP_HDR_SIZE + getTokenLength();
  uint16_t optionDelta = 0, optionValueLength = 0;
  uint16_t currentOptionNumber = 0;
  while(optionPos<_pduLength && _pdu[optionPos]!=0xFF) {
    optionDelta = getOptionDelta(&_pdu[optionPos]);
    currentOptionNumber += optionDelta;
    optionValueLength = getOptionValueLength(&_pdu[optionPos]);
    // test if this is insertion position
    if(currentOptionNumber>optionNumber) {
      return optionPos;
    }
    // keep track of the last valid option number
    *prevOptionNumber = currentOptionNumber;
    // move onto next option
    optionPos += computeExtraBytes(optionDelta);
    optionPos += computeExtraBytes(optionValueLength);
    optionPos += optionValueLength;
    optionPos++; // (for mandatory option header byte)
  }
  return optionPos;

}

/// CoAP uses a minimal-byte representation for length fields. This returns the number of bytes needed to represent a given length.
int CoAPMessage::computeExtraBytes(uint16_t n) {
  if(n<269) {
    return (n<13) ? 0 : 1;
  }
  return 2;
}

/// Set the option delta to the specified value.
/**
 * This assumes space has been made for the option delta.
 * \param optionPosition The index of the option in the PDU.
 * \param optionDelta The option delta value to set.
 */
void CoAPMessage::setOptionDelta(int optionPosition, uint16_t optionDelta) {
  int headerStart = optionPosition;
  // clear the old option delta bytes
  _pdu[headerStart] &= 0x0F;

  // set the option delta bytes
  if(optionDelta<13) {
    _pdu[headerStart] |= (optionDelta << 4);
  }
  else if(optionDelta<269) {
     // 1 extra byte
    _pdu[headerStart] |= 0xD0; // 13 in first nibble
    _pdu[++optionPosition] &= 0x00;
    _pdu[optionPosition] |= (optionDelta-13);
  }
  else {
    // 2 extra bytes, network byte order uint16_t
    _pdu[headerStart] |= 0xE0; // 14 in first nibble
    optionDelta -= 269;
    uint8_t *to = &_pdu[optionPosition];
    optionPosition += 2;
    endian_store16(to, optionDelta);
  }
}

/// Insert an option in-memory at the specified location.
/**
 * This assumes that there is enough space at the location specified.
 * \param insertionPosition Position in the PDU where the option should be placed.
 * \param optionDelta The delta value for the option.
 * \param optionValueLength The length of the option value.
 * \param optionValue A pointer to the sequence of bytes representing the option value.
 * \return 0 on success, 1 on failure.
 */
int CoAPMessage::insertOption(
  int insertionPosition,
  uint16_t optionDelta,
  uint16_t optionValueLength,
  uint8_t *optionValue) {

  int headerStart = insertionPosition;

  // clear old option header start
  _pdu[headerStart] &= 0x00;

  // set the option delta bytes
  if(optionDelta<13) {
    _pdu[headerStart] |= (optionDelta << 4);
  } else if(optionDelta<269) {
     // 1 extra byte
    _pdu[headerStart] |= 0xD0; // 13 in first nibble
    _pdu[++insertionPosition] &= 0x00;
    _pdu[insertionPosition] |= (optionDelta-13);
  } else {
    // 2 extra bytes, network byte order uint16_t
    _pdu[headerStart] |= 0xE0; // 14 in first nibble
    optionDelta -= 269;
    uint8_t *to = &_pdu[insertionPosition];
    insertionPosition += 2;
    endian_store16(to, optionDelta);
  }

  // set the option value length bytes
  if(optionValueLength<13) {
    _pdu[headerStart] |= (optionValueLength & 0x000F);
  }
  else if(optionValueLength<269) {
    _pdu[headerStart] |= 0x0D; // 13 in second nibble
    _pdu[++insertionPosition] &= 0x00;
    _pdu[insertionPosition] |= (optionValueLength-13);
  }
  else {
    _pdu[headerStart] |= 0x0E; // 14 in second nibble
    // this is in network byte order
    coap_log.concatf("optionValueLength: %u\n",optionValueLength);
    uint8_t *to = &_pdu[insertionPosition];
    optionValueLength -= 269;
    endian_store16(to, optionValueLength);
    insertionPosition += 2;
  }

  // and finally copy the option value itself
  memcpy(&_pdu[++insertionPosition],optionValue,optionValueLength);
  Kernel::log(&coap_log);
  return 0;
}

// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

/// A routine for printing an option in human-readable format.
/**
 * \param option This is a pointer to where the option begins in the PDU.
 */
void CoAPMessage::printOptionHuman(uint8_t *option) {
  // compute some useful stuff
  uint16_t optionDelta = getOptionDelta(option);
  uint16_t optionValueLength = getOptionValueLength(option);
  int extraDeltaBytes = computeExtraBytes(optionDelta);
  int extraValueLengthBytes = computeExtraBytes(optionValueLength);
  int totalLength = 1+extraDeltaBytes+extraValueLengthBytes+optionValueLength;

  if(totalLength>_pduLength) {
    totalLength = &_pdu[_pduLength-1]-option;
    coap_log.concatf("New length: %u\n",totalLength);
  }

  // print summary
  coap_log.concat("~~~~~~ Option ~~~~~~\n");
  coap_log.concatf("Delta: %u, Value length: %u\n",optionDelta,optionValueLength);

  // print all bytes
  coap_log.concatf("All bytes (%d):  ",totalLength);
  for(int i=0; i<totalLength; i++) {
    if(i%4==0) {
      coap_log.concatf("   %.2d ",i);
    }
		coap_log.concatf("0x%02x ", option[i]);
  }

  // print header byte
  coap_log.concat("Header byte:  ");
	coap_log.concatf("0x%02x ", *option++);
	coap_log.concat("\n");

  // print extended delta bytes
  if(extraDeltaBytes) {
    coap_log.concatf("Extended delta bytes (%d) in network order:  ",extraDeltaBytes);
    while(extraDeltaBytes--) {
			coap_log.concatf("0x%02x ", *option++);
    }
  }
  else {
    coap_log.concat("No extended delta bytes\n");
  }

  // print extended value length bytes
  if(extraValueLengthBytes) {
    coap_log.concatf("Extended value length bytes (%d) in network order: ",extraValueLengthBytes);
    while(extraValueLengthBytes--) {
			coap_log.concatf("0x%02x ", *option++);
    }
  }
  else {
    coap_log.concat("No extended value length bytes\n");
  }
  coap_log.concat("\n");

  // print option value
  coap_log.concat("Option value bytes:  ");
  for(int i=0; i<optionValueLength; i++) {
    if(i%4==0) {
      coap_log.concatf("   %.2d ",i);
    }
		coap_log.concatf("0x%02x ", *option++);
  }
  coap_log.concat("\n");
  Kernel::log(&coap_log);
}


/**
* This method is called to flatten this message (and its Event) into a string
*   so that the session can provide it to the transport.
*
* @return  The total size of the string that is meant for the transport,
*            or -1 if something went wrong.
*/
int CoAPMessage::serialize(StringBuilder* buffer) {
  return 0;
}


/**
* This function should be called by the session to feed bytes to a message.
*
* @return  The number of bytes consumed, or a negative value on failure.
*/
int CoAPMessage::accumulate(unsigned char* _buf, int _len) {
  return 0;
}

/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void CoAPMessage::printDebug(StringBuilder *output) {
  XenoMessage::printDebug(output);
  if(_constructedFromBuffer) {
    output->concatf("\t PDU was constructed from buffer of %d bytes\n",_bufferLength);
  }
	output->concatf("\t CoAP v%d code:   %s\n", getVersion(), codeToString(getCode()));
	output->concatf("\t Message ID:     %u\n", getMessageID());
	output->concatf("\t Message Type:   %s\n", typeToString(getType()));
	output->concatf("\t PDU (%d bytes): ", _pduLength);
  for (int i = 0; i < _pduLength; i++) output->concatf("0x%02x ", _pdu[i]);
	output->concat("\n");

  int tokenLength = getTokenLength();
	if (0 < tokenLength) {
		uint8_t *tokenPointer = getPDUPointer()+COAP_HDR_SIZE;
  	output->concatf("\n\t Token length:   %d\t", tokenLength);
		for (int i = 0; i < tokenLength; i++) output->concatf("0x%02x ", tokenPointer[i]);
		output->concat("\n");
	}

  // print options
  CoAPMessage::CoapOption* options = getOptions();
  if(nullptr != options) {
		output->concat("\t Options:\n");
  	for(int i = 0; i < _numOptions; i++) {
    	output->concatf("OPTION (%d/%d)\n",i + 1,_numOptions);
    	output->concatf("   Option number (delta): %hu (%hu)\n",options[i].optionNumber,options[i].optionDelta);
    	output->concatf("   Name: %s\n", optionNumToString(options[i].optionNumber));
    	output->concatf("   Value length: %u\n",options[i].optionValueLength);
    	output->concat("   Value: \"");
    	for(int j = 0; j < options[i].optionValueLength; j++) {
      	char c = options[i].optionValuePointer[j];
      	if((c>='!'&&c<='~')||c==' ') {
        	output->concatf("%c",c);
      	}
      	else {
        	output->concatf("\\%.2d",c);
      	}
    	}
    	output->concat("\"\n");
  	}
		free(options);
  }

  // print payload
  if(0 < _payloadLength) {
    output->concatf("\t Payload (%d bytes)\t\"",_payloadLength);
		for(int i = 0; i < _payloadLength; i++) {
      char c = _payloadPointer[i];
      if((c>='!'&&c<='~')||c==' ') {
        output->concatf("%c",c);
      }
      else {
        output->concatf("\\%.2x",c);
      }
    }
    output->concat("\"\n");
  }
}
