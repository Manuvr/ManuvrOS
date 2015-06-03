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
    
    void set(float x, float y, float z, float w);

    void setDown(float n_x, float n_y, float n_z);

    void toString(StringBuilder *);
    void printDebug(StringBuilder *);
};

typedef Quaternion Vector4f;


#endif
