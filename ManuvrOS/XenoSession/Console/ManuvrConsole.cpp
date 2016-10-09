/*
File:   ManuvrConsole.cpp
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

#if defined(__MANUVR_DEBUG) || defined(MANUVR_CONSOLE_SUPPORT)

#include "ManuvrConsole.h"


void printHelp() {
  // TODO: Hack. Needs to generate help based on context. But since I don't want
  //         to get mired in building a generalized CLI into this class (yet),
  //         this will suffice for now.
  Kernel::log("Help would ordinarily be displayed here.\n");
}


/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   BufferPipe* All sessions must have one (and only one) transport.
*/
ManuvrConsole::ManuvrConsole(BufferPipe* _near_side) : XenoSession(_near_side) {
  setReceiverName("Console");
  Kernel::attachToLogger((BufferPipe*) this);
  mark_session_state(XENOSESSION_STATE_ESTABLISHED);
  _bp_set_flag(BPIPE_FLAG_IS_BUFFERED, true);
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
ManuvrConsole::~ManuvrConsole() {
  session_buffer.clear();
  _log_accumulator.clear();
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t ManuvrConsole::toCounterparty(ManuvrPipeSignal _sig, void* _args) {
  switch (_sig) {
    case ManuvrPipeSignal::FLUSH:
      // In this context, we flush out log accumulator back to the counterparty.
      if (haveNear() && _log_accumulator.length() > 0) {
        _near->toCounterparty(&_log_accumulator, MEM_MGMT_RESPONSIBLE_BEARER);
      }
      break;
    default:
      break;
  }

  /* By doing this, we propagate the signal further toward the counterparty. */
  return BufferPipe::toCounterparty(_sig, _args);
}

/**
* Inward toward the transport.
* Or into the accumulator.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrConsole::toCounterparty(StringBuilder* buf, int8_t mm) {
  _log_accumulator.concatHandoff(buf);
  if (isConnected() && erAttached()) {
    BufferPipe::toCounterparty(&_log_accumulator, MEM_MGMT_RESPONSIBLE_BEARER);
  }
  return MEM_MGMT_RESPONSIBLE_BEARER;
}

/**
* Taking user input from the transport...
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrConsole::fromCounterparty(StringBuilder* buf, int8_t mm) {
  session_buffer.concatHandoff(buf);  // buf check will fail if the precedes it.
  // If the console doesn't see a CR OR LF, it will not register a command.
  if ((session_buffer.contains('\n') || session_buffer.contains('\r'))) {
    for (int toks = session_buffer.split("\n"); toks > 0; toks--) {
      char* temp_ptr = session_buffer.position(0);
      int temp_len   = strlen(temp_ptr);
      // Begin the cases...
      #if defined(_GNU_SOURCE)
        if (strcasestr(temp_ptr, "QUIT")) {
          ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_REBOOT);
          event->setOriginator((EventReceiver*) platform.kernel());
          Kernel::staticRaiseEvent(event);
        }
        else if (strcasestr(temp_ptr, "HELP"))  printHelp();      // Show help.
        else
      #endif
      {
        // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
        // run in idle time.
        StringBuilder* dispatched = new StringBuilder((uint8_t*) temp_ptr, temp_len);
        ManuvrRunnable* event  = Kernel::returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
        event->specific_target = (EventReceiver*) platform.kernel();
        event->setOriginator((EventReceiver*) this);
        event->addArg(dispatched)->reapValue(true);
        raiseEvent(event);
      }
      session_buffer.drop_position(0);
    }
  }
  return MEM_MGMT_RESPONSIBLE_BEARER;
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
int8_t ManuvrConsole::attached() {
  EventReceiver::attached();
  //toCounterparty((uint8_t*) temp, strlen(temp), MEM_MGMT_RESPONSIBLE_BEARER);
  // This is a console. Presumable it should also render log output.
  return 1;
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
int8_t ManuvrConsole::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrConsole::printDebug(StringBuilder *output) {
  XenoSession::printDebug(output);

  int ses_buf_len = session_buffer.length();
  int la_len      = _log_accumulator.length();
  output->concatf("-- Console echoes:           %s\n", _local_echo ? "yes" : "no");
  if (ses_buf_len > 0) {
    #if defined(__MANUVR_DEBUG)
      output->concatf("-- Session Buffer (%d bytes):  ", ses_buf_len);
      session_buffer.printDebug(output);
      output->concat("\n");
    #else
      output->concatf("-- Session Buffer (%d bytes)\n", ses_buf_len);
    #endif
  }

  if (la_len > 0) {
    output->concatf("-- Accumulated log length (%d bytes)\n", la_len);
  }
}


/*
* This is the override from EventReceiver, but there is a bit of a twist this time.
* Following the normal processing of the incoming event, this class compares it against
*   a list of events that it has been instructed to relay to the counterparty. If the event
*   meets the relay criteria, we serialize it and send it to the transport that we are bound to.
*/
int8_t ManuvrConsole::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    /* Things that only this class is likely to care about. */
    default:
      return_value += XenoSession::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


// We don't bother casing this off for the preprocessor. In this case, it
//   is a given that we have MANUVR_CONSOLE_SUPPORT.
// This may be a strange loop that we might optimize later, but this is
//   still a valid call target that deals with allowing the console to operate
//   on itself.
// TODO: Change local_echo.
// TODO: Terminal reset.
// TODO: Terminal forsakes logger.
void ManuvrConsole::procDirectDebugInstruction(StringBuilder *input) {
  XenoSession::procDirectDebugInstruction(input);
  flushLocalLog();
}

#endif  // MANUVR_CONSOLE_SESSION  &  MANUVR_CONSOLE_SUPPORT
