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
  IDLE,       // Bus op is waiting somewhere outside of the queue.

  /* These states are unstable and should decay into a "finish" state. */
  QUEUED,     // Bus op is idle and waiting for its turn.
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



class BusOp {
  public:
    static const char* getOpcodeString(BusOpcode);


  protected:
  private:
};

#endif  // __MANUVR_BUS_QUEUE_H__
