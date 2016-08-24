/*
* Taken from Intel's iotivity-constrained package and
*   reworked slightly.
*                                 ---J. Ian Lindsay
*/

/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "uuid.h"
#include <Platform/Platform.h>


void uuid_from_str(const char *str, UUID *uuid) {
  int j = 0;
  int k = 1;
  uint8_t c = 0;
  uint8_t a = 0;

  for (int i = 0; i < strlen(str); i++) {
    switch (str[i]) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
        c |= str[i] - 48;
        a++;
        break;
      case 65:
      case 97:
        c |= 0x0a;
        a++;
        break;
      case 66:
      case 98:
        c |= 0x0b;
        a++;
        break;
      case 67:
      case 99:
        c |= 0x0c;
        a++;
        break;
      case 68:
      case 100:
        c |= 0x0d;
        a++;
        break;
      case 69:
      case 101:
        c |= 0x0e;
        a++;
        break;
      case 70:
      case 102:
        c |= 0x0f;
        a++;
        break;
    }

    if (a) {
      if ((j + 1) * 2 == k) {
        uuid->id[j++] = c;
        c = 0;
      }
      else {
        c = c << 4;
      }
      k++;
      a = 0;
    }
  }
}

void uuid_to_str(const UUID* uuid, char *buffer, int buflen) {
  if (buflen < 37) return;
  int j = 0;
  for (int i = 0; i < 16; i++) {
    switch(i) {
      case 4:
      case 6:
      case 8:
      case 10:
        snprintf(&buffer[j], 2, "-");
        j++;
        break;
    }
    snprintf(&buffer[j], 3, "%02x", uuid->id[i]);
    j += 2;
  }
}

void uuid_gen(UUID *uuid) {
  for (int i = 0; i < 4; i++) {
    *((uint8_t*)&uuid->id[i * 4]) = randomInt();
  }

  /*  From RFC 4122
      Set the two most significant bits of the
      clock_seq_hi_and_reserved (8th octect) to
      zero and one, respectively.
  */
  uuid->id[8] &= 0x3f;
  uuid->id[8] |= 0x40;

  /*  From RFC 4122
      Set the four most significant bits of the
      time_hi_and_version field (6th octect) to the
      4-bit version number from (0 1 0 0 => type 4)
      Section 4.1.3.
  */
  uuid->id[6] &= 0x0f;
  uuid->id[6] |= 0x40;
}
