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

// Our notion of buffer.
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
* Flag definitions.
*/
#define BPIPE_FLAG_WE_ALLOCD_NEAR   0x0001  // We are responsible for near-side teardown.
#define BPIPE_FLAG_WE_ALLOCD_FAR    0x0002  // We are responsible for far-side teardown.
#define BPIPE_FLAG_IS_TERMINUS      0x0004  // This pipe is an endpoint.
#define BPIPE_FLAG_PIPE_LOCKED      0x1000  // The pipe is locked.
#define BPIPE_FLAG_PIPE_PACKETIZED  0x4000  // The pipe's issuances coincide with packet boundaries.
#define BPIPE_FLAG_IS_BUFFERED      0x8000  // This pipe has the capability to buffer.

/*
* Definition of signals passable via the pipe's control channel.
* These should be kept as general as possible, and have no direct relationship
*   to POSIX signals.
* For one, POSIX signals are async, and there are ALWAYS synchronous.
* For another, they are not used for IPC. The primary function is to signal
*   setup, tear-down, and a handful of transport ideas.
*/
enum class ManuvrPipeSignal {
  UNDEF = 0,           // Undefined.
  FLUSH,               // Flush the pipe.
  FLUSHED,             // The pipe is flushed.
  FAR_SIDE_DETACH,     // Far-side informing the near-side of its departure.
  NEAR_SIDE_DETACH,    // Near-side informing the far-side of its departure.
  FAR_SIDE_ATTACH,     // Far-side informing the near-side of its arrival.
  NEAR_SIDE_ATTACH,    // Near-side informing the far-side of its arrival.

  // These are signals for transport-compat. They might be generalized.
  XPORT_CONNECT,       // connect()/connected()
  XPORT_DISCONNECT     // disconnect()/disconnected()
};

/*
* Notes regarding pipe-strategies...
* This is an experiment for testing an idea about building dynamic chains of
*   pipes.
* Something like this is needed to avoid having to code for a small, static set
*   of pipe combinations (IE: "Bluetooth <--> DTLS <--> Console").
*   It might be a good idea to have it in BufferPipe since this is where the
*   allocation would occur, and the datastructure nicely fits the problem.
* On the other-hand, it stands to cause us some linguistic grief of necessitating
*   this class containing knowledge of higher-layer components (thus hurting its
*   purity as a data structure).
* If the transport is always to be the progenitor of the chain, it might be better
*   to contain the definitional aspects of this idea in ManuvrXport (or the Kernel).
* Same problem as we have with Message definitions.
*
* If it remains here, the pipe-strategy should be executed on first-call to haveFar()
*   that would otherwise return false.
*/

// Hopefully, this will be enough until we think of something smarter.
#define MAXIMUM_PIPE_DIVERSITY  16

class BufferPipe;

// TODO: Might-should pass arguments as well?
typedef BufferPipe* (*bpFactory) (BufferPipe*, BufferPipe*);

/*
* TODO: This structure might evolve into ZooInmate.
* This structure tracks the types of pipes this build supports. It is
*   needed for runtime instantiation of new pipes to service strategy in
*   the absense of RTTI.
*/
typedef struct {
  int         pipe_code;
  bpFactory   factory;
} PipeDef;


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
    * Sending signals through pipes...
    */
    virtual int8_t toCounterparty(ManuvrPipeSignal, void*);
    virtual int8_t fromCounterparty(ManuvrPipeSignal, void*);

    /*
    * Sending dynamically-alloc'd buffers through pipes...
    * This is the override point for extending classes.
    */
    virtual int8_t toCounterparty(StringBuilder*, int8_t mm);
    virtual int8_t fromCounterparty(StringBuilder*, int8_t mm);

    /*
    * Override to allow a pointer-length interface.
    */
    int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    /*
    * Set the identity of the near-side.
    * The side closer to the counterparty.
    */
    int8_t setNear(BufferPipe*);

    /*
    * Set the identity of the far-side.
    * The side closer to the application logic.
    */
    int8_t setFar(BufferPipe*);

    /* Join the ends of this pipe to one-another. */
    int8_t joinEnds();

    /* Members pertaining to pipe-strategy. */
    inline const uint8_t* getPipeStrategy()           {  return _pipe_strategy;   };
    inline void setPipeStrategy(const uint8_t* strat) {  _pipe_strategy = strat;  };
    inline uint8_t pipeCode() {  return _pipe_code;  };


    virtual const char* pipeName();

    // These inlines are for convenience of extending classes.
    // TODO: Ought to be private.
    inline bool _bp_flag(uint16_t flag) {        return (_flags & flag);  };
    inline void _bp_set_flag(uint16_t flag, bool nu) {
      if (nu) _flags |= flag;
      else    _flags &= ~flag;
    };


    /*
    * This is the list of all supported pipe types in the system. It is
    *   NULL-terminated.
    */
    static PipeDef _supported_strategies[];
    static int registerPipe(int, bpFactory);
    static BufferPipe* spawnPipe(int, BufferPipe*, BufferPipe*);

    /* Debug and logging support */
    static const char* memMgmtString(int8_t);
    static const char* signalString(ManuvrPipeSignal);



  protected:
    const uint8_t* _pipe_strategy;  // See notes.
    BufferPipe* _near;  // These two members create a double-linked-list.
    BufferPipe* _far;   // Need such topology for bi-directional pipe.

    BufferPipe();           // Protected constructor with no params for class init.
    virtual ~BufferPipe();  // Protected destructor.

    /* Simple checks that we will need to do. */
    inline bool haveNear() {  return (nullptr != _near);  };
    bool haveFar();

    virtual void printDebug(StringBuilder*);



  private:
    uint16_t _flags;
    uint8_t  _pipe_code;
};

#endif   // __MANUVR_BUFFER_PIPE_H__
