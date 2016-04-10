/*
File:   ManuvrSession.cpp
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


#include "ManuvrSession.h"


/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
ManuvrSession::ManuvrSession(ManuvrXport* _xport) : XenoSession(_xport) {
  __class_initializer();

  // These are messages that we want to relay from the rest of the system.
  tapMessageType(MANUVR_MSG_SESS_ESTABLISHED);
  tapMessageType(MANUVR_MSG_SESS_HANGUP);
  tapMessageType(MANUVR_MSG_LEGEND_MESSAGES);

  bootComplete();    // Because we are instantiated well after boot, we call this on construction.
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
ManuvrSession::~ManuvrSession() {
  sync_event.enableSchedule(false);
  __kernel->removeSchedule(&sync_event);
}


/****************************************************************************************************
* Functions for managing and reacting to sync states.                                               *
****************************************************************************************************/

int8_t ManuvrSession::sendSyncPacket() {
  if (owner->connected()) {
    StringBuilder sync_packet((unsigned char*) XenoMessage::SYNC_PACKET_BYTES, 4);
    owner->sendBuffer(&sync_packet);

    //ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_XPORT_SEND);
    //event->specific_target = owner;  //   event to be the transport that instantiated us.
    //raiseEvent(event);
  }
  return 0;
}


/**
* This fxn is destructive to the session buffer. The point is to eliminate bytes from the
*   buffer until we have the following 4-byte sequence at offset 0. In that case, we consume
*   all the sync data from the buffer and return non-zero.
* If no sync packets were found in the buffer, we should return zero after having fully-consumed
*   the buffer.
*
* @return  int8_t 0 if we did not find a sync packet. 1 if we did. -1 on meltdown.
*/
int8_t ManuvrSession::scan_buffer_for_sync() {
  int8_t return_value = 0;
  int len = session_buffer.length();
  int last_sync_offset = 0;
  int offset = 0;
  uint32_t sync_value = 0x04 + (CHECKSUM_PRELOAD_BYTE << 24);
  unsigned char *buf = session_buffer.string();
  while (len >= offset+4) {
    if (parseUint32Fromchars(buf + offset) == sync_value) {
      // If we have those 4 bytes in sequence, we have found a sync packet.
      return_value = 1;
      last_sync_offset = offset;
      offset += 4;
      // Notice that we continue the loop. If we found one sync packet, we will likely find more.
    }
    else {
      offset++;
    }
  }

  if (return_value) {
    /* We found a sync! Now to cull the sync data from the session buffer. We COULD
    *  only cull the modulus of the offset and let the parser handle the sync packets,
    *  but that would just waste resources. We might as well eliminate all the junk now. */
    session_buffer.cull(last_sync_offset + 4);
  }
  else if (len > 7) {
    /* We did not find a sync packet, but we have more in the buffer than we need to find it.
    *  Cull all but the last 3 bytes. The next time we get data from the transport, we will
    *  concat to the buffer and run this check again. */
    session_buffer.cull(len - 3);
  }
  return return_value;
}


/**
* Calling this fxn puts the session into sync mode. This will throw the session into a tizzy
*   so be careful to call it only as a last resort.
*
* Calling this fxn has the following side-effects:
*   - Starts the schedule to send sync packets, which will continue until the session is marked sync'd.
*
* @param   uint8_t The sync state code that represents where in the sync-state-machine we should be.
*/
void ManuvrSession::mark_session_desync(uint8_t ds_src) {
  session_last_state = session_state;               // Stack our session state.
  session_state = getState() | ds_src;
  if (session_state & ds_src) {
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) local_log.concatf("Session 0x%08x is already in the requested sync state (%s). Doing nothing.\n", (uint32_t) this, getSessionSyncString());
    #endif
  }
  else {
    switch (ds_src) {
      case XENOSESSION_STATE_SYNC_INITIATED:    // CP-initiated sync
        break;
      case XENOSESSION_STATE_SYNC_INITIATOR:    // We initiated sync
        break;
      case XENOSESSION_STATE_SYNC_PEND_EXIT:    // Sync has been recognized and we are rdy for a real packet.
        break;
      case XENOSESSION_STATE_SYNC_SYNCD:        // Nominal state. Session is in sync.
        break;
      case XENOSESSION_STATE_SYNC_CASTING:      //
      default:
        break;
    }
  }
  sync_event.enableSchedule(true);

  if (local_log.length() > 0) Kernel::log(&local_log);
}



/**
* Calling this fxn puts the session back into a sync'd state. Resets the tracking data, restores
*   the session_state to whatever it was before the desync, and if there are messages in the queues,
*   fires an event to cause the transport to resume pulling from this class.
*
* @param   bool Is the sync state machine pending exit (true), or fully-exited (false)?
*/
void ManuvrSession::mark_session_sync(bool pending) {
  sequential_parse_failures = 0;
  sequential_ack_failures   = 0;
  session_last_state = session_state;               // Stack our session state.

  if (pending) {
    // We *think* we might be done sync'ing...
    session_state = getState() | XENOSESSION_STATE_SYNC_PEND_EXIT;
    sendEvent(Kernel::returnEvent(MANUVR_MSG_SYNC_KEEPALIVE));
  }
  else {
    // We are definately done sync'ing.
    session_state = getState();

    if (!isEstablished()) {
      // When (if) the session syncs, various components in the firmware might
      //   want a message put through.
      mark_session_state(XENOSESSION_STATE_ESTABLISHED);
      //sendEvent(Kernel::returnEvent(MANUVR_MSG_SESS_ESTABLISHED));
      raiseEvent(Kernel::returnEvent(MANUVR_MSG_SELF_DESCRIBE));
      //sendEvent(Kernel::returnEvent(MANUVR_MSG_LEGEND_MESSAGES));
    }
  }

  sync_event.enableSchedule(false);
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
int8_t ManuvrSession::bin_stream_rx(unsigned char *buf, int len) {
  int8_t return_value = 0;

  session_buffer.concat(buf, len);

  const char* statcked_sess_str = getSessionStateString();

  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) {
    local_log.concatf("Bytes received into session 0x%08x buffer: \n\t", (uint32_t) this);
    for (int i = 0; i < len; i++) {
      local_log.concatf("0x%02x ", *(buf + i));
    }
    local_log.concat("\n\n");
  }
  #endif

  switch (getSync()) {   // Consider the top four bits of the session state.
    case XENOSESSION_STATE_SYNC_SYNCD:       // The nominal case. Session is in-sync. Do nothing.
    case XENOSESSION_STATE_SYNC_PEND_EXIT:   // We have exchanged sync packets with the counterparty.
      break;
    case XENOSESSION_STATE_SYNC_INITIATED:   // The counterparty noticed the problem.
    case XENOSESSION_STATE_SYNC_INITIATOR:   // We noticed a problem. We wait for a sync packet...
      /* At this point, we shouldn't be adding to the inbound queue. We should simply add
         to the session buffer and scan it for sync packets. */
      if (scan_buffer_for_sync()) {   // We are getting sync back now.
        /* Since we are going to fall-through into the general parser case, we should reset
           the values that it will use to index and make decisions... */
        mark_session_sync(true);   // Indicate that we are done with sync, but may still see such packets.
        #ifdef __MANUVR_DEBUG
        if (verbosity > 3) local_log.concatf("Session 0x%08x re-sync'd with %d bytes remaining in the buffer. Sync'd state is now pending.\n", (uint32_t) this, len);
        #endif
      }
      else {
        #ifdef __MANUVR_DEBUG
          if (verbosity > 2) local_log.concat("Session still out of sync.\n");
        #endif
        if (local_log.length() > 0) Kernel::log(&local_log);
        return return_value;
      }
      break;
    default:
      #ifdef __MANUVR_DEBUG
      if (verbosity > 1) local_log.concatf("ILLEGAL session_state: 0x%02x (top 4)\n", session_state);
      #endif
      break;
  }


  switch (getState()) {   // Consider the bottom four bits of the session state.
    case XENOSESSION_STATE_UNINITIALIZED:
      break;
    case XENOSESSION_STATE_PENDING_SETUP:
      break;
    case XENOSESSION_STATE_PENDING_AUTH:
      break;
    case XENOSESSION_STATE_ESTABLISHED:
      break;
    case XENOSESSION_STATE_PENDING_HANGUP:
      break;
    case XENOSESSION_STATE_HUNGUP:
      break;
    default:
      #ifdef __MANUVR_DEBUG
      //if (verbosity > 1) local_log.concatf("ILLEGAL session_state: 0x%02x (bottom 4)\n", session_state);
      #endif
      break;
  }

  if (NULL == working) {
    working = XenoMessage::fetchPreallocation(this);
  }

  // If the working message is not in a RECEIVING state, it means something has gone sideways.
  if ((XENO_MSG_PROC_STATE_RECEIVING | XENO_MSG_PROC_STATE_UNINITIALIZED) & working->getState()) {
    int consumed = working->feedBuffer(&session_buffer);
    #ifdef __MANUVR_DEBUG
    if (verbosity > 5) local_log.concatf("Feeding message 0x%08x. Consumed %d of %d bytes.\n", (uint32_t) working, consumed, len);
    #endif

    if (consumed > 0) {
      // Be sure to cull any bytes in the session buffer that were claimed.
      session_buffer.cull(consumed);
    }

    switch (working->getState()) {
      case XENO_MSG_PROC_STATE_AWAITING_PROC:
        // If the message is completed, we can move forward with proc'ing it...
        take_message();
        break;
      case XENO_MSG_PROC_STATE_SYNC_PACKET:
        // T'was a sync packet. Consider our own state and react appropriately.
        XenoMessage::reclaimPreallocation(working);
        working = NULL;
        if (0 == getState()) {
          // If we aren't dealing with sync at the moment, and the counterparty sent a sync packet...
          #ifdef __MANUVR_DEBUG
            if (verbosity > 4) local_log.concat("Counterparty wants to sync,,, changing session state...\n");
          #endif
          mark_session_desync(XENOSESSION_STATE_SYNC_INITIATED);
        }
        break;
      case (XENO_MSG_PROC_STATE_ERROR | XENO_MSG_PROC_STATE_AWAITING_REAP):
        // There was some sort of problem...
        if (verbosity > 3) {
          local_log.concatf("XenoMessage 0x%08x reports an error:\n", (uint32_t) this);
          working->printDebug(&local_log);
        }
        if (MAX_PARSE_FAILURES == ++sequential_parse_failures) {
          #ifdef __MANUVR_DEBUG
            if (verbosity > 2) local_log.concat("\nThis was the session's last straw. Session marked itself as desync'd.\n");
          #endif
          mark_session_desync(XENOSESSION_STATE_SYNC_INITIATOR);   // We will complain.
        }
        else {
          // Send a retransmit request.
        }
        // NOTE: No break.
      case XENO_MSG_PROC_STATE_AWAITING_REAP:
        XenoMessage::reclaimPreallocation(working);
        working = NULL;
        break;
    }
  }
  else {
    if (verbosity > 3) local_log.concatf("XenoMessage 0x%08x is in the wrong state to accept bytes: %s\n", (uint32_t) working, working->getMessageStateString());
  }


  if (statcked_sess_str != getSessionStateString()) {
    // The session changed state. Print it.
    #ifdef __MANUVR_DEBUG
      if (verbosity > 3) local_log.concatf("XenoSession state change:\t %s ---> %s\n", statcked_sess_str, getSessionStateString());
    #endif
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}




/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrSession::getReceiverName() {  return "XenoSession";  }


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrSession::printDebug(StringBuilder *output) {
  output->concatf("-- Sync state           %s\n", getSessionSyncString());
  XenoSession::printDebug(output);
}


/**
* Logging support fxn.
*/
const char* ManuvrSession::getSessionSyncString() {
  switch (getSync()) {
    case XENOSESSION_STATE_SYNC_SYNCD:       return "SYNCD";
    case XENOSESSION_STATE_SYNC_CASTING:     return "CASTING";
    case XENOSESSION_STATE_SYNC_PEND_EXIT:   return "PENDING EXIT";
    case XENOSESSION_STATE_SYNC_INITIATOR:   return "SYNCING (our choice)";
    case XENOSESSION_STATE_SYNC_INITIATED:   return "SYNCING (counterparty choice)";
    default:                                 return "<UNHANDLED SYNC CODE>";
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
int8_t ManuvrSession::bootComplete() {
  EventReceiver::bootComplete();

  sync_event.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG);
  sync_event.isManaged(true);
  sync_event.specific_target = (EventReceiver*) this;
  sync_event.alterScheduleRecurrence(-1);
  sync_event.alterSchedulePeriod(30);
  sync_event.autoClear(false);
  sync_event.enableSchedule(false);

  __kernel->addSchedule(&sync_event);

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
int8_t ManuvrSession::callback_proc(ManuvrRunnable *event) {
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
int8_t ManuvrSession::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    /* General system events */
    case MANUVR_MSG_BT_CONNECTION_LOST:
      session_state = XENOSESSION_STATE_DISCONNECTED;
      //msg_relay_list.clear();
      purgeInbound();
      purgeOutbound();
      #ifdef __MANUVR_DEBUG
      if (verbosity > 3) local_log.concatf("0x%08x Session is now in state %s.\n", (uint32_t) this, getSessionStateString());
      #endif
      return_value++;
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SESS_HANGUP:
      {
        int out_purge = purgeOutbound();
        int in_purge  = purgeInbound();
        #ifdef __MANUVR_DEBUG
        if (verbosity > 5) local_log.concatf("0x%08x Purged (%d) msgs from outbound and (%d) from inbound.\n", (uint32_t) this, out_purge, in_purge);
        #endif
      }
      return_value++;
      break;

    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      sendSyncPacket();
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

    case MANUVR_MSG_SESS_DUMP_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += XenoSession::notify(active_event);
      break;
  }

  /* We don't want to resonate... Don't react to Events that have us as the originator. */
  if (active_event->originator != (EventReceiver*) this) {
    if ((XENO_SESSION_IGNORE_NON_EXPORTABLES) && (active_event->isExportable())) {
      /* This is the block that allows the counterparty to intercept events of its choosing. */

      if (syncd()) {
        if (msg_relay_list.contains(active_event->getMsgDef())) {
          // If we are in this block, it means we need to serialize the event and send it.
          sendEvent(active_event);
          return_value++;
        }
      }
    }
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}
