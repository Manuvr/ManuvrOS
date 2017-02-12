/*
File:   InertialMeasurement.cpp
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


#include "InertialMeasurement.h"

/**
* Vanilla constructor. Vector class will initiallize conponents to zero.
*/
InertialMeasurement::InertialMeasurement() {
  read_time = 0.0f;
}


/**
* Given pointers to the gyro, mag, and acceleration, copies the values into this class instance.
* Leaves time at the default of 0.0.
*/
InertialMeasurement::InertialMeasurement(Vector3<float>* _g, Vector3<float>* _m, Vector3<float>* _a) {
  g_data.set(_g);
  a_data.set(_a);
  m_data.set(_m);
}


/**
* Given pointers to the gyro, mag, acceleration, and time, copies the values into this class instance.
*/
void InertialMeasurement::set(Vector3<float>* _g, Vector3<float>* _m, Vector3<float>* _a, float _rtime) {
  g_data.set(_g);
  a_data.set(_a);
  m_data.set(_m);
  read_time = _rtime;
}


/**
* Wipes the instance. Sets all member data to 0.0.
*/
void InertialMeasurement::wipe() {
  a_data(0.0f, 0.0f, 0.0f);
  g_data(0.0f, 0.0f, 0.0f);
  m_data(0.0f, 0.0f, 0.0f);
  read_time = 0.0f;
}


#if defined(MANUVR_DEBUG)
void InertialMeasurement::printDebug(StringBuilder* output) {
  output->concatf("Measurement taken at %uus\n", (unsigned long)read_time);
  output->concatf("  G: (%f, %f, %f) taken at %uus\n", (double)g_data.x, (double)g_data.y, (double)g_data.z);
  output->concatf("  A: (%f, %f, %f) taken at %uus\n", (double)a_data.x, (double)a_data.y, (double)a_data.z);
  output->concatf("  M: (%f, %f, %f) taken at %uus\n", (double)m_data.x, (double)m_data.y, (double)m_data.z);
}
#endif
