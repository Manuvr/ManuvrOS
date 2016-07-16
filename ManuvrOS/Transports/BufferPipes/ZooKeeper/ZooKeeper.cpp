/*
File:   ZooKeeper.cpp
Author: J. Ian Lindsay
Date:   2016.07.15

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


*/


#include "ZooKeeper.h"


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
PriorityQueue<BufferPipe*> ZooKeeper::_protocol_zoo;

int8_t ZooKeeper::registerAtZoo(ZooInmate* inmate) {
  return 0;
}

BufferPipe* ZooKeeper::searchZoo(uint8_t* _pattern, int _p_len) {
  return NULL;
}

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Constructor. The simple case. Usually for static-allocation.
*/
ZooKeeper::ZooKeeper() : BufferPipe() {
}

/**
* Constructor. We are passed the source of the data we are to inspect.
* We only ever use the far slot for instancing potential sessions.
*/
ZooKeeper::ZooKeeper(BufferPipe* source) : BufferPipe() {
  setNear(source);
}

/**
* Destructor.
*/
ZooKeeper::~ZooKeeper() {
  // Wonkey tear-down order is for concurrency-hardness.
  //TODO: deleting object of abstract class type BufferPipe which has
  //         non-virtual destructor will cause undefined behaviour
  //BufferPipe* _local_far = _far;
  //_far = NULL;
  //if (_local_far) delete _local_far;
}


void ZooKeeper::printDebug(StringBuilder* output) {
  output->concat("\t-- ZooKeeper ----------------------------------\n");
  if (_near) {
    output->concatf("\t _near         \t[0x%08x] %s\n", (unsigned long)_near, BufferPipe::memMgmtString(_near_mm_default));
  }
  if (_far) {
    output->concatf("\t _far          \t[0x%08x] %s\n", (unsigned long)_far, BufferPipe::memMgmtString(_far_mm_default));
  }
  if (_accumulator.length() > 0) {
    output->concatf("\t _accumulator (%d bytes):\t", _accumulator.length());
    _accumulator.printDebug(output);
  }
}
