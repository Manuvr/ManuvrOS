/*
File:   ManuvrTLSServer.cpp
Author: J. Ian Lindsay
Date:   2016.07.15

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


This class is meant to provide a transport translation layer for TLS.
Initial implentation is via mbedTLS.
*/

#include "ManuvrTLS.h"
#include <Kernel.h>

#if defined(__MANUVR_MBEDTLS)


ManuvrTLSServer::ManuvrTLSServer(BufferPipe* _n) : ManuvrTLS(_n, MBEDTLS_DEBUG_LEVEL) {
}



/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
const char* ManuvrTLSServer::pipeName() { return "ManuvrTLSServer"; }

/**
* Inward toward the transport.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSServer::toCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSServer::fromCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ManuvrTLSServer::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);
}


#endif
