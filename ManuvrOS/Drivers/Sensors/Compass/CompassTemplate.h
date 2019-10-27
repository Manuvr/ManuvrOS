#ifndef __COMPASS_TEMPLATE_H__
#define __COMPASS_TEMPLATE_H__

#include <Platform/Platform.h>
#include <DataStructures/Vector3.h>


enum class HeadingType : uint8_t {
  COMPASS         = 0,   // Direction of magnetic North without corrections.
  MAGNETIC_NORTH  = 1,   // Direction of magnetic North with corrections.
  TRUE_NORTH      = 2,   // Direction of geographic North.
  WAYPOINT        = 3    // Direction towards the given lat/lon.
};

enum class CompassErr : int8_t {
  PARAM_MISSING  = -3,  // A parameter was not supplied or isn't available.
  PARAM_RANGE    = -2,  // A parameter was out of range.
  UNSPECIFIED    = -1,  // Something failed. Not sure what.
  NONE           = 0,   // No errors.
  UNLIKELY       = 1,   // No errors, but the field is too strong to be Earth's.
  STALE          = 2    // No errors, but data is stale since last check.
};


class Compass {
  public:
    Compass();
    virtual ~Compass() {};

    virtual void       printDebug() =0;
    virtual CompassErr calibrate()  =0;

    CompassErr getBearing(HeadingType, float*);
    CompassErr getBearingToWaypoint(float*, double, double);
    CompassErr setGnomons(GnomonType mag, GnomonType grav);
    CompassErr setOrientation(Vector3f64* v);
    CompassErr setGravity(float x, float y, float z);
    inline CompassErr setGravity(Vector3f64* v) { return setGravity(v->x, v->y, v->z); };
    inline void setDelination(float v) {        _declination = v;             };
    inline void setLatLong(double lat, double lon) {
      _latitude  = lat;
      _longitude = lon;
    };
    inline Vector3f64 getFieldVector() {   return &_field;   };

    inline bool deviceFound() {        return _dev_found;    };
    inline bool initialized() {        return _initialized;  };
    inline bool isCalibrated() {       return _calibrated;   };

    static const char* errorString(CompassErr);


  protected:
    bool     _dev_found    = false;
    bool     _initialized  = false;
    bool     _calibrated   = false;
    GnomonType _orient_mag = GnomonType::RH_POS_X;
    GnomonType _orient_grv = GnomonType::RH_NEG_Z;  // Most sensors point "up".
    double   _latitude     = 0.0;
    double   _longitude    = 0.0;
    float    _declination  = 0.0;
    float    _bearings[3]  = {0.0, 0.0, 0.0};
    uint32_t _bearing_last_update[3] = {0, 0, 0};

    // Where is "down"? Always normalized and construed as RH_POS_X.
    //   Setter fxn is responsible for making the correction, if necessary.
    // TODO: Should be a quat?
    Vector3f64 _gravity;

    // Static orientation of the sensor within the unit. This is always
    //   normalized and construed as being RH_POS_X.
    // TODO: Should be a quat?
    Vector3f64 _orientation;

    // Generated during calibration. This is a corrective vector applied to
    //   _field to account for static distortions in the sensor's environment.
    //   Setter fxn is responsible for making the correction, if necessary.
    Vector3f64 _offset_vector;

    // Where is "Magnetic North"? Always construed as RH_POS_X.
    // Setter fxn is responsible for making the correction, if necessary.
    Vector3f64 _field;


    int8_t _set_field(float, float, float);
};

#endif   // __COMPASS_TEMPLATE_H__
