/*
File:   MQTTSession.cpp
Author: J. Ian Lindsay
Date:   2016.04.09

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


#if defined (MANUVR_SUPPORT_MQTT)

#include "MQTTSession.h"



/****************************************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here. Initializers first, functions second.
****************************************************************************************************/




/****************************************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
MQTTSession::MQTTSession(ManuvrXport* _xport) : XenoSession(_xport) {
  __class_initializer();

  for (int i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
    messageHandlers[i].topicFilter = 0;
  }


  bootComplete();    // Because we are instantiated well after boot, we call this on construction.
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
MQTTSession::~MQTTSession() {
  _ping_timer.enableSchedule(false);
  __kernel->removeSchedule(&_ping_timer);
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
int8_t MQTTSession::bin_stream_rx(unsigned char *buf, int len) {
  int8_t return_value = 0;
  return return_value;
}




int8_t MQTTSession::sendKeepAlive() {
  if (isEstablished()) {
    unsigned char* buf  = (unsigned char*) alloca(16);
    size_t buf_size     = 0;

    int len = MQTTSerialize_pingreq(buf, buf_size);
    if (len > 0) {
      return (0 == sendPacket(buf, len));
    }
  }
  return -1;
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
int8_t MQTTSession::bootComplete() {
  XenoSession::bootComplete();

  _ping_timer.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG);
  _ping_timer.isManaged(true);
  _ping_timer.specific_target = (EventReceiver*) this;
  _ping_timer.alterScheduleRecurrence(-1);
  _ping_timer.alterSchedulePeriod(30);
  _ping_timer.autoClear(false);
  _ping_timer.enableSchedule(false);

  __kernel->addSchedule(&_ping_timer);

  if (!owner->alwaysConnected() && owner->connected()) {
    // Are we connected right now?
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
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t MQTTSession::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      // Sending a KA
      sendKeepAlive();
      break;
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
int8_t MQTTSession::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    /* Things that only this class is likely to care about. */
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


void MQTTSession::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'q':  // Manual message queue purge.
      purgeOutbound();
      purgeInbound();
      break;
    case 'w':  // Manual session poll.
      Kernel::raiseEvent(MANUVR_MSG_SESS_ORIGINATE_MSG, NULL);
      break;

    default:
      XenoSession::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}



/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* MQTTSession::getReceiverName() {  return "MQTTSession";  }


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void MQTTSession::printDebug(StringBuilder *output) {
  XenoSession::printDebug(output);
}


#endif
