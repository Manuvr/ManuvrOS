/*
File:   BufferPipe.h
Author: J. Ian Lindsay
Date:   2016.06.29

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


This class is meant to be the glue between a platform-abstracted transport,
  and any parser/session/presentation layer that sits on top of it (if any).

Extending classes may be...
 ManuvrXports, which use it to move buffers across the I/O layer.
 XenoSessions, which use it to move buffers across the application layer.
 XportXformers, which optionally connects these things together.

The idea is have a strategy flexible-enough to facillitate basic
 buffer-processing tasks without incurring the complexity of extending
 XenoSession or wrapping ManuvrXport.

For instance: A simple parser that sits on a serial port or net socket, and
  does not require a session. This would encompass GPS, and also any transport
  bridges.
See ManuvrGPS for the GPS parser case.
See XportBridge for a bi-directional implementation.

Another (more complex) case would be TLS. Two-way interchange with the transport
  happens independently of the attached Session (if any). This also allows for
  the easy selection and abstraction of TLS/DTLS from the underlying transport,
  allowing the choice to be made based on the nature of the transport.
See ManuvrTLS.

A third use-case would be a traffic-analysis class that looks at connections
  originating in a listening transport and chooses what flavor of session
  is most-appropriate for servicing it. Thereafter performing the necessary
  association and distancing itself from further interaction.
See ZooKeeper.

Meaningful distinctions between datagram and stream-oriented transports ought
  to end here, with all downstream software experiencing the transport as a
  buffered stream, with the divisions between messages (if any) left to the
  classes that care.
See UDPPipe for a case where this matters.
*/


#ifndef __MANUVR_BUFFER_PIPE_H__
#define __MANUVR_BUFFER_PIPE_H__

#include <stdlib.h>
#include <DataStructures/StringBuilder.h>

/*
* These are return codes and directives for keeping track of where the
*   responsibility lies for cleaning up memory for buffers that are passed.
* As return codes, these are interpreted as a declaration of the beliefs of the
*   callee instance. Therefore, a return value of zero is an error.
* As function arguments, these are interpreted as directives given by the
*   sending instance, and should be treated appropriately.
* At this point, we cannot unambiguously support partial buffer transfers. The
*   receiving instance should take everything or nothing at all.
* Remember: The issuer of the buffer has sole-authority to dictate its fate.
*   This implies that we can safely copy and transform buffers in this class if
*   we feel the need. But we need to make copies under certain circumstances.
*/
#define MEM_MGMT_RESPONSIBLE_ERROR      -2  // Only used as a return code.
#define MEM_MGMT_RESPONSIBLE_CALLER     -1  // The passive instance refused the buffer.
#define MEM_MGMT_RESPONSIBLE_UNKNOWN     0  // TODO: God save you.
#define MEM_MGMT_RESPONSIBLE_CREATOR     1  // The allocator is responsible.
#define MEM_MGMT_RESPONSIBLE_BEARER      2  // The bearer is responsible.

/*
* Here, "far" refers to "farther from the counterparty". That is: closer to our
*   local idea of "the application".
*
* _near should _always_ be closer to the transport driver, if present.
* _far should _always_ be closer to the application-layer end-point, if present.
*
* The _near and _far members cannot be relied upon to be present for a variety
*   of reasons. But calling those methods should always be made to be safe.
*
* TODO: Notes for the future:
* BufferPipe could be implemented as a bi-directional FIFO queue. But it isn't.
* This is a good place to wrap various IPC strategies.
* This is an *excellent* place for concurrency-control and memory-management.
*/
class BufferPipe {
  public:
    /*
    * Generally, this will be called from within the instance pointed-at by _far.
    */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm) =0;
    inline int8_t toCounterparty(uint8_t* buf, unsigned int len) {
      return toCounterparty(buf, len, _near_mm_default);
    };

    /*
    * Generally, this will be called from within the instance pointed-at by _near.
    */
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm) =0;
    inline int8_t fromCounterparty(uint8_t* buf, unsigned int len) {
      return fromCounterparty(buf, len, _far_mm_default);
    };

    /* This is here to simplify the logic of endpoints. */
    int8_t emitBuffer(uint8_t* buf, unsigned int len, int8_t mm);
    inline int8_t emitBuffer(uint8_t* buf, unsigned int len) {
      return emitBuffer(buf, len, (this == _near) ? _near_mm_default : _far_mm_default);
    };

    /* Set the identity of the near-side. */
    int8_t setNear(BufferPipe* nu, int8_t _mm);
    inline int8_t setNear(BufferPipe* nu) {
      return setNear(nu, MEM_MGMT_RESPONSIBLE_UNKNOWN);
    };

    /* Set the identity of the far-side. */
    int8_t setFar(BufferPipe* nu, int8_t _mm);
    inline int8_t setFar(BufferPipe* nu) {
      return setFar(nu, MEM_MGMT_RESPONSIBLE_UNKNOWN);
    };

    static const char* memMgmtString(int8_t);


  protected:
    BufferPipe* _near;  // These two members create a double-linked-list.
    BufferPipe* _far;   // Need such topology for bi-directional pipe.

    /* Default mem-mgmt responsibilities... */
    int8_t _near_mm_default;   // ...for buffers originating from the near side.
    int8_t _far_mm_default;    // ...for buffers originating from the far side.

    BufferPipe();           // Protected constructor with no params for class init.
    virtual ~BufferPipe();  // Protected destructor.

    /* Notification of destruction. Needed to maintain chain integrity. */
    void _bp_notify_drop(BufferPipe*);

    /* Simple checks that we will need to do. */
    inline bool haveNear() {  return (NULL != _near);  };
    inline bool haveFar() {   return (NULL != _far);   };


  private:
};

#endif   // __MANUVR_BUFFER_PIPE_H__
