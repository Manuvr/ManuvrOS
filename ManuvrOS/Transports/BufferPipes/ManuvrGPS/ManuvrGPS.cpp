/*
File:   ManuvrGPS.cpp
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

#include "ManuvrGPS.h"

void ManuvrGPS::printDebug(StringBuilder* output) {
  output->concat ("\t-- ManuvrGPS ----------------------------------\n");
  output->concatf("\t Sentences     \t%u\n", _sentences_parsed);

  if (_near) {
    output->concatf("\t _near         \t[0x%08x] %s\n", (unsigned long)_near, BufferPipe::memMgmtString(_near_mm_default));
  }
  if (_accumulator.length() > 0) {
    output->concatf("\t _accumulator (%d bytes):  ", _accumulator.length());
    _accumulator.printDebug(output);
  }
}
