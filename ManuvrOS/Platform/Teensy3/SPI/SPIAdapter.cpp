/*
File:   SPIAdapter.cpp
Author: J. Ian Lindsay
Date:   2016.12.17

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


This is a peripheral wraspper around the Teensyduino SPI driver.
*/

#include <Platform/Peripherals/SPI/SPIAdapter.h>
#include <SPI.h>  // Teensyduino SPI


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
/* Used to minimize software burden incurred by timeout support. */
volatile static bool timeout_punch = false;

SPIBusOp  SPIAdapter::preallocated_bus_jobs[PREALLOCATED_SPI_JOBS];
SPIBusOp* SPIAdapter::current_queue_item = nullptr;

const MessageTypeDef spi_message_defs[] = {
  {  MANUVR_MSG_SPI_QUEUE_READY,    0x0000,  "SPI_Q_RDY",  ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SPI_CB_QUEUE_READY, 0x0000,  "SPICB_RDY",  ManuvrMsg::MSG_ARGS_NONE }, //
};



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Constructor. Also populates the global pointer reference.
*/
SPIAdapter::SPIAdapter(uint8_t idx, uint8_t d_in, uint8_t d_out, uint8_t clk) : EventReceiver(), BusAdapter(PREALLOCATED_SPI_JOBS) {
  setReceiverName("TeensySPI");
  ManuvrMsg::registerMessages(spi_message_defs, sizeof(spi_message_defs) / sizeof(MessageTypeDef));

  // Build some pre-formed Events.
  event_spi_callback_ready.repurpose(MANUVR_MSG_SPI_CB_QUEUE_READY, (EventReceiver*) this);
  event_spi_callback_ready.incRefs();
  event_spi_callback_ready.specific_target = (EventReceiver*) this;
  event_spi_callback_ready.priority(5);

  SPIBusOp::event_spi_queue_ready.repurpose(MANUVR_MSG_SPI_QUEUE_READY, (EventReceiver*) this);
  SPIBusOp::event_spi_queue_ready.incRefs();
  SPIBusOp::event_spi_queue_ready.specific_target = (EventReceiver*) this;
  SPIBusOp::event_spi_queue_ready.priority(5);

  // Mark all of our preallocated SPI jobs as "No Reap" and pass them into the prealloc queue.
  for (uint8_t i = 0; i < PREALLOCATED_SPI_JOBS; i++) {
    preallocated_bus_jobs[i].returnToPrealloc(true);     // Implies SHOuLD_REAP = false.
    preallocated.insert(&preallocated_bus_jobs[i]);
  }

  current_queue_item = NULL;
  _er_set_flag(SPI_FLAG_QUEUE_IDLE);
}


/**
* Destructor. Should never be called.
*/
SPIAdapter::~SPIAdapter() {
  purge_queued_work();
}


/*******************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄   Members related to the work queue
* ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌  and SPI bus I/O
* ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌ ▀▀▀▀█░█▀▀▀▀
* ▐░▌          ▐░▌       ▐░▌     ▐░▌       SPI transactions have two phases:
* ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌       1) ADDR (Addressing) Max of 4 bytes.
* ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌       2) IO_WAIT (Transfer)
*  ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀      ▐░▌
*           ▐░▌▐░▌               ▐░▌       Jobs having NULL buffers don't have
*  ▄▄▄▄▄▄▄▄▄█░▌▐░▌           ▄▄▄▄█░█▄▄▄▄     transfer phases. Jobs without ADDR
* ▐░░░░░░░░░░░▌▐░▌          ▐░░░░░░░░░░░▌    parameters don't have address
*  ▀▀▀▀▀▀▀▀▀▀▀  ▀            ▀▀▀▀▀▀▀▀▀▀▀     phases.
*******************************************************************************/

/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return SPI_CALLBACK_NOMINAL on success, or appropriate error code.
*/
int8_t SPIAdapter::io_op_callback(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;

  // There is zero chance this object will be a null pointer unless it was done on purpose.
  if (getVerbosity() > 2) {
    local_log.concatf("Probably shouldn't be in the default callback case...\n");
    op->printDebug(&local_log);
  }

  flushLocalLog();
  return SPI_CALLBACK_NOMINAL;
}


/**
* This is what we call when this class wants to conduct a transaction on the SPI bus.
* Note that this is different from other class implementations, in that it checks for
*   callback population before clobbering it. This is because this class is also the
*   SPI driver. This might end up being reworked later.
*
* @param  _op  The bus operation to execute.
* @return Zero on success, or appropriate error code.
*/
int8_t SPIAdapter::queue_io_job(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;

  if (op) {
    if (nullptr == op->callback) {
      // We assign the bus adapter itself to be notified on job completion.
      op->callback = (BusOpCallback*) this;
    }

    if (getVerbosity() > 6) {
      op->profile(true);
    }

    if (op->get_state() != XferState::IDLE) {
      if (getVerbosity() > 3) Kernel::log("Tried to fire a bus op that is not in IDLE state.\n");
      return -4;
    }

    if ((nullptr == current_queue_item) && (work_queue.size() == 0)){
      // If the queue is empty, fire the operation now.
      current_queue_item = op;
      advance_work_queue();
      //if (bus_timeout_millis) event_spi_timeout.delaySchedule(bus_timeout_millis);  // Punch the timeout schedule.
    }
    else {    // If there is something already in progress, queue up.
      if (_er_flag(SPI_FLAG_QUEUE_GUARD) && (SPI_MAX_Q_DEPTH <= work_queue.size())) {
        if (getVerbosity() > 3) Kernel::log("SPIAdapter::queue_io_job(): \t Bus queue at max size. Dropping transaction.\n");
        op->abort(XferFault::QUEUE_FLUSH);
        callback_queue.insertIfAbsent(op);
        if (callback_queue.size() == 1) Kernel::staticRaiseEvent(&event_spi_callback_ready);
        return -1;
      }

      if (0 > work_queue.insertIfAbsent(op)) {
        if (getVerbosity() > 2) {
          local_log.concat("SPIAdapter::queue_io_job(): \t Double-insertion. Dropping transaction with no status change.\n");
          op->printDebug(&local_log);
          Kernel::log(&local_log);
        }
        return -3;
      }
    }
    return 0;
  }
  return -5;
}


/**
* Execute any I/O callbacks that are pending. The function is present because
*   this class contains the bus implementation.
*
* @return the number of callbacks proc'd.
*/
int8_t SPIAdapter::service_callback_queue() {
  int8_t return_value = 0;
  SPIBusOp* temp_op = callback_queue.dequeue();

  while ((nullptr != temp_op) && (return_value < spi_cb_per_event)) {
  //if (nullptr != temp_op) {
    if (getVerbosity() > 6) temp_op->printDebug(&local_log);
    if (temp_op->callback) {
      int8_t cb_code = temp_op->callback->io_op_callback(temp_op);
      switch (cb_code) {
        case SPI_CALLBACK_RECYCLE:
          temp_op->set_state(XferState::IDLE);
          queue_io_job(temp_op);
          break;

        case SPI_CALLBACK_ERROR:
        case SPI_CALLBACK_NOMINAL:
          // No harm in this yet, since this fxn respects preforms and prealloc.
          reclaim_queue_item(temp_op);
          break;
        default:
          local_log.concatf("Unsure about SPI_CALLBACK_CODE %d.\n", cb_code);
          reclaim_queue_item(temp_op);
          break;
      }
    }
    else {
      // We are the responsible party.
      reclaim_queue_item(temp_op);
    }
    return_value++;
    temp_op = callback_queue.dequeue();
  }

  flushLocalLog();
  return return_value;
}


/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t SPIAdapter::advance_work_queue() {
  int8_t return_value = 0;

  timeout_punch = false;
  if (current_queue_item) {
    switch (current_queue_item->get_state()) {
       case XferState::TX_WAIT:
       case XferState::RX_WAIT:
         if (current_queue_item->hasFault()) {
           if (getVerbosity() > 3) local_log.concat("SPIAdapter::advance_work_queue():\t Failed at IO_WAIT.\n");
         }
         else {
           current_queue_item->markComplete();
         }
         // NOTE: No break on purpose.
       case XferState::COMPLETE:
         callback_queue.insert(current_queue_item);
         current_queue_item = nullptr;
         if (callback_queue.size() == 1) {
           Kernel::staticRaiseEvent(&event_spi_callback_ready);
         }
         break;

       case XferState::IDLE:
       case XferState::INITIATE:
         switch (current_queue_item->begin()) {
           case 0:     // Nominal outcome. Transfer started with no problens...
             break;
           case -1:    // Bus appears to be in-use. State did not change.
             // Re-throw queue_ready event and try again later.
             if (getVerbosity() > 2) local_log.concat("  advance_work_queue() tried to clobber an existing transfer on chain.\n");
             //Kernel::staticRaiseEvent(&event_spi_queue_ready);  // Bypass our method. Jump right to the target.
             break;
           case -2:    // Began the transfer, and it barffed... was aborted.
             if (getVerbosity() > 3) local_log.concat("SPIAdapter::advance_work_queue():\t Failed to begin transfer after starting.\n");
             callback_queue.insert(current_queue_item);
             current_queue_item = NULL;
             if (callback_queue.size() == 1) Kernel::staticRaiseEvent(&event_spi_callback_ready);
             break;
         }
         break;

       /* Cases below ought to be handled by ISR flow... */
       case XferState::ADDR:
         current_queue_item->advance_operation(0, 0);
       case XferState::STOP:
         if (getVerbosity() > 5) local_log.concatf("State might be corrupted if we tried to advance_queue(). \n");
         break;
       default:
         if (getVerbosity() > 3) local_log.concatf("advance_work_queue() default state \n");
         break;
    }
  }


  if (nullptr == current_queue_item) {
    current_queue_item = work_queue.dequeue();
    // Begin the bus operation.
    if (current_queue_item) {
      if (current_queue_item->begin()) {
        if (getVerbosity() > 2) local_log.concatf("advance_work_queue() tried to clobber an existing transfer on the pick-up.\n");
        Kernel::staticRaiseEvent(&SPIBusOp::event_spi_queue_ready);  // Bypass our method. Jump right to the target.
      }
      return_value++;
    }
    else {
      // No Queue! Relax...
      event_spi_timeout.enableSchedule(false);  // Punch the timeout schedule.
    }
  }

  flushLocalLog();
  return return_value;
}


/**
* Purges only the jobs belonging to the given device from the work_queue.
* Leaves the currently-executing job.
*
* @param  dev  The device pointer that owns jobs we wish purged.
*/
void SPIAdapter::purge_queued_work_by_dev(BusOpCallback *dev) {
  if (NULL == dev) return;
  SPIBusOp* current = NULL;

  if (work_queue.size() > 0) {
    int i = 0;
    while (i < work_queue.size()) {
      current = work_queue.get(i);
      if (current->callback == dev) {
        current->abort(XferFault::QUEUE_FLUSH);
        work_queue.remove(current);
        reclaim_queue_item(current);
      }
      else {
        i++;
      }
    }
  }

  // Lastly... initiate the next bus transfer if the bus is not sideways.
  advance_work_queue();
}


/**
* Purges only the work_queue. Leaves the currently-executing job.
*/
void SPIAdapter::purge_queued_work() {
  SPIBusOp* current = NULL;
  while (work_queue.hasNext()) {
    current = work_queue.dequeue();
    current->abort(XferFault::QUEUE_FLUSH);
    reclaim_queue_item(current);
  }

  // Check this last to head off any silliness with bus operations colliding with us.
  purge_stalled_job();
}


/**
* Return a vacant SPIBusOp to the caller, allocating if necessary.
*
* @return an SPIBusOp to be used. Only NULL if out-of-mem.
*/
SPIBusOp* SPIAdapter::new_op() {
  SPIBusOp* return_value = preallocated.dequeue();
  if (nullptr == return_value) {
    _prealloc_misses++;
    return_value = new SPIBusOp();
    //if (getVerbosity() > 5) Kernel::log("new_op(): Fresh allocation!\n");
  }
  return return_value;
}


/**
* Return a vacant SPIBusOp to the caller, allocating if necessary.
*
* @param  _op   The desired bus operation.
* @param  _req  The device pointer that is requesting the job.
* @return an CPLDBusOp to be used. Only NULL if out-of-mem.
*/
SPIBusOp* SPIAdapter::new_op(BusOpcode _op, BusOpCallback* _req) {
  SPIBusOp* return_value = new_op();
  return_value->set_opcode(_op);
  return_value->callback = _req;
  return return_value;
}


/**
* This fxn will either free() the memory associated with the SPIBusOp object, or it
*   will return it to the preallocation queue.
*
* @param item The SPIBusOp to be reclaimed.
*/
void SPIAdapter::reclaim_queue_item(SPIBusOp* op) {
  if (op->hasFault() && (getVerbosity() > 1)) {    // Print failures.
    StringBuilder log;
    op->printDebug(&log);
    Kernel::log(&log);
  }

  if (op->returnToPrealloc()) {
    //if (getVerbosity() > 6) local_log.concatf("SPIAdapter::reclaim_queue_item(): \t About to wipe.\n");
    op->wipe();
    preallocated.insert(op);
  }
  else if (op->shouldReap()) {
    //if (getVerbosity() > 6) local_log.concatf("SPIAdapter::reclaim_queue_item(): \t About to reap.\n");
    delete op;
    specificity_burden++;
  }
  else {
    /* If we are here, it must mean that some other class fed us a const SPIBusOp,
       and wants us to ignore the memory cleanup. But we should at least set it
       back to IDLE.*/
    //if (getVerbosity() > 6) local_log.concatf("SPIAdapter::reclaim_queue_item(): \t Dropping....\n");
    op->set_state(XferState::IDLE);
  }

  flushLocalLog();
}


/**
* Purges a stalled job from the active slot.
*/
void SPIAdapter::purge_stalled_job() {
  if (current_queue_item != NULL) {
    current_queue_item->abort(XferFault::QUEUE_FLUSH);
    reclaim_queue_item(current_queue_item);
    current_queue_item = NULL;
  }
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t SPIAdapter::attached() {
  if (EventReceiver::attached()) {
    // We should init the SPI library...
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    return 1;
  }
  return 0;
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
int8_t SPIAdapter::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_SPI_CB_QUEUE_READY:
      if (callback_queue.size()) {
        return EVENT_CALLBACK_RETURN_RECYCLE;
      }
      break;
    default:
      break;
  }

  return return_value;
}


int8_t SPIAdapter::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_SPI_QUEUE_READY:
      advance_work_queue();
      return_value++;
      break;
    case MANUVR_MSG_SPI_CB_QUEUE_READY:
      service_callback_queue();
      return_value++;
      break;
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SPIAdapter::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  if (getVerbosity() > 2) {
    output->concatf("-- Guarding queue      %s\n",       (_er_flag(SPI_FLAG_QUEUE_GUARD)?"yes":"no"));
    output->concatf("-- spi_cb_per_event    %d\n--\n",   spi_cb_per_event);
  }
  output->concatf("-- prealloc queue size %d\n",     preallocated.size());
  output->concatf("-- prealloc_misses     %u\n",     (unsigned long) _prealloc_misses);
  output->concatf("-- total_transfers     %u\n",     (unsigned long) SPIBusOp::total_transfers);
  output->concatf("-- failed_transfers    %u\n",     (unsigned long) SPIBusOp::failed_transfers);

  output->concatf("-- bus queue depth:    %d\n-- callback q depth    %d\n\n", work_queue.size(), callback_queue.size());

  if (getVerbosity() > 3) {
    if (current_queue_item != NULL) {
      output->concat("\tCurrently being serviced:\n");
      current_queue_item->printDebug(output);
    }

    if (work_queue.size() > 0) {
      unsigned int print_depth = strict_min((uint8_t) 3, (uint8_t) SPI_MAX_QUEUE_PRINT);
      output->concatf("\nQueue Listing (top %d of %d total)\n", print_depth, work_queue.size());
      for (unsigned int i = 0; i < print_depth; i++) {
        work_queue.get(i)->printDebug(output);
      }
      output->concat("\n");
    }
  }
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void SPIAdapter::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  char c    = *(str);

  switch (c) {
    case 'g':     // SPI1 queue-guard (overflow protection).
      _er_flip_flag(SPI_FLAG_QUEUE_GUARD);
      local_log.concatf("SPI queue overflow guard?  %s\n", _er_flag(SPI_FLAG_QUEUE_GUARD)?"yes":"no");
      break;
    case 'p':     // Purge the SPI1 work queue.
      purge_queued_work();
      local_log.concat("SPI queue purged.\n");
      break;
    case '&':
      local_log.concatf("Advanced CPLD SPI work queue.\n");
      Kernel::raiseEvent(MANUVR_MSG_SPI_QUEUE_READY, nullptr);   // Raise an event
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT







/**
* Calling this member will cause the bus operation to be started.
*
* @return 0 on success, or non-zero on failure.
*/
int8_t SPIBusOp::begin() {
  //time_began    = micros();
  //if (0 == _param_len) {
  //  // Obvious invalidity. We must have at least one transfer parameter.
  //  abort(XferFault::BAD_PARAM);
  //  return -1;
  //}

  //if (SPI1->SR & SPI_FLAG_BSY) {
  //  Kernel::log("SPI op aborted before taking bus control.\n");
  //  abort(XferFault::BUS_BUSY);
  //  return -1;
  //}

  set_state(XferState::INITIATE);  // Indicate that we now have bus control.

  _assert_cs(true);

  if (_param_len) {
    set_state(XferState::ADDR);
    for (int i = 0; i < _param_len; i++) {
      SPI.transfer(xfer_params[i]);
    }
  }

  if (buf_len) {
    set_state((opcode == BusOpcode::TX) ? XferState::TX_WAIT : XferState::RX_WAIT);
    for (int i = 0; i < buf_len; i++) {
      SPI.transfer(*(buf + i));
    }
  }

  markComplete();
  return 0;
}


/**
* Called from the ISR to advance this operation on the bus.
* Stay brief. We are in an ISR.
*
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::advance_operation(uint32_t status_reg, uint8_t data_reg) {
  //debug_log.concatf("advance_op(0x%08x, 0x%02x)\n\t %s\n\t status: 0x%08x\n", status_reg, data_reg, getStateString(), (unsigned long) hspi1.State);
  //Kernel::log(&debug_log);

  /* These are our transfer-size-invariant cases. */
  switch (xfer_state) {
    case XferState::COMPLETE:
      abort(XferFault::HUNG_IRQ);
      return 0;

    case XferState::TX_WAIT:
    case XferState::RX_WAIT:
      markComplete();
      return 0;

    case XferState::FAULT:
      return 0;

    case XferState::QUEUED:
    case XferState::ADDR:
    case XferState::STOP:
    case XferState::UNDEF:

    /* Below are the states that we shouldn't be in at this point... */
    case XferState::INITIATE:
    case XferState::IDLE:
      abort(XferFault::ILLEGAL_STATE);
      return 0;
  }

  return -1;
}