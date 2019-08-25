/*
File:   SensorWrapper.cpp
Author: J. Ian Lindsay
Date:   2013.11.28

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

#if defined(__BUILD_HAS_JSON)
  #include <jansson.h>

  /*
  * Static function that takes a SensorWrapper and returns a JSON object that constitutes its definition.
  */
  void SensorWrapper::issue_json_map(json_t* obj, SensorWrapper* sw) {
    //provided_buffer->concatf("\"%s\":{\"name\":\"%s\",\"hardware\":%s,\"data\":{", sw->s_id, sw->name, ((sw->isHardware()) ? "true" : "false"));
    //SensorDatum* current = sw->datum_list;
    //while (current) {
    //  provided_buffer->concatf("\"%d\":{\"desc\":\"%s\",\"unit\":\"%s\",\"autoreport\":\"%s\"%s", current->v_id, current->def->desc, current->def->units, current->autoreport() ? "yes" : "no", ((nullptr == current->next()) ? "}" : "},"));
    //  current = current->next();
    //}
    //provided_buffer->concat("}}");
  }

  /*
  * Static function that takes a SensorWrapper and returns a JSON object that contains all of its values.
  */
  void SensorWrapper::issue_json_value(json_t* obj, SensorWrapper* sw) {
    //provided_buffer->concatf("{\"%s\":{\"value\":{", sw->s_id);
    //for (uint8_t i = 0; i < sw->data_count; i++) {
    //    provided_buffer->concatf("\"%d\":\"", i);
    //    sw->readAsString(i, provided_buffer);
    //    provided_buffer->concat((i == sw->data_count) ? "\"" : "\",");
    //}
    //provided_buffer->concatf("},\"updated_at\":%d}}", (unsigned int)sw->updated_at);
  }

  /*
  * Static function that takes a SensorWrapper and returns a JSON object that contains
  *   the value that is specified. If the value is out of range, returns an empty set
  *   where we would otherwise expect to find values.
  */
  void SensorWrapper::issue_json_value(json_t* obj, SensorWrapper* sw, uint8_t dat) {
    //provided_buffer->concatf("{\"%s\":{\"value\":{", sw->s_id);
    //if (dat > sw->data_count) {
    //    provided_buffer->concatf("\"%d\":\"", dat);
    //    sw->readAsString(dat, provided_buffer);
    //    provided_buffer->concat("\"");
    //}
    //provided_buffer->concatf("},\"updated_at\":%d}}", (unsigned int)sw->updated_at);
  }
#endif   //__BUILD_HAS_JSON


const char* SensorWrapper::errorString(SensorError err) {
  switch (err) {
    case SensorError::NO_ERROR:          return "NO_ERROR";
    case SensorError::ABSENT:            return "ABSENT";
    case SensorError::OUT_OF_MEMORY:     return "OUT_OF_MEMORY";
    case SensorError::WRONG_IDENTITY:    return "WRONG_IDENTITY";
    case SensorError::NOT_LOCAL:         return "NOT_LOCAL";
    case SensorError::INVALID_PARAM_ID:  return "INVALID_PARAM_ID";
    case SensorError::INVALID_PARAM:     return "INVALID_PARAM";
    case SensorError::NOT_CALIBRATED:    return "NOT_CALIBRATED";
    case SensorError::INVALID_DATUM:     return "INVALID_DATUM";
    case SensorError::UNHANDLED_TYPE:    return "UNHANDLED_TYPE";
    case SensorError::NULL_POINTER:      return "NULL_POINTER";
    case SensorError::BUS_ERROR:         return "BUS_ERROR";
    case SensorError::BUS_ABSENT:        return "BUS_ABSENT";
    case SensorError::NOT_WRITABLE:      return "NOT_WRITABLE";
    case SensorError::REG_NOT_DEFINED:   return "REG_NOT_DEFINED";
    case SensorError::DATA_EXHAUSTED:    return "DATA_EXHAUSTED";
    case SensorError::BAD_TYPE_CONVERT:  return "BAD_TYPE_CONVERT";
    case SensorError::MISSING_CONF:      return "MISSING_CONF";
    case SensorError::NOT_INITIALIZED:   return "NOT_INITIALIZED";
    case SensorError::UNDEFINED_ERR:
    default:
      break;
  }
  return "UNDEFINED_ERR";
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/* Constructor */
SensorWrapper::SensorWrapper(const char* n, const char* uid_str) {
  name = n;
  uuid_from_str(uid_str, &uuid);
}

/* Constructor */
SensorWrapper::SensorWrapper(const char* n) {
  name = n;
  uuid_gen(&uuid);
}


/* Destructor */
SensorWrapper::~SensorWrapper() {
  if (datum_list) {
    delete datum_list;
  }
}


/*******************************************************************************
* Fxns that deal with sensor representation...                                 *
*******************************************************************************/

void SensorWrapper::setSensorId(const char *uid_str) {
  uuid_from_str(uid_str, &uuid);
}


/*******************************************************************************
* Functions that manage the "dirty" state and autoreporting behavior of data.  *
*******************************************************************************/
/* Returns the timestamp of the last sensor read in milliseconds. */
long SensorWrapper::lastUpdate() {
  return updated_at;
}

/* Is ANY datum in this sensor dirty? */
bool SensorWrapper::isDirty() {
  SensorDatum* current = datum_list;
  while (current) {
    if (current->dirty()) {
      return true;
    }
    current = current->next();
  }
  return false;
}


/*
* Mark a given datum in the sensor as having been updated.
* Also marks the sensor-as-a-whole as having been updated.
*/
SensorError SensorWrapper::mark_dirty(uint8_t dat) {
  SensorDatum* current = get_datum(dat);
  if (current) {
    current->dirty(true);
    updated_at = micros();
    return SensorError::NO_ERROR;
  }
  return SensorError::INVALID_DATUM;
}


/*
* Mark each datum in this sensor as having been read.
*/
bool SensorWrapper::isHardware() {
  SensorDatum* current = datum_list;
  while (current) {
    if (current->hardware()) {
      return true;
    }
    current = current->next();
  }
  return false;
}


/*
* Mark each datum in this sensor as having been read.
*/
SensorError SensorWrapper::markClean() {
  SensorDatum* current = datum_list;
  while (current) {
    current->dirty(false);
    current = current->next();
  }
  return SensorError::NO_ERROR;
}


/*
* Sets all data in this sensor to the given autoreporting state.
*/
void SensorWrapper::setAutoReporting(SensorReporting nu_ar_state) {
  SensorDatum* current = datum_list;
  while (current) {
    current->autoreport(nu_ar_state);
    current = current->next();
  }
}

/*
* Sets a given datum in this sensor to the given autoreporting state.
*/
SensorError SensorWrapper::setAutoReporting(uint8_t dat, SensorReporting nu_ar_state) {
  SensorDatum* current = get_datum(dat);
  if (current) {
    current->autoreport(nu_ar_state);
    return SensorError::NO_ERROR;
  }
  return SensorError::INVALID_DATUM;
}



/*******************************************************************************
* Functions that generate string outputs.                                      *
*******************************************************************************/

SensorError SensorWrapper::readAsString(StringBuilder* buffer) {
  return SensorError::NO_ERROR;
}


/*
* Writes a string representation of the given datum to the provided StringBuffer.
* Returns an error code.
*/
SensorError SensorWrapper::readAsString(uint8_t dat, StringBuilder* buffer) {
  SensorDatum* current = get_datum(dat);
  if (current) {
    current->dirty(false);
    return current->printValue(buffer);
  }
  return SensorError::INVALID_DATUM;
}


void SensorWrapper::printSensorSummary(StringBuilder* output) {
  uuid_to_sb(&uuid, output);
  output->concatf(": %12s %c %c %c   %ld",
    name,
    isActive() ? 'x':' ',
    isCalibrated() ? 'x':' ',
    isDirty() ? 'x':' ',
    lastUpdate()
  );
}


void SensorWrapper::printSensorDataDefs(StringBuilder* output) {
  int idx = 0;
  SensorDatum* current = get_datum(idx);
  while (nullptr != current) {
    output->concatf("%d: 0x%02x  %s (%s)    %s\n", idx, current->def->flgs,
      current->def->units, getTypeCodeString(current->def->type_id), current->def->desc
    );
    current = get_datum(++idx);
  }
}


void SensorWrapper::printSensorData(StringBuilder* output) {
  int idx = 0;
  SensorDatum* current = get_datum(idx);
  while (nullptr != current) {
    output->concatf("%d:  ", idx);
    current->printValue(output);
    output->concatf(" %s\n", current->def->units);
    current = get_datum(++idx);
  }
}


SensorError SensorWrapper::issue_def_map(TCode tcode, StringBuilder* output) {
  int idx = 0;
  SensorDatum* current = get_datum(idx);

  #if defined(__BUILD_HAS_CBOR)
  cbor::output_dynamic coutput;
  cbor::encoder encoder(coutput);
  // We shall have an array of maps...
  encoder.write_map(3);
  encoder.write_string("name");
  encoder.write_string(name);
  encoder.write_string("uid");
  encoder.write_bytes((uint8_t*) &uuid, sizeof(UUID));
  encoder.write_string("data");
  uint8_t arg_count = datum_list->argCount();
  encoder.write_array(arg_count);
  while (nullptr != current) {
    encoder.write_map(4);
    encoder.write_string("units");
    encoder.write_string(current->def->units);
    encoder.write_string("desc");
    encoder.write_string(current->def->desc);
    encoder.write_string("type");
    encoder.write_string(getTypeCodeString(current->def->type_id));
    encoder.write_string("flgs");
    encoder.write_int((unsigned int) current->def->flgs);
    current = current->next();
  }
  int final_size = coutput.size();
  if (final_size) {
    output->concat(coutput.data(), final_size);
  }
  #endif   //__BUILD_HAS_CBOR
  return SensorError::NO_ERROR;
}


SensorError SensorWrapper::issue_value_map(TCode tcode, StringBuilder* output) {
  switch (tcode) {
    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        cbor::output_dynamic coutput;
        cbor::encoder encoder(coutput);
        encoder.write_map(1);
        encoder.write_string(sensorName());

        int final_size = coutput.size();
        if (final_size) {
          output->concat(coutput.data(), final_size);
        }
        if (0 == Argument::encodeToCBOR((Argument*) &datum_list, output)) {
          return SensorError::NO_ERROR;
        }
      }
      break;
    #endif   //__BUILD_HAS_CBOR

    #if defined(__BUILD_HAS_JSON)
    case TCode::JSON:
      return SensorError::NO_ERROR;
    #endif   //__BUILD_HAS_CBOR
    default:
      break;
  }
  return SensorError::BAD_TYPE_CONVERT;
}





/*******************************************************************************
* Linked-list and datum-management functions...                                *
*******************************************************************************/

SensorError SensorWrapper::readDatumRaw(uint8_t dat, void* void_buffer) {
  uint8_t* buffer = (uint8_t*) void_buffer;   // Done to avoid compiler warnings.
  SensorDatum* current = get_datum(dat);
  if (current) {
    if (0 == current->getValueAs(void_buffer)) {
      return SensorError::NO_ERROR;
    }
    else {
      return SensorError::NULL_POINTER;
    }
  }
  return SensorError::INVALID_DATUM;
}

/*
* This is the function that sensor classes call to update their values. There are numerous inlines
*   in the header file for the sake of eliminating the need to type-cast to void*.
* Marks the datum (and the sensor) dirty with timestamp.
* Returns an error code.
*/
SensorError SensorWrapper::updateDatum(uint8_t dat, void *reading) {
  SensorDatum* current = get_datum(dat);
  if (current) {
    if (0 != current->setValue(reading, current->length(), current->typeCode())) {
      Kernel::log("Bad type conversion\n");
      return SensorError::BAD_TYPE_CONVERT;
    }
  }
  else {
    Kernel::log("INVALID_DATUM\n");
    return SensorError::INVALID_DATUM;
  }
  return mark_dirty(dat);   // If we've come this far, mark this datum as dirty.
}


/*
* Sensor classes need to define their own datums. They do that by calling this function.
* def:     Datum definition.
* ar:      Autoreporting behavior.
* Returns an error code.
*/
SensorError SensorWrapper::define_datum(const DatumDef* def) {
  SensorDatum* nu = new SensorDatum(def);
  if (nu) {
    if (nu->mem_ready()) {
      insert_datum(nu);
    }
  }
  return SensorError::NO_ERROR;
}


/*
* Private function that inserts the freshly-formed datum into the linked-list of data in this sensor.
*/
void SensorWrapper::insert_datum(SensorDatum* nu) {
  if (datum_list) {
    datum_list->link(nu);
  }
  else {
    datum_list = nu;
  }
}


/*
* Returns the requested datum, or NULL if the datum is not defined.
*/
SensorDatum* SensorWrapper::get_datum(uint8_t idx) {
  if (datum_list) {
    return (SensorDatum*) datum_list->retrieveArgByIdx(idx);
  }
  return nullptr;
}
