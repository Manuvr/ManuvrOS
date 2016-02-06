/*
File:   XenoMessage.cpp
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


XenoMessage is the class that is the interface between ManuvrRunnables and
  XenoSessions. 
     ---J. Ian Lindsay
*/

#include "XenoSession.h"
#include <ManuvrOS/Kernel.h>
#include <ManuvrOS/Platform/Platform.h>



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
const uint8_t XenoMessage::SYNC_PACKET_BYTES[4] = {0x04, 0x00, 0x00, CHECKSUM_PRELOAD_BYTE};

// TODO: This is probably useless except in debug builds.
uint32_t XenoMessage::_heap_instantiations = 0;
uint32_t XenoMessage::_heap_freeds         = 0;

uint16_t _ring_buf_idx = 0; 

XenoMessage XenoMessage::__prealloc_pool[XENOMESSAGE_PREALLOCATE_COUNT];

XenoMessage* XenoMessage::fetchPreallocation(XenoSession* _ses) {
  XenoMessage* return_value;

  for (int i = 0; i < XENOMESSAGE_PREALLOCATE_COUNT; i++) {
    if (XENO_MSG_PROC_STATE_UNINITIALIZED == __prealloc_pool[_ring_buf_idx % XENOMESSAGE_PREALLOCATE_COUNT].getState()) {
      __prealloc_pool[_ring_buf_idx % XENOMESSAGE_PREALLOCATE_COUNT].claim(_ses);
      return &__prealloc_pool[_ring_buf_idx++ % XENOMESSAGE_PREALLOCATE_COUNT];
    }
    _ring_buf_idx++;
  }

  // We have exhausted our preallocated pool. Note it.
  return_value = new XenoMessage();
  return_value->claim(_ses);
  _heap_instantiations++;
  return return_value;
}


/**
* At present, our criteria for preallocation is if the pointer address passed in
*   falls within the range of our __prealloc_pool array. I see nothing "non-portable"
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
* @param XenoMessage* obj is the pointer to the object to be reclaimed.
*/
void XenoMessage::reclaimPreallocation(XenoMessage* obj) {
  uint32_t obj_addr = ((uint32_t) obj);
  uint32_t pre_min  = ((uint32_t) __prealloc_pool);
  uint32_t pre_max  = pre_min + (sizeof(XenoMessage) * XENOMESSAGE_PREALLOCATE_COUNT);
  
  if ((obj_addr < pre_max) && (obj_addr >= pre_min)) {
    // If we are in this block, it means obj was preallocated. wipe and reclaim it.
    #ifdef __MANUVR_DEBUG
      StringBuilder local_log;
      local_log.concatf("reclaim via prealloc. addr: 0x%08x\n", obj_addr);
      Kernel::log(&local_log);
    #endif
    obj->wipe();
  }
  else {
    #ifdef __MANUVR_DEBUG
      StringBuilder local_log;
      local_log.concatf("reclaim via delete. addr: 0x%08x\n", obj_addr);
      Kernel::log(&local_log);
    #endif
    // We were created because our prealloc was starved. we are therefore a transient heap object.
    _heap_freeds++;
    delete obj;
  }
}


/**
* Scan a buffer for the protocol's sync pattern.
*
* @param buf  The buffer to search through.
* @param len  How far should we go?
* @return The offset of the sync pattern, or -1 if the buffer contained no such pattern.
*/
int XenoMessage::contains_sync_pattern(uint8_t* buf, int len) {
  int i = 0;
  while (i < len-3) {
    if (*(buf + i + 0) == XenoMessage::SYNC_PACKET_BYTES[0]) {
      if (*(buf + i + 1) == XenoMessage::SYNC_PACKET_BYTES[1]) {
        if (*(buf + i + 2) == XenoMessage::SYNC_PACKET_BYTES[2]) {
          if (*(buf + i + 3) == XenoMessage::SYNC_PACKET_BYTES[3]) {
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
int XenoMessage::locate_sync_break(uint8_t* buf, int len) {
  int i = 0;
  while (i < len-3) {
    if (*(buf + i + 0) != XenoMessage::SYNC_PACKET_BYTES[0]) return i;
    if (*(buf + i + 1) != XenoMessage::SYNC_PACKET_BYTES[1]) return i;
    if (*(buf + i + 2) != XenoMessage::SYNC_PACKET_BYTES[2]) return i;
    if (*(buf + i + 3) != XenoMessage::SYNC_PACKET_BYTES[3]) return i;
    i += 4;
  }
  return i;
}


/****************************************************************************************************
*   ___ _              ___      _ _              _      _       
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___ 
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

XenoMessage::XenoMessage() {
  wipe();
}


XenoMessage::XenoMessage(ManuvrRunnable* existing_event) {
  wipe();
  // Should maybe set a flag in the event to indicate that we are now responsible
  //   for memory upkeep? Don't want it to get jerked out from under us and cause a crash.
  provideEvent(existing_event);
}


XenoMessage::~XenoMessage() {
  if (NULL != event) {
    // TODO: Now we are worried about this.
    event = NULL;
  }
}


/**
* Sometimes we might want to re-use this allocated object rather than free it.
* Do not change the unique_id. One common use-case for this fxn is to reply to a message.
*/
void XenoMessage::wipe() {
  session         = NULL;
  proc_state      = XENO_MSG_PROC_STATE_UNINITIALIZED;
  checksum_c      = CHECKSUM_PRELOAD_BYTE;     // The checksum of the data that we calculate.
  retries         = 0;     // How many times have we retried this packet?
  bytes_received  = 0;     // How many bytes of this command have we received? Meaningless for the sender.
  arg_count       = 0;

  unique_id       = 0;
  bytes_total     = 0;     // How many bytes does this message occupy?
  checksum_i      = 0;     // The checksum of the data that we receive.
  message_code    = 0;     //
  
  if (NULL != event) {
    // TODO: Now we are worried about this.
    event = NULL;
  }

  millis_at_begin = 0;     // This is the milliseconds reading when we sent.
  time_created    = millis();
}


/**
* Calling this fxn will cause this Message to be populated with the given Event and unique_id.
* Calling this converts this XenoMessage into an outbound, and has the same general effect as
*   calling the constructor with an Event argument.
* TODO: For safety's sake, the Event is not retained. This has caused us some grief. Re-evaluate...
*
* @param   ManuvrRunnable* The Event that is to be communicated.
* @param   uint16_t          An explicitly-provided unique_id so that a dialog can be perpetuated.
*/
void XenoMessage::provideEvent(ManuvrRunnable *existing_event, uint16_t manual_id) {
  event = existing_event;
  unique_id = manual_id;
  message_code = event->event_code;                // 
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;  // Implies we are sending.
}


/**
* We need to ACK certain messages. This converts this message to an ACK of the message
*   that it used to be. Then it can be simply fed back into the outbound queue.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::ack() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY);
  provideEvent(&temp_event, unique_id);
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  return 0;
}


/**
* Calling this sends a message to the counterparty asking them to retransmit the same message. 
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::retry() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY_RETRY);
  provideEvent(&temp_event, unique_id);
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  retries++;
  return 0;
}


/**
* Calling this will mark this message as critical fail and send a reply to the counterparty
*   telling them that we can't cope with it.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::fail() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY_FAIL);
  provideEvent(&temp_event, unique_id);
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  return 0;
}



/**
* This method is called to flatten this message (and its Event) into a string
*   so that the session can provide it to the transport.
*
* @return  The total size of the string that is meant for the transport,
*            or -1 if something went wrong.
*/
int XenoMessage::serialize(StringBuilder* buffer) {
  if (NULL == event) {
    return 0;
  }
  
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
  int arg_count  = event->serialize(buffer);
  if (arg_count >= 0) {
    bytes_total = (uint32_t) buffer->length() + 8;   // +8 because: checksum byte
    unsigned char* prepend_array = (unsigned char*) alloca(8);

    *(prepend_array + 4) = (uint8_t) (unique_id & 0xFF);
    *(prepend_array + 5) = (uint8_t) (unique_id >> 8);
    *(prepend_array + 6) = (uint8_t) (event->event_code & 0xFF);
    *(prepend_array + 7) = (uint8_t) (event->event_code >> 8);
    
    // Calculate and append checksum.
    uint8_t checksum_temp = CHECKSUM_PRELOAD_BYTE;
    checksum_temp = (uint8_t) checksum_temp + *(prepend_array + 4);
    checksum_temp = (uint8_t) checksum_temp + *(prepend_array + 5);
    checksum_temp = (uint8_t) checksum_temp + *(prepend_array + 6);
    checksum_temp = (uint8_t) checksum_temp + *(prepend_array + 7);
    
    unsigned char* full_xport_array = buffer->string();
    for (int i = 0; i < buffer->length(); i++) {
      checksum_temp = (uint8_t) checksum_temp + *(full_xport_array + i);
    }
    checksum_c = checksum_temp;
    
    *(prepend_array + 0) = (uint8_t) (bytes_total & 0xFF);
    *(prepend_array + 1) = (uint8_t) (bytes_total >> 8);
    *(prepend_array + 2) = (uint8_t) (bytes_total >> 16);
    *(prepend_array + 3) = (uint8_t) checksum_temp;

    buffer->prepend(prepend_array, 8);
  }

  return buffer->length();
}



/**
* This function should be called by the session to feed bytes to a message.
*
* @return  The number of bytes consumed, or a negative value on failure.
*/
int XenoMessage::feedBuffer(StringBuilder *sb_buf) {
  if (NULL == sb_buf) return 0;

  StringBuilder output;
  int return_value = 0;
  int buf_len  = sb_buf->length();
  uint8_t* buf = (uint8_t*) sb_buf->string();

  /* Ok... by this point, we know we are eligible to receive data. So stop worrying about that. */
  if (0 == bytes_received) {      // If we haven't been fed any bytes yet...
    if (buf_len < 4) {
      // Not enough to act on, since we have none buffered ourselves.
      // Return 0 to indicate we've taken no bytes, and have not errored.
      return 0;
    }
    else {  // We have at least enough for a sync-check...
      if (0 == XenoMessage::contains_sync_pattern(buf, 4)) {
        // Get the offset of the last instance in this sync-stream relative to (buf+0) (we already know there is at least one).
        int x = XenoMessage::locate_sync_break(buf+4, buf_len-4) + 4;
        #ifdef __MANUVR_DEBUG
          output.concatf("About to cull %d bytes of sync stream from the buffer..\n", x);
        #endif
        proc_state = XENO_MSG_PROC_STATE_SYNC_PACKET;  // Mark ourselves as a sync packet.
        bytes_received = 4;
        
        if (output.length() > 0) Kernel::log(&output);
        return x;
      }
      else {
        // It isn't sync, and we have at least 4 bytes. Let's take those...
        bytes_total = parseUint32Fromchars(buf);
        checksum_i  = (bytes_total & 0xFF000000) >> 24;
        bytes_total = (bytes_total & 0x00FFFFFF);
        bytes_received += 4;
        return_value   += 4;
      }
    }
    
    /* Adjust buffer for down-stream code. */
    buf_len -= 4;
    buf     += 4;
  }
  

  /* Partial-packet case... */
  if (4 == bytes_received) {
    // Do we have a whole minimum packet yet?
    if (buf_len < 4) {
      // Not enough to act on.
      if (output.length() > 0) Kernel::log(&output);
      return return_value;
    }
    else {
      // Take the bytes remaining 
      unique_id = parseUint16Fromchars(buf);
      buf += 2;
      message_code = parseUint16Fromchars(buf);
      bytes_received += 4;
      return_value   += 4;
      
      // TODO: Now that we have a message code, we can go get a message def, and possible arg
      //   forms in anticipation of the parse being accurate and the message valid. Not sure if
      //   that is a good idea at this point.    ---J. Ian Lindsay   Thu Feb 04 12:01:54 PST 2016

      /* Adjust buffer for down-stream code. */
      buf += 2;
      buf_len -= 4;
    }
  }


  /* Do we have the whole packet yet? */
  int bytes_remaining = bytesRemaining();
  if ((bytes_received >= 8) && (buf_len >= bytes_remaining)) { 
    /* We might be done... */
    StringBuilder argbuf(buf, bytes_remaining);
    bytes_received += bytes_remaining;
    return_value   += bytes_remaining;
    buf_len        -= bytes_remaining;
    buf            += bytes_remaining;

    // Test the checksum...
    checksum_c = (checksum_c + (unique_id >> 8)    + (unique_id    & 0x00FF) ) % 256;
    checksum_c = (checksum_c + (message_code >> 8) + (message_code & 0x00FF) ) % 256;
    uint8_t* temp    = argbuf.string();
    uint8_t temp_len = argbuf.length();
    
    for (int i = 0; i < temp_len; i++) checksum_c = (checksum_c + *(temp+i) ) % 256;

    if (checksum_c == checksum_i) {
      // Checksum passes. Build the event.
      if (event != NULL) {
        #ifdef __MANUVR_DEBUG
        output.concat("XenoMessage::feedBuffer(): Ooops. Clobbered a runnable pointer. Expect leaks...\n");
        #endif
      }

      event = Kernel::returnEvent(message_code);

      if (event->event_code) {
        proc_state = XENO_MSG_PROC_STATE_AWAITING_PROC;
        // The event code was found. Do something about it.
        if (temp_len > 0) {
          if (event->inflateArgumentsFromBuffer(temp, temp_len) <= 0) {
            output.concat("XenoMessage::inflateArgs():\t inflate fxn returned failure...\n");
          }
        }
        else {
          output.concat("XenoMessage::inflateArgs():\t argbuf was zero-length.\n");
        }
      }
      else {
        /* By convention, 0x0000 is the message code for "Undefined event". In this case it means that
             we understood what our counterparty said, but ve used a phrase or idiom that we don't 
             understand. We might at this point choose to do any of the following:
           a) Check against an independently-tracked message legend in a session somewhere.
           b) Ask the counterparty for a message legend if we don't have one yet.
           c) NACK the message with a fail condition so he quits using that idiom.
        */
        proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP | XENO_MSG_PROC_STATE_ERROR;
        #ifdef __MANUVR_DEBUG
          output.concatf("XenoMessage::feedBuffer(): Couldn't find a message definition for the code 0x%04x.\n", message_code);
        #endif
      }
    }
    else {
      // TODO: We might send a retry request at this point...
      proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP | XENO_MSG_PROC_STATE_ERROR;
      #ifdef __MANUVR_DEBUG
        output.concatf("XenoMessage::feedBuffer() Message failed to checksum. Got 0x%02x. Expected 0x%02x. \n", checksum_c, checksum_i);
      #endif
    }
  }
  
  if (output.length() > 0) Kernel::log(&output);
  return return_value;
}




/**
* Is this a reply to a message we previously sent? Only considers the Message ID.
*
* @return  true if this message is a reply to a prior message.
*/
bool XenoMessage::isReply() {
  if (NULL == event) return false;
  switch (event->event_code) {
    case MANUVR_MSG_REPLY:
    case MANUVR_MSG_REPLY_RETRY:
    case MANUVR_MSG_REPLY_FAIL:
      return true;
    default:
      return false;
  }
}


/**
* Does this message type demand an ACK?
*
* @return  true if this message demands an ACK.
*/
bool XenoMessage::expectsACK() {
  if (NULL != event) {
    return event->demandsACK();
  }
  return false;
}




/**
* Called by a session object to claim the message.
*
* @param   XenoSession* The session that is claiming us.
*/
void XenoMessage::claim(XenoSession* _ses) {
  proc_state = XENO_MSG_PROC_STATE_RECEIVING;
  session = _ses;
}



/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoMessage::printDebug(StringBuilder *output) {
  if (NULL == output) return;

  output->concatf("\t Message ID      0x%08x\n", (uint32_t) this);
  if (NULL != event) {
    output->concatf("\t Message type    %s\n", event->getMsgTypeString());
  }
  output->concatf("\t Message state   %s\n", getMessageStateString());
  output->concatf("\t unique_id       0x%04x\n", unique_id);
  output->concatf("\t checksum_i      0x%02x\n", checksum_i);
  output->concatf("\t checksum_c      0x%02x\n", checksum_c);
  output->concatf("\t arg_count       %d\n", arg_count);
  output->concatf("\t bytes_total     %d\n", bytes_total);
  output->concatf("\t bytes_received  %d\n", bytes_received);
  output->concatf("\t time_created    0x%08x\n", time_created);
  output->concatf("\t retries         %d\n\n", retries);
}


/**
* @param   uint8_t The integer code that represents message state.
* @return  A pointer to a human-readable string indicating the message state.
*/
const char* XenoMessage::getMessageStateString() {
  switch (proc_state) {
    case XENO_MSG_PROC_STATE_UNINITIALIZED:         return "UNINITIALIZED";
    case XENO_MSG_PROC_STATE_RECEIVING:             return "RECEIVING";
    case XENO_MSG_PROC_STATE_AWAITING_PROC:         return "AWAITING_PROC";
    case XENO_MSG_PROC_STATE_PROCESSING_RUNNABLE:   return "PROCESSING";
    case XENO_MSG_PROC_STATE_AWAITING_SEND:         return "AWAITING_SEND";
    case XENO_MSG_PROC_STATE_AWAITING_REPLY:        return "AWAITING_REPLY";
    case XENO_MSG_PROC_STATE_SYNC_PACKET:           return "SYNC_PACKET";
    case XENO_MSG_PROC_STATE_AWAITING_REAP:         return "AWAITING_REAP";
    case (XENO_MSG_PROC_STATE_AWAITING_REAP | XENO_MSG_PROC_STATE_ERROR):
      return "SERIALIZING";
    default:                                        return "<UNKNOWN>";
  }
}

