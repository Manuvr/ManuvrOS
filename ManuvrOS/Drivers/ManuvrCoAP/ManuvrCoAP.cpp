/*
File:   ManuvrCoAP.cpp
Author: J. Ian Lindsay
Date:   2016.03.31

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


This class implements CoAP over UDP.

*/


#include "ManuvrCoAP.h"
#include <DataStructures/StringBuilder.h>

bool ManuvrCoAP::__msgs_registered = false;
const MessageTypeDef coap_message_defs[] = {
//  #if defined (__ENABLE_MSG_SEMANTICS)
//  {  MANUVR_MSG_COAP_GET       , 0,  "COAP_GET",         ManuvrMsg::MSG_ARGS_NONE }, //
//  {  MANUVR_MSG_COAP_PUT       , 0,  "COAP_PUT",         ManuvrMsg::MSG_ARGS_NONE }, //
//  {  MANUVR_MSG_COAP_DELETE    , 0,  "COAP_DELETE",      ManuvrMsg::MSG_ARGS_NONE }, //
//  {  MANUVR_MSG_COAP_UPDATE    , 0,  "COAP_UPDATE",      ManuvrMsg::MSG_ARGS_NONE }, //
//  #else
//  {  MANUVR_MSG_COAP_GET       , 0,  "COAP_GET",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
//  {  MANUVR_MSG_COAP_PUT       , 0,  "COAP_PUT",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
//  {  MANUVR_MSG_COAP_DELETE    , 0,  "COAP_DELETE",      ManuvrMsg::MSG_ARGS_NONE, NULL }, //
//  {  MANUVR_MSG_COAP_UPDATE    , 0,  "COAP_UPDATE",      ManuvrMsg::MSG_ARGS_NONE, NULL }, //
//  #endif
};




#if defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)
  /*
  * Since listening for connections on this transport involves blocking, we have a
  *   thread dedicated to the task...
  */
  void* coap_listener_loop(void* active_xport) {
    if (NULL != active_xport) {
      sigset_t set;
      sigemptyset(&set);
      //sigaddset(&set, SIGIO);
      sigaddset(&set, SIGQUIT);
      sigaddset(&set, SIGHUP);
      sigaddset(&set, SIGTERM);
      sigaddset(&set, SIGVTALRM);
      sigaddset(&set, SIGINT);
      int s = pthread_sigmask(SIG_BLOCK, &set, NULL);

      ManuvrCoAP* listening_inst = (ManuvrCoAP*) active_xport;
      ManuvrRunnable* event = NULL;
      StringBuilder output;
      //while (listening_inst->listening()) {
      //}

      // Close the context...
    }
    else {
      Kernel::log("Tried to listen with a NULL context.");
    }

    return NULL;
  }

#else
  // Threads are unsupported here.
#endif


// TODO: Until I devise something smarter, we get ugly static callbacks...
#ifdef __cplusplus
 extern "C" {
#endif





#ifdef __cplusplus
 }
#endif



ManuvrCoAP::ManuvrCoAP(const char* addr, const char* port) {
  __class_initializer();
  _port_number = port;
  _addr        = addr;
}


ManuvrCoAP::~ManuvrCoAP() {
  __kernel->unsubscribe(this);
  if (NULL != _ctx) {
  }
}


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrCoAP::__class_initializer() {
  EventReceiver::__class_initializer();

  _options           = 0;
  _port_number       = NULL;
  _ctx               = NULL;

  // TODO: Singleton due-to-feature thrust. Need to ditch the singleton...
  if (!__msgs_registered) {
    __msgs_registered = this;
    ManuvrMsg::registerMessages(coap_message_defs, sizeof(coap_message_defs) / sizeof(MessageTypeDef));
  }

  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.originator      = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  //read_abort_event.addArg(xport_id);  // Add our assigned transport ID to our pre-baked argument.
}



/****************************************************************************************************
* Message bridges                                                                                   *
****************************************************************************************************/

// TODO: This is the point at which we ask the Kernel for all exportable messages and map them into CoAP.
int8_t ManuvrCoAP::init_resources() {

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
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrCoAP::getReceiverName() {  return "ManuvrCoAP";  }


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ManuvrCoAP::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
  output->concatf("-- _addr           %s:%s\n",  _addr, _port_number);
  output->concatf("-- _options        0x%08x\n", _options);
}



/**
* There is a NULL-check performed upstream for the scheduler member. So no need
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrCoAP::bootComplete() {
  EventReceiver::bootComplete();   // Call up to get scheduler ref and class init.
  // We will suffer a 300ms latency if the platform's networking stack doesn't flush
  //   its buffer in time.
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(300);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);
  read_abort_event.enableSchedule(false);
  //__kernel->addSchedule(&read_abort_event);

  if (NULL == _ctx) {
    //_ctx = get_context(_addr, _port_number);
    if (_ctx) {
      init_resources();
      #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
      createThread(&_thread_id, NULL, coap_listener_loop, (void*) this);
      #else
        // TODO: Presently, we need threading.
      #endif
    }
    else {
      Kernel::log("Failed to obtain a CoAP context.");
    }
  }

  return 1;
}



/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ManuvrCoAP::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }

  return return_value;
}



int8_t ManuvrCoAP::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
  return return_value;
}



void ManuvrCoAP::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    case 'W':
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

#endif
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
