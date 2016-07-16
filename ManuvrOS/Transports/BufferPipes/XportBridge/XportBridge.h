/*
File:   XportBridge.h
Author: J. Ian Lindsay
Date:   2016.07.14

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


Probably the simplest-possible general XportXform that isn't a bitbucket.

Takes two transports and cross-wires them such that one's output
  becomes the other's input. This is only really useful for deploying
  ManuvrOS as a transport adapter, but it will doubtless lend itself to such
  hackery as piping GPS data from serial ports over the internet without SLIP.

This class does not concern itself with network-layer tasks or anything
  above it in the OSI. It just shuttles payloads between transports as
  transparently as possible.

Unlike most other implementations of this class, this one is equidistant from
  (we presume) two counterparties. Therefore, "near" and "far" distinction is
  meaningless, provided we keep straight which is which. Combined with the
  fact that we do no transforms, this means that we simply pass traffic.
*/


#ifndef __MANUVR_XPORT_BRIDGE_H__
#define __MANUVR_XPORT_BRIDGE_H__

#include <DataStructures/BufferPipe.h>


/*
* A common use-case: Bridging two transports together.
*/
class XportBridge : public BufferPipe {
  public:
    XportBridge();
    XportBridge(BufferPipe*);
    XportBridge(BufferPipe*, BufferPipe*);
    ~XportBridge();

    /* Override from BufferPipe. */
    virtual unsigned int toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual unsigned int fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    virtual void printDebug(StringBuilder*);


  protected:
};


#endif   // __MANUVR_XPORT_BRIDGE_H__
