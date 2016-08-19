#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include "DataStructures/PriorityQueue.h"
#include "DataStructures/Vector3.h"
#include "DataStructures/StringBuilder.h"
#include "DataStructures/BufferPipe.h"
#include "DataStructures/ManuvrOptions.h"


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




Vector3<float> test_vect_0(-0.4f, -0.1f, 0.4f);


int vector3_float_test(float x, float y, float z) {
  StringBuilder log("===< Vector3<float> >===================================\n");
  Vector3<float> test;
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


/*
* These tests are meant to test the mechanics of the pointer-hack on PODs.
*/
int test_Arguments_KVP() {
  StringBuilder log("===< Arguments KVP >====================================\n");
  Argument a;

  log.concat("Adding arguments...\n\n");

  uint32_t val0  = 45;
  uint16_t val1  = 44;
  uint8_t  val2  = 43;
  int32_t  val3  = 42;
  int16_t  val4  = 41;
  int8_t   val5  = 40;
  float    val6  = 0.523;

  a.append(val0);
  a.append(val1);
  a.append(val2);
  a.append(val3);
  a.append(val4);
  a.append(val5);
  a.append(val6);

  a.printDebug(&log);
  log.concat("\n");

  log.concatf("Total Arguments:      %d\n", a.argCount());
  log.concatf("Total payload size:   %d\n", a.sumAllLengths());
  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return 0;
}


/*
* These tests are meant to test the mechanics of the pointer-hack on PODs.
*/
int test_Arguments_PODs() {
  int return_value = 1;
  StringBuilder log("===< Arguments POD >====================================\n");
  const char* val_base = "This is the base argument";
  uint32_t val0  = 45;
  uint16_t val1  = 44;
  uint8_t  val2  = 43;
  int32_t  val3  = 42;
  int16_t  val4  = 41;
  int8_t   val5  = 40;
  float    val6  = 0.523;

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

  if ((a.argCount() == 8) && (a.sumAllLengths() == real_size)) {
    log.concatf("Test passes.\n", a.argCount());
    return_value = 0;
  }
  else {
    log.concatf("Total Arguments:      %d\tExpected 8.\n", a.argCount());
    log.concatf("Total payload size:   %d\tExpected %d.\n", a.sumAllLengths(), real_size);
    log.concat("\n");
    a.printDebug(&log);
  }

  log.concat("========================================================\n\n");
  printf((const char*) log.string());
  return return_value;
}


int test_Arguments() {
  int return_value = test_Arguments_PODs();
  if (0 == return_value) {
    return_value = test_Arguments_MEM_MGMT();
    if (0 == return_value) {
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

  if (0 == test_StringBuilder()) {
    if (0 == test_PriorityQueue()) {
      if (0 == vector3_float_test(0.7f, 0.8f, 0.01f)) {
        if (0 == test_BufferPipe()) {
          if (0 == test_Arguments()) {
            printf("**********************************\n");
            printf("*  DataStructure tests all pass  *\n");
            printf("**********************************\n");
            exit_value = 0;
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
