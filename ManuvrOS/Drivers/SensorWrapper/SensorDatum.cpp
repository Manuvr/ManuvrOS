/*
File:   SensorDatum.cpp
Author: J. Ian Lindsay
Date:   2016.12.23

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


#include "SensorWrapper.h"


SensorDatum::SensorDatum(const DatumDef* d) {
  def = d;
  _flags = def->flgs & SENSE_DATUM_FLAG_PRELOAD_MASK;
  switch (def->type_id) {
  	// TODO: Needs to handle strings and other pointer types.
    case INT8_FM:
    case UINT8_FM:
    case BOOLEAN_FM:
      data_len = 1;
      break;
    case INT16_FM:
    case UINT16_FM:
      data_len = 2;
      break;
    case INT32_FM:
    case UINT32_FM:
    case FLOAT_FM:
      data_len = 4;
      break;
    case INT64_FM:
    case UINT64_FM:
    case DOUBLE_FM:
      data_len = 8;
      break;
    default:
      break;
  }

  if (data_len) {
    data = malloc(data_len);
    if (data) {
      memset(data, 0x00, data_len);
      _flags |= SENSE_DATUM_FLAG_MEM_ALLOC;
    }
  }
}


SensorDatum::~SensorDatum() {
  if (next) {
    delete next;
  }
  if (data) {
    data_len = 0;
    free(data);
  }
}


void SensorDatum::autoreport(SensorReporting ar) {
  switch (ar) {
    case SensorReporting::OFF:
      break;
    case SensorReporting::EVERY_READ:
      reportAll();
      break;
    case SensorReporting::NEW_VALUE:
      reportChanges();
      break;
  }
}


SensorError SensorDatum::printValue(StringBuilder* output) {
  switch (def->type_id) {
    case INT8_FM:
    case INT16_FM:
    case INT32_FM:
    case INT64_FM:
      output->concat(*((int*) data));
      break;
    case UINT8_FM:
    case UINT16_FM:
    case UINT32_FM:
    case UINT64_FM:
      output->concat(*((unsigned int*) data));
      break;
    case BOOLEAN_FM:
      output->concat((*((bool*) data) == 0) ? "false":"true");
      break;
    case FLOAT_FM:
      output->concat(*((float*) data));
      break;
    case DOUBLE_FM:
      output->concat(*((double*) data));
      break;
    case STR_FM:
      output->concat((char*) data);
      break;
    default:
      return SensorError::UNHANDLED_TYPE;
  }
  if (def->units) output->concatf(" %s", def->units);
  return SensorError::NO_ERROR;
}


void SensorDatum::printDebug(StringBuilder* output) {
}
