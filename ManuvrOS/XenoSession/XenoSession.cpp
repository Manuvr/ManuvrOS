/*
File:   XenoSession.cpp
Author: J. Ian Lindsay
Date:   2014.11.20

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


XenoSession is the class that manages dialog with other systems via some
  transport (IRDa, Bluetooth, USB VCP, etc).
     ---J. Ian Lindsay
*/

#include "XenoSession.h"
#include "StaticHub/StaticHub.h"



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

/* This is what a sync packet looks like. Always. So common, we'll hard-code it. */
const uint8_t XenoSession::SYNC_PACKET_BYTES[4] = {0x04, 0x00, 0x00, CHECKSUM_PRELOAD_BYTE};



XenoMessage* XenoSession::fetchPreallocation() {
  XenoMessage* return_value;

  if (0 == preallocated.size()) {
    // We have exhausted our preallocated pool. Note it.
    return_value = new XenoMessage();
    _heap_instantiations++;
  }
  else {
    return_value = preallocated.dequeue();
  }
  return return_value;
}



/**
* At present, our criteria for preallocation is if the pointer address passed in
*   falls within the range of our __prealloc array. I see nothing "non-portable"
*   about this, it doesn't require a flag or class member, and it is fast to check.
* However, this strategy only works for types that are never used in DMA or code
*   execution on the STM32F4. It may work for other architectures (PIC32, x86?).
*   I also feel like it ought to be somewhat slower than a flag or member, but not
*   by such an amount that the memory savings are not worth the CPU trade-off.
* Consider writing all new cyclical queues with preallocated members to use this
*   strategy. Also, consider converting the most time-critical types to this strategy
*   up until we hit the boundaries of the STM32 CCM.
*                                 ---J. Ian Lindsay   Mon Apr 13 10:51:54 MST 2015
* 
* @param Measurement* obj is the pointer to the object to be reclaimed.
*/
void XenoSession::reclaimPreallocation(XenoMessage* obj) {
  uint32_t obj_addr = ((uint32_t) obj);
  uint32_t pre_min  = ((uint32_t) __prealloc_pool);
  uint32_t pre_max  = pre_min + (sizeof(XenoMessage) * PREALLOCATED_XENOMESSAGES);
  
  if ((obj_addr < pre_max) && (obj_addr >= pre_min)) {
    // If we are in this block, it means obj was preallocated. wipe and reclaim it.
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) {
      local_log.concatf("reclaim via prealloc. addr: 0x%08x\n", obj_addr);
      StaticHub::log(&local_log);
    }
    #endif
    obj->wipe();
    preallocated.insert(obj);
  }
  else {
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) {
      local_log.concatf("reclaim via delete. addr: 0x%08x\n", obj_addr);
      StaticHub::log(&local_log);
    }
    #endif
    // We were created because our prealloc was starved. we are therefore a transient heap object.
    _heap_freeds++;
    delete obj;
  }
}






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
*/
XenoSession::XenoSession(ManuvrXport* _xport) {
  __class_initializer();

  /* Populate all the static preallocation slots for messages. */
  for (uint16_t i = 0; i < XENOMESSAGE_PREALLOCATE_COUNT; i++) {
    __prealloc_pool[i].wipe();
    preallocated.insert(&__prealloc_pool[i]);
  }

  // These are messages that we to relay from the rest of the system.
  tapMessageType(MANUVR_MSG_SESS_ESTABLISHED);
  tapMessageType(MANUVR_MSG_SESS_HANGUP);
  tapMessageType(MANUVR_MSG_LEGEND_MESSAGES);
  tapMessageType(MANUVR_MSG_SELF_DESCRIBE);

  owner = _xport;
  
  authed                    = false;
  session_state             = XENOSESSION_STATE_UNINITIALIZED;
  session_last_state        = XENOSESSION_STATE_UNINITIALIZED;
  sequential_parse_failures = 0;
  sequential_ack_failures   = 0;
  initial_sync_count        = 24;
  session_overflow_guard    = true;
  pid_sync_timer            = 0;
  pid_ack_timeout           = 0;
  current_rx_message        = NULL;
  
  _heap_instantiations = 0;
  _heap_freeds = 0;
  
  MAX_PARSE_FAILURES  = 3;  // How many failures-to-parse should we tolerate before SYNCing?
  MAX_ACK_FAILURES    = 3;  // How many failures-to-ACK should we tolerate before SYNCing?
  bootComplete();    // Because we are instantiated well after boot, we call this on construction.
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
XenoSession::~XenoSession() {
  scheduler->disableSchedule(pid_sync_timer);
  scheduler->removeSchedule(pid_sync_timer);

  StaticHub::getInstance()->fetchEventManager()->unsubscribe((EventReceiver*) this);  // Unsubscribe
  
  purgeInbound();  // Need to do careful checks in here for open comm loops.
  purgeOutbound(); // Need to do careful checks in here for open comm loops.

  while (preallocated.dequeue() != NULL);
  
  EventManager::raiseEvent(MANUVR_MSG_SESS_HANGUP, NULL);
}



/**
* Tell the session to relay the given message code.
* We have a list of blacklisted codes that the session is not allowed to subscribe to.
*   The reason for this is to avoid an Event loop. The blacklist might be removed once
*   the EXPORT flag in the Message legend is being respected.
*
* @param   uint16_t The message type code to subscribe the session to.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::tapMessageType(uint16_t code) {
  switch (code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
    case MANUVR_MSG_SESS_DUMP_DEBUG:
//    case MANUVR_MSG_SESS_HANGUP:
      #ifdef __MANUVR_DEBUG
      if (verbosity > 3) StaticHub::log("tapMessageType() tried to tap a blacklisted code.\n");
      #endif
      return -1;
  }

  MessageTypeDef* temp_msg_def = (MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(code);
  if (!msg_relay_list.contains(temp_msg_def)) {
    msg_relay_list.insert(temp_msg_def);
  }
  return 0;
}


/**
* Tell the session not to relay the given message code.
*
* @param   uint16_t The message type code to unsubscribe the session from.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapMessageType(uint16_t code) {
  msg_relay_list.remove((MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(code));
  return 0;
}


/**
* Unsubscribes the session from all optional messages.
* Whatever messages this session was watching for, it will not be after this call.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapAll() {
  msg_relay_list.clear();
  return 0;
}


/**
* Mark a given message as complete and ready for reap.
*
* @param   uint16_t The message unique_id to mark completed.
* @return  0 if nothing done, negative if there was a problem.
*/
int8_t XenoSession::markMessageComplete(uint16_t target_id) {
  XenoMessage *working_xeno;
  for (int i = 0; i < outbound_messages.size(); i++) {
    working_xeno = outbound_messages.get(i);
    if (target_id == working_xeno->unique_id) {
      switch (working_xeno->proc_state) {
        case XENO_MSG_PROC_STATE_AWAITING_REAP:
          outbound_messages.remove(working_xeno);
          if (pid_ack_timeout) {
            // If the message had a timeout (waiting for ACK), we should clean up the schedule.
            scheduler->disableSchedule(pid_ack_timeout);
            scheduler->removeSchedule(pid_ack_timeout);
          }
          reclaimPreallocation(working_xeno);
          return 1;
      }
    }
  }
  return 0;
}




int8_t XenoSession::markSessionConnected(bool conn_state) {
  if (conn_state) {
    mark_session_state(XENOSESSION_STATE_CONNECTED);
    // Barrage the counterparty with sync until they reply in-kind.
    mark_session_desync(XENOSESSION_STATE_SYNC_INITIATOR);
  }
  else {
    mark_session_state(XENOSESSION_STATE_DISCONNECTED);
    
  }
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
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t XenoSession::bootComplete() {
  EventReceiver::bootComplete();
  
  sync_event.repurpose(MANUVR_MSG_SESS_ORIGINATE_MSG);
  sync_event.isManaged(true);
  sync_event.specific_target = (EventReceiver*) this;

  pid_sync_timer = scheduler->createSchedule(30,  -1, false, (EventReceiver*) this, &sync_event);
  scheduler->disableSchedule(pid_sync_timer);

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
int8_t XenoSession::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {

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
int8_t XenoSession::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    /* General system events */
    case MANUVR_MSG_INTERRUPTS_MASKED:
      break;
    case MANUVR_MSG_BT_CONNECTION_LOST:
      session_state = XENOSESSION_STATE_DISCONNECTED;
      //msg_relay_list.clear();
      purgeInbound();
      purgeOutbound();
      #ifdef __MANUVR_DEBUG
      if (verbosity > 3) local_log.concat("Session is now in state XENOSESSION_STATE_DISCONNECTED.\n");
      #endif
      return_value++;
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SESS_HANGUP:
      {
        int out_purge = purgeOutbound();
        int in_purge  = purgeInbound();
        #ifdef __MANUVR_DEBUG
        if (verbosity > 5) local_log.concatf("Purged (%d) msgs from outbound and (%d) from inbound.\n", out_purge, in_purge);
        #endif
      }
      return_value++;
      break;
      
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      sendSyncPacket();
      return_value++;
      break;
      
    case MANUVR_MSG_SESS_DUMP_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  /* We don't want to resonate... Don't react to Events that have us as the callback. */
  if (active_event->callback != (EventReceiver*) this) {
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
  
  if (local_log.length() > 0) StaticHub::log(&local_log);
  return return_value;
}




/****************************************************************************************************
* Functions for managing dialogs and message queues.                                                *
****************************************************************************************************/

/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged. 
*/
int XenoSession::purgeOutbound() {
  int return_value = outbound_messages.size();
  XenoMessage* temp;
  while (outbound_messages.hasNext()) {
    temp = outbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (verbosity > 6) {
      local_log.concat("\nDestroying outbound msg:\n");
      temp->printDebug(&local_log);
      StaticHub::log(&local_log);
    }
    #endif
    delete temp;
    outbound_messages.remove();
  }
  return return_value;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged. 
*/
int XenoSession::purgeInbound() {
  int return_value = inbound_messages.size();
  XenoMessage* temp;
  while (inbound_messages.hasNext()) {
    temp = inbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (verbosity > 6) {
      local_log.concat("\nDestroying inbound msg:\n");
      temp->printDebug(&local_log);
      StaticHub::log(&local_log);
    }
    #endif
    delete temp;
    inbound_messages.remove();
  }
  session_buffer.clear();   // Also purge whatever hanging RX buffer we may have had.
  return return_value;
}



/****************************************************************************************************
* Functions for managing and reacting to sync states.                                               *
****************************************************************************************************/

int8_t XenoSession::sendSyncPacket() {
  if (owner->connected()) {
    StringBuilder sync_packet((unsigned char*) SYNC_PACKET_BYTES, 4);
    owner->sendBuffer(&sync_packet);
    
    ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_XPORT_SEND);
    event->specific_target = owner;  //   event to be the transport that instantiated us.
    raiseEvent(event);
  }
  return 0;
}


/**
* We may decide to send a no-argument packet that demands acknowledgement so that we can...
*  1. Unambiguously recover from a desync state without resorting to timers.
*  2. Periodically ping the counterparty to ensure that we have not disconnected.
*
* The keep-alive system is not handled in the transport because it is part of the protocol.
* A transport might have its own link-layer-appropriate keep-alive mechanism, which can be
*   used in-place of the KA at this (Session) layer. In such case, the Transport class would 
*   carry configuration flags/members that co-ordinate with this class so that the Session
*   doesn't feel the need to use case (2) given above.
*               ---J. Ian Lindsay   Tue Aug 04 23:12:55 MST 2015
*/
int8_t XenoSession::sendKeepAlive() {
  if (owner->connected()) {
    ManuvrEvent* ka_event = EventManager::returnEvent(MANUVR_MSG_SYNC_KEEPALIVE);
    sendEvent(ka_event);

    ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_XPORT_SEND);
    event->specific_target = owner;  //   event to be the transport that instantiated us.
    raiseEvent(event);
  }
  return 0;
}


/**
* Passing an Event into this fxn will cause the Event to be serialized and sent to our counter-party.
* This is the point at which choices are made about what happens to the event's life-cycle.
*/
int8_t XenoSession::sendEvent(ManuvrEvent *active_event) {
  XenoMessage nu_outbound_msg(active_event);
  nu_outbound_msg.expecting_ack = active_event->demandsACK();
  nu_outbound_msg.proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;

  owner->sendBuffer(&(nu_outbound_msg.buffer));
  //outbound_messages.insert(nu_outbound_msg);
  
  // We are about to pass a message across the transport.
  ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_XPORT_SEND);
  event->callback        = this;  // We want the callback and the only receiver of this
  event->specific_target = owner;  //   event to be the transport that instantiated us.
  raiseEvent(event);
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
int8_t XenoSession::scan_buffer_for_sync() {
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
void XenoSession::mark_session_desync(uint8_t ds_src) {
  session_last_state = session_state;               // Stack our session state.
  session_state = getState() | ds_src;
  if (session_state & ds_src) {
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) local_log.concatf("%s\t%s   We are already in the requested sync state. There is no point to entering it again. Doing nothing.\n", __PRETTY_FUNCTION__, getSessionSyncString());
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
  scheduler->enableSchedule(pid_sync_timer);
  
  if (local_log.length() > 0) StaticHub::log(&local_log);
}



/**
* Calling this fxn puts the session back into a sync'd state. Resets the tracking data, restores
*   the session_state to whatever it was before the desync, and if there are messages in the queues,
*   fires an event to cause the transport to resume pulling from this class.
*
* @param   bool Is the sync state machine pending exit (true), or fully-exited (false)? 
*/
void XenoSession::mark_session_sync(bool pending) {
  sequential_parse_failures = 0;
  sequential_ack_failures   = 0;
  session_last_state = session_state;               // Stack our session state.
    
  if (pending) {
    // We *think* we might be done sync'ing...
    session_state = getState() | XENOSESSION_STATE_SYNC_PEND_EXIT;
  }
  else {
    // We are definately done sync'ing.
    session_state = getState();
  }

  if (!isEstablished()) {
    // When (if) the session syncs, various components in the firmware might
    //   want a message put through.
    mark_session_state(XENOSESSION_STATE_ESTABLISHED);
    raiseEvent(EventManager::returnEvent(MANUVR_MSG_SESS_ESTABLISHED));
    raiseEvent(EventManager::returnEvent(MANUVR_MSG_LEGEND_MESSAGES));
    raiseEvent(EventManager::returnEvent(MANUVR_MSG_SELF_DESCRIBE));
  }

  scheduler->disableSchedule(pid_sync_timer);
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
int8_t XenoSession::bin_stream_rx(unsigned char *buf, int len) {
  int8_t return_value = 0;

  session_buffer.concat(buf, len);

  const char* statcked_sess_str = getSessionStateString();

  #ifdef __MANUVR_DEBUG
  if (verbosity > 6) {
    local_log.concat("Bytes received into session buffer: \n\t");
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
        mark_session_sync(false);   // Indicate that we are done with sync, but may still see such packets.
        #ifdef __MANUVR_DEBUG
        if (verbosity > 3) local_log.concatf("Session re-sync'd with %d bytes remaining in the buffer. Sync'd state is now pending.\n", len);
        #endif
      }
      else {
        #ifdef __MANUVR_DEBUG
        if (verbosity > 2) local_log.concat("Session still out of sync.\n");
        StaticHub::log(&local_log);
        #endif
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
    case XENOSESSION_STATE_CONNECTED:
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
      if (verbosity > 1) local_log.concatf("ILLEGAL session_state: 0x%02x (bottom 4)\n", session_state);
      #endif
      break;
  }



  XenoMessage* working = (NULL != current_rx_message) ? current_rx_message : new XenoMessage();
  current_rx_message = working;  // TODO: ugly hack for now to maintain compatibility elsewhere.
  
  int consumed = working->feedBuffer(&session_buffer);
  #ifdef __MANUVR_DEBUG
  if (verbosity > 5) local_log.concatf("Feeding the working_message buffer. Consumed %d of %d bytes.\n", consumed, len);
  #endif

  switch (working->proc_state) {
    case XENO_MSG_PROC_STATE_AWAITING_UNSERIALIZE: //
      #ifdef __MANUVR_DEBUG
      if (verbosity > 3) local_log.concat("About to inflate arguments.\n");
      #endif
      if (working->inflateArgs()) {
        // If we had a problem inflating....
        #ifdef __MANUVR_DEBUG
        if (verbosity > 3) local_log.concat("There was a failure taking arguments apart.\n");
        #endif
        working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP;
      }
      working->proc_state = XENO_MSG_PROC_STATE_AWAITING_PROC;
    case XENO_MSG_PROC_STATE_AWAITING_PROC:  // This message is fully-formed.
      current_rx_message = NULL;
      inbound_messages.insert(working);   // ...and drop it into the inbound message queue.
      if (XENOSESSION_STATE_SYNC_PEND_EXIT & session_state) {   // If we were pending sync, and got this result, we are almost certainly syncd.
        #ifdef __MANUVR_DEBUG
        if (verbosity > 5) local_log.concat("Session became syncd for sure...\n");
        #endif
        mark_session_sync(false);   // Mark it so.
      }
      raiseEvent(working->event);
      break;
    case XENO_MSG_PROC_STATE_SYNC_PACKET:  // This message is a sync packet. 
      current_rx_message = NULL;
      working->wipe();
      if (0 == getState()) {
        // If we aren't dealing with sync at the moment, and the counterparty sent a sync packet...
        #ifdef __MANUVR_DEBUG
        if (verbosity > 4) local_log.concat("Counterparty wants to sync,,, changing session state...\n");
        #endif
        mark_session_desync(XENOSESSION_STATE_SYNC_INITIATED);
      }
      break;
    case XENO_MSG_PROC_STATE_AWAITING_REAP: // Something must have gone wrong.
      #ifdef __MANUVR_DEBUG
      if (verbosity > 2) local_log.concat("About to reap a XenoMessage.\n");
      #endif
      {
        working->printDebug(&local_log);
        if (MAX_PARSE_FAILURES == ++sequential_parse_failures) {
          mark_session_desync(XENOSESSION_STATE_SYNC_INITIATOR);   // We will complain.
          #ifdef __MANUVR_DEBUG
          if (verbosity > 2) local_log.concat("\nThis was the session's last straw. Session marked itself as desync'd.\n");
          #endif
        }
        else {
          // Send a retransmit request.
        }
        working->wipe();
      }
      current_rx_message = NULL;
      break;
    case XENO_MSG_PROC_STATE_RECEIVING: // 
      current_rx_message = working;
      break;
    default:
      #ifdef __MANUVR_DEBUG
      if (verbosity > 1) local_log.concatf("ILLEGAL proc_state: 0x%02x\n", working->proc_state);
      #endif
      break;
  }
  
  if (statcked_sess_str != getSessionStateString()) {
    // The session changed state. Print it.
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) local_log.concatf("XenoSession state change:\t %s ---> %s\n", statcked_sess_str, getSessionStateString());
    #endif
  }
  
  if (local_log.length() > 0) StaticHub::log(&local_log);
  return return_value;
}





/****************************************************************************************************
* Statics...
****************************************************************************************************/


/**
* Scan a buffer for the protocol's sync pattern.
*
* @param buf  The buffer to search through.
* @param len  How far should we go?
* @return The offset of the sync pattern, or -1 if the buffer contained no such pattern.
*/
int XenoSession::contains_sync_pattern(uint8_t* buf, int len) {
  int i = 0;
  while (i < len-3) {
    if (*(buf + i + 0) == XenoSession::SYNC_PACKET_BYTES[0]) {
      if (*(buf + i + 1) == XenoSession::SYNC_PACKET_BYTES[1]) {
        if (*(buf + i + 2) == XenoSession::SYNC_PACKET_BYTES[2]) {
          if (*(buf + i + 3) == XenoSession::SYNC_PACKET_BYTES[3]) {
            return i;
          }
        }
      }
    }
    i++;
  }
  return -1;
}


/**
* Scan a buffer for the protocol's sync pattern, returning the offset of the
*   first byte that breaks the pattern. 
*
* @param buf  The buffer to search through.
* @param len  How far should we go?
* @return The offset of the first byte that is NOT sync-stream.
*/
int XenoSession::locate_sync_break(uint8_t* buf, int len) {
  int i = 0;
  while (i < len-3) {
    if (*(buf + i + 0) != XenoSession::SYNC_PACKET_BYTES[0]) return i;
    if (*(buf + i + 1) != XenoSession::SYNC_PACKET_BYTES[1]) return i;
    if (*(buf + i + 2) != XenoSession::SYNC_PACKET_BYTES[2]) return i;
    if (*(buf + i + 3) != XenoSession::SYNC_PACKET_BYTES[3]) return i;
    i += 4;
  }
  return i;
}




/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
* ▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄ 
* ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
* ▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
* ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ 
****************************************************************************************************/

/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* XenoSession::getReceiverName() {  return "XenoSession";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoSession::printDebug(StringBuilder *output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
  
  output->concatf("--- Session state:   %s\n", getSessionStateString());
  output->concatf("--- Sync state:      %s\n", getSessionSyncString());

  output->concatf("--- Sequential parse failures:  %d\n", sequential_parse_failures);
  output->concatf("--- sequential_ack_failures:    %d\n--- \n", sequential_ack_failures);
  output->concatf("--- __prealloc_pool addres:     0x%08x\n", (uint32_t) __prealloc_pool);
  output->concatf("--- prealloc depth:             %d\n", preallocated.size());
  output->concatf("--- _heap_instantiations:       %u\n", (unsigned long) _heap_instantiations);
  output->concatf("--- _heap_frees:                %u\n", (unsigned long) _heap_freeds);
  
  int ses_buf_len = session_buffer.length();
  if (ses_buf_len > 0) {
    output->concatf("\n--- Session Buffer (%d bytes) --------------------------\n", ses_buf_len);
    for (int i = 0; i < ses_buf_len; i++) {
      output->concatf("0x%02x ", *(session_buffer.string() + i));
    }
    output->concat("\n\n");
  }

  output->concat("\n--- Listening for the following event codes:\n");
  int x = msg_relay_list.size();
  for (int i = 0; i < x; i++) {
    output->concatf("\t%s\n", msg_relay_list.get(i)->debug_label);
  }
  
  x = outbound_messages.size();
  if (x > 0) {
    output->concatf("\n--- Outbound Queue %d total, showing top %d ------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    for (int i = 0; i < min(x, XENO_SESSION_MAX_QUEUE_PRINT); i++) {
        outbound_messages.get(i)->printDebug(output);
    }
  }
  
  x = inbound_messages.size();
  if (x > 0) {
    output->concatf("\n--- Inbound Queue %d total, showing top %d -------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    for (int i = 0; i < min(x, XENO_SESSION_MAX_QUEUE_PRINT); i++) {
      inbound_messages.get(i)->printDebug(output);
    }
  }
  
  if (preallocated.size()) {
    if (preallocated.get()->bytes_received > 0) {
      output->concat("\n--- XenoMessage in process  ----------------------------\n");
      preallocated.get()->printDebug(output);
    }
  }
}


/**
* Logging support fxn.
*/
const char* XenoSession::getSessionStateString() {
  switch (getState()) {
    case XENOSESSION_STATE_UNINITIALIZED:    return "UNINITIALIZED";
    case XENOSESSION_STATE_CONNECTED:        return "CONNECTED";
    case XENOSESSION_STATE_PENDING_SETUP:    return "PENDING_SETUP";
    case XENOSESSION_STATE_PENDING_AUTH:     return "PENDING_AUTH";
    case XENOSESSION_STATE_ESTABLISHED:      return "ESTABLISHED";
    case XENOSESSION_STATE_PENDING_HANGUP:   return "HANGUP IN PROGRESS";
    case XENOSESSION_STATE_HUNGUP:           return "HUNGUP";
    case XENOSESSION_STATE_DISCONNECTED:     return "DISCONNECTED";
    default:                                 return "<UNKNOWN SESSION STATE>";
  }
}

/**
* Logging support fxn.
*/
const char* XenoSession::getSessionSyncString() {
  switch (getSync()) {
    case XENOSESSION_STATE_SYNC_SYNCD:       return "SYNCD";
    case XENOSESSION_STATE_SYNC_CASTING:     return "CASTING";
    case XENOSESSION_STATE_SYNC_PEND_EXIT:   return "PENDING EXIT";
    case XENOSESSION_STATE_SYNC_INITIATOR:   return "SYNCING (our choice)";
    case XENOSESSION_STATE_SYNC_INITIATED:   return "SYNCING (counterparty choice)";
    default:                                 return "<UNHANDLED SYNC CODE>";
  }
}


void XenoSession::procDirectDebugInstruction(StringBuilder *input) {
  uint8_t temp_byte = 0;
  
  char* str = input->position(0);
  
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'S':  // Send a mess of sync packets.
      initial_sync_count = 24;
      if (scheduler) {
        scheduler->alterScheduleRecurrence(pid_sync_timer, (int16_t) initial_sync_count);
        scheduler->fireSchedule(pid_sync_timer);
      }
      break;
    case 'i':  // Send a mess of sync packets.
      if (1 == temp_byte) {
      }
      else if (2 == temp_byte) {
      }
      else {
        printDebug(&local_log);
      }
      break;
    case 'q':  // Manual message queue purge.s
      purgeOutbound();
      purgeInbound();
      break;
    case 'w':  // Manual session poll.
      EventManager::raiseEvent(MANUVR_MSG_SESS_ORIGINATE_MSG, NULL);
      break;
      

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


