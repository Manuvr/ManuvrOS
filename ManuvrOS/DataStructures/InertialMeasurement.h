#ifndef __IIU_MEASUREMENT_H__
#define __IIU_MEASUREMENT_H__

#include <inttypes.h>
#include "Vector3.h"

class StringBuilder;


/*
* This is the measurement class that is pushed downstream to the IIU class, which will integrate it
*   into a sensible representation of position and orientation, taking error into account.
*/
class InertialMeasurement {
  public:
    Vector3<float>      g_data;         // The vector of the gyro data.
    Vector3<float>      a_data;         // The vector of the accel data.
    Vector3<float>      m_data;         // The vector of the mag data.

    float            read_time;         // The system time when the value was read from the sensor.

    InertialMeasurement();
    InertialMeasurement(Vector3<float>*, Vector3<float>*, Vector3<float>*);

    void set(Vector3<float>*, Vector3<float>*, Vector3<float>*, float);
    void wipe();
    void printDebug(StringBuilder* output);


  private:
};



#endif  //__IIU_MEASUREMENT_H__
