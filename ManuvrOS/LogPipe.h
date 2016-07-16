/*
File:   LogPipe.h
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


#ifndef __MANUVR_PROTOCOL_ZOOKEEPER_H__
#define __MANUVR_PROTOCOL_ZOOKEEPER_H__

#include <DataStructures/BufferPipe.h>

/*
* A BufferPipe that is specialized for handling logs.
*/
class LogPipe : public BufferPipe {
  public:
    LogPipe();
    LogPipe(BufferPipe*);
    ~LogPipe();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    void printDebug(StringBuilder*);



  private:
    uint32_t      _logs_handled;
    StringBuilder _accumulator;
};


#endif //__MANUVR_PROTOCOL_ZOOKEEPER_H__
