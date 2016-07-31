/*
File:   ZooKeeper.h
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


IoT sucks. There, I said it.
This is an experiment in protocol discovery in situations where no faculty such
  as the one described in IETF rfc7301 can be relied-upon to exist.

Some protocols (mostly those designed for stream-oriented transports) have some
  sort of exploitable "sync" feature so that message can be properly framed and
  extracted from the stream. If such a protocol is differentiable before any
  response is required from us, it is a candidate for this discovery mechanism.

Participating protocol wrappers need to pass a ZooInmate definition to
  registerAtZoo(ZooInmate*) in order to be included in the discovery process.

This class would be a prime place to integrate Avro or protobuf so that packers
  and parsers can be built in a uniform manner, versus being wrapped
  implementations pulled in externally.
*/


#ifndef __MANUVR_PROTOCOL_ZOOKEEPER_H__
#define __MANUVR_PROTOCOL_ZOOKEEPER_H__

#include <DataStructures/BufferPipe.h>
#include <DataStructures/PriorityQueue.h>

class ZooInmate {
  public:
    const char* _inmate_name;
    uint8_t* _pattern;
    int      _p_len;
};

/*
* Another commonly-useful case for BufferPipe:
* Performing simple protocol discovery.
*/
class ZooKeeper : public BufferPipe {
  public:
    ZooKeeper();
    ZooKeeper(BufferPipe*);
    ~ZooKeeper();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    void printDebug(StringBuilder*);

    /* Static members for registering protocols and returning new instances
         of their classes. */
    static int8_t registerAtZoo(ZooInmate*);
    static BufferPipe* searchZoo(uint8_t* _pattern, int _p_len);


  protected:
    const char* pipeName();


  private:
    StringBuilder _accumulator;
    static PriorityQueue<BufferPipe*> _protocol_zoo;
};


#endif //__MANUVR_PROTOCOL_ZOOKEEPER_H__
