#include <Platform/Platform.h>
#include <DataStructures/Vector3.h>
#include "CompassTemplate.h"


const char* Compass::errorString(CompassErr x) {
  switch (x) {
    case CompassErr::PARAM_MISSING:  return "Paramter unavailable";
    case CompassErr::PARAM_RANGE:    return "Paramter out of range";
    case CompassErr::UNSPECIFIED:    return "Unspecified";
    case CompassErr::NONE:           return "None";
    case CompassErr::STALE:          return "Stale result";
    case CompassErr::UNLIKELY:       return "Field is too big to be Earth";
  }
  return "UNKNOWN";
}



Compass::Compass() {
}


CompassErr Compass::getBearing(HeadingType ht, float* bearing) {
  uint8_t idx = 2;
  switch (ht) {
    case HeadingType::COMPASS:        idx--;
    case HeadingType::MAGNETIC_NORTH: idx--;
    case HeadingType::TRUE_NORTH:     *bearing = _bearings[idx];
      break;
    case HeadingType::WAYPOINT:
      return CompassErr::PARAM_RANGE;
    default:
      return CompassErr::UNSPECIFIED;
  }
  return CompassErr::NONE;
}


CompassErr Compass::getBearingToWaypoint(float* bearing, double lat, double lon) {
  return CompassErr::UNSPECIFIED;
}


CompassErr Compass::setGnomons(GnomonType mag, GnomonType grv) {
  uint8_t mag_int = (uint8_t) mag;
  uint8_t grv_int = (uint8_t) grv;
  if ((GnomonType::UNDEFINED != mag) && ((mag_int & 0x0F) == mag_int)) {
    _orient_mag = mag;
  }
  if ((GnomonType::UNDEFINED != grv) && ((grv_int & 0x0F) == grv_int)) {
    _orient_grv = grv;
  }
  return CompassErr::NONE;
}


// Converts the input vector into our default internal coordinates.
int8_t Compass::_set_field(float x, float y, float z) {
  switch (_orient_mag) {
    case GnomonType::RH_POS_X:
    case GnomonType::LH_NEG_X:    _field(x, y, z);         break;
    case GnomonType::RH_POS_Y:
    case GnomonType::LH_NEG_Y:    _field(y, z, x);         break;
    case GnomonType::RH_POS_Z:
    case GnomonType::LH_NEG_Z:    _field(z, x, y);         break;
    case GnomonType::RH_NEG_X:
    case GnomonType::LH_POS_X:    _field(-1.0 * x, y, z);  break;
    case GnomonType::RH_NEG_Y:
    case GnomonType::LH_POS_Y:    _field(-1.0 * y, z, x);  break;
    case GnomonType::RH_NEG_Z:
    case GnomonType::LH_POS_Z:    _field(-1.0 * z, x, y);  break;
    case GnomonType::UNDEFINED:   return -1;
  }
  return 0;
}


// Converts the input vector into our default internal coordinates and
//   normalizes it.
CompassErr Compass::setGravity(float x, float y, float z) {
  switch (_orient_grv) {
    case GnomonType::RH_POS_X:
    case GnomonType::LH_NEG_X:    _gravity(x, y, z);         break;
    case GnomonType::RH_POS_Y:
    case GnomonType::LH_NEG_Y:    _gravity(y, z, x);         break;
    case GnomonType::RH_POS_Z:
    case GnomonType::LH_NEG_Z:    _gravity(z, x, y);         break;
    case GnomonType::RH_NEG_X:
    case GnomonType::LH_POS_X:    _gravity(-1.0 * x, y, z);  break;
    case GnomonType::RH_NEG_Y:
    case GnomonType::LH_POS_Y:    _gravity(-1.0 * y, z, x);  break;
    case GnomonType::RH_NEG_Z:
    case GnomonType::LH_POS_Z:    _gravity(-1.0 * z, x, y);  break;
    case GnomonType::UNDEFINED:   return CompassErr::PARAM_MISSING;
  }
  _gravity.normalize();
  return CompassErr::NONE;
}

// Converts the input vector into our default internal coordinates and
//   normalizes it.
CompassErr Compass::setOrientation(Vector3f64* v) {
  _orientation.set(v);
  _orientation.normalize();
  return CompassErr::NONE;
}
