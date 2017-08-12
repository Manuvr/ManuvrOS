/*
File:   RingBuffer.h
Author: J. Ian Lindsay
Date:   2014.04.28

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


Template for a ring buffer.

*/

#include <stdlib.h>


template <class T> class RingBuffer {
	public:
		RingBuffer(unsigned int c);  // Constructor takes the number of slots as its sole argument.
		~RingBuffer();

		void clear();     // Wipe the buffer.

		inline unsigned int capcity() {		return _CAPAC;	};

		/* Returns an integer representing how many items are buffered. */
		inline unsigned int count() {     return _count;  };

		int insert(T);
		T get();


	private:
		const unsigned int _CAPAC;
		const unsigned int _E_SIZE;
		unsigned int _count;
		unsigned int _w;
		unsigned int _r;
		uint8_t* root;
};


template <class T> RingBuffer<T>::RingBuffer(unsigned int c) : _CAPAC(c), _E_SIZE(sizeof(T)) {
	root = (uint8_t*) malloc(_E_SIZE * _CAPAC);
	clear();
}


template <class T> RingBuffer<T>::~RingBuffer() {
	_count = 0;
	free(root);
	root = nullptr;
}


template <class T> void RingBuffer<T>::clear() {
  _w = 0;
  _r = 0;
	_count = 0;
  for (int i = 0; i < (_E_SIZE * _CAPAC)-1; i++) {
    *((uint8_t*) root + i) = 0x00;
  }
}


/*
* This template makes copies of whatever is passed into it. There is no reason
*   for the caller to maintain local copies of data (unless T is a pointer).
*/
template <class T> int RingBuffer<T>::insert(T d) {
	if (_count >= _CAPAC) {
	  return -1;
	}
	T *ref = &d;
	unsigned int offset = _E_SIZE * _w;
	for (unsigned int i = 0; i < _E_SIZE; i++) {
		*((uint8_t*) root + offset + i) = *((uint8_t*)ref + i);
	}
	_w = (_w % _CAPAC) + 1;   // TODO: Convert to pow(2) later and convert to bitmask.
	_count++;
	return 0;
}


template <class T> T RingBuffer<T>::get() {
  if (0 == _count) {
    return (T)0;
  }
  T *return_value = (T*) (root + (_r * _E_SIZE));
	_r = (_r % _CAPAC) + 1;   // TODO: Convert to pow(2) later and convert to bitmask.
	_count--;
  return *return_value;
}
