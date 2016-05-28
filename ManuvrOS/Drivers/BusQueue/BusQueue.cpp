/*
File:   BusQueue.cpp
Author: J. Ian Lindsay
Date:   2016.05.28

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

#include "BusQueue.h"


/****************************************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here. Initializers first, functions second.
****************************************************************************************************/


/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of an opcode.
*/
const char* BusOp::getStateString(XferState state) {
  switch (state) {
    case XferState::IDLE:        return "IDLE";
    case XferState::QUEUED:      return "QUEUED";
    case XferState::INITIATE:    return "INITIATE";
    case XferState::ADDR:        return "ADDR";
    case XferState::IO_WAIT:     return "IO-WAIT";
    case XferState::STOP:        return "STOP";
    case XferState::COMPLETE:    return "COMPLETE";
    case XferState::FAULT:       return "FAULT";
    default:                     return "<UNDEF>";
  }
}

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of an opcode.
*/
const char* BusOp::getOpcodeString(BusOpcode code) {
  switch (code) {
    case BusOpcode::RX:               return "RX";
    case BusOpcode::TX:               return "TX";
    case BusOpcode::TX_WAIT_RX:       return "TX/RX";
    case BusOpcode::TX_CMD:           return "TX_CMD";
    case BusOpcode::TX_CMD_WAIT_RX:   return "TX_CMD/RX";
    default:                          return "<UNDEF>";
  }
}
