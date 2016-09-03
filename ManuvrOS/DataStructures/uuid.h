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

#ifndef OC_UUID_H
#define OC_UUID_H

#include <inttypes.h>

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct {
  uint8_t id[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
} UUID;

void uuid_from_str(const char *str, UUID*);
void uuid_to_str(const UUID*, char *buffer, int buflen);
void uuid_gen(UUID*);
int  uuid_compare(UUID*, UUID*);
void uuid_copy(UUID* src, UUID* dest);


#ifdef __cplusplus
}
#endif

#endif /* OC_UUID_H */
