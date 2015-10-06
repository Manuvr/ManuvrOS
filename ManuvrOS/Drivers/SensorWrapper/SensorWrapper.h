/*
File:   SensorWrapper.h
Author: J. Ian Lindsay
Date:   2013.11.28


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


This class is a generic interface to a sensor. That sensor might measure many things.

*/


#ifndef SENSOR_WRAPPER_H
#define SENSOR_WRAPPER_H

using namespace std;

#include <inttypes.h>
#include <string.h>
#include "StringBuilder/StringBuilder.h"

#include "StaticHub/StaticHub.h"

#ifdef ARDUINO
  #include "Arduino.h"
#elif defined(STM32F4XX)
  #include "FirmwareDefs.h"
#endif


/**************************************************************************
* Integer type-codes...                                                   *
* When sensor data is being handled in an abstract way, we need some way  *
*   of determining the format that the sensor intended. By doing this, we *
*   can offload the burden of type sizes, alignment and endianness onto   *
*   the compiler for whatever system is running this code.                *
**************************************************************************/
#include "ManuvrOS/EnumeratedTypeCodes.h"

/* Examples of JSON data returned by this class...
* A map (definition):
* {"9b5d0e9bd2f593a89343328dcdb47b52":{"name":"GPS","hardware":true,"data":[{"v_id":0,"desc":"Latitude","unit":"degrees","autoreport":0},{"v_id":1,"desc":"Longitude","unit":"degrees","autoreport":1},{"v_id":2,"desc":"Altitude","unit":"meters","autoreport":0}]}}
*
* Note that the map doesn't contain any tag near the root to tell the host that a sensor is being discussed. That needs to be handled by another mechanism.
* It was done this way to allow many such map strings to be wrapped into a single JSON array.
*
* Now suppose that sensor automatically reports some of its values on changes...
* {"9b5d0e9bd2f593a89343328dcdb47b52":{"value":{"vid_1":"698"},"time":1386118856}}
*
* Again, note that there is no mention of sensors anywhere. 
*/

#include "ManuvrOS/Drivers/DeviceWithRegisters/DeviceRegister.h"



/*
* Sometimes, we want to direct a datum to autoreport by calling a function.
* If provided in the datum definition, when the datum becomes dirty, it will
*   call the function of this form.
* The first argument is the s_id of the sensor, and the second argument is the datum ID
*   within the sensor.
*/
typedef void (*SensorCallBack) (char*, int8_t);


/*
* This struct helps us present and manage the unique data that comes out of each sensor.
* Every instance of this class should have at least one of these defined for it.
*
* Regarding the memory cost associated with tracking a datum, the analysis below is ONLY overhead. It does not
*   take the size of the actual data into consideration. It also assumes 32-bit pointers. So if we have a sensor
*   that gives us a float (32-bit [4 byte]), then tracking that datum actually costs us...
*       sizeof(SensorDatum) + sizeof(float)
*
*   This might seem wasteful for data types whose size is <= 4 bytes, but that is the cost of abstraction.
*/
typedef struct datum_list_t {
  const char*    description;   // A brief description of the datum for humans.                                        4 bytes
  const char*    units;         // Real-world units that this datum measures.                                          4 bytes
  uint8_t        autoreport;    // Auto-reporting setting. Only meaningful if ar_callback is non-null.                 1 byte
  SensorCallBack ar_callback;   // The pointer to the callback function used for autoreporting.                        4 bytes
  int8_t         v_id;          // This datum ID within the sensor object.                                             1 byte
  bool           is_dirty;      // Is this datum dirty (IE, the last operation was a write)?                           1 byte
  void*          data;          // The actual data returned from the sensor.                                           4 bytes
  uint8_t        type_id;       // The type of the data member.                                                        1 byte
  int32_t        data_len;      // The length of the data member.                                                      4 bytes
  datum_list_t*  next;          // Linked-list. This is the ref to the next datum, if there is one.                    4 bytes
} SensorDatum;                  //                                                       Total memory use per-datum:  28 bytes




class SensorWrapper {
  public:
    const char* name;     // This is the name of the sensor.
    const char* s_id;     // A cross-platform unique ID for this sensor.
    bool isHardware;      // Is this sensor a piece of hardware?
    bool sensor_active;   // Is this sensor able to be read? IE: Did it init() correctly?

    SensorWrapper(void);
    ~SensorWrapper(void);
    
    void setSensorId(const char *);
    
    long lastUpdate(void);                                 // Datetime stamp from the last update.
    bool isDirty(uint8_t);                                 // Is the given particular datum dirty?
    bool isDirty(void);                                    // Is ANYTHING in this sensor dirty?
    void setAutoReporting(uint8_t);                        // Sets all data in this sensor to the given autoreporting state.
    int8_t setAutoReporting(uint8_t, uint8_t);             // Sets a given datum in this sensor to the given autoreporting state.
    void setAutoReportCallback(SensorCallBack);            // Sets a callback function for all data in this sensor.
    int8_t setAutoReportCallback(uint8_t, SensorCallBack); // Sets a callback function for a given datum in this sensor.
    int8_t markClean(void);                                // Marks all data in the sensor clean.
    int8_t markClean(uint8_t);                             // Marks all data in the sensor clean.

    int8_t readAsString(StringBuilder*);                   // Returns the sensor read as a string.
    int8_t readAsString(uint8_t, StringBuilder*);          // Returns the given datum read as a string.

    // Virtual functions. These MUST be overridden by the extending class.
    virtual int8_t init(void) =0;                                    // Call this to initialize the sensor.
    virtual int8_t readSensor(void) =0;                              // Actually read the sensor. Returns an error-code.
    virtual int8_t setParameter(uint16_t reg, int len, uint8_t*) =0; // Used to set operational parameters for the sensor.
    virtual int8_t getParameter(uint16_t reg, int len, uint8_t*) =0; // Used to read operational parameters from the sensor.
    /* A quick note about how setParameter() should be implemented....
    The idea behind this was to work like common i2c sensors in that sensor classes have
    software "registers" that are written to to control operational aspects of the represented sensor. 
    Sensor classes are free to implement whatever registers they like (up to 16-bit's worth), but
    convention of the sensor should probably take priority. IE... if you are writing a class for a
    sensor that has a control register at 0x40, you should probably case off that value to cause minimal
    confusion later. See some of the provided sensor classes for examples.
    */
    

    // Static functions...
    static void issue_json_map(StringBuilder*, SensorWrapper*);              // Returns a pointer to a buffer containing the JSON def of the given sensor.
    static void issue_json_value(StringBuilder*, SensorWrapper*);            // Returns a pointer to a buffer containing the JSON values for the given sensor.
    static void issue_json_value(StringBuilder*, SensorWrapper*, uint8_t);   // Returns a pointer to a buffer containing the JSON value for the given sensor's given datum.
#ifndef ARDUINO
    static long millis();
#endif
    // Sensors can automatically report their values when something in the sensor changes...
    static constexpr const uint8_t SENSOR_REPORTING_OFF         = 0;
    static constexpr const uint8_t SENSOR_REPORTING_NEW_VALUE   = 1;
    static constexpr const uint8_t SENSOR_REPORTING_EVERY_READ  = 2;

    // These are possible error codes...
    static constexpr const int8_t SENSOR_ERROR_NO_ERROR         = 0;    // There was no error.
    static constexpr const int8_t SENSOR_ERROR_ABSENT           = -1;   // We failed to talk to the sensor.
    static constexpr const int8_t SENSOR_ERROR_OUT_OF_MEMORY    = -2;   // Couldn't allocate memory for some sensor-related task.
    static constexpr const int8_t SENSOR_ERROR_WRONG_IDENTITY   = -3;   // Some sensors come with ID markers in them. If we get one wrong, this happens.
    static constexpr const int8_t SENSOR_ERROR_NOT_LOCAL        = -4;   // If we try to read or change parameters of a sensor that isn't attached to us.
    static constexpr const int8_t SENSOR_ERROR_INVALID_PARAM_ID = -5;   // If we try to read or change a sensor parameter that isn't supported.
    static constexpr const int8_t SENSOR_ERROR_INVALID_PARAM    = -6;   // If we try to set a sensor parameter to something invalid for an extant register.
    static constexpr const int8_t SENSOR_ERROR_NOT_CALIBRATED   = -7;   // If we try to read a sensor that is uncalibrated. Not all sensors require this.
    static constexpr const int8_t SENSOR_ERROR_INVALID_DATUM    = -8;   // If we try to do an operation on a datum that doesn't exist for this sensor.
    static constexpr const int8_t SENSOR_ERROR_UNHANDLED_TYPE   = -9;   // Issued when we ask for a string conversion that we don't support.
    static constexpr const int8_t SENSOR_ERROR_NULL_POINTER     = -10;  // What happens when we try to do I/O on a null pointer.
    static constexpr const int8_t SENSOR_ERROR_BUS_ERROR        = -11;  // If there was some generic bus error that we otherwise can't pinpoint.
    static constexpr const int8_t SENSOR_ERROR_BUS_ABSENT       = -12;  // If the requested bus is not accessible.
    static constexpr const int8_t SENSOR_ERROR_NOT_WRITABLE     = -13;  // If we try to write to a read-only register.
    static constexpr const int8_t SENSOR_ERROR_REG_NOT_DEFINED  = -14;  // If we try to do I/O on a non-existent register.
    static constexpr const int8_t SENSOR_ERROR_DATA_EXHAUSTED   = -15;  // If we try to poll sensor data faster than the sensor can produce it.
    static constexpr const int8_t SENSOR_ERROR_BAD_TYPE_CONVERT = -16;  // If we ask the class to convert types in a way that isn't possible.
    static constexpr const int8_t SENSOR_ERROR_MISSING_CONF     = -17;  // If we ask the sensor class to perform an operation on parameters that it doesn't have.
    static constexpr const int8_t SENSOR_ERROR_NOT_INITIALIZED  = -18;  // 
    static constexpr const int8_t SENSOR_ERROR_UNDEFINED_ERR    = -128; // If we try to set a sensor parameter to something invalid for an extant register.

    static constexpr const char* SENSOR_DATUM_NOT_FOUND = "Sensor datum not found";


  protected:
    bool     is_dirty;          // Has the sensor been updated?
    long     updated_at;        // When was the last update?
    uint8_t  data_count;        // How many datums does this sensor produce?
    SensorDatum* datum_list;    // Linked list of data that this sensor might carry.

    bool defineDatum(const char*, const char*, uint8_t);
    bool defineDatum(int, const char*, const char*, uint8_t, uint8_t);
    SensorDatum* get_datum(uint8_t);

    /* These are inlines that we define for ease-of-use from extending classes. If we didn't do this, we
         would need to cast all sensor readings to void* wherever we updateDatum(). All wind up at the
         same place. */
    inline int8_t updateDatum(uint8_t dat, double val) {         return updateDatum(dat, (void*) &val); }
    inline int8_t updateDatum(uint8_t dat, float val) {          return updateDatum(dat, (void*) &val); }
    inline int8_t updateDatum(uint8_t dat, int val) {            return updateDatum(dat, (void*) &val); }
    inline int8_t updateDatum(uint8_t dat, unsigned int val) {   return updateDatum(dat, (void*) &val); }
    inline int8_t updateDatum(uint8_t dat, char* val) {          return updateDatum(dat, (void*) val); }
    inline int8_t updateDatum(uint8_t dat, unsigned char* val) { return updateDatum(dat, (void*) val); }
    int8_t updateDatum(uint8_t, void*);   // This is the actual implementation.

    inline int8_t readDatumRaw(uint8_t dat, double *val) {         return readDatumRaw(dat, (void*) &val); }
    inline int8_t readDatumRaw(uint8_t dat, float *val) {          return readDatumRaw(dat, (void*) &val); }
    inline int8_t readDatumRaw(uint8_t dat, int *val) {            return readDatumRaw(dat, (void*) &val); }
    inline int8_t readDatumRaw(uint8_t dat, unsigned int *val) {   return readDatumRaw(dat, (void*) &val); }
    inline int8_t readDatumRaw(uint8_t dat, char** val) {          return readDatumRaw(dat, (void*) &val); }
    inline int8_t readDatumRaw(uint8_t dat, unsigned char** val) { return readDatumRaw(dat, (void*) &val); }
    int8_t readDatumRaw(uint8_t, void*);   // This is the actual implementation.
    
    // Some constants for units...
    static const char* COMMON_UNITS_DEGREES;
    static const char* COMMON_UNITS_DEG_SEC;
    static const char* COMMON_UNITS_C;
    static const char* COMMON_UNITS_ONOFF;
    static const char* COMMON_UNITS_LUX;
    static const char* COMMON_UNITS_METERS;
    static const char* COMMON_UNITS_GAUSS;
    static const char* COMMON_UNITS_MET_SEC;
    static const char* COMMON_UNITS_ACCEL;
    static const char* COMMON_UNITS_EPOCH;
    static const char* COMMON_UNITS_PRESSURE;
    static const char* COMMON_UNITS_VOLTS;
    static const char* COMMON_UNITS_U_VOLTS;
    static const char* COMMON_UNITS_AMPS;
    static const char* COMMON_UNITS_WATTS;
    static const char* COMMON_UNITS_PERCENT;
    static const char* COMMON_UNITS_U_TESLA;


  private:
    void insert_datum(SensorDatum*);
    void clear_data(SensorDatum*);
    int8_t writeDatumDataToSB(SensorDatum*, StringBuilder*);
    
    int8_t mark_dirty(void);      // Marks this sensor dirty.
    int8_t mark_dirty(uint8_t);   // Marks a specific datum in this sensor as dirty.
};

#endif

