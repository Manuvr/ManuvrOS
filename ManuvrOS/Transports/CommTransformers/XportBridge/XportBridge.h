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


Probably the simplest-possible XportXform that isn't a bitbucket.
Takes two transports and cross-wires them such that one's output
  becomes the other's input. This is only really useful for deploying
  ManuvrOS as a transport adapter.
This class does not concern itself with network-layer tasks or anything
  above it in the OSI. It just shuttles payloads between transports as
  transparently as possible.
*/


#ifndef __MANUVR_XPORT_BRIDGE_H__
#define __MANUVR_XPORT_BRIDGE_H__

#include "../CommTransformer.h"


class XportBridge {
  public:
    XportBridge();
    ~XportBridge();


  protected:


  private:
};

#endif   // __MANUVR_XPORT_BRIDGE_H__
