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


TODO: BusOp lifecycle....

*/

#ifndef __MANUVR_BUS_QUEUE_H__
#define __MANUVR_BUS_QUEUE_H__

#include <CommonConstants.h>
#include <DataStructures/PriorityQueue.h>
#include <DataStructures/StringBuilder.h>
#include <Platform/Platform.h>


/*
* These are possible transfer states.
*/
enum class XferState : uint8_t {
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
enum class BusOpcode : uint8_t {
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
enum class XferFault : uint8_t {
  NONE,            // No error on this transfer.
  NO_REASON,       // No reason provided, but still errored.
  TIMEOUT,         // We ran out of patience.
  BAD_PARAM,       // Invalid transfer parameters.
  ILLEGAL_STATE,   // The bus operation is in an illegal state.
  BUS_BUSY,        // The bus didn't get back to us in time.
  BUS_FAULT,       // The bus had a meltdown and failed this transfer.
  DEV_FAULT,       // The device we we transacting with failed this transfer.
  HUNG_IRQ,        // One too many IRQs happened for this operation.
  DMA_FAULT,       // Something went sideways with DMA that wasn't a timeout.
  DEV_NOT_FOUND,   // When an addressed device we expected to find is not found.
  RO_REGISTER,     // We tried to write to a register defined as read-only.
  UNDEFD_REGISTER, // The requested register was not defined.
  IO_RECALL,       // The class that spawned this request changed its mind.
  QUEUE_FLUSH      // The work queue was flushed and this was a casualty.
};

/* Forward declarations. */
class BusOp;

/*
* This is an interface class that implements a callback path for I/O operations.
* If a class wants to put operations into the SPI queue, it must either implement this
*   interface, or delegate its callback duties to a class that does.
* Generally-speaking, this will be a device that transacts on the bus, but is
*   not itself the bus adapter.
*/
class BusOpCallback {
  public:
    virtual int8_t io_op_callahead(BusOp*) =0;  // Called ahead of op.
    virtual int8_t io_op_callback(BusOp*)  =0;  // Called behind completed op.
    virtual int8_t queue_io_job(BusOp*)    =0;  // Queue an I/O operation.
};


/*
* This class represents a single transaction on the bus, but is devoid of
*   implementation details. This is an interface class that should be extended
*   by classes that require hardware-level specificity.
*
* Since C++ has no formal means of declaring an interface, we will use Java's
*   conventions. That is, state-bearing members in this interface are ok, but
*   there should be no function members that are not pure virtuals or inlines.
*/
class BusOp {
  public:
    BusOpCallback* callback = nullptr;  // Which class gets pinged when we've finished?
    uint8_t* buf            = 0;        // Pointer to the data buffer for the transaction.
    uint16_t buf_len        = 0;        // How large is the above buffer?
    //uint32_t time_began     = 0;        // This is the time when bus access begins.
    //uint32_t time_ended     = 0;        // This is the time when bus access stops (or is aborted).

    /* Mandatory overrides from the BusOp interface... */
    //virtual XferFault advance() =0;
    virtual XferFault begin() =0;
    virtual void wipe()  =0;
    virtual void printDebug(StringBuilder*)  =0;

    /**
    * @return true if this operation is idle.
    */
    inline bool isIdle() {     return (XferState::IDLE == xfer_state);  };

    /**
    * @return true if this operation is idle.
    */
    inline int8_t execCA() {   return ((callback) ? callback->io_op_callahead(this) : 0);  };

    /**
    * @return true if this operation is idle.
    */
    inline int8_t execCB() {   return ((callback) ? callback->io_op_callback(this) : 0);  };

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


    static const char* getStateString(XferState);
    static const char* getOpcodeString(BusOpcode);
    static const char* getErrorString(XferFault);
    static void        printBusOp(const char*, BusOp*, StringBuilder*);


  protected:
    uint8_t   _flags     = 0;                 // Specifics are left to the extending class.
    BusOpcode opcode     = BusOpcode::UNDEF;  // What is the particular operation being done?
    XferState xfer_state = XferState::UNDEF;  // What state is this transfer in?
    XferFault xfer_fault = XferFault::NONE;   // Fault code.

    //static void        initBusOp(const char*, BusOp*, StringBuilder*);


  private:
};


/*
* This class represents a generic bus adapter. We are not so concerned with
*   performance and memory overhead in this class, because there is typically
*   only a handful of such classes, and they have low turnover rates.
*/
template <class T> class BusAdapter : public BusOpCallback {
  public:
    inline T* currentJob() {  return current_job;  };


  protected:
    T*       current_job      = nullptr;
    uint32_t _total_xfers     = 0;  // Transfer stats.
    uint32_t _failed_xfers    = 0;  // Transfer stats.
    uint16_t _prealloc_misses = 0;  // How many times have we starved the preallocation queue?
    uint16_t _heap_frees      = 0;  // How many times have we freed a BusOp?
    uint16_t _queue_floods    = 0;  // How many times has the queue rejected work?
    const uint16_t MAX_Q_DEPTH;     // Maximum tolerable queue depth.
    //TODO: const uint8_t  MAX_Q_PRINT;     // Maximum tolerable queue depth.
    //TODO: const uint8_t  PREALLOC_SIZE;   // Maximum tolerable queue depth.
    PriorityQueue<T*> work_queue;   // A work queue to keep transactions in order.
    PriorityQueue<T*> preallocated; // TODO: Convert to ring buffer. This is the whole reason you embarked on this madness.

    BusAdapter(uint16_t max) : MAX_Q_DEPTH(max) {};

    /* Mandatory overrides... */
    virtual int8_t advance_work_queue() =0;  // The nature of the bus dictates this implementation.
    virtual int8_t bus_init()           =0;  // Hardware-specifics.
    virtual int8_t bus_deinit()         =0;  // Hardware-specifics.
    //virtual int8_t io_op_callback(T*)   =0;  // From BusOpCallback
    //virtual int8_t queue_io_job(T*)     =0;  // From BusOpCallback

    void return_op_to_pool(T* obj) {
      obj->wipe();
      preallocated.insert(obj);
    };

    /**
    * Return a vacant BusOp to the caller, allocating if necessary.
    *
    * @return an BusOp to be used. Only NULL if out-of-mem.
    */
    T* new_op() {
      T* return_value = preallocated.dequeue();
      if (nullptr == return_value) {
        _prealloc_misses++;
        return_value = new T();
      }
      return return_value;
    };

    ///**
    //* Return a vacant BusOp to the caller, allocating if necessary.
    //*
    //* @param  _op   The desired bus operation.
    //* @param  _req  The device pointer that is requesting the job.
    //* @return an BusOp to be used. Only NULL if out-of-mem.
    //*/
    //T* new_op(BusOpcode _op, BusOpCallback* _req) {
    //  T* return_value = new T(_op, _req);
    //  return return_value;
    //};

    ///*
    //* Purges only the work_queue. Leaves the currently-executing job.
    //*/
    //void purge_queued_work() {
    //  T* current = work_queue.dequeue();
    //  while (current) {
    //    current->abort(XferFault::QUEUE_FLUSH);
    //    if (current->callback) {
    //      current->callback->io_op_callback(current);
    //    }
    //    reclaim_queue_item(current);
    //    current = work_queue.dequeue();
    //  }
    //};


    /* Convenience function for guarding against queue floods. */
    inline bool roomInQueue() {    return !(work_queue.size() < MAX_Q_DEPTH);  }

    // TODO: I hate that I'm doing this in a template.
    void printAdapter(StringBuilder* output) {
      output->concatf("-- Xfers (fail/total)  %u/%u\n", _failed_xfers, _total_xfers);
      output->concat("-- Prealloc:\n");
      output->concatf("--    available        %d\n",  preallocated.size());
      output->concatf("--    misses/frees     %u/%u\n", _prealloc_misses, _heap_frees);
      output->concat("-- Work queue:\n");
      output->concatf("--    depth/max        %u/%u\n", work_queue.size(), MAX_Q_DEPTH);
      output->concatf("--    floods           %u\n",  _queue_floods);
    };

    // TODO: I hate that I'm doing this in a template.
    void printWorkQueue(StringBuilder* output, int8_t max_print) {
      if (current_job) {
        output->concat("--\n- Current active job:\n");
        current_job->printDebug(output);
      }
      else {
        output->concat("--\n-- No active job.\n--\n");
      }
      int wqs = work_queue.size();
      if (wqs > 0) {
        int print_depth = strict_min((int8_t) wqs, max_print);
        output->concatf("-- Queue Listing (top %d of %d total)\n", print_depth, wqs);
        for (int i = 0; i < print_depth; i++) {
          work_queue.get(i)->printDebug(output);
        }
      }
      else {
        output->concat("-- Empty queue.\n");
      }
    };

  private:
};

#endif  // __MANUVR_BUS_QUEUE_H__
