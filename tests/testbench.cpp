/*
File:   testbench.cpp
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


This is the harness that executes our battery of runtime tests.
It was meant to run on Linux.
*/

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
              printf("**********************************\n");
              printf("*  DataStructure tests all pass  *\n");
              printf("**********************************\n");
              exit_value = 0;
            }
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
