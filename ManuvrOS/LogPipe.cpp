/*
File:   LogPipe.cpp
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


#include "LogPipe.h"


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
LogPipe::LogPipe() : BufferPipe() {
  setFar(this, MEM_MGMT_RESPONSIBLE_BEARER);
}

/**
* Constructor. We are passed the source of the data we are to inspect.
* We only ever use the far slot for instancing potential sessions.
*/
LogPipe::LogPipe(BufferPipe* log_target) : BufferPipe() {
  setNear(log_target);
  setFar(this, MEM_MGMT_RESPONSIBLE_BEARER);
}

/**
* Destructor.
*/
LogPipe::~LogPipe() {
  // Wonkey tear-down order is for concurrency-hardness.
  //TODO: deleting object of abstract class type BufferPipe which has
  //         non-virtual destructor will cause undefined behaviour
  //BufferPipe* _local_far = _far;
  //_far = NULL;
  //if (_local_far) delete _local_far;
}


/*******************************************************************************
* Functions specific to this class                                             *
*******************************************************************************/
/*
* Take log data
*/
int8_t LogPipe::toCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        //return MEM_MGMT_RESPONSIBLE_CREATOR;
        return _near->toCounterparty(buf, len, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        //return MEM_MGMT_RESPONSIBLE_BEARER;
        return _near->toCounterparty(buf, len, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/*
* We don't accept data from the counterparty. Logging is a one-way thing.
*/
int8_t LogPipe::fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  return MEM_MGMT_RESPONSIBLE_CALLER;
}


int8_t LogPipe::log(StringBuilder *str, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      _accumulator.concat(str);
      return MEM_MGMT_RESPONSIBLE_CREATOR;
    case MEM_MGMT_RESPONSIBLE_BEARER:
      _accumulator.concatHandoff(str);
      return MEM_MGMT_RESPONSIBLE_BEARER;
    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
};


void LogPipe::printDebug(StringBuilder* output) {
  output->concat("\t-- LogPipe ----------------------------------\n");
  if (_near) {
    output->concatf("\t _near         \t[0x%08x] %s\n", (unsigned long)_near, BufferPipe::memMgmtString(_near_mm_default));
  }
  if (_accumulator.length() > 0) {
    output->concatf("\t _accumulator (%d bytes):\t", _accumulator.length());
    _accumulator.printDebug(output);
  }
}
