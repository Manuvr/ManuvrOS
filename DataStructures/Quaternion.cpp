#include "Quaternion.h"
Quaternion::Quaternion() {
  x = 0.0f;
  y = 0.0f;
  z = 0.0f;
  w = 0.0f;
}

Quaternion::Quaternion(float n_x, float n_y, float n_z, float n_w) {
  w = n_w;
  x = n_x;
  y = n_y;
  z = n_z;
}



/**
* TODO: Is this output order correct?
*/
void Quaternion::toString(StringBuilder *output) {
  output->concat((unsigned char*) &w, 4);
  output->concat((unsigned char*) &x, 4);
  output->concat((unsigned char*) &y, 4);
  output->concat((unsigned char*) &z, 4);
  
}

