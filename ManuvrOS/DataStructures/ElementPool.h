/*
File:   ElementPool.h
Author: J. Ian Lindsay
Date:   2017.08.13

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


Template for a preallocated pool manager.
*/

#include <stdlib.h>
#include "RingBuffer.h"

#ifndef __MANUVR_DS_ELEMENT_POOL_H
#define __MANUVR_DS_ELEMENT_POOL_H

#if defined(MANUVR_DEBUG)
  #include "StringBuilder.h"
#endif


template <class T> class ElementPool : public RingBuffer<T*>  {
  public:
    ElementPool(const unsigned int c, const T* _pool);
    ~ElementPool();

    int give(T*);
    T*  take();

    inline bool ready() {  return (RingBuffer<T*>::allocated() && (nullptr != _pool));  };

    #if defined(MANUVR_DEBUG)
    void printDebug(StringBuilder* output) {
      output->concatf("-- ElementPool (%sReady)\n", ready() ? "" : "Not ");
      output->concatf("\tPool(%p): %u bytes\n", (uintptr_t) _pool, (RingBuffer<T*>::capacity() * sizeof(T)));
      output->concatf("\tCapacity:       %u/%u\n\tLow Watermark:  %u\n", RingBuffer<T*>::count(), RingBuffer<T*>::capacity(), _low_watermark);
      output->concatf("\tStarves/Frees:  %u/%u\n", _starves, _heap_frees);
    };
    #endif

  private:
    const T* _pool;
    unsigned int _heap_frees     = 0;
    unsigned int _starves        = 0;
    unsigned int _low_watermark  = 0;
};



template <class T> ElementPool<T>::ElementPool(const unsigned int c, const T* _mem) : RingBuffer<T*>(c), _pool(_mem) {
  _low_watermark = c;
  if (ready()) {
    T* cur = (T*) _mem;
    for (unsigned int i = 0; i < c; i++) {
      RingBuffer<T*>::insert(cur++);  // TODO: Feels untidy...
    }
  }
}

template <class T> ElementPool<T>::~ElementPool() {
}


/**
* Reclaims the given object so its memory can be re-used.
*
* At present, our criteria for preallocation is if the pointer address passed in
*   falls within the range of our _pool array. I see nothing "non-portable"
*   about this, it doesn't require a flag or class member, and it is fast to check.
* However, this strategy only works for types that are never used in DMA or code
*   execution on the STM32F4. It may work for other architectures (PIC32, x86?).
*   I also feel like it ought to be somewhat slower than a flag or member, but not
*   by such an amount that the memory savings are not worth the CPU trade-off.
* Consider writing all new cyclical queues with preallocated members to use this
*   strategy. Also, consider converting the most time-critical types to this strategy
*   up until we hit the boundaries of the STM32 CCM.
*                                 ---J. Ian Lindsay   Mon Apr 13 10:51:54 MST 2015
*
* @param T* obj is the pointer to the object to be reclaimed.
*/
template <class T> int ElementPool<T>::give(T* e) {
  const uintptr_t obj_addr = ((uintptr_t) e);
  const uintptr_t pre_min  = ((uintptr_t) _pool);
  const uintptr_t pre_max  = pre_min + (uintptr_t) (RingBuffer<T*>::capacity() * sizeof(T));

  if ((obj_addr < pre_max) && (obj_addr >= pre_min)) {
    // If we are in this block, it means obj was preallocated. reclaim it.
    // Note that we are not wiping.
    RingBuffer<T*>::insert(e);
    return 1;
  }
  else {
    // We were created because our prealloc was starved. we are therefore a transient heap object.
    _heap_frees++;
    delete e;
    return 0;
  }
}

template <class T> T* ElementPool<T>::take() {
  T* return_value = RingBuffer<T*>::get();
  if (nullptr == return_value) {
    return_value = new T();
    _starves++;
  }
  if (_low_watermark > RingBuffer<T*>::count()) {
    _low_watermark = RingBuffer<T*>::count();
  }
  return return_value;
}


#endif // __MANUVR_DS_ELEMENT_POOL_H
