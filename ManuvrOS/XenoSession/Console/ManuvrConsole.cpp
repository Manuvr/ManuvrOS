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

#include "ManuvrConsole.h"

#if defined(MANUVR_CONSOLE_SUPPORT)

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
ManuvrConsole::ManuvrConsole(BufferPipe* _near_side) : XenoSession("Console", _near_side) {
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


/**
* Responsible for taking any accumulated console input, doing some basic
*   error-checking, and routing it to its intended target.
*/
int8_t ManuvrConsole::_route_console_input(StringBuilder* last_user_input) {
  StringBuilder _raw_from_console;  // We do this to avoid leaks.
  // Now we take the data from the buffer so that further input isn't lost. JIC.
  _raw_from_console.concatHandoff(last_user_input);

  _raw_from_console.split(" ");
  if (_raw_from_console.count() > 0) {
    const char* str = (const char *) _raw_from_console.position(0);
    int subscriber_idx = atoi(str);
    switch (*str) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        // If the first position is a number, we drop the first position, since
        //   it was essentially a directive aimed at this class.
        _raw_from_console.drop_position(0);
        break;
    }

    if (_raw_from_console.count() > 0) {
      // If there are still positions, lookup the subscriber and send it the input.
      EventReceiver* subscriber = platform.kernel()->getSubscriber(subscriber_idx);
      if (nullptr != subscriber) {
        subscriber->procDirectDebugInstruction(&_raw_from_console);
      }
      else {
        local_log.concatf("No such subscriber: %d\n", subscriber_idx);
      }
    }
  }

  flushLocalLog();
  return 0;
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
          ManuvrMsg* event = Kernel::returnEvent(MANUVR_MSG_SYS_REBOOT);
          event->setOriginator((EventReceiver*) platform.kernel());
          Kernel::staticRaiseEvent(event);
        }
        else if (strcasestr(temp_ptr, "HELP"))  printHelp();      // Show help.
        else
      #endif
      {
        // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
        // run in idle time.
        // TODO: This copies the string from session_buffer into another string
        //   that will be free'd. Needless. Complicates concurrency following
        //   migration of this code from Kernel.
        StringBuilder* dispatched = new StringBuilder((uint8_t*) temp_ptr, temp_len);
        ManuvrMsg* event  = Kernel::returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
        event->specific_target = (EventReceiver*) this;
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
  if (EventReceiver::attached()) {
    //toCounterparty((uint8_t*) temp, strlen(temp), MEM_MGMT_RESPONSIBLE_BEARER);
    // This is a console. Presumable it should also render log output.
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
int8_t ManuvrConsole::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

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
    #if defined(MANUVR_DEBUG)
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
int8_t ManuvrConsole::notify(ManuvrMsg* active_runnable) {
  int8_t return_value = 0;

  switch (active_runnable->eventCode()) {
    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_USER_DEBUG_INPUT:
      if (active_runnable->argCount()) {
        // If the event came with a StringBuilder, concat it onto the last_user_input.
        StringBuilder* _tmp = nullptr;
        if (0 == active_runnable->getArgAs(&_tmp)) {
          _route_console_input(_tmp);
        }
      }
      return_value++;
      break;
    default:
      return_value += XenoSession::notify(active_runnable);
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

#endif  // MANUVR_CONSOLE_SUPPORT
