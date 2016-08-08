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

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  printf("\n");

  test_PriorityQueue();
  test_StringBuilder();
  vector3_float_test(0.7f, 0.8f, 0.01f);

  return 0;
}
