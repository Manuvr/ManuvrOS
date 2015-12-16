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
  
  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();
  
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


int statistical_mode_test() {
  PriorityQueue<double> mode_bins;
  double dubs[20] = {0.54, 0.10, 0.68, 0.54, 0.54, \
                     0.10, 0.17, 0.67, 0.54, 0.09, \
                     0.57, 0.15, 0.68, 0.54, 0.67, \
                     0.11, 0.10, 0.64, 0.54, 0.09};
   
    
  for (int i = 0; i < 20; i++) {
      if (mode_bins.contains(dubs[i])) {
        mode_bins.incrementPriority(dubs[i]);
      }
      else {
        mode_bins.insert(dubs[i], 1);
      }
  }
  
  double temp_double = 0;
  double most_common = mode_bins.get();
  int stat_mode      = mode_bins.getPriority(most_common);
  printf("Most common:  %lf\n", most_common);
  printf("Mode:         %d\n\n",  stat_mode);
  
  // Now let's print a simple histogram...
  for (int i = 0; i < mode_bins.size(); i++) {
    temp_double = mode_bins.get(i);
    stat_mode = mode_bins.getPriority(temp_double);
    printf("%lf\t", temp_double);
    for (int n = 0; n < stat_mode; n++) {
      printf("*");
    }
    printf("  (%d)\n", stat_mode);
  }

  for (int i = 0; i < mode_bins.size(); i++) {
    printf("\nRecycling  (%lf)", mode_bins.recycle());
  }
  printf("\n");
  
  // Now let's print a simple histogram...
  while (mode_bins.hasNext()) {
    temp_double = mode_bins.get();
    stat_mode = mode_bins.getPriority(temp_double);
    printf("%lf\t", temp_double);
    for (int n = 0; n < stat_mode; n++) {
      printf("*");
    }
    printf("  (%d)\n", stat_mode);
    mode_bins.dequeue();
  }
  return 0;
}


int recycle_test() {
  PriorityQueue<double> recycle;
  double dubs[20] = {0.01, 0.02, 0.03, 0.04, 0.05, \
                     0.06, 0.07, 0.08, 0.09, 0.10, \
                     0.11, 0.12, 0.13, 0.14, 0.15, \
                     0.16, 0.17, 0.18, 0.19, 0.20};
   
  for (int i = 0; i < 20; i++) recycle.insert(dubs[19-i]);
    
  for (int i = 0; i < 200; i++) {
    printf("\nRecycling  (%lf)\n", recycle.recycle());
  }
  
  for (int i = 0; i < recycle.size(); i++) {
    printf("%lf\t", recycle.get(i));
  }

  for (int i = 0; i < recycle.size(); i++) {
    printf("\nReversing list  %d\n", i);
    recycle.insert(recycle.dequeue(), i);
  }
  
  for (int i = 0; i < recycle.size(); i++) {
    printf("%lf\t", recycle.get(i));
  }
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
  statistical_mode_test();
  recycle_test();
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