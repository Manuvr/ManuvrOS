/*
File:   InertialMeasurement.h
Author: J. Ian Lindsay
Date:   2014.05.12

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


This is a container class for a single IMU measurement frame.
*/

#ifndef __MANUVR_DS_IIU_MEASUREMENT_H
#define __MANUVR_DS_IIU_MEASUREMENT_H

#include <inttypes.h>
#include "Vector3.h"

#if defined(MANUVR_DEBUG)
#include "StringBuilder.h"
#endif

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
    #if defined(MANUVR_DEBUG)
      void printDebug(StringBuilder* output);
    #endif

  private:
};



#endif  // __MANUVR_DS_IIU_MEASUREMENT_H
