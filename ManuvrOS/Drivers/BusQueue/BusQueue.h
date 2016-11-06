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
  UNDEF    = 0,  // Freshly instanced (or wiped, if preallocated).
  IDLE     = 1,  // Bus op is allocated and waiting somewhere outside of the queue.

  /* These states are unstable and should decay into a "finish" state. */
  QUEUED   = 2,  // Bus op is idle and waiting for its turn. No bus control.
  INITIATE = 3,  // Waiting for initiation phase.
  ADDR     = 5,  // Addressing phase. Sending the address.
  TX_WAIT  = 7,  // I/O operation in-progress.
  RX_WAIT  = 8,  // I/O operation in-progress.
  STOP     = 10,  // I/O operation in cleanup phase.

  /* These are finish states. */
  COMPLETE = 14, // I/O op complete with no problems.
  FAULT    = 15  // Fault condition.
};


/*
* These are the opcodes that we use to represent different bus operations.
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
* Possible fault conditions that might occur.
*/
enum class XferFault {
  NONE,            // No error on this transfer.
  NO_REASON,       // No reason provided, but still errored.
  TIMEOUT,         // We ran out of patience.
  BAD_PARAM,       // Invalid rtansfer parameters.
  ILLEGAL_STATE,   // The bus operation is in an illegal state.
  BUS_BUSY,        // The bus didn't get back to us in time..
  BUS_FAULT,       // The bus had a meltdown and failed this transfer.
  HUNG_IRQ,        // One too many IRQs happened for this operation.
  DMA_FAULT,       // Something went sideways with DMA that wasn't a timeout.
  DEV_NOT_FOUND,   // When an addressed device we expected to find is not found.
  RO_REGISTER,     // We tried to write to a register defined as read-only.
  UNDEFD_REGISTER, // The requested register was not defined.
  IO_RECALL,       // The class that spawned this request changed its mind.
  QUEUE_FLUSH      // The work queue was flushed and this was a casualty.
};



/*
* This class represents a single transaction on the bus, but is devoid of
*   implementation details. This is an interface class that should be extended
*   by classes that require hardware-level specificity.
*
* Since C++ has no formal means of declaring an interface, we will use Java's
*   conventions. That is, state-bearing members in this interface are ok, but
*   there should be no function members that are not pure virtuals or inlines.
*
* Note also, that a class is not required to inherrit from this definition of
*   a bus operation to use the enums defined above, with their associated static
*   support functions.
*/
class BusOp {
  public:
    uint8_t* buf         = 0;                  // Pointer to the data buffer for the transaction.
    uint16_t buf_len     = 0;                  // How large is the above buffer?

    /* Mandatory overrides... */
    //virtual void wipe()  =0;
    //virtual void begin() =0;

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {       return (XferState::IDLE     == xfer_state);  };

    /**
    * This only works because of careful defines. Tread lightly.
    *
    * @return true if this operation completed without problems.
    */
    inline bool has_bus_control() {
      return (
        (xfer_state == XferState::STOP) | inIOWait() | \
        (xfer_state == XferState::INITIATE) | (xfer_state == XferState::ADDR)
      );
    };

    /**
    * @return true if this operation completed without problems.
    */
    inline bool isComplete() {   return (XferState::COMPLETE <= xfer_state);  };

    /**
    * @return true if this operation is enqueued and inert.
    */
    inline bool isQueued() {     return (XferState::QUEUED == xfer_state);    };
    inline void markQueued() {   set_state(XferState::QUEUED);                };

    /**
    * @return true if this operation is waiting for IO to complete.
    */
    inline bool inIOWait() {
      return (XferState::RX_WAIT == xfer_state) || (XferState::TX_WAIT == xfer_state);
    };

    /**
    * @return true if this operation has been intiated, but is not yet complete.
    */
    inline bool inProgress() {
      return (XferState::INITIATE <= xfer_state) && (XferState::COMPLETE > xfer_state);
    };

    /**
    * @return true if this operation experienced any abnormal condition.
    */
    inline bool hasFault() {     return (XferFault::NONE != xfer_fault);     };


    /* Inlines for protected access... TODO: These should be eliminated over time. */
    inline XferState get_state() {                 return xfer_state;       };
    inline void      set_state(XferState nu) {     xfer_state = nu;         };
    inline BusOpcode get_opcode() {                return opcode;           };
    inline void      set_opcode(BusOpcode nu) {    opcode = nu;             };

    /* Inlines for object-style usage of static functions... */
    inline const char* getOpcodeString() {  return BusOp::getOpcodeString(opcode);     };
    inline const char* getStateString() {   return BusOp::getStateString(xfer_state);  };
    inline const char* getErrorString() {   return BusOp::getErrorString(xfer_fault);  };


    static int next_txn_id;
    static const char* getStateString(XferState);
    static const char* getOpcodeString(BusOpcode);
    static const char* getErrorString(XferFault);



  protected:
    BusOpcode opcode     = BusOpcode::UNDEF;   // What is the particular operation being done?
    XferState xfer_state = XferState::UNDEF;   // What state is this transfer in?
    XferFault xfer_fault = XferFault::NONE;    // Fault code.
    // TODO: Call-ahead, call-back

  private:
};

/*
* This class represents a generic bus adapter. We are not so concerned with
*   performance and memory overhead in this class, because there is typically
*   only a handful of such classes, and they have low turnover rates.
*/
class BusAdapter {
  public:
    /* Mandatory overrides... */
    virtual int8_t insert_work_item()   =0;
    virtual void   advance_work_queue() =0;

  protected:

  private:
};


/*
* This is an interface class that implements a callback path for I/O operations.
* If a class wants to put operations into the SPI queue, it must either implement this
*   interface, or delegate its callback duties to a class that does.
*/
class BusOpCallback {
  public:
    // The SPI driver will call this fxn when the bus op finishes.
    virtual int8_t io_op_callback(BusOp*) =0;

    /*
    */
    virtual int8_t queue_io_job(BusOp*) =0;

};

#endif  // __MANUVR_BUS_QUEUE_H__
