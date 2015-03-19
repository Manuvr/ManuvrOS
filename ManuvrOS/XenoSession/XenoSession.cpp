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
#ifndef TEST_BENCH
  #include "StaticHub/StaticHub.h"
#else
  #include "demo/StaticHub.h"
#endif

/* This is what a sync packet looks like. Always. So common, we'll hard-code it. */
const uint8_t XenoSession::SYNC_PACKET_BYTES[4] = {0x04, 0x00, 0x00, CHECKSUM_PRELOAD_BYTE};


void oneshot_session_sync_send() {
  EventManager::raiseEvent(MANUVR_MSG_SESS_ORIGINATE_MSG, NULL);
}



/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*/
XenoSession::XenoSession() {
  __class_initializer();
	StaticHub *sh = StaticHub::getInstance();
	scheduler = sh->fetchScheduler();
	sh->fetchEventManager()->subscribe((EventReceiver*) this);  // Subscribe to the EventManager.

	while (preallocated.size() < XENOMESSAGE_PREALLOCATE_COUNT) {
	  preallocated.insert(new XenoMessage());
	}

	tapMessageType(MANUVR_MSG_SESS_ESTABLISHED);
	tapMessageType(MANUVR_MSG_SESS_HANGUP);
	tapMessageType(MANUVR_MSG_LEGEND_MESSAGES);
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
XenoSession::~XenoSession() {
	StaticHub::getInstance()->fetchEventManager()->unsubscribe((EventReceiver*) this);  // Subscribe to the EventManager.
	while (preallocated.size() > 0) {
	  delete preallocated.get();
	  preallocated.remove();
	}
	purgeInbound();  // Need to do careful checks in here for open comm loops.
	purgeOutbound(); // Need to do careful checks in here for open comm loops.
}


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void XenoSession::__class_initializer() {
	authed                    = false;
	session_state             = XENOSESSION_STATE_UNINITIALIZED;
	session_last_state        = XENOSESSION_STATE_UNINITIALIZED;
	sequential_parse_failures = 0;
	sequential_ack_failures   = 0;
	initial_sync_count        = 24;
	session_overflow_guard    = true;
	pid_sync_timer            = 0;
	pid_ack_timeout           = 0;
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
      if (verbosity > 3) StaticHub::log("XenoSession::tapMessageType() tried to tap a blacklisted code.\n");
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
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::markMessageComplete(uint16_t target_id) {
  XenoMessage *working_xeno;
  for (int i = 0; i < outbound_messages.size(); i++) {
    working_xeno = outbound_messages.get(i);
    if (target_id == working_xeno->unique_id) {
      switch (working_xeno->proc_state) {
        case XENO_MSG_PROC_STATE_AWAITING_REAP:
          outbound_messages.remove(working_xeno);
          delete working_xeno;
          return 1;
      }
    }
  }
  return 0;
}




int8_t XenoSession::markSessionConnected(bool conn_state) {
  mark_session_state(conn_state ? XENOSESSION_STATE_CONNECTED : XENOSESSION_STATE_DISCONNECTED);
  return 0;
}


/**
* The transport calls this fxn to get data to send.
*
* Returns 1 on message written.
* Returns 0 on nothing done.
* Returns -1 on failure.
*
* @param   unsigned char** The location of the buffer.
* @param   uint32_t*       The length of the buffer.
* @return  uint16_t        0 if we have nothing for the transport, 1 if we return a sync packet, 
*            and the message unique_id if we return non-trivial data.
*/
//uint16_t XenoSession::nextMessage(unsigned char **buffer, uint32_t *b_len) {
//  if (!isEstablished()) {
//    if (verbosity > 2) StaticHub::log("XenoSession::nextMessage() returning 0 due to no initialization.\n");
//    return 0;
//  }
//  
//  if (syncd()) {   // Don't pull from the queue if we are not syncd. 
//    int q_size = outbound_messages.size();
//    XenoMessage *working;
//    for (int i = 0; i < q_size; i++) {
//      working = outbound_messages.get(i);
//      if (NULL != working) {
//        if (XENO_MSG_PROC_STATE_AWAITING_SEND == working->proc_state) {
//          *(buffer) = working->buffer.string();
//          uint32_t temp_32 = working->buffer.length();
//          *(b_len)  = temp_32;
//          if (working->expecting_ack) {
//            working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REPLY;
//          }
//          else {
//            working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP;
//          }
//          //local_log.concatf("XenoSession:nextMessage():\t %d / %d (%d)\n", temp_32, *b_len, *(b_len));
//          //StaticHub::log(&local_log);
//          return working->unique_id;
//        }
//      }
//    }
//  }
//  else {    // If we are not sync'd, send a sync packet.
//    *buffer = SYNC_PACKET_RAW;
//    *b_len = 4;
//    if (verbosity > 2) StaticHub::log("XenoSession::nextMessage() returning sync packet.\n");
//    return 1;
//  }
//  if (verbosity > 2) StaticHub::log("XenoSession::nextMessage() returning 0.\n");
//  return 0;
//}


/**
* The transport calls this fxn to get data to send.
*
* Returns 1 on message written.
* Returns 0 on nothing done.
* Returns -1 on failure.
*
* @param   StringBuilder*  The location of the buffer.
* @return  uint16_t        0 if we have nothing for the transport, 1 if we return a sync packet, 
*            and the message unique_id if we return non-trivial data.
*/
uint16_t XenoSession::nextMessage(StringBuilder* buffer) {
  if (!isEstablished()) {
    if (verbosity > 2) local_log.concat("XenoSession::nextMessage() returning 0 due to no initialization.\n");
    return 0;
  }
  
  if (syncd()) {   // Don't pull from the queue if we are not syncd.
    int q_size = outbound_messages.size();
    XenoMessage *working;
    for (int i = 0; i < q_size; i++) {
      working = outbound_messages.get(i);
      if (NULL != working) {
        if (XENO_MSG_PROC_STATE_AWAITING_SEND == working->proc_state) {
          buffer->concat(&working->buffer);
          if (working->expecting_ack) {
            working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REPLY;
          }
          else {
            working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP;
            markMessageComplete(working->unique_id);  // TODO: bad.... dangerous...
          }
          //local_log.concatf("XenoSession:nextMessage():\t %d / %d (%d)\n", temp_32, *b_len, *(b_len));
          //StaticHub::log(&local_log);
          return working->unique_id;
        }
      }
    }
  }
  else {    // If we are not sync'd, send a sync packet.
    buffer->concat((unsigned char*) SYNC_PACKET_BYTES, 4);
    if (verbosity > 2) local_log.concat("XenoSession::nextMessage() returning sync packet.\n");
    if (initial_sync_count) {
      scheduler->fireSchedule(pid_sync_timer);
      initial_sync_count--;
    }
    return 1;
  }
  if (verbosity > 2) local_log.concat("XenoSession::nextMessage() returning 0.\n");
  
  if (local_log.length() > 0) StaticHub::log(&local_log);
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


int8_t XenoSession::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  /* We don't want to resonate... Don't react to Events that have us as the callback. */
  if (active_event->callback != (EventReceiver*) this) {
    switch (active_event->event_code) {
      /* General system events */
      case MANUVR_MSG_INTERRUPTS_MASKED:
        break;
      case MANUVR_MSG_BT_CONNECTION_LOST:
        session_state = XENOSESSION_STATE_DISCONNECTED;
        //msg_relay_list.clear();
        purgeInbound();
        purgeOutbound();
        if (verbosity > 3) local_log.concat("Session is now in state XENOSESSION_STATE_DISCONNECTED.\n");
        return_value++;
        break;

  
      /* Things that only this class is likely to care about. */
      case MANUVR_MSG_SESS_HANGUP:
        if (verbosity > 5) local_log.concatf("Purged (%d) XenoMessages from outbound and (%d) from inbound.\n", purgeOutbound(), purgeInbound());
        return_value++;
        break;
      case MANUVR_MSG_SESS_DUMP_DEBUG:
        printDebug(&local_log);
        return_value++;
        break;
      // TODO: Wrap into SELF_DESCRIBE
      //case MANUVR_MSG_VERSION_PROTOCOL:
      //  if (0 == active_event->args.size()) {
      //    active_event->addArg((uint32_t) PROTOCOL_VERSION);
      //    if (verbosity > 5) local_log.concatf("Added argument to PROTOCOL_VERSION request\n");
      //    return_value++;
      //  }
      //  else {
      //    if (verbosity > 3) local_log.concatf("Found PROTOCOL_VERSION request, but it already has an argument.\n");
      //  }
      //  break;
      default:
        return_value += EventReceiver::notify(active_event);
        break;
    }
  }
  
  if ((XENO_SESSION_IGNORE_NON_EXPORTABLES) && (active_event->isExportable())) {
    /* This is the block that allows the counterparty to intercept events of its choosing. */
    if (msg_relay_list.contains(active_event->getMsgDef())) {
      // If we are in this block, it means we need to serialize the event and send it.
      XenoMessage* nu_outbound_msg = new XenoMessage(active_event);
    
      nu_outbound_msg->expecting_ack = false;
      nu_outbound_msg->proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
      //TODO: Should at this point dump the serialized data into a BT data message or something.
      // for now, we're just going to dump it to the console.
      // TODOING: Hopefully this will fly.
      // TODONE: It flew.
      outbound_messages.insert(nu_outbound_msg);
      ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_SESS_ORIGINATE_MSG);
      raiseEvent(event);
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
		if (verbosity > 6) {
		  local_log.concat("\nDestroying outbound message:\n");
		  temp->printDebug(&local_log);
		  StaticHub::log(&local_log);
		}
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
		if (verbosity > 6) {
		  local_log.concat("\nDestroying inbound message:\n");
		  temp->printDebug(&local_log);
		  StaticHub::log(&local_log);
		}
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
  oneshot_session_sync_send();
  return 0;
}


int8_t XenoSession::sendEvent(ManuvrEvent *active_event) {
  XenoMessage* nu_outbound_msg = new XenoMessage(active_event);

  nu_outbound_msg->expecting_ack = false;
  nu_outbound_msg->proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;

  outbound_messages.insert(nu_outbound_msg);
  ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_SESS_ORIGINATE_MSG);
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
  session_state = (session_state & 0x0F) | ds_src;
  if (session_state & ds_src) {
    if (verbosity > 3) local_log.concatf("%s\t%s   We are already in the requested sync state. There is no point to entering it again. Doing nothing.\n", __PRETTY_FUNCTION__, getSessionSyncString());
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
        if (verbosity > 3) local_log.concat("We were called.\n");
        break;
    }
    
    if (pid_sync_timer) {
      scheduler->enableSchedule(pid_sync_timer);
    }
    else if (NULL == scheduler) {
      //StaticHub::log("Tried to find the scheduler and could not. Session is not sending sync packets, despite being in a de-sync state.\n");
      scheduler = StaticHub::getInstance()->fetchScheduler();
      if ((NULL != scheduler) && (0 == pid_sync_timer)) {
        pid_sync_timer = scheduler->createSchedule(20,  -1, false, oneshot_session_sync_send);
      }
    }
  }
  
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
    session_state = (session_state & 0x0F) | XENOSESSION_STATE_SYNC_PEND_EXIT;
  }
  else {
    // We are definately done sync'ing.
    session_state = session_state & 0x0F;
  }

  if (NULL == scheduler) {
    scheduler = StaticHub::getInstance()->fetchScheduler();
    if ((NULL != scheduler) && (0 == pid_sync_timer)) {
      pid_sync_timer = scheduler->createSchedule(20,  -1, false, oneshot_session_sync_send);
    }
  }
  
  if (0 != pid_sync_timer) {
    scheduler->enableSchedule(pid_sync_timer);
  }
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

  if (verbosity > 6) {
    local_log.concat("Bytes received into session buffer: \n\t");
    for (int i = 0; i < len; i++) {
      local_log.concatf("0x%02x ", *(buf + i));
    }
    local_log.concat("\n\n");
  }

  switch (session_state & 0xF0) {   // Consider the top four bits of the session state.
    case 0:       // The nominal case. Session is in-sync. Do nothing.
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
        if (verbosity > 3) local_log.concatf("Session re-sync'd with %d bytes remaining in the buffer. Sync'd state is now pending.\n", len);
      }
      else {
        if (verbosity > 2) local_log.concat("Session still out of sync.\n");
        StaticHub::log(&local_log);
        return return_value;
      }
      break;
    default:
      if (verbosity > 1) local_log.concatf("ILLEGAL session_state: 0x%02x (top 4)\n", session_state);
      break;
  }


  switch (session_state & 0x0F) {   // Consider the bottom four bits of the session state.
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
      if (verbosity > 1) local_log.concatf("ILLEGAL session_state: 0x%02x (bottom 4)\n", session_state);
      break;
  }



  XenoMessage* working;
  if (preallocated.size() > 0) {
    working = preallocated.get();
  }
  else {
    // If we had an empty prealloc, then instantiate and add to the prealloc
    //   so that the next time we pass through here, we won't leave a partial
    //   message hanging.
    working = new XenoMessage();
    preallocated.insert(working);
  }
  
  int consumed = working->feedBuffer(&session_buffer);
  if (verbosity > 5) local_log.concatf("Feeding the working_message buffer. Consumed %d of %d bytes.\n", consumed, len);

  switch (working->proc_state) {
    case XENO_MSG_PROC_STATE_AWAITING_UNSERIALIZE: // 
      if (verbosity > 3) local_log.concat("About to inflate arguments.\n");
      if (working->inflateArgs()) {
        // If we had a problem inflating....
        if (verbosity > 3) local_log.concat("There was a failure taking arguments apart.\n");
        working->proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP;
      }
      working->proc_state = XENO_MSG_PROC_STATE_AWAITING_PROC;
    case XENO_MSG_PROC_STATE_AWAITING_PROC:  // This message is fully-formed.
      preallocated.remove(working);       // Take it out of the preallocated queue...
      inbound_messages.insert(working);   // ...and drop it into the inbound message queue.
      if (XENOSESSION_STATE_SYNC_PEND_EXIT & session_state) {   // If we were pending sync, and got this result, we are almost certainly syncd.
        if (verbosity > 5) local_log.concat("Session became syncd for sure...\n");
        mark_session_sync(false);   // Mark it so.
      }
      raiseEvent(working->event);
      break;
    case XENO_MSG_PROC_STATE_SYNC_PACKET:  // This message is a sync packet. 
      working->wipe();
      if (0 == (session_state & 0x0F)) {
        // If we aren't dealing with sync at the moment, and the counterparty sent a sync packet...
        if (verbosity > 4) local_log.concat("Counterparty wants to sync,,, changing session state...\n");
        mark_session_desync(XENOSESSION_STATE_SYNC_INITIATED);
      }
      break;
    case XENO_MSG_PROC_STATE_AWAITING_REAP: // Something must have gone wrong.
      {
        if (verbosity > 2) local_log.concat("About to reap a XenoMessage.\n");
        working->printDebug(&local_log);
        if (MAX_PARSE_FAILURES == ++sequential_parse_failures) {
          mark_session_desync(XENOSESSION_STATE_SYNC_INITIATOR);   // We will complain.
          if (verbosity > 2) local_log.concat("\nThis was the session's last straw. Session marked itself as desync'd.\n");
        }
        else {
          // Send a retransmit request.
        }
        working->wipe();
      }
      break;
    case XENO_MSG_PROC_STATE_RECEIVING: // 
      break;
    default:
      if (verbosity > 1) local_log.concatf("ILLEGAL proc_state: 0x%02x\n", working->proc_state);
      break;
  }
  
  if (statcked_sess_str != getSessionStateString()) {
    // The session changed state. Print it.
    if (verbosity > 3) local_log.concatf("XenoSession state change:\t %s ---> %s\n", statcked_sess_str, getSessionStateString());
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
	output->concatf("--- sequential_ack_failures:    %d\n", sequential_ack_failures);
	
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
  switch (session_state & 0x0F) {
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
  switch (session_state & 0xF0) {
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

    default:
      local_log.concat("No case in XenoSession debug.\n\n");
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


