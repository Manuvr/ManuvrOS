/*
File:   CommTransformer.h
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


This class is meant to be the glue between a platform-abstracted transport,
  and any parser/session/presentation layer that sits on top of it (if any).

For instance: A simple parser that sits on a serial port or net socket, and
  does not require a session.
*/


#ifndef __MANUVR_XPORT_XFORMR_H__
#define __MANUVR_XPORT_XFORMR_H__

#include "../Kernel.h"
#include "DataStructures/StringBuilder.h"

#include <Platform/Platform.h>


class XportXformer {
  public:
    XportXformer();
    ~XportXformer();


  protected:


  private:
};

#endif   // __MANUVR_XPORT_XFORMR_H__
