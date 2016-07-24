/*
File:   BufferPipe.cpp
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

#include "BufferPipe.h"


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
const char* BufferPipe::memMgmtString(int8_t code) {
  switch (code) {
    case MEM_MGMT_RESPONSIBLE_CALLER:   return "MM_CALLER";
    case MEM_MGMT_RESPONSIBLE_BEARER:   return "MM_BEARER";
    case MEM_MGMT_RESPONSIBLE_CREATOR:  return "MM_CREATOR";
    case MEM_MGMT_RESPONSIBLE_ERROR:    return "MM_CODE_ERROR";
    case MEM_MGMT_RESPONSIBLE_UNKNOWN:  return "MM_CODE_UNKNOWN";
    default:                            return "MM_CODE_UNDEFINED";
  }
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
* Constructor.
*/
BufferPipe::BufferPipe() {
  _near = NULL;
  _far  = NULL;
  _near_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
  _far_mm_default  = MEM_MGMT_RESPONSIBLE_UNKNOWN;
}

/**
* Destructor.
* Tell the other sides of the links that we are departing.
*/
BufferPipe::~BufferPipe() {
  if (NULL != _near) {
    _near->_bp_notify_drop(this);
    _near = NULL;
  }
  if (NULL != _far) {
    _far->_bp_notify_drop(this);
    _far  = NULL;
  }
  _near_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
  _far_mm_default  = MEM_MGMT_RESPONSIBLE_UNKNOWN;
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Basal implementations.
*******************************************************************************/

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  An MM return code.
*/
int8_t BufferPipe::toCounterparty(ManuvrPipeSignal _sig, void* _args) {
  return haveNear() ? _near->toCounterparty(_sig, _args) : 0;
}

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  An MM return code.
*/
int8_t BufferPipe::fromCounterparty(ManuvrPipeSignal _sig, void* _args) {
  return haveFar() ? _far->fromCounterparty(_sig, _args) : 0;
}


/**
* This allows us to minimize the burden of dynamic mem alloc if it has already
*   been incurred. This implementation will simply punt to the far side, if
*   possible. If not, we fail and don't take the buffer.
*
* Any override of this method should probably concatHandoff into it's local
*   StringBuilder instance.
*
* @param   buf  A pointer to the transport-bound buffer.
* @param   mm   The length of the buffer.
* @return  An MM return code.
*/
int8_t BufferPipe::toCounterparty(StringBuilder* buf, int8_t mm) {
  return haveNear() ? _near->toCounterparty(buf, mm) : MEM_MGMT_RESPONSIBLE_CALLER;
}

/**
* This allows us to minimize the burden of dynamic mem alloc if it has already
*   been incurred. This implementation will simply punt to the far side, if
*   possible. If not, we fail and don't take the buffer.
*
* Any override of this method should probably concatHandoff into it's local
*   StringBuilder instance.
*
* @param   buf  A pointer to the transport-spawned buffer.
* @param   mm   The length of the buffer.
* @return  An MM return code.
*/
int8_t BufferPipe::fromCounterparty(StringBuilder* buf, int8_t mm) {
  return haveFar() ? _far->fromCounterparty(buf, mm) : MEM_MGMT_RESPONSIBLE_CALLER;
}

/**
* Called by another instance to tell us they are being destructed.
* Depending on class implementation, we might choose to propagate this call.
*
* @param  _drop  A pointer to the instance that is being destroyed.
*/
void BufferPipe::_bp_notify_drop(BufferPipe* _drop) {
  if (_drop == _near) {
    _near = NULL;
    _near_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
  }
  if (_drop == _far) {
    _far = NULL;
    _far_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
  }
}

/**
* Sets the slot that sits nearer to the counterparty, as well as the default
*   memory-managemnt strategy for buffers moving toward the application.
* This allows a class setting itself up in a buffer chain to control the
*   default memory-management policy on buffers it isses. This default may be
*   overridden on a buffer-by-buffer basis by calling the overloaded transfer
*   method.
*
* @param   BufferPipe*  The newly-introduced BufferPipe.
* @param   int8_t       The default mem-mgmt strategy for this BufferPipe.
* @return  An MM return code.
*/
int8_t BufferPipe::setNear(BufferPipe* nu, int8_t _mm) {
  if (NULL == _near) {
    // If the slot is vacant..
    if ((NULL != nu) && (_mm >= 0)) {
      // ...and nu is itself non-null, and the _mm is valid...
      _near_mm_default = _mm;
      _near = nu;
      return _mm;
    }
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}


/**
* Sets the slot that sits nearer to the counterparty, as well as the default
*   memory-managemnt strategy for buffers moving toward the counterparty.
* This allows a class setting itself up in a buffer chain to control the
*   default memory-management policy on buffers it isses. This default may be
*   overridden on a buffer-by-buffer basis by calling the overloaded transfer
*   method.
*
* @param   BufferPipe*  The newly-introduced BufferPipe.
* @param   int8_t       The default mem-mgmt strategy for this BufferPipe.
* @return  A result code.
*/
int8_t BufferPipe::setFar(BufferPipe* nu, int8_t _mm) {
  if (NULL == _far) {
    // If the slot is vacant..
    if ((NULL != nu) && (_mm >= 0)) {
      // ...and nu is itself non-null, and the _mm is valid...
      _far_mm_default = _mm;
      _far = nu;
      return _mm;
    }
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}


/**
* Simplification for single-ended BufferPipes.
*
* @param   BufferPipe*  A pointer to the sender.
* @param   int8_t       The default mem-mgmt strategy for this BufferPipe.
* @return  A result code.
*/
int8_t BufferPipe::emitBuffer(uint8_t* buf, unsigned int len, int8_t _mm) {
  if (this == _near) {
    if (haveFar() && (_mm >= 0)) {
      return _far->fromCounterparty(buf, len, _mm);
    }
  }
  else if (this == _far) {
    if (haveNear() && (_mm >= 0)) {
      return _near->toCounterparty(buf, len, _mm);
    }
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}
