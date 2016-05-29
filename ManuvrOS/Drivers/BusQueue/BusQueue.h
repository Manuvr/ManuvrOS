/*
File:   BusQueue.h
Author: J. Ian Lindsay
Date:   2016.05.28

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


#ifndef __MANUVR_BUS_QUEUE_H__
#define __MANUVR_BUS_QUEUE_H__

#include <inttypes.h>

/*
* These are possible transfer states.
*/
enum class XferState {
  /* These are start states. */
  UNDEF,      // Freshly instanced (or wiped, if preallocated).
  IDLE,       // Bus op is allocated and waiting somewhere outside of the queue.

  /* These states are unstable and should decay into a "finish" state. */
  QUEUED,     // Bus op is idle and waiting for its turn. No bus control.
  INITIATE,   // Waiting for initiation phase.
  ADDR,       // Addressing phase. Sending the address.
  IO_WAIT,    // I/O operation in-progress.
  STOP,       // I/O operation in cleanup phase.

  /* These are finish states. */
  COMPLETE,   // I/O op complete with no problems.
  FAULT       // Fault condition.
};


/*
* These are the opcodes that we use to represent different types of messages to the RN.
*/
enum class BusOpcode {
  UNDEF,          // Freshly instanced (or wiped, if preallocated).
  RX,             // We are receiving without having asked for it.
  TX,             // Simple transmit. No waiting for a reply.
  TX_WAIT_RX,     // Send to the bus and capture the reply.
  TX_CMD,         // Send to the bus command register without expecting a reply.
  TX_CMD_WAIT_RX  // Send to the bus command register and capture a reply.
};



/*
* This class represents a single transaction on the bus, but is dewvoid of
*   implementation details. This is an interface class that should be extended
*   by classes that require hardware-level specificity.
*
* Since C++ has no formal means of declaring an interface, we will use Java's
*   conventions. That is, state-bearing members in this interface are ok, but
*   there should be no function members that are not pure virtuals or inlines.
*
* Note also, that a class is not required to inherrit from this ddefinition of
*   a bus operation to use the XferState and Opcode enums, with their associated
*   static support functions.
*/
class BusOp {
  public:
    //virtual void wipe()  =0;
    //virtual void begin() =0;

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {       return (XferState::IDLE     == xfer_state);  };

    /* Inlines for protected access... TODO: These should be eliminated over time. */
    inline XferState get_state() {                 return xfer_state;       };
    inline void      set_state(XferState nu) {     xfer_state = nu;         };
    inline BusOpcode get_opcode() {                return opcode;           };
    inline void      set_opcode(BusOpcode nu) {    opcode = nu;             };

    /* Inlines for object-style usage of static functions... */
    inline const char* getOpcodeString() {  return BusOp::getOpcodeString(opcode);     };
    inline const char* getStateString() {   return BusOp::getStateString(xfer_state);  };


    static int next_txn_id;
    static const char* getStateString(XferState);
    static const char* getOpcodeString(BusOpcode);



  protected:
    BusOpcode opcode     = BusOpcode::UNDEF;      // What is the particular operation being done?
    XferState xfer_state = XferState::UNDEF;      // What state is this transfer in?

    // Call-ahead, call-back

  private:
};

#endif  // __MANUVR_BUS_QUEUE_H__
