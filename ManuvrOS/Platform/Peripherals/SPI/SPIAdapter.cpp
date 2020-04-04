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


This is the platform-invariant implementation of a peripheral wraspper
  for SPI adapters.
*/

#include <Platform/Peripherals/SPI/SPIAdapter.h>

#if defined(CONFIG_MANUVR_SUPPORT_SPI)


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

SPIBusOp SPIAdapter::preallocated_bus_jobs[CONFIG_SPIADAPTER_PREALLOC_COUNT];

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
SPIAdapter::SPIAdapter(const SPIAdapterOpts* o) : EventReceiver("SPIAdapter"), BusAdapter(o->idx, CONFIG_SPIADAPTER_PREALLOC_COUNT), _opts(o) {
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
}


/**
* Destructor. Should never be called.
*/
SPIAdapter::~SPIAdapter() {
  bus_deinit();
  purge_queued_work();
}


int8_t SPIAdapter::init() {
  int8_t ret = -1;
  // Mark all of our preallocated SPI jobs as "No Reap" and pass them into the prealloc queue.
  for (uint8_t i = 0; i < CONFIG_SPIADAPTER_PREALLOC_COUNT; i++) {
    preallocated_bus_jobs[i].wipe();
    preallocated_bus_jobs[i].returnToPrealloc(true);
    preallocated.insert(&preallocated_bus_jobs[i]);
  }
  _er_set_flag(SPI_FLAG_QUEUE_IDLE);
  return bus_init();
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

/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t SPIAdapter::io_op_callahead(BusOp* _op) {
  // Bus adapters don't typically do anything here, other
  //   than permit the transfer.
  return 0;
}


/**
* When a bus operation completes, it is passed back to its issuing class.
* This is typically not called except as the fallback case for transfers that
*   are either misconfigured, or that explicitly *don't want* a callback event
*   for some reason.
*
* @param  _op  The bus operation that was completed.
* @return 0 on success, or appropriate error code.
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
  int8_t ret = -5;

  if (op) {
    ret = -4;
    op->setAdapter(this);
    if (nullptr == op->callback) {
      op->callback = (BusOpCallback*) this;
    }

    if (op->get_state() == XferState::IDLE) {
      ret = 0;
      if ((nullptr == current_job) && (work_queue.size() == 0)){
        // If the queue is empty, fire the operation now.
        current_job = op;
        advance_work_queue();
      }
      else {    // If there is something already in progress, queue up.
        if (_adapter_flag(SPI_FLAG_QUEUE_GUARD) && !roomInQueue()) {
          ret = -1;
          StringBuilder log;
          log.concatf("SPI:\t Bus queue at max size. Dropping transaction.\n", ADAPTER_NUM);
          Kernel::log(&log);
          op->abort(XferFault::QUEUE_FLUSH);
          callback_queue.insertIfAbsent(op);
        }
        else if (0 > work_queue.insertIfAbsent(op)) {
          ret = -3;
          if (getVerbosity() > 2) {
            StringBuilder log;
            log.concatf("SPI%u:\t Double-insertion. Dropping transaction with no status change.\n", ADAPTER_NUM);
            op->printDebug(&log);
            Kernel::log(&log);
          }
        }
        else {
          op->set_state(XferState::QUEUED);
        }
      }
    }
    else {
      if (getVerbosity() > 3) {
        StringBuilder log;
        log.concatf("SPI%u:\t Tried to fire a bus op that is not in IDLE state.\n", ADAPTER_NUM);
        Kernel::log(&log);
      }
    }
  }
  return ret;
}


/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

// These are platform-specific. They should be supplied in another file.
// int8_t SPIAdapter::bus_init() {
//   return 0;
// }
//
// int8_t SPIAdapter::bus_deinit() {
//   return 0;
// }


/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t SPIAdapter::advance_work_queue() {
  int8_t return_value = 0;

  timeout_punch = false;
  if (current_job) {
    switch (current_job->get_state()) {
       case XferState::TX_WAIT:
       case XferState::RX_WAIT:
         if (current_job->hasFault()) {
           if (getVerbosity() > 3) local_log.concat("SPIAdapter::advance_work_queue():\t Failed at IO_WAIT.\n");
         }
         else {
           current_job->markComplete();
         }
         // NOTE: No break on purpose.
       case XferState::COMPLETE:
         callback_queue.insert(current_job);
         current_job = nullptr;
         if (callback_queue.size() == 1) {
           Kernel::staticRaiseEvent(&event_spi_callback_ready);
         }
         break;

       case XferState::IDLE:
       case XferState::INITIATE:
         switch (current_job->begin()) {
           case XferFault::NONE:     // Nominal outcome. Transfer started with no problens...
             break;
           case XferFault::BUS_BUSY:    // Bus appears to be in-use. State did not change.
             // Re-throw queue_ready event and try again later.
             if (getVerbosity() > 2) local_log.concat("  advance_work_queue() tried to clobber an existing transfer on chain.\n");
             //Kernel::staticRaiseEvent(&event_spi_queue_ready);  // Bypass our method. Jump right to the target.
             break;
           default:    // Began the transfer, and it barffed... was aborted.
             if (getVerbosity() > 3) local_log.concat("SPIAdapter::advance_work_queue():\t Failed to begin transfer after starting.\n");
             callback_queue.insert(current_job);
             current_job = nullptr;
             if (callback_queue.size() == 1) Kernel::staticRaiseEvent(&event_spi_callback_ready);
             break;
         }
         break;

       /* Cases below ought to be handled by ISR flow... */
       case XferState::ADDR:
         //current_job->advance_operation(0, 0);
       case XferState::STOP:
         if (getVerbosity() > 5) local_log.concatf("State might be corrupted if we tried to advance_queue(). \n");
         break;
       default:
         if (getVerbosity() > 3) local_log.concatf("advance_work_queue() default state \n");
         break;
    }
  }


  if (nullptr == current_job) {
    current_job = work_queue.dequeue();
    // Begin the bus operation.
    if (current_job) {
      if (XferFault::NONE != current_job->begin()) {
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
void SPIAdapter::purge_queued_work_by_dev(BusOpCallback* dev) {
  if (NULL == dev) return;

  if (work_queue.size() > 0) {
    SPIBusOp* current = nullptr;
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
* Execute any I/O callbacks that are pending. The function is present because
*   this class contains the bus implementation.
*
* @return the number of callbacks proc'd.
*/
int8_t SPIAdapter::service_callback_queue() {
  int8_t return_value = 0;
  SPIBusOp* temp_op = callback_queue.dequeue();

  while ((nullptr != temp_op) && (return_value < spi_cb_per_event)) {
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
  printAdapter(output);
  output->concatf("-- callback q depth    %d\n\n", callback_queue.size());

  if (getVerbosity() > 3) {
    printWorkQueue(output, CONFIG_SPIADAPTER_MAX_QUEUE_PRINT);
  }
}


//#if defined(MANUVR_CONSOLE_SUPPORT)
//void SPIAdapter::procDirectDebugInstruction(StringBuilder *input) {
//  char* str = input->position(0);
//  char c    = *(str);
//
//  switch (c) {
//    case 'g':     // SPI1 queue-guard (overflow protection).
//      _er_flip_flag(SPI_FLAG_QUEUE_GUARD);
//      local_log.concatf("SPI queue overflow guard?  %s\n", _er_flag(SPI_FLAG_QUEUE_GUARD)?"yes":"no");
//      break;
//    case 'p':     // Purge the SPI1 work queue.
//      purge_queued_work();
//      local_log.concat("SPI queue purged.\n");
//      break;
//    case '&':
//      local_log.concatf("Advanced CPLD SPI work queue.\n");
//      Kernel::raiseEvent(MANUVR_MSG_SPI_QUEUE_READY, nullptr);   // Raise an event
//      break;
//
//    default:
//      break;
//  }
//
//  flushLocalLog();
//}
//#endif  // MANUVR_CONSOLE_SUPPORT


#endif  // CONFIG_MANUVR_SUPPORT_SPI
