#ifndef QUATERNION_H
#define QUATERNION_H

#include "StringBuilder/StringBuilder.h"


class Quaternion {
  public:
    float x;
    float y;
    float z;
    float w;
    
    Quaternion();
    Quaternion(float x, float y, float z, float w);
    void toString(StringBuilder *);
};

typedef Quaternion Vector4f;


#endif
