/*
File:   SensorFilter.cpp
Author: J. Ian Lindsay
Date:   2020.01.30

Copyright 2020 Manuvr, Inc

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


#include "Utilities.h"
#include "SensorFilter.h"
#include "StringBuilder.h"



/******************************************************************************
* Statics
******************************************************************************/
extern const char* FILTER_HEADER_STRING;

const char* getFilterStr(FilteringStrategy x) {
  switch (x) {
    case FilteringStrategy::RAW:            return "NULL";
    case FilteringStrategy::MOVING_AVG:     return "MOVING_AVG";
    case FilteringStrategy::MOVING_MED:     return "MOVING_MED";
    case FilteringStrategy::HARMONIC_MEAN:  return "HARMONIC_MEAN";
    case FilteringStrategy::GEOMETRIC_MEAN: return "GEOMETRIC_MEAN";
    case FilteringStrategy::QUANTIZER:      return "QUANTIZER";
  }
  return "UNKNOWN";
}
