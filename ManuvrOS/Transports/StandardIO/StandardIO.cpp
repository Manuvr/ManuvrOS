/*
File:   StandardIO.cpp
Author: J. Ian Lindsay
Date:   2016.07.23

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


StandardIO is the transport driver for wrapping POSIX-style STDIN/STDOUT/STDERR.
*/


#if defined(__MANUVR_LINUX) && defined(MANUVR_STDIO)

#include "StandardIO.h"
#include <XenoSession/XenoSession.h>
#include <XenoSession/Console/ManuvrConsole.h>

#include <Kernel.h>
#include <Platform/Platform.h>

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>

// Threaded platforms will need this to compensate for a loss of ISR.
extern void* xport_read_handler(void* active_xport);


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


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Constructor.
*
*/
StandardIO::StandardIO() : ManuvrXport() {
  __class_initializer();
  _xport_mtu = 255;
}

/**
* Destructor
*/
StandardIO::~StandardIO() {
}

/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void StandardIO::__class_initializer() {
  setReceiverName("StandardIO");
  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.originator      = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  read_abort_event.addArg(xport_id);  // Add our assigned transport ID to our pre-baked argument.
  _bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED, true);
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Log has reached the end of its journey. This class will render it to the user.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t StandardIO::toCounterparty(StringBuilder* buf, int8_t mm) {
  if (connected()) {
    char* working_chunk = NULL;
    while (buf->count()) {
      working_chunk = buf->position(0);
      printf("%s", (const char*) working_chunk);
      bytes_sent += strlen(working_chunk);
      buf->drop_position(0);
    }

    // TODO: This prompt ought to be in the console session.
    printf("\n%c[36mManuvr> %c[39m", 0x1B, 0x1B);
    fflush(stdout);
    return MEM_MGMT_RESPONSIBLE_BEARER;
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}


/*******************************************************************************
* ___________                                                  __
* \__    ___/___________    ____   ____________   ____________/  |_
*   |    |  \_  __ \__  \  /    \ /  ___/\____ \ /  _ \_  __ \   __\
*   |    |   |  | \// __ \|   |  \\___ \ |  |_> >  <_> )  | \/|  |
*   |____|   |__|  (____  /___|  /____  >|   __/ \____/|__|   |__|
*                       \/     \/     \/ |__|
* These members are particular to the transport driver and any implicit
*   protocol it might contain.
*******************************************************************************/

int8_t StandardIO::connect() {
  // StandardIO, if instantiated, is ALWAYS connected.
  connected(true);
  return 0;
}

int8_t StandardIO::disconnect() {
  // Perhaps this makes sense in some context. But I cannot imagine it.
  //ManuvrXport::disconnect();
  return 0;
}


int8_t StandardIO::listen() {
  // StandardIO, if instantiated, is ALWAYS listening.
  listening(true);
  return 0;
}

// TODO: Perhaps reset the terminal?
int8_t StandardIO::reset() {
  #if defined(__MANUVR_DEBUG)
    if (getVerbosity() > 3) local_log.concatf("StandardIO initialized.\n");
  #endif
  initialized(true);
  listen();
  connect();

  flushLocalLog();
  return 0;
}

/**
* Read input from local keyboard.
*/
int8_t StandardIO::read_port() {
  char *input_text	= (char*) alloca(getMTU());	// Buffer to hold user-input.
  int read_len = 0;

  while (connected()) {
    bzero(input_text, getMTU());
    if (fgets(input_text, getMTU(), stdin) != NULL) {
      read_len = strlen(input_text);
      if (read_len) {
        bytes_received += read_len;
        BufferPipe::fromCounterparty((uint8_t*) input_text, read_len, MEM_MGMT_RESPONSIBLE_BEARER);
      }
    }
    else {
      // User insulted fgets()...
      Kernel::log("StandardIO: fgets() failed.\n");
    }
  }
  flushLocalLog();
  return 0;
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
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void StandardIO::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;
  ManuvrXport::printDebug(temp);
  temp->concatf("-- Class size      %d\n",     sizeof(StandardIO));
}


/**
* The Kernel is booting, or has-booted prior to our being instanced.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t StandardIO::bootComplete() {
  EventReceiver::bootComplete();

  // Tolerate 30ms of latency on the line before flushing the buffer.
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(30);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);

  reset();

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
int8_t StandardIO::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_XPORT_SEND:
      event->clearArgs();
      break;
    default:
      break;
  }

  return return_value;
}


int8_t StandardIO::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}

#endif  // __MANUVR_LINUX && MANUVR_STDIO
