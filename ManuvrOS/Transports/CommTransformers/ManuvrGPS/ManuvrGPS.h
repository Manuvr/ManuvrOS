/*
File:   ManuvrGPS.h
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


This is a basic class for parsing NMEA sentences from a GPS and emitting
  them as annotated messages.

This class in unidirectional in the sense that it only reads from the
  associated transport. Hardware that has bidirectional capability for
  whatever reason can extend this class into something with non-trivial
  write methods.
*/


#ifndef __MANUVR_BASIC_GPS_H__
#define __MANUVR_BASIC_GPS_H__

#include "../CommTransformer.h"


class ManuvrGPS {
  public:
    ManuvrGPS();
    ~ManuvrGPS();


  protected:


  private:
};

#endif   // __MANUVR_BASIC_GPS_H__
