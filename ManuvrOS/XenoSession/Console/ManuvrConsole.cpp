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

#ifdef defined(__MANUVR_CONSOLE_SUPPORT)

#include "ManuvrConsole.h"

/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
ManuvrConsole::ManuvrConsole(ManuvrXport* _xport) : XenoSession(_xport) {
  // These are messages that we want to relay from the rest of the system.
  tapMessageType(MANUVR_MSG_SESS_ESTABLISHED);
  tapMessageType(MANUVR_MSG_SESS_HANGUP);
  tapMessageType(MANUVR_MSG_LEGEND_MESSAGES);

  if (_xport->booted()) {
    bootComplete();   // Because we are instantiated well after boot, we call this on construction.
  }
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
ManuvrConsole::~ManuvrConsole() {
}



/****************************************************************************************************
* Functions for interacting with the transport driver.                                              *
****************************************************************************************************/

/**
* When we take bytes from the transport, and can't use them all right away,
*   we store them to prepend to the next group of bytes that come through.
*
* @param
* @param
* @return  int8_t  // TODO!!!
*/
int8_t ManuvrConsole::bin_stream_rx(unsigned char *buf, int len) {
  int8_t return_value = 0;

  session_buffer.concat(buf, len);

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}



/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrConsole::getReceiverName() {  return "Console";  }


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrConsole::printDebug(StringBuilder *output) {
  XenoSession::printDebug(output);

  int ses_buf_len = session_buffer.length();
  if (ses_buf_len > 0) {
    #if defined(__MANUVR_DEBUG)
      output->concatf("-- Session Buffer (%d bytes):  ", ses_buf_len);
      session_buffer.printDebug(output);
      output->concat("\n\n");
    #else
      output->concatf("-- Session Buffer (%d bytes)\n\n", ses_buf_len);
    #endif
  }
}




/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀
*
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/**
* Boot done finished-up.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrConsole::bootComplete() {
  EventReceiver::bootComplete();
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
  switch (event->event_code) {
    case MANUVR_MSG_SELF_DESCRIBE:
      //sendEvent(event);
      break;
    default:
      break;
  }

  return return_value;
}


/*
* This is the override from EventReceiver, but there is a bit of a twist this time.
* Following the normal processing of the incoming event, this class compares it against
*   a list of events that it has been instructed to relay to the counterparty. If the event
*   meets the relay criteria, we serialize it and send it to the transport that we are bound to.
*/
int8_t ManuvrConsole::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SESS_HANGUP:
      {
        int out_purge = purgeOutbound();
        int in_purge  = purgeInbound();
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 5) local_log.concatf("0x%08x Purged (%d) msgs from outbound and (%d) from inbound.\n", (uint32_t) this, out_purge, in_purge);
        #endif
      }
      return_value++;
      break;

    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      return_value++;
      break;

    case MANUVR_MSG_XPORT_RECEIVE:
      {
        StringBuilder* buf;
        if (0 == active_event->getArgAs(&buf)) {
          bin_stream_rx(buf->string(), buf->length());
        }
      }
      return_value++;
      break;

    default:
      return_value += XenoSession::notify(active_event);
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}


void ManuvrConsole::procDirectDebugInstruction(StringBuilder *input) {
  uint8_t temp_byte = 0;

  char* str = input->position(0);

  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'i':
      printDebug(&local_log);
      break;

    default:
      XenoSession::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}


#endif  // __MANUVR_CONSOLE_SUPPORT
