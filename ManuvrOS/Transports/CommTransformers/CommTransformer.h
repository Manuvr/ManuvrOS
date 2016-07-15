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
  does not require a session. This would encompass GPS, and also any transport
  bridges.

Another (more complex) case would be TLS. Two-way interchange with the transport
  happens independently of the attached Session (if any). This also allows for
  the easy selection and abstraction of TLS/DTLS from the underlying transport,
  allowing the choice to be made based on the nature of the transport.

A third use-case would be a traffic-analysis class that looks at connections
  originating in a listening transport and chooses what flavor of session
  is most-appropriate for servicing it. Thereafter performing the necessary
  association and distancing itself from further interaction.

Meaningful distinctions between datagram and stream-oriented transports ought
  to end here, with all downstream software experiencing the transport as a
  buffered stream, with the divisions between messages (if any) left to the
  classes that care.

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
