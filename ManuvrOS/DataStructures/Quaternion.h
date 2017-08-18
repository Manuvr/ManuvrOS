/*
File:   Quaternion.h
Author: J. Ian Lindsay
Date:   2014.03.10

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

#ifndef __MANUVR_DS_QUATERNION_H
#define __MANUVR_DS_QUATERNION_H

#include "DataStructures/StringBuilder.h"


class Quaternion {
  public:
    float x;
    float y;
    float z;
    float w;

    Quaternion();
    Quaternion(float x, float y, float z, float w);

    void set(float x, float y, float z, float w);
    float normalize();

    void setDown(float n_x, float n_y, float n_z);

    void toString(StringBuilder *);
    void printDebug(StringBuilder *);
};

typedef Quaternion Vector4f;


#endif   // __MANUVR_DS_QUATERNION_H
