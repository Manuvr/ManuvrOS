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

#ifdef ARDUINO
    #include "Arduino.h"
#else
  /* Returns the current timestamp in milliseconds. */
  long SensorWrapper::millis() {
    return millis();
  }
#endif



/* Constructor */
SensorWrapper::SensorWrapper() {
  datum_list    = nullptr;
  name          = nullptr;
  s_id          = nullptr;
  data_count    = 0;
  updated_at    = 0;
  isHardware    = false;
  is_dirty      = false;
  sensor_active = false;
}


/* Destructor */
SensorWrapper::~SensorWrapper() {
  clear_data(datum_list);
}


/****************************************************************************************************
* Fxns that deal with sensor representation...                                                      *
****************************************************************************************************/

void SensorWrapper::setSensorId(const char *nu_s_id) {
	s_id = nu_s_id;
}


/****************************************************************************************************
* Functions that manage the "dirty" state of sensors and their autoreporting behavior.              *
****************************************************************************************************/
/* Returns the timestamp of the last sensor read in milliseconds. */
long SensorWrapper::lastUpdate() {
  return updated_at;
}

/* Is ANY datum in this sensor dirty? */
bool SensorWrapper::isDirty() {
  return is_dirty;
}

/* Is the given datum in this sensor dirty? */
bool SensorWrapper::isDirty(uint8_t dat) {
  if (dat > data_count) {
    return get_datum(dat)->dirty();
  }
  return false;
}


/* Mark a given datum in the sensor as having been updated. */
SensorError SensorWrapper::mark_dirty(uint8_t dat) {
  if (dat < data_count) {
    (get_datum(dat))->dirty(false);
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return mark_dirty();
}

/* Mark the sensor-as-a-whole as having been updated. */
SensorError SensorWrapper::mark_dirty() {
  is_dirty = true;
  updated_at = micros();
  return SensorError::NO_ERROR;
}


/*
* Mark each datum in this sensor as having been read.
*/
SensorError SensorWrapper::markClean() {
  SensorDatum* current = datum_list;
  while (current) {
    current->dirty(false);
    current = current->next;
  }
  is_dirty = false;
  return SensorError::NO_ERROR;
}


/*
* Mark a given datum in this sensor as having been read.
*/
SensorError SensorWrapper::markClean(uint8_t dat) {
  SensorDatum* current = get_datum(dat);
  if (current) {
    current->dirty(false);
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return SensorError::NO_ERROR;
}


/*
* Sets a given datum in this sensor to the given autoreporting state.
*/
void SensorWrapper::setAutoReporting(SensorReporting nu_ar_state) {
  SensorDatum* current = datum_list;
  while (current) {
    switch (nu_ar_state) {
      case SensorReporting::OFF:
        break;
      case SensorReporting::EVERY_READ:
        current->reportAll();
        break;
      case SensorReporting::NEW_VALUE:
        current->reportChanges();
        break;
    }
    current = current->next;
  }
}

/*
* Sets all data in this sensor to the given autoreporting state.
*/
SensorError SensorWrapper::setAutoReporting(uint8_t dat, SensorReporting nu_ar_state) {
  if (dat > data_count) {
    switch (nu_ar_state) {
      case SensorReporting::OFF:
        break;
      case SensorReporting::EVERY_READ:
        get_datum(dat)->reportAll();
        break;
      case SensorReporting::NEW_VALUE:
        get_datum(dat)->reportChanges();
        break;
    }
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return SensorError::NO_ERROR;
}


/*
* Sets a given datum in this sensor to the given autoreporting state.
*/
void SensorWrapper::setAutoReportCallback(SensorCallBack cb_fxn) {
  SensorDatum* current = datum_list;
  while (current) {
    current->ar_callback = cb_fxn;
    current = current->next;
  }
}

/*
* Sets all data in this sensor to the given autoreporting state.
*/
SensorError SensorWrapper::setAutoReportCallback(uint8_t dat, SensorCallBack cb_fxn) {
  if (dat > data_count) {
    get_datum(dat)->ar_callback = cb_fxn;
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return SensorError::NO_ERROR;
}


/****************************************************************************************************
* Functions that generate string outputs.                                                           *
****************************************************************************************************/

/*
* This function takes two pointers as arguments. The datum is read, and written to
* the StringBuilder.
*/
SensorError SensorWrapper::writeDatumDataToSB(SensorDatum* current, StringBuilder* buffer) {
  switch (current->type_id) {
    case INT8_FM:
    case INT16_FM:
    case INT32_FM:
    case INT64_FM:
      buffer->concat(*((int *) current->data));
      break;
    case UINT8_FM:
    case UINT16_FM:
    case UINT32_FM:
    case UINT64_FM:
      buffer->concat(*((unsigned int *) current->data));
      break;
    case BOOLEAN_FM:
      buffer->concat((*((bool *) current->data) == 0) ? "false":"true");
      break;
    case FLOAT_FM:
      buffer->concat(*((float *) current->data));
      break;
    case DOUBLE_FM:
      buffer->concat(*((double *) current->data));
      break;
    case STR_FM:
      buffer->concat((char*) current->data);
      break;
    default:
      return SensorError::UNHANDLED_TYPE;
  }
  return SensorError::NO_ERROR;
}


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
    if (writeDatumDataToSB(current, buffer) == SensorError::NO_ERROR) {
      buffer->concatf(" %s", current->units);
    }
    else {
      return SensorError::UNHANDLED_TYPE;
    }
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return SensorError::NO_ERROR;
}



/****************************************************************************************************
* Linked-list and datum-management functions...                                                     *
****************************************************************************************************/

SensorError SensorWrapper::readDatumRaw(uint8_t dat, void* void_buffer) {
  uint8_t* buffer = (uint8_t*) void_buffer;   // Done to avoid compiler warnings.
  SensorDatum* current = get_datum(dat);
  if (current) {
    if ((current->data == NULL) || (buffer == NULL)) {
      for (int i = 0; i < current->data_len; i++) {
        *(uint8_t*)(buffer + i) = *((uint8_t*)current->data + i);
      }
    }
    else {
      return SensorError::NULL_POINTER;
    }
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return SensorError::NO_ERROR;
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
    if (current->data == NULL) {
      current->data = malloc(current->data_len);
      if (current->data == NULL) {
        return SensorError::OUT_OF_MEMORY;
      }
    }
    switch (current->type_id) {
      case INT8_FM:
      case UINT8_FM:
      case BOOLEAN_FM:
      case INT16_FM:
      case UINT16_FM:
      case INT32_FM:
      case UINT32_FM:
      case FLOAT_FM:
      case INT64_FM:
      case UINT64_FM:
      case DOUBLE_FM:
        // All of these types are primatives, and so need to be accessed as such...
        memcpy(current->data, reading, current->data_len);
        break;
      case STR_FM:
      case BINARY_FM:
        // All of these types are pointers to something else.
      default:
        // Not sure what to do with this type...
        return SensorError::UNHANDLED_TYPE;
    }
  }
  else {
    return SensorError::INVALID_DATUM;
  }
  return mark_dirty(dat);   // If we've come this far, mark this datum as dirty.
}


/*
* This is just an override that specifies the most common options for creating datums.
* Returns an error code.
*/
bool SensorWrapper::defineDatum(const char* desc, const char* units, uint8_t type_id) {
  return defineDatum(-1, desc, units, SensorReporting::OFF, type_id);
}

/*
* Sensor classes need to define their own datums. They do that by calling this function.
* vid:     The unique id for this datum. Should be autogenerated if it is left as -1.
* desc:    A human-readable description of this datum.
* units:   A human readable unit.
* ar:      Autoreporting behavior.
* type_id: Defines the type of the data represented by this datum. Is is very important that this be accurate.
*
* Returns an error code.
*/
bool SensorWrapper::defineDatum(int vid, const char* desc, const char* units, SensorReporting ar, uint8_t type_id) {
  SensorDatum* nu = new SensorDatum();
  if (nu) {
    nu->description = desc;
    nu->units       = units;
    switch (ar) {
      case SensorReporting::OFF:
        break;
      case SensorReporting::EVERY_READ:
        nu->reportAll();
        break;
      case SensorReporting::NEW_VALUE:
        nu->reportChanges();
        break;
    }
    nu->v_id        = vid;
    nu->type_id     = type_id;

    switch (type_id) {
    	// TODO: Needs to handle strings and other pointer types.
        case INT8_FM:
        case UINT8_FM:
        case BOOLEAN_FM:
            nu->data = malloc(1);
            nu->data_len = 1;
            break;
        case INT16_FM:
        case UINT16_FM:
            nu->data = malloc(2);
            nu->data_len = 2;
            break;
        case INT32_FM:
        case UINT32_FM:
        case FLOAT_FM:
            nu->data = malloc(4);
            nu->data_len = 4;
            break;
        case INT64_FM:
        case UINT64_FM:
        case DOUBLE_FM:
            nu->data = malloc(8);
            nu->data_len = 8;
            break;
        default:
            nu->data = nullptr;
            nu->data_len = 0;
            break;
    }
    if ((nu->data_len > 0) && (nu->data != nullptr)) {
        memset(nu->data, 0x00, nu->data_len);
        insert_datum(nu);
        return true;
    }
    else {
      // We allocated a slot, but no data to put in it. Since we do not want to leak,
      // we must free the slot that we are declining to track.
      delete nu;
    }
  }
  return false;
}


/*
* Frees the memory associated with the given datum. Please note that this does not change the
*   IDs of datums that were added after the given datum.
*/
void SensorWrapper::clear_data(SensorDatum* current) {
  if (current) {
    clear_data(current->next);
    if (current->data) {
      free(current->data);
    }
    delete current;
  }
}


/*
* Private function that inserts the freshly-formed datum into the linked-list of data in this sensor.
*/
void SensorWrapper::insert_datum(SensorDatum* nu) {
  if (datum_list == NULL) {
    datum_list = nu;
  }
  else {
    SensorDatum* current = datum_list;
    while (current->next) {
      current = current->next;
    }
    current->next = nu;
  }

  if (nu->v_id == -1) {
    nu->v_id = data_count;
  }
  data_count++;
}


/*
* Returns the requested datum, or NULL if the datum is not defined.
*/
SensorDatum* SensorWrapper::get_datum(uint8_t idx) {
  SensorDatum* current = datum_list;
  while (current) {
    if (current->v_id == idx) {
      return current;
    }
    current = current->next;
  }
  return nullptr;
}



/****************************************************************************************************
* Static functions live below this block...                                                         *
****************************************************************************************************/

/*
* Static function that takes a SensorWrapper and returns a JSON object that constitutes its definition.
*/
void SensorWrapper::issue_json_map(StringBuilder* provided_buffer, SensorWrapper* sw) {
  provided_buffer->concatf("\"%s\":{\"name\":\"%s\",\"hardware\":%s,\"data\":{", sw->s_id, sw->name, ((sw->isHardware) ? "true" : "false"));
  SensorDatum* current = sw->datum_list;
  while (current) {
  	  provided_buffer->concatf("\"%d\":{\"desc\":\"%s\",\"unit\":\"%s\",\"autoreport\":\"%s\"%s", current->v_id, current->description, current->units, current->autoreport() ? "yes" : "no", ((nullptr == current->next) ? "}" : "},"));
  	  current = current->next;
  }
  provided_buffer->concat("}}");
}


/*
* Static function that takes a SensorWrapper and returns a JSON object that contains all of its values.
*/
void SensorWrapper::issue_json_value(StringBuilder* provided_buffer, SensorWrapper* sw) {
  provided_buffer->concatf("{\"%s\":{\"value\":{", sw->s_id);
  for (uint8_t i = 0; i < sw->data_count; i++) {
  	  provided_buffer->concatf("\"%d\":\"", i);
      sw->readAsString(i, provided_buffer);
      provided_buffer->concat((i == sw->data_count) ? "\"" : "\",");
  }
  provided_buffer->concatf("},\"updated_at\":%d}}", (unsigned int)sw->updated_at);
}


/*
* Static function that takes a SensorWrapper and returns a JSON object that contains
*   the value that is specified. If the value is out of range, returns an empty set
*   where we would otherwise expect to find values.
*/
void SensorWrapper::issue_json_value(StringBuilder* provided_buffer, SensorWrapper* sw, uint8_t dat) {
  provided_buffer->concatf("{\"%s\":{\"value\":{", sw->s_id);
  if (dat > sw->data_count) {
  	  provided_buffer->concatf("\"%d\":\"", dat);
      sw->readAsString(dat, provided_buffer);
      provided_buffer->concat("\"");
  }
  provided_buffer->concatf("},\"updated_at\":%d}}", (unsigned int)sw->updated_at);
}
