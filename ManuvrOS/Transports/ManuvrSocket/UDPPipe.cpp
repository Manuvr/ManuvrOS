/*
File:   UDPPipe.cpp
Author: J. Ian Lindsay
Date:   2016.06.29

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

#if defined(MANUVR_SUPPORT_UDP)
#include "ManuvrUDP.h"

#include <XenoSession/CoAP/CoAPSession.h>  // TODO: Need Zookeeper.

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

UDPPipe::UDPPipe() : BufferPipe() {
  _ip    = 0;
  _port  = 0;
  _udp   = NULL;
  _udpflags = 0;

  _bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED | BPIPE_FLAG_IS_BUFFERED, true);
}

UDPPipe::UDPPipe(ManuvrUDP* udp, uint32_t ip, uint16_t port) : BufferPipe() {
  // The near-side will always be feeding us from buffers on the stack. Since
  //   the buffer will vanish after return, we must copy it.
  _ip    = ip;
  _port  = port;
  _udp   = udp;   // TODO: setNear(udp); and thereafter cast to ManuvrUDP?
  _udpflags = 0;

  _bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED | BPIPE_FLAG_IS_BUFFERED, true);
}


UDPPipe::~UDPPipe() {
  _accumulator.clear();
  if (_udp) _udp->udpPipeDestroyCallback(this);
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
const char* UDPPipe::pipeName() { return "UDPPipe"; }

/**
* Back toward ManuvrUDP....
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t UDPPipe::toCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      return (_udp->write_datagram(buf, len, _ip, _port, 0) ? MEM_MGMT_RESPONSIBLE_CREATOR : MEM_MGMT_RESPONSIBLE_CALLER);

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      // TODO: Freeing the buffer? Let UDP do it?
      return (_udp->write_datagram(buf, len, _ip, _port, 0) ? MEM_MGMT_RESPONSIBLE_BEARER : MEM_MGMT_RESPONSIBLE_CALLER);

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t UDPPipe::fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, len, mm);
      }
      else {
        CoAPSession* _nu = new CoAPSession((BufferPipe*) this);
        if (nullptr != _nu) {
          Kernel::getInstance()->subscribe(_nu);
          setFar(_nu);
          // We allocated this class. We need to tear it down, ultimately.
          _bp_set_flag(BPIPE_FLAG_WE_ALLOCD_FAR, true);

          // Also, we want to hold the UDPPipe open for subsequent exchanges.
          persistAfterReply(true);

          return _far->fromCounterparty(buf, len, mm);
        }
        _accumulator.concat(buf, len);
        return MEM_MGMT_RESPONSIBLE_BEARER;   // We take responsibility.
      }

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}


/*******************************************************************************
* UDPPipe-specific functions                                                   *
*******************************************************************************/

/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void UDPPipe::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);
  struct in_addr _inet_addr;
  _inet_addr.s_addr = _ip;
  if (_udp) {
    output->concatf("--\t_udp          \t[0x%08x]\n", (uint32_t) _udp);
  }
  output->concatf("--\tCounterparty: \t%s:%d\n", (char*) inet_ntoa(_inet_addr), _port);

  if (_accumulator.length() > 0) {
    output->concatf("--\t_accumulator (%d bytes):  ", _accumulator.length());
    _accumulator.printDebug(output);
  }
  output->concat("\n");
}


/**
* Called by the far-side when it first attaches.
* Mem-mgmt issues should be handled by StringBuilder.
*
* Because we are dealing with datagrams, we can't simply glom the entire
*   accumulator onto the far-side in a single call. Many datagram-oriented
*   protocols rely on the natural divisions between packets to avoid having
*   to codify length data in the message explicitly.
* This saves space on the wire, but without even simple framing it means we must
*   preserve those natural boundaries by supporting the ability for a far-side
*   protocol stack to retrieve packets from the accumulator chunkwise.
* Far-side consumers of the accumulator need to call this function until it
*   returns zero, or it will miss all packets after the first.
*
* @param A pointer to a StringBuffer object to receive the output.
* @return The number of bytes retrieved into the buffer on this call.
*/
int UDPPipe::takeAccumulator(StringBuilder* target) {
  int return_value = _accumulator.length();
  if (return_value > 0) {
    target->concatHandoff(&_accumulator);
  }
  return return_value;
}

#endif //MANUVR_SUPPORT_UDP
