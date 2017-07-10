/*
File:   Sensors.h
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


This is a modernization of SensorWrapper. It will be in conflict with that API.

*/


#ifndef __MANUVR_SENSORS_H
#define __MANUVR_SENSORS_H

#include <inttypes.h>
#include <string.h>
#include <DataStructures/StringBuilder.h>
#include <DataStructures/Argument.h>
#include <DataStructures/uuid.h>

#include <Platform/Platform.h>

#include <CommonConstants.h>
#include <EnumeratedTypeCodes.h>


class SensorDatum;

/* These are possible error codes. */
enum class SensorError {
  NONE               =    0,   // There was no error.
  UNDEFINED          =   -1,   // If we try to set a sensor parameter to something invalid for an extant register.
  OUT_OF_MEMORY      =   -2,   // Couldn't allocate memory for some sensor-related task.
  WRONG_IDENTITY     =   -3,   // Some sensors come with ID markers in them. If we get one wrong, this happens.
  NOT_LOCAL          =   -4,   // If we try to read or change parameters of a sensor that isn't attached to us.
  INVALID_PARAM_ID   =   -5,   // If we try to read or change a sensor parameter that isn't supported.
  INVALID_PARAM      =   -6,   // If we try to set a sensor parameter to something invalid for an extant register.
  NOT_CALIBRATED     =   -7,   // If we try to read a sensor that is uncalibrated. Not all sensors require this.
  INVALID_DATUM      =   -8,   // If we try to do an operation on a datum that doesn't exist for this sensor.
  UNHANDLED_TYPE     =   -9,   // Issued when we ask for a string conversion that we don't support.
  NULL_POINTER       =  -10,   // What happens when we try to do I/O on a null pointer.
  BUS_ERROR          =  -11,   // If there was some generic bus error that we otherwise can't pinpoint.
  BUS_ABSENT         =  -12,   // If the requested bus is not accessible.
  NOT_WRITABLE       =  -13,   // If we try to write to a read-only register.
  REG_NOT_DEFINED    =  -14,   // If we try to do I/O on a non-existent register.
  DATA_EXHAUSTED     =  -15,   // If we try to poll sensor data faster than the sensor can produce it.
  BAD_TYPE_CONVERT   =  -16,   // If we ask the class to convert types in a way that isn't possible.
  MISSING_CONF       =  -17,   // If we ask the sensor class to perform an operation on parameters that it doesn't have.
  NOT_INITIALIZED    =  -18,   //
  ABSENT             =  -19,   // We failed to talk to the sensor.
};

/* Sensors can automatically report their values. */
enum class SensorReporting {
  OFF        = 0,   // No automatic reporting.
  NEW_VALUE  = 1,   // Only report on value change.
  EVERY_READ = 2    // Report every read.
};


/* Convenience union for Datum flags.. */
typedef struct sense_datum_flag_def_t {
  union {
    struct {
      // This member holds descriptive and invariant flags about the datum.
      uint32_t is_hardware:   1;  // Is this reading rooted in hardware?
      uint32_t is_proxied:    1;  // Is this a sensor reading proxied from another device?
      uint32_t autoreporting: 2;  // Autoreporting behavior (SensorReporting).
      uint32_t data_valid:    1;  // Is the data valid?
      uint32_t is_active:     1;  // Is the provider updating this value?
    };
    uint32_t val;
  } flgs;
} DatumFlags;


/*
* We have the option of directing a datum to autoreport under certain conditions.
* When such a datum becomes dirty, the SensorWrapper class can call a provided
*   callback with the entire sensor wrapper as an argument.
*/
typedef void (*SensorCallBack) (SensorDatum*);


/*
* This class helps us present and manage the unique data that comes out of each
*   sensor. This is a purpose-specific wrapper around Argument, which handles
*   types and encodings of data for us.
*/
class SensorDatum {
  public:
    const char*     desc;    // A brief description of the datum for humans.
    const char*     units;   // Real-world units that this datum measures.
    Argument value;    // The data itself.

    SenseDatum(const char* _desc, const char* _units, uint32_t _flgs);
    ~SenseDatum();

    inline void printDebug(StringBuilder* out) {    value.printDebug(out);    };
    inline void printValue(StringBuilder* out) {    value.valToString(out);   };

    /* Auto-reporting setting. Only meaningful if ar_callback is non-null. */
    inline void autoreport(SensorReporting r) { _flags.autoreporting = (uint_8_t) r; };
    inline void reportsOff() {     _flags.autoreporting = (uint_8_t) SensorReporting::OFF;        };
    inline void reportAll() {      _flags.autoreporting = (uint_8_t) SensorReporting::EVERY_READ; };
    inline void reportChanges() {  _flags.autoreporting = (uint_8_t) SensorReporting::NEW_VALUE;  };
    inline bool autoreport() {     return (_flags.autoreporting);   };

    inline bool isHardware() {     return (_flags.is_hardware);     };
    inline bool isActive() {       return (_flags.is_active);       };
    inline bool isProxied() {      return (_flags.is_proxied);      };
    inline bool readingValid() {   return (_flags.data_valid);      };

    inline void markUpdated() {    _last_update = millis();         };

  private:
    DatumFlags _flags;
    uint32_t _last_update;
};



class SensorDriver {
  public:
    SensorDriver(const char*);
    SensorDriver(const char*, const char*);
    ~SensorDriver();

    long lastUpdate();                       // Datetime stamp from the last update.


    SensorError readAsString(StringBuilder*);           // Returns the sensor read as a string.
    SensorError readAsString(uint8_t, StringBuilder*);  // Returns the given datum read as a string.

    /* Sets a callback function for all data in this sensor. */
    inline void setAutoReportCallback(SensorCallBack nu) {   ar_callback = nu;  };

    /* Is this sensor active? */
    inline bool isActive() {        return (MANUVR_SENSOR_FLAG_ACTIVE == (_flags & MANUVR_SENSOR_FLAG_ACTIVE));  };
    /* Is this sensor calibrated? */
    inline bool isCalibrated() {    return (MANUVR_SENSOR_FLAG_CALIBRATED == (_flags & MANUVR_SENSOR_FLAG_CALIBRATED));  };

    // Virtual functions. These MUST be overridden by the extending class.
    virtual SensorError init() =0;                                        // Call this to initialize the sensor.
    virtual SensorError readSensor() =0;                                  // Actually read the sensor. Returns an error-code.
    virtual SensorError setParameter(uint16_t reg, int len, uint8_t*) =0; // Used to set operational parameters for the sensor.
    virtual SensorError getParameter(uint16_t reg, int len, uint8_t*) =0; // Used to read operational parameters from the sensor.
    /* A quick note about how setParameter() should be implemented....
    The idea behind this was to work like common i2c sensors in that sensor classes have
    software "registers" that are written to to control operational aspects of the represented sensor.
    Sensor classes are free to implement whatever registers they like (up to 16-bit's worth), but
    convention of the sensor should probably take priority. IE... if you are writing a class for a
    sensor that has a control register at 0x40, you should probably case off that value to cause minimal
    confusion later. See some of the provided sensor classes for examples.
    */

    // Static functions...
    static const char* errorString(SensorError);
    #if defined(__BUILD_HAS_JSON)
      static void issue_json_map(json_t*, SensorWrapper*);              // Returns a pointer to a buffer containing the JSON def of the given sensor.
      static void issue_json_value(json_t*, SensorWrapper*);            // Returns a pointer to a buffer containing the JSON values for the given sensor.
      static void issue_json_value(json_t*, SensorWrapper*, uint8_t);   // Returns a pointer to a buffer containing the JSON value for the given sensor's given datum.
    #endif   //__BUILD_HAS_JSON


  protected:
    SensorDatum* get_datum(uint8_t);

    /* These are inlines that we define for ease-of-use from extending classes. If we didn't do this, we
         would need to cast all sensor readings to void* wherever we updateDatum(). All wind up at the
         same place. */
    inline SensorError updateDatum(uint8_t dat, double val) {         return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, float val) {          return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, int val) {            return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, unsigned int val) {   return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, char* val) {          return updateDatum(dat, (void*) val); }
    inline SensorError updateDatum(uint8_t dat, unsigned char* val) { return updateDatum(dat, (void*) val); }
    SensorError updateDatum(uint8_t, void*);   // This is the actual implementation.

    inline void isActive(bool x) {
      _flags = (x) ? (_flags | MANUVR_SENSOR_FLAG_ACTIVE) : (_flags & ~MANUVR_SENSOR_FLAG_ACTIVE);
    };

    inline void isCalibrated(bool x) {
      _flags = (x) ? (_flags | MANUVR_SENSOR_FLAG_CALIBRATED) : (_flags & ~MANUVR_SENSOR_FLAG_CALIBRATED);
    };

    SensorError define_datum(const DatumDef*);

  private:
    const char* name;   // This is the name of the sensor.
    SensorDatum*   datum_list  = nullptr; // Linked list of data that this sensor might carry.
    SensorCallBack ar_callback = nullptr; // The pointer to the callback function used for autoreporting.
    uint8_t        _flags      = 0;       // Holds elementary state and capability info.

    void insert_datum(SensorDatum*);
    SensorError mark_dirty(uint8_t);   // Marks a specific datum in this sensor as dirty.
};





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////






class SensorWrapper {
  public:
    SensorWrapper(const char*);
    SensorWrapper(const char*, const char*);
    ~SensorWrapper();

    void setSensorId(const char*);

    long lastUpdate();                       // Datetime stamp from the last update.
    bool isDirty();                          // Is ANYTHING in this sensor dirty?
    bool isHardware();                       // Is this sensor a piece of hardware?
    void setAutoReporting(SensorReporting);  // Sets all data in this sensor to the given autoreporting state.
    SensorError markClean();                 // Marks all data in the sensor clean.
    SensorError setAutoReporting(uint8_t, SensorReporting); // Sets a given datum in this sensor to the given autoreporting state.

    SensorError readAsString(StringBuilder*);           // Returns the sensor read as a string.
    SensorError readAsString(uint8_t, StringBuilder*);  // Returns the given datum read as a string.

    /* Sets a callback function for all data in this sensor. */
    inline void setAutoReportCallback(SensorCallBack nu) {   ar_callback = nu;  };

    /* Is this sensor active? */
    inline bool isActive() {        return (MANUVR_SENSOR_FLAG_ACTIVE == (_flags & MANUVR_SENSOR_FLAG_ACTIVE));  };
    /* Is this sensor calibrated? */
    inline bool isCalibrated() {    return (MANUVR_SENSOR_FLAG_CALIBRATED == (_flags & MANUVR_SENSOR_FLAG_CALIBRATED));  };

    // Virtual functions. These MUST be overridden by the extending class.
    virtual SensorError init() =0;                                        // Call this to initialize the sensor.
    virtual SensorError readSensor() =0;                                  // Actually read the sensor. Returns an error-code.
    virtual SensorError setParameter(uint16_t reg, int len, uint8_t*) =0; // Used to set operational parameters for the sensor.
    virtual SensorError getParameter(uint16_t reg, int len, uint8_t*) =0; // Used to read operational parameters from the sensor.
    /* A quick note about how setParameter() should be implemented....
    The idea behind this was to work like common i2c sensors in that sensor classes have
    software "registers" that are written to to control operational aspects of the represented sensor.
    Sensor classes are free to implement whatever registers they like (up to 16-bit's worth), but
    convention of the sensor should probably take priority. IE... if you are writing a class for a
    sensor that has a control register at 0x40, you should probably case off that value to cause minimal
    confusion later. See some of the provided sensor classes for examples.
    */

    // Static functions...
    static const char* errorString(SensorError);
    #if defined(__BUILD_HAS_JSON)
      static void issue_json_map(json_t*, SensorWrapper*);              // Returns a pointer to a buffer containing the JSON def of the given sensor.
      static void issue_json_value(json_t*, SensorWrapper*);            // Returns a pointer to a buffer containing the JSON values for the given sensor.
      static void issue_json_value(json_t*, SensorWrapper*, uint8_t);   // Returns a pointer to a buffer containing the JSON value for the given sensor's given datum.
    #endif   //__BUILD_HAS_JSON


  protected:
    SensorDatum* get_datum(uint8_t);

    /* These are inlines that we define for ease-of-use from extending classes. If we didn't do this, we
         would need to cast all sensor readings to void* wherever we updateDatum(). All wind up at the
         same place. */
    inline SensorError updateDatum(uint8_t dat, double val) {         return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, float val) {          return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, int val) {            return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, unsigned int val) {   return updateDatum(dat, (void*) &val); }
    inline SensorError updateDatum(uint8_t dat, char* val) {          return updateDatum(dat, (void*) val); }
    inline SensorError updateDatum(uint8_t dat, unsigned char* val) { return updateDatum(dat, (void*) val); }
    SensorError updateDatum(uint8_t, void*);   // This is the actual implementation.

    inline SensorError readDatumRaw(uint8_t dat, double *val) {         return readDatumRaw(dat, (void*) &val); }
    inline SensorError readDatumRaw(uint8_t dat, float *val) {          return readDatumRaw(dat, (void*) &val); }
    inline SensorError readDatumRaw(uint8_t dat, int *val) {            return readDatumRaw(dat, (void*) &val); }
    inline SensorError readDatumRaw(uint8_t dat, unsigned int *val) {   return readDatumRaw(dat, (void*) &val); }
    inline SensorError readDatumRaw(uint8_t dat, char** val) {          return readDatumRaw(dat, (void*) &val); }
    inline SensorError readDatumRaw(uint8_t dat, unsigned char** val) { return readDatumRaw(dat, (void*) &val); }
    SensorError readDatumRaw(uint8_t, void*);   // This is the actual implementation.

    inline void isActive(bool x) {
      _flags = (x) ? (_flags | MANUVR_SENSOR_FLAG_ACTIVE) : (_flags & ~MANUVR_SENSOR_FLAG_ACTIVE);
    };

    inline void isCalibrated(bool x) {
      _flags = (x) ? (_flags | MANUVR_SENSOR_FLAG_CALIBRATED) : (_flags & ~MANUVR_SENSOR_FLAG_CALIBRATED);
    };

    SensorError define_datum(const DatumDef*);

  private:
    UUID uuid;          // A cross-platform unique ID for this sensor.
    const char* name;   // This is the name of the sensor.
    SensorDatum*   datum_list  = nullptr; // Linked list of data that this sensor might carry.
    SensorCallBack ar_callback = nullptr; // The pointer to the callback function used for autoreporting.
    long           updated_at  = 0;       // When was the last update?
    uint8_t        _flags      = 0;       // Holds elementary state and capability info.

    void insert_datum(SensorDatum*);
    SensorError mark_dirty(uint8_t);   // Marks a specific datum in this sensor as dirty.
};

#endif  // __MANUVR_SENSORS_H
