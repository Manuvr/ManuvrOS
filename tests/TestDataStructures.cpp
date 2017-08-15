/*
File:   TestDataStructures.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests on raw data-handling classes.
*/

#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <DataStructures/PriorityQueue.h>
#include <DataStructures/Vector3.h>
#include <DataStructures/Quaternion.h>
#include <DataStructures/StringBuilder.h>
#include <DataStructures/RingBuffer.h>
#include <DataStructures/BufferPipe.h>
#include <DataStructures/uuid.h>

#include <Platform/Platform.h>
#include <Drivers/Sensors/SensorWrapper.h>

#include <XenoSession/XenoSession.h>
#include <XenoSession/CoAP/CoAPSession.h>
#include <XenoSession/MQTT/MQTTSession.h>
#include <XenoSession/Manuvr/ManuvrSession.h>
#include <XenoSession/Console/ManuvrConsole.h>

#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <Transports/StandardIO/StandardIO.h>
#include <Transports/ManuvrSocket/ManuvrSocket.h>
#include <Transports/ManuvrSocket/ManuvrUDP.h>
#include <Transports/ManuvrSocket/ManuvrTCP.h>
#include <Transports/ManuvrXport.h>

#if defined(MANUVR_CBOR)
  #include <Types/cbor-cpp/cbor.h>
#endif

int test_StringBuilder(void) {
  StringBuilder log("===< StringBuilder >====================================\n");
  StringBuilder *heap_obj = new StringBuilder("This is datas we want to transfer.");
  StringBuilder stack_obj;
  StringBuilder tok_obj;

  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();

  tok_obj.concat("This");
  log.concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
  log.concatf("\t tok_obj count:   %d\n", tok_obj.count());
  tok_obj.concat(" This");
  log.concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
  log.concatf("\t tok_obj count:   %d\n", tok_obj.count());
  tok_obj.concat("   This");
  log.concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
  log.concatf("\t tok_obj count:   %d\n", tok_obj.count());

  log.concatf("\t Heap obj before culling:   %s\n", heap_obj->string());

  while (heap_obj->length() > 10) {
    heap_obj->cull(5);
    log.concatf("\t Heap obj during culling:   %s\n", heap_obj->string());
  }

  log.concatf("\t Heap obj after culling:   %s\n", heap_obj->string());

  heap_obj->prepend("Meaningless data ");
  heap_obj->concat(" And stuff tackt onto the end.");

  stack_obj.concatHandoff(heap_obj);

  delete heap_obj;

  stack_obj.split(" ");

  log.concatf("\t Final Stack obj:          %s\n", stack_obj.string());

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}


/**
 * [vector3_float_test description]
 * @param  x float
 * @param  y float
 * @param  z float
 * @return   0 on success. Non-zero on failure.
 */
int vector3_float_test(float x, float y, float z) {
  StringBuilder log("===< Vector3<float> >===================================\n");
  Vector3<float> test;
  Vector3<float> test_vect_0(-0.4f, -0.1f, 0.4f);
  Vector3<float> *test1 = &test_vect_0;
  log.concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(1.0f, 0.5f, 0.24f);
  log.concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test_vect_0.x, test_vect_0.y, test_vect_0.z);
  log.concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test1->x, test1->y, test1->z);
  log.concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(x, y, z);
  log.concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));
  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}



int test_PriorityQueue(void) {
  StringBuilder log("===< PriorityQueue >====================================\n");
  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}

/*
* These tests are meant to test the memory-management implications of
*   the Argument class.
*/
int test_Arguments_MEM_MGMT() {
  return 0;
}


/**
* [test_CBOR_Argument description]
* @return [description]
*/
int test_CBOR_Argument() {
  int return_value = -1;
  StringBuilder log("===< Arguments CBOR >===================================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  randomInt();
  int16_t  val1  = (int16_t)  randomInt();
  int8_t   val2  = (int8_t)   randomInt();
  uint32_t val3  = (uint32_t) randomInt();
  uint16_t val4  = (uint16_t) randomInt();
  uint8_t  val5  = (uint8_t)  randomInt();
  float    val6  = 0.4123f;

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Argument a(val0);

  a.setKey("value0");
  a.append(val1)->setKey("value1");
  a.append(val2)->setKey("value");;  // NOTE: Mixed in with non-KVP.
  a.append(val3);
  a.append(val4)->setKey("value4");
  a.append(val5)->setKey("value5");
  a.append(val6)->setKey("value6");

  a.printDebug(&log);

  if (0 <= Argument::encodeToCBOR(&a, &shuttle)) {
    log.concatf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    shuttle.printDebug(&log);
    log.concat("\n");

    Argument* r = Argument::decodeFromCBOR(&shuttle);
    if (nullptr != r) {
      log.concat("CBOR decoded:\n");
      r->printDebug(&log);

      log.concat("\n");
      if ((0 == r->getValueAs((uint8_t) 0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->getValueAs((uint8_t) 1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->getValueAs((uint8_t) 2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->getValueAs((uint8_t) 3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->getValueAs((uint8_t) 4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->getValueAs((uint8_t) 5, &ret5)) && (ret5 == val5)) {
                  if ((0 == r->getValueAs((uint8_t) 6, &ret6)) && (ret6 == val6)) {
                    if (r->argCount() == a.argCount()) {
                      return_value = 0;
                    }
                    else log.concatf("Arg counts don't match: %d vs %d\n", r->argCount(), a.argCount());
                  }
                  else log.concatf("Failed to vet key 'value6'... %.3f vs %.3f\n", ret6, val6);
                }
                else log.concatf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else log.concatf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else log.concatf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else log.concatf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else log.concatf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else log.concatf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else log.concat("Failed to decode Argument chain from CBOR...\n");
  }
  else log.concat("Failed to encode Argument chain into CBOR...\n");

  if (return_value) {
    a.printDebug(&log);
  }
  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


/**
* Test the capability of Arguments to hold KVP data.
* @return 0 on pass. Non-zero otherwise.
*/
int test_Arguments_KVP() {
  int return_value = -1;
  StringBuilder log("===< Arguments KVP >====================================\n");
  Argument a;

  uint32_t val0  = (uint32_t) randomInt();
  uint16_t val1  = (uint16_t) randomInt();
  uint8_t  val2  = (uint8_t)  randomInt();
  int32_t  val3  = (int32_t)  randomInt();
  int16_t  val4  = (int16_t)  randomInt();
  int8_t   val5  = (int8_t)   randomInt();

  uint32_t ret0 = 0;
  uint16_t ret1 = 0;
  uint8_t  ret2 = 0;
  int32_t  ret3 = 0;
  int16_t  ret4 = 0;
  int8_t   ret5 = 0;

  log.concat("Adding arguments...\n\n");

  a.append(val0)->setKey("value0");
  a.append(val1)->setKey("value1");
  a.append(val2);  // NOTE: Mixed in with non-KVP.
  a.append(val3);
  a.append(val4)->setKey("value4");
  a.append(val5)->setKey("value5");

  a.printDebug(&log);
  log.concat("\n");

  StringBuilder key_set;
  int key_count = a.collectKeys(&key_set);
  log.concatf("Breadth-first keyset (%d total keys):   ", key_count);
  for (int i = 0; i < key_count; i++) {
    log.concatf("%s ", key_set.position(i));
  }
  log.concat("\n");

  if ((0 == a.getValueAs("value0", &ret0)) && (ret0 == val0)) {
    if ((0 == a.getValueAs("value4", &ret4)) && (ret4 == val4)) {
      if ((0 == a.getValueAs("value5", &ret5)) && (ret5 == val5)) {
        if (0 != a.getValueAs("non-key", &ret0)) {
          // We shouldn't be able to get a value for a key that doesn't exist...
          if (0 != a.getValueAs("non-key", &ret0)) {
            // Nor for a NULL key...
            return_value = 0;
          }
        }
      }
      else log.concat("Failed to vet key 'value5'...\n");
    }
    else log.concat("Failed to vet key 'value4'...\n");
  }
  else log.concat("Failed to vet key 'value0'...\n");

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


/**
* These tests are for reference handling and proper type-assignment of internal
*   types.
* @return 0 on pass. Non-zero otherwise.
*/
int test_Arguments_InternalTypes() {
  int return_value = 1;
  StringBuilder log("===< Arguments Internal Types >=========================\n");
  StringBuilder val0("Some string");
  Argument a(&val0);
  a.printDebug(&log);

  StringBuilder* ret0 = nullptr;

  if (0 == a.getValueAs(&ret0)) {
    if (&val0 == ret0) {
      return_value = 0;
    }
    else log.concat("StringBuilder pointer retrieved from Argument is not the same as what went in. Fail...\n");
  }
  else log.concat("Failed to retrieve StringBuilder pointer.\n");

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


/**
* These tests are meant to test the mechanics of the pointer-hack on PODs.
* Failure here might result in segfaults. This also needs to be tested against
*   both 32/64-bit builds.
* @return 0 on pass. Non-zero otherwise.
*/
int test_Arguments_PODs() {
  int return_value = 1;
  StringBuilder log("===< Arguments POD >====================================\n");
  const char* val_base = "This is the base argument";
  uint32_t val0  = (uint32_t) randomInt();
  uint16_t val1  = (uint16_t) randomInt();
  uint8_t  val2  = (uint8_t)  randomInt();
  int32_t  val3  = (int32_t)  randomInt();
  int16_t  val4  = (int16_t)  randomInt();
  int8_t   val5  = (int8_t)   randomInt();
  float    val6  = (float)    randomInt()/1000000.0f;

  Argument a(val_base);

  int real_size = strlen(val_base)+1;  // We count the null-terminator.
  real_size += sizeof(val0);
  real_size += sizeof(val1);
  real_size += sizeof(val2);
  real_size += sizeof(val3);
  real_size += sizeof(val4);
  real_size += sizeof(val5);
  real_size += sizeof(val6);

  log.concatf("Adding arguements with a real size of %d bytes...\n", real_size);

  a.append(val0);
  a.append(val1);
  a.append(val2);
  a.append(val3);
  a.append(val4);
  a.append(val5);
  a.append(val6);
  a.printDebug(&log);

  if ((a.argCount() == 8) && (a.sumAllLengths() == real_size)) {
    uint32_t ret0 = 0;
    uint16_t ret1 = 0;
    uint8_t  ret2 = 0;
    int32_t  ret3 = 0;
    int16_t  ret4 = 0;
    int8_t   ret5 = 0;
    float    ret6 = 0.0;
    if ((0 == a.getValueAs(1, &ret0)) && (ret0 == val0)) {
      if ((0 == a.getValueAs(2, &ret1)) && (ret1 == val1)) {
        if ((0 == a.getValueAs(3, &ret2)) && (ret2 == val2)) {
          if ((0 == a.getValueAs(4, &ret3)) && (ret3 == val3)) {
            if ((0 == a.getValueAs(5, &ret4)) && (ret4 == val4)) {
              if ((0 == a.getValueAs(6, &ret5)) && (ret5 == val5)) {
                //if ((0 == a.getValueAs(7, (float*) &ret6)) && (ret6 == val6)) {
                  log.concatf("Test passes.\n", a.argCount());
                  return_value = 0;
                //}
                //else log.concatf("float failed (%f vs %f)...\n", (double) val6, (double) ret6);
              }
              else log.concatf("int8_t failed (%d vs %d)...\n", val5, ret5);
            }
            else log.concatf("int16_t failed (%d vs %d)...\n", val4, ret4);
          }
          else log.concatf("int32_t failed (%d vs %d)...\n", val3, ret3);
        }
        else log.concatf("uint8_t failed (%u vs %u)...\n", val2, ret2);
      }
      else log.concatf("uint16_t failed (%u vs %u)...\n", val1, ret1);
    }
    else log.concatf("uint32_t failed (%u vs %u)...\n", val0, ret0);
  }
  else {
    log.concatf("Total Arguments:      %d\tExpected 8.\n", a.argCount());
    log.concatf("Total payload size:   %d\tExpected %d.\n", a.sumAllLengths(), real_size);
    log.concat("\n");
  }

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


int test_Arguments() {
  int return_value = test_Arguments_KVP();
  if (0 == return_value) {
    return_value = test_Arguments_InternalTypes();
    if (0 == return_value) {
      return_value = test_Arguments_PODs();
      if (0 == return_value) {
      #if defined(MANUVR_CBOR)
        return_value = test_CBOR_Argument();
        if (0 == return_value) {

        }
      #endif
      }
    }
  }
  return return_value;
}


int test_BufferPipe() {
  StringBuilder log("===< BufferPipe >=======================================\n");
  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}


int test_RingBuffer() {
  int return_value = -1;
  StringBuilder log("===< RingBuffer >=======================================\n");
  const int TEST_SIZE = 18;
  RingBuffer<uint32_t> a(TEST_SIZE);
  if (a.allocated()) {
    log.concatf("RingBuffer under test is using %u bytes of heap to hold %u elements.\n", a.heap_use(), a.capacity());
    if (0 == a.count()) {
      unsigned int test_num = TEST_SIZE/3;
      uint32_t val;
      log.concat("\tInserting:");
      for (unsigned int i = 0; i < test_num; i++) {
        val = randomInt();
        if (a.insert(val)) {
          log.concat("\nFailed to insert.\n");
          printf((const char*) log.string());
          return -1;
        }
        log.concatf(" (%u: %08x)", a.count(), val);
      }
      if (test_num == a.count()) {
        log.concat("\n\tGetting:  ");
        for (unsigned int i = 0; i < test_num/2; i++) {
          unsigned int count = a.count();
          val = a.get();
          log.concatf(" (%u: %08x)", count, val);
        }
        unsigned int n = TEST_SIZE - a.count();
        log.concatf("\n\tRingBuffer should have space for %u more elements... ", n);
        for (unsigned int i = 0; i < n; i++) {
          if (a.insert(randomInt())) {
            log.concatf("Falsified. Count is %u\n", a.count());
            printf((const char*) log.string());
            return -1;
          }
        }
        if (a.count() == TEST_SIZE) {
          log.concatf("Verified. Count is %u\n", a.count());
          log.concat("\tOverflowing... ");
          if (a.insert(randomInt())) {
            log.concatf("Is handled correctly. Count is %u\n", a.count());
            log.concat("\tDraining... ");
            for (unsigned int i = 0; i < TEST_SIZE; i++) {
              val = a.get();
            }
            if (0 == a.count()) {
              log.concat("done.\n\tTrying to drive count negative... ");
              if (0 == a.get()) {
                if (0 == a.count()) {
                  log.concat("pass.\n");
                  return_value = 0;
                }
                else log.concatf("Count should still be 0 but is %u\n", a.count());
              }
              else log.concatf("Get on an empty buffer should return 0.\n");
            }
            else log.concatf("Count should have been 0 but is %u\n", a.count());
          }
          else log.concatf("Sadly worked. Count is %u\n", a.count());
        }
        else log.concatf("Count mismatch. Got %u but was expecting %u.\n", a.count(), TEST_SIZE);
      }
      else log.concatf("Fairly certain we inserted %u elements, but the count says %u.\n", test_num, a.count());
    }
    else log.concatf("Newly created RingBuffers ought to be empty. This one reports %u.\n", a.count());
  }
  else log.concat("\nFailed to allocate.\n");

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


/**
* UUID battery.
* @return 0 on pass. Non-zero otherwise.
*/
int test_UUID() {
  StringBuilder log("===< UUID >=============================================\n");
  StringBuilder temp;
  UUID test0;
  UUID test1;

  // Do UUID's initialize to zero?
  for (int i = 0; i < 16; i++) {
    if (0 != *((uint8_t*) &test0.id[i])) {
      log.concat("UUID should be initialized to zeros. It was not. Failing...\n");
      printf((const char*) log.string());
      return -1;
    }
  }

  // Does the comparison function work?
  if (uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these distinct. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf((const char*) log.string());
    return -1;
  }
  uuid_gen(&test0);
  // Does the comparison function work?
  if (0 == uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these the same. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf((const char*) log.string());
    return -1;
  }

  // Generate a whole mess of UUID and ensure that they are different.
  for (int i = 0; i < 10; i++) {
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    log.concat("temp0 bytes:  ");
    temp.printDebug(&log);
    temp.clear();

    if (0 == uuid_compare(&test0, &test1)) {
      log.concat("UUID generator gave us a repeat UUID. Fail...\n");
      printf((const char*) log.string());
      return -1;
    }
    uuid_copy(&test0, &test1);
    if (0 != uuid_compare(&test0, &test1)) {
      log.concat("UUID copy appears to have failed...\n");
      temp.concat((uint8_t*) &test0.id, sizeof(test0));
      temp.printDebug(&log);
      temp.clear();
      temp.concat((uint8_t*) &test1.id, sizeof(test1));
      temp.printDebug(&log);
      printf((const char*) log.string());
      return -1;
    }
    uuid_gen(&test0);
  }

  char str_buffer[40] = "";
  uuid_to_str(&test0, str_buffer, 40);
  log.concatf("temp0 string: %s\n", str_buffer);

  uuid_from_str(str_buffer, &test1);
  log.concat("temp1 bytes:  ");
  temp.concat((uint8_t*) &test1.id, sizeof(test1));
  temp.printDebug(&log);

  // TODO: This is the end of the happy-path. Now we should abuse the program
  // by feeding it garbage and ensure that its behavior is defined.

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}

/**
* Prints the sizes of various types. Informational only. No test.
*/
void printTypeSizes() {
  StringBuilder output("===< Type sizes >=======================================\n-- Primitives:\n");
  output.concatf("\tvoid*                 %u\n", sizeof(void*));
  output.concatf("\tFloat                 %u\n", sizeof(float));
  output.concatf("\tDouble                %u\n", sizeof(double));
  output.concat("\n-- Elemental data structures:\n");
  output.concatf("\tStringBuilder         %u\n", sizeof(StringBuilder));
  output.concatf("\tVector3<float>        %u\n", sizeof(Vector3<float>));
  output.concatf("\tQuaternion            %u\n", sizeof(Quaternion));
  output.concatf("\tBufferPipe            %u\n", sizeof(BufferPipe));
  output.concatf("\tLinkedList<void*>     %u\n", sizeof(LinkedList<void*>));
  output.concatf("\tPriorityQueue<void*>  %u\n", sizeof(PriorityQueue<void*>));
  output.concatf("\tRingBuffer<void*>     %u\n", sizeof(RingBuffer<void*>));
  output.concatf("\tArgument              %u\n", sizeof(Argument));
  output.concatf("\tUUID                  %u\n", sizeof(UUID));
  output.concatf("\tTaskProfilerData      %u\n", sizeof(TaskProfilerData));
  output.concatf("\tSensorWrapper         %u\n", sizeof(SensorWrapper));

  output.concat("\n-- Core singletons:\n");
  output.concatf("\tManuvrPlatform        %u\n", sizeof(ManuvrPlatform));
  output.concatf("\t  Storage             %u\n", sizeof(Storage));
  output.concatf("\t  Identity            %u\n", sizeof(Identity));
  output.concatf("\t    IdentityUUID      %u\n", sizeof(IdentityUUID));
  output.concatf("\tKernel                %u\n", sizeof(Kernel));

  output.concat("\n-- Messaging components:\n");
  output.concatf("\tEventReceiver         %u\n", sizeof(EventReceiver));
  output.concatf("\tManuvrMsg             %u\n", sizeof(ManuvrMsg));
  output.concatf("\t  ManuvrMsg      %u\n", sizeof(ManuvrMsg));

  output.concat("\n-- Transports:\n");
  output.concatf("\tManuvrXport           %u\n", sizeof(ManuvrXport));
  output.concatf("\t  StandardIO          %u\n", sizeof(StandardIO));
  output.concatf("\t  ManuvrSerial        %u\n", sizeof(ManuvrSerial));
  output.concatf("\t  ManuvrSocket        %u\n", sizeof(ManuvrSocket));
  output.concatf("\t    ManuvrTCP         %u\n", sizeof(ManuvrTCP));
  output.concatf("\t    ManuvrUDP         %u\n", sizeof(ManuvrUDP));
  output.concatf("\t      UDPPipe         %u\n", sizeof(UDPPipe));

  output.concat("\n-- Sessions:\n");
  output.concatf("\tXenoSession           %u\n", sizeof(XenoSession));
  output.concatf("\t  ManuvrConsole       %u\n", sizeof(ManuvrConsole));
  output.concatf("\t  ManuvrSession       %u\n", sizeof(ManuvrSession));
  output.concatf("\t  CoAPSession         %u\n", sizeof(CoAPSession));
  output.concatf("\t  MQTTSession         %u\n", sizeof(MQTTSession));
  output.concatf("\tXenoMessage           %u\n", sizeof(XenoMessage));
  output.concatf("\t  MQTTMessage         %u\n", sizeof(MQTTMessage));
  output.concatf("\t  CoAPMessage         %u\n", sizeof(CoAPMessage));
  output.concatf("\t  XenoManuvrMessage   %u\n", sizeof(XenoManuvrMessage));
  printf("%s\n", output.string());
}

void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.
  printTypeSizes();

  platform.platformPreInit();   // Our test fixture needs random numbers.

  if (0 == test_StringBuilder()) {
    if (0 == test_PriorityQueue()) {
      if (0 == vector3_float_test(0.7f, 0.8f, 0.01f)) {
        if (0 == test_BufferPipe()) {
          if (0 == test_Arguments()) {
            if (0 == test_UUID()) {
              if (0 == test_RingBuffer()) {
                printf("**********************************\n");
                printf("*  DataStructure tests all pass  *\n");
                printf("**********************************\n");
                exit_value = 0;
              }
              else printTestFailure("RingBuffer");
            }
            else printTestFailure("UUID");
          }
          else printTestFailure("Argument");
        }
        else printTestFailure("BufferPipe");
      }
      else printTestFailure("Vector3");
    }
    else printTestFailure("PriorityQueue");
  }
  else printTestFailure("StringBuilder");

  exit(exit_value);
}
