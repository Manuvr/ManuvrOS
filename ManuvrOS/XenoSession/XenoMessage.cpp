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


XenoMessage::XenoMessage() {
  __class_initializer();
  proc_state = XENO_MSG_PROC_STATE_UNINITIALIZED;
  event      = NULL;  // Associates this XenoMessage to an event.
  unique_id  = 0;     //
}


XenoMessage::XenoMessage(ManuvrRunnable* existing_event) {
  __class_initializer();
  // Should maybe set a flag in the event to indicate that we are now responsible
  //   for memory upkeep? Don't want it to get jerked out from under us and cause a crash.
  provideEvent(existing_event);
}


XenoMessage::~XenoMessage() {
  if (NULL != event) {
    //Right now we aren't worried about this.
    if (XENO_MSG_PROC_STATE_AWAITING_REAP == proc_state) {
      delete event;
    }
  }
}


void XenoMessage::__class_initializer() {
  bytes_total     = 0;     // How many bytes does this message occupy?
  arg_count       = 0;
  checksum_i      = 0;     // The checksum of the data that we receive.
  checksum_c      = CHECKSUM_PRELOAD_BYTE;     // The checksum of the data that we calculate.
  bytes_received  = 0;     // How many bytes of this command have we received? Meaningless for the sender.
  retries         = 0;     // How many times have we retried this packet?
  time_created = millis(); // Optional: What time did this message come into existance?
  millis_at_begin = 0;     // This is the milliseconds reading when we sent.
  message_code    = 0;     // 
}



/**
* Calling this fxn will cause this Message to be populated with the given Event and unique_id.
* Calling this converts this XenoMessage into an outbound, and has the same general effect as
*   calling the constructor with an Event argument.
*
* @param   ManuvrRunnable* The Event that is to be communicated.
* @param   uint16_t          An explicitly-provided unique_id so that a dialog can be perpetuated.
*/
void XenoMessage::provideEvent(ManuvrRunnable *existing_event) {
  event = existing_event;
  unique_id = (uint16_t) randomInt();
  proc_state = XENO_MSG_PROC_STATE_SERIALIZING;  // Implies we are sending.
  message_code = event->event_code;                // 
  serialize();   // We should do this immediately to preserve the message.
  event = NULL;  // Don't risk the event getting ripped out from under us.
  proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;  // Implies we are sending.
}


/**
* An override for provide_event(ManuvrRunnable*) that allows us to supply unique_id explicitly.
* TODO: For safety's sake, the Event is not retained. This has caused us some grief. Re-evaluate...
*
* @param   ManuvrRunnable* The Event that is to be communicated.
* @param   uint16_t          An explicitly-provided unique_id so that a dialog can be perpetuated.
*/
void XenoMessage::provide_event(ManuvrRunnable *existing_event, uint16_t manual_id) {
  event = existing_event;
  unique_id = manual_id;
  proc_state = XENO_MSG_PROC_STATE_SERIALIZING;  // Implies we are sending.
  serialize();   // We should do this immediately to preserve the message.
  event = NULL;  // Don't risk the event getting ripped out from under us.
}


/**
* Sometimes we might want to re-use this allocated object rather than free it.
* Do not change the unique_id. One common use-case for this fxn is to reply to a message.
*/
void XenoMessage::wipe() {
  argbuf.clear();
  buffer.clear();
  proc_state     = 0;
  bytes_received = 0;
  unique_id      = 0;

  bytes_total  = 0;
  arg_count    = 0;
  checksum_i   = 0;
  checksum_c   = CHECKSUM_PRELOAD_BYTE;
  message_code = 0;
  
  if (NULL != event) {
    //Right now we aren't worried about this.
    if (XENO_MSG_PROC_STATE_AWAITING_REAP == proc_state) {
      delete event;
    }
  }
  proc_state = XENO_MSG_PROC_STATE_UNINITIALIZED;
  time_created = millis();
}


/**
* We need to ACK certain messages. This converts this message to an ACK of the message
*   that it used to be. Then it can be simply fed back into the outbound queue.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoMessage::ack() {
  ManuvrRunnable temp_event(MANUVR_MSG_REPLY);
  provide_event(&temp_event, unique_id);
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
  provide_event(&temp_event, unique_id);
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
  provide_event(&temp_event, unique_id);
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
int XenoMessage::serialize() {
  int return_value = -1;
  if (NULL == event) {
  }
  else if (0 == buffer.length()) {
    int arg_count  = event->serialize(&buffer);
    if (arg_count >= 0) {
      bytes_total = (uint32_t) buffer.length() + 8;   // +8 because: checksum byte
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
      
      unsigned char* full_xport_array = buffer.string();
      for (int i = 0; i < buffer.length(); i++) {
          checksum_temp = (uint8_t) checksum_temp + *(full_xport_array + i);
      }
      checksum_c = checksum_temp;
      
      *(prepend_array + 0) = (uint8_t) (bytes_total & 0xFF);
      *(prepend_array + 1) = (uint8_t) (bytes_total >> 8);
      *(prepend_array + 2) = (uint8_t) (bytes_total >> 16);
      *(prepend_array + 3) = (uint8_t) checksum_temp;

      buffer.prepend(prepend_array, 8);
      
      proc_state = XENO_MSG_PROC_STATE_AWAITING_SEND;
      return_value = bytes_total & 0x00FFFFFF;
    }
  }
  else {
      return_value = buffer.length();
  }
  
  return return_value;
}



/**
* 
*
* @return  0 on success. Non-zero on failure.
*/
int8_t XenoMessage::inflateArgs() {
  int8_t return_value = -1;
  if (argbuf.length() > 0) {
    if (event->inflateArgumentsFromBuffer(argbuf.string(), argbuf.length()) > 0) {
      return_value = 0;
    }
    else {
      Kernel::log("XenoMessage::inflateArgs():\t inflate fxn returned failure...\n");
    }
  }
  else {
    return_value = -1;
    Kernel::log("XenoMessage::inflateArgs():\t argbuf was zero-length.\n");
  }
  return return_value;
}



/**
* This function should be called by the session to feed bytes to a message.
*
* @return  The number of bytes consumed, or a negative value on failure.
*/
int XenoMessage::feedBuffer(StringBuilder *sb_buf) {
  if (NULL == sb_buf) return -3;

  StringBuilder output;
  int return_value = 0;
  int buf_len  = sb_buf->length();
  uint8_t* buf = (uint8_t*) sb_buf->string();

  //output.concatf("\n\nXenoMessage::feedBuffer(): Received %d bytes.\n", buf_len);

  switch (proc_state) {
    case XENO_MSG_PROC_STATE_UNINITIALIZED:
    case XENO_MSG_PROC_STATE_CLAIMED:
      //output.concatf("XENO_MSG_PROC_STATE_UNINITIALIZED is getting bytes. Slopppy...\n");
      proc_state = XENO_MSG_PROC_STATE_RECEIVING;
    case XENO_MSG_PROC_STATE_RECEIVING:
      break;

    case XENO_MSG_PROC_STATE_AWAITING_REPLY:
      proc_state = XENO_MSG_PROC_STATE_RECEIVING_REPLY;
      //output.concat("First bytes to a reply are arriving...\n");
      break;
    case XENO_MSG_PROC_STATE_RECEIVING_REPLY:
      break;
    default:
      #ifdef __MANUVR_DEBUG
        output.concatf("XenoMessage::feedBuffer() rejecting bytes because it is not in the right state. Is %s\n", XenoMessage::getMessageStateString(proc_state));
      #endif
      if (output.length() > 0) Kernel::log(&output);
      return -2;
  }
  
  /* Ok... by this point, we know we are eligible to receive data. So stop worrying about that. */
  if (0 == bytes_received) {      // If we haven't been fed any bytes yet...
    if (buf_len < 4) {
      // Not enough to act on, since we have none buffered ourselves...
      #ifdef __MANUVR_DEBUG
        output.concat("Rejecting bytes because there aren't enough of them yet.\n");
      #endif
      if (output.length() > 0) Kernel::log(&output);
      return 0;
    }
    else {  // We have at least enough for a sync-check...
      if (0 == XenoSession::contains_sync_pattern(buf, 4)) {
        // Get the offset of the last instance in this sync-stream relative to (buf+0) (we already know there is at least one).
        int x = XenoSession::locate_sync_break(buf+4, buf_len-4) + 4;
        #ifdef __MANUVR_DEBUG
          output.concatf("About to cull %d bytes of sync stream from the buffer..\n", x);
        #endif
        proc_state = XENO_MSG_PROC_STATE_SYNC_PACKET;  // Mark ourselves as a sync packet.
        bytes_received = 4;
        /* Cull the Session's buffer down to the offset of the next message (if there is any left). */
        if (x == buf_len)  sb_buf->clear();
        else               sb_buf->cull(x);
        
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
      if (return_value == buf_len)  sb_buf->clear();
      else if (return_value > 0)    sb_buf->cull(return_value);
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


  /* Do we have a whole minimum packet yet? */
  if (bytes_received >= 8) {
    /* Yup. Depending on how much is incoming versus what we have, take everything, or just enough. */
    int x = (bytes_total <= (bytes_received + buf_len)) ? (bytes_total - bytes_received) : buf_len;
    argbuf.concat(buf, x);
    bytes_received += x;
    return_value   += x;
    buf_len        -= x;
    buf            += x;
  }
  

  /* We might be done.... */
  if (bytes_received == bytes_total) {
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
        output.concat("XenoMessage::feedBuffer(): Ooops. Clobbered an event pointer. Expect leaks...\n");
        #endif
      }

      event = Kernel::returnEvent(message_code);
      
      if (event->event_code) {
        // The event code was found. Do something about it.
        switch (proc_state) {
          case XENO_MSG_PROC_STATE_RECEIVING:
            proc_state = XENO_MSG_PROC_STATE_AWAITING_UNSERIALIZE;
            #ifdef __MANUVR_DEBUG
              output.concat("XenoMessage::feedBuffer() Ready to unserialize...\n");
            #endif
            break;
          case XENO_MSG_PROC_STATE_RECEIVING_REPLY:
            proc_state = XENO_MSG_PROC_STATE_REPLY_RECEIVED;
            #ifdef __MANUVR_DEBUG
              output.concat("XenoMessage::feedBuffer() Received reply!\n");
            #endif
            break;
          default:
            #ifdef __MANUVR_DEBUG
              output.concatf("XenoMessage::feedBuffer() Message received, and is ok, but not sure about state.... Is %s\n", XenoMessage::getMessageStateString(proc_state));
            #endif
            if (output.length() > 0) Kernel::log(&output);
            return -2;
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
        #ifdef __MANUVR_DEBUG
          output.concatf("XenoMessage::feedBuffer(): Couldn't find a message definition for the code 0x%04x.\n", message_code);
        #endif
      }
    }
    else {
      // TODO: We might send a retry request at this point...
      proc_state = XENO_MSG_PROC_STATE_AWAITING_REAP;
      #ifdef __MANUVR_DEBUG
        output.concatf("XenoMessage::feedBuffer() Message failed to checksum. Got 0x%02x. Expected 0x%02x. \n", checksum_c, checksum_i);
      #endif
    }
  }
  
  /* Any bytes we've claimed, we cull. */
  if (return_value == buf_len)  sb_buf->clear();
  else if (return_value > 0)    sb_buf->cull(return_value);
  
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
  proc_state = XENO_MSG_PROC_STATE_CLAIMED;
  session = _ses;
}



/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoMessage::printDebug(StringBuilder *output) {
  if (NULL == output) return;

  if (NULL != event) {
    output->concatf("\t Message type    %s\n", event->getMsgTypeString());
  }
  output->concatf("\t Message state   %s\n", getMessageStateString(proc_state));
  output->concatf("\t unique_id       0x%04x\n", unique_id);
  output->concatf("\t checksum_i      0x%02x\n", checksum_i);
  output->concatf("\t checksum_c      0x%02x\n", checksum_c);
  output->concatf("\t arg_count       %d\n", arg_count);
  output->concatf("\t bytes_total     %d\n", bytes_total);
  output->concatf("\t bytes_received  %d\n", bytes_received);
  output->concatf("\t time_created    0x%08x\n", time_created);
  output->concatf("\t retries         %d\n", retries);

  output->concatf("\t Buffer length:  %d\n", buffer.length());
  output->concatf("\t Argbuf length:  %d\n", argbuf.length());
  output->concat("\n\n");
}


/**
* @param   uint8_t The integer code that represents message state.
* @return  A pointer to a human-readable string indicating the message state.
*/
const char* XenoMessage::getMessageStateString(uint8_t code) {
  switch (code) {
    case XENO_MSG_PROC_STATE_UNINITIALIZED:         return "UNINITIALIZED";
    case XENO_MSG_PROC_STATE_SYNC_PACKET:           return "SYNC_PACKET";
    case XENO_MSG_PROC_STATE_AWAITING_REAP:         return "AWAITING_REAP";
    case XENO_MSG_PROC_STATE_SERIALIZING:           return "SERIALIZING";
    case XENO_MSG_PROC_STATE_AWAITING_SEND:         return "AWAITING_SEND";
    case XENO_MSG_PROC_STATE_SENDING_COMMAND:       return "SENDING_COMMAND";
    case XENO_MSG_PROC_STATE_AWAITING_REPLY:        return "AWAITING_REPLY";
    case XENO_MSG_PROC_STATE_RECEIVING_REPLY:       return "RECEIVING_REPLY";
    case XENO_MSG_PROC_STATE_REPLY_RECEIVED:        return "REPLY_RECEIVED";
    case XENO_MSG_PROC_STATE_RECEIVING:             return "STATE_RECEIVING";
    case XENO_MSG_PROC_STATE_AWAITING_UNSERIALIZE:  return "AWAITING_UNSERIALIZE";
    case XENO_MSG_PROC_STATE_AWAITING_PROC:         return "AWAITING_PROC";
    case XENO_MSG_PROC_STATE_PROCESSING_COMMAND:    return "PROCESSING_COMMAND";
    case XENO_MSG_PROC_STATE_AWAITING_WRITE:        return "AWAITING_WRITE";
    case XENO_MSG_PROC_STATE_WRITING_REPLY:         return "WRITING_REPLY";
    default:                                        return "<UNKNOWN>";
  }
}

