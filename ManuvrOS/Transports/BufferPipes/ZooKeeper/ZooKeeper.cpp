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

/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
const char* ZooKeeper::pipeName() { return "ZooKeeper"; }

/**
* Inward toward the transport.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ZooKeeper::toCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ZooKeeper::fromCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ZooKeeper::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);

  if (_accumulator.length() > 0) {
    output->concatf("--\t_accumulator (%d bytes):  ", _accumulator.length());
    _accumulator.printDebug(output);
  }
}
