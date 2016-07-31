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
#if defined(__MANUVR_PIPE_DEBUG)
  #include <Kernel.h>
#endif

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

const char* BufferPipe::signalString(ManuvrPipeSignal code) {
  switch (code) {
    case ManuvrPipeSignal::FAR_SIDE_DETACH:   return "FAR_DETACH";
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:  return "NEAR_DETACH";
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:   return "FAR_ATTACH";
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:  return "NEAR_ATTACH";
    case ManuvrPipeSignal::UNDEF:
    default:                                  return "SIGNAL_UNDEF";
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
  _near            = NULL;
  _far             = NULL;
  _flags           = 0;
  _near_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
  _far_mm_default  = MEM_MGMT_RESPONSIBLE_UNKNOWN;
}

/**
* Destructor.
* Tell the other sides of the links that we are departing.
*/
BufferPipe::~BufferPipe() {
  if (NULL != _near) {
    _near->toCounterparty(ManuvrPipeSignal::FAR_SIDE_DETACH, NULL);
    _near = NULL;
  }
  if (NULL != _far) {
    _far->toCounterparty(ManuvrPipeSignal::NEAR_SIDE_DETACH, NULL);
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
* @return  Negative on error. Zero on success.
*/
int8_t BufferPipe::toCounterparty(ManuvrPipeSignal _sig, void* _args) {
  switch (_sig) {
    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
      _far = NULL;
      _far_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
      break;
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:
      #if defined(__MANUVR_PIPE_DEBUG)
      Kernel::log("toCounterparty(): Possible misdirected NEAR_SIDE_DETACH.\n");
      #endif
      break;
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }
  return haveNear() ? _near->toCounterparty(_sig, _args) : 0;
}

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t BufferPipe::fromCounterparty(ManuvrPipeSignal _sig, void* _args) {
  switch (_sig) {
    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
      #if defined(__MANUVR_PIPE_DEBUG)
      Kernel::log("fromCounterparty(): Possible misdirected FAR_SIDE_DETACH.\n");
      #endif
      break;
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:   // The near side is detaching.
      _near = NULL;
      _near_mm_default = MEM_MGMT_RESPONSIBLE_UNKNOWN;
      break;
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }
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
* Sets the slot that sits nearer to the counterparty, as well as the default
*   memory-management strategy for buffers moving toward the application.
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
  else {
    #if defined(__MANUVR_PIPE_DEBUG)
    Kernel::log("setNear() tried to clobber another pipe.\n");
    #endif
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
  else {
    #if defined(__MANUVR_PIPE_DEBUG)
    Kernel::log("setFar() tried to clobber another pipe.\n");
    #endif
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}


/**
* Joins the ends of this pipe in preparation for our own tear-down.
* Retains the default mem-mgmt settings from the data we have on-hand.
*
* @return  Non-zero on error. Otherwise implies success.
*/
int8_t BufferPipe::joinEnds() {
  if ((NULL == _far) || (NULL == _near)) {
    #if defined(__MANUVR_PIPE_DEBUG)
    Kernel::log("joinEnds(): The pipe does not have both ends defined.\n");
    #endif
  }
  else {
    BufferPipe* temp_far  = _far;
    BufferPipe* temp_near = _near;
    _far  = NULL;
    _near = NULL;

    temp_far->_near  = temp_near;
    temp_near->_far  = temp_far;
    return 0;
  }
  return -1;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void BufferPipe::printDebug(StringBuilder* out) {
  #if defined(__MANUVR_PIPE_DEBUG)
    out->concatf("-- Pipe: %s [0x%04x]\n", pipeName(), _flags);
    if (haveFar())  out->concatf("--\t_far:    %s\t%s\n", _far->pipeName(),  BufferPipe::memMgmtString(_far_mm_default));
    if (haveNear()) out->concatf("--\t_near:   %s\t%s\n", _near->pipeName(), BufferPipe::memMgmtString(_near_mm_default));
  #endif
}
