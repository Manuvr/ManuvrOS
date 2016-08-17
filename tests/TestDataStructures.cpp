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
  StringBuilder *heap_obj = new StringBuilder("This is datas we want to transfer.");
  StringBuilder stack_obj;
  StringBuilder tok_obj;

  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();

  tok_obj.concat("This");
  printf("tok_obj split:   %d\n", tok_obj.split(" "));
  printf("tok_obj count:   %d\n", tok_obj.count());
  tok_obj.concat(" This");
  printf("tok_obj split:   %d\n", tok_obj.split(" "));
  printf("tok_obj count:   %d\n", tok_obj.count());
  tok_obj.concat("   This");
  printf("tok_obj split:   %d\n", tok_obj.split(" "));
  printf("tok_obj count:   %d\n", tok_obj.count());

  printf("Heap obj before culling:   %s\n", heap_obj->string());

  while (heap_obj->length() > 10) {
    heap_obj->cull(5);
    printf("Heap obj during culling:   %s\n", heap_obj->string());
  }

  printf("Heap obj after culling:   %s\n", heap_obj->string());

  heap_obj->prepend("Meaningless data ");
  heap_obj->concat(" And stuff tackt onto the end.");

  stack_obj.concatHandoff(heap_obj);

  delete heap_obj;

  stack_obj.split(" ");

  printf("Final Stack obj:          %s\n", stack_obj.string());

  return 0;
}




Vector3<float> test_vect_0(-0.4f, -0.1f, 0.4f);


int vector3_float_test(float x, float y, float z) {
  Vector3<float> test;
  Vector3<float> *test1 = &test_vect_0;
  printf("--- (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));



  test(1.0f, 0.5f, 0.24f);
  printf("--- (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test_vect_0.x, test_vect_0.y, test_vect_0.z);
  printf("--- (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test1->x, test1->y, test1->z);
  printf("--- (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(x, y, z);
  printf("--- (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));
  return 0;
}



int test_PriorityQueue(void) {
  return 0;
}


int test_Arguments() {
  return 0;
}

int test_ManuvrOpts() {
  const char* key0 = "key0";
  const char* key1 = "key1";
  const char* key2 = "key2";
  // Add some mixed-types...

  return 0;
}

int test_BufferPipe() {
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
