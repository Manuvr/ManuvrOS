/*
File:   ManuvrSession.h
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


/**
* Manuvr's protocol is formatted this way:
*
* All multibyte values are stored "little-endian".
* Checksum = 0x55 (Preload) + Every other byte after it. Then truncate to uint8.
*
* |<-------------------HEADER------------------------>|
* |--------------------------------------------------|  /---------------\
* | Total Length | Checksum | Unique ID | Message ID |  | Optional data |
* |--------------------------------------------------|  \---------------/
*       3             1           2           2            <---- Bytes
*
* Minimum packet size is the sync packet (4 bytes). Sync packet is
*   always the same (for a given CHECKSUM_PRELOAD):
*        0x04 0x00 0x00 0x55
*                        |----- Checksum preload
*
* Sync packets are the way in which we establish initial offset for a dialog over a stream.
* We need to sync initially on connection and if something goes sideways in the course
*   of a normal dialog.
*
* Receiving an unsolicited sync packet should be an indication that the counterparty either
*   can't understand us, or the transport dropped a message and they timed it out. When we see
*   a sync packet on the wire, we should allow the currently-active transaction to complete and
*   then send back a sync stream of our own. When the counterparty notices this, communication
*   can resume.
*/

#ifndef __XENOSESSION_MANUVR_H__
#define __XENOSESSION_MANUVR_H__


#define CHECKSUM_PRELOAD_BYTE 0x55    // Calculation of new checksums should start with this byte,
#define XENOSESSION_INITIAL_SYNC_COUNT    24      // How many sync packets to send before giving up.

#include "../XenoSession.h"


class ManuvrSession : public XenoSession {
  public:
    ManuvrSession(ManuvrXport*);
    ~ManuvrSession();

    int8_t sendSyncPacket();

    // Returns and isolates the sync bits.
    inline uint8_t getSync() {
      return (session_state & 0x00F0);
    };

    // Returns the answer to: "Is this session in sync?"
    inline bool syncd() {
      return (0 == ((session_state & 0x00F0) | XENOSESSION_STATE_SYNC_SYNCD));
    }

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    const char* getReceiverName();


  protected:
    //int8_t bootComplete();
    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

  private:
    ManuvrRunnable sync_event;

    int8_t scan_buffer_for_sync();
    void   mark_session_desync(uint8_t desync_source);
    void   mark_session_sync(bool pending);

    const char* getSessionSyncString();
};


#endif //__XENOSESSION_MANUVR_H__
