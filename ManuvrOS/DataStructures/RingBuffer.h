/*
File:   RingBuffer.h
Author: J. Ian Lindsay
Date:   2014.04.28


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


Template for a ring buffer.

*/

#include <stdlib.h>


template <class T> class RingBuffer {
	public:
		unsigned int first_element;
		unsigned int last_element;
		
		RingBuffer(unsigned int capacity);  // Constructor takes the number of slots as its sole argument.
		~RingBuffer(void);
		
		void clear(void);                // Wipe the buffer.
		bool hasNext(void);              // Returns false if this list is empty. True otherwise.
		int pending(void);               // Returns an integer representing how many members are unread.

		int insert(T);     // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload. 
		T get(void);       // Returns the data from the first element. Only appropriate for native types (and pointers).
		int get(T*);       // Writes the data from the first element into the provided buffer.


	private:
		unsigned int size_of_element;
		unsigned int capacity;
		unsigned int element_count;
		unsigned char *root;
};


template <class T> RingBuffer<T>::RingBuffer(unsigned int cap) {
	capacity        = cap;
	size_of_element = sizeof(T);
	root = (unsigned char*) malloc(size_of_element * capacity);
	clear();
}


template <class T> RingBuffer<T>::~RingBuffer() {
	first_element = 0;
	last_element  = 0;
	element_count = 0;
	free(root);
	root = NULL;
}


template <class T> bool RingBuffer<T>::hasNext(void) {    return (element_count > 0);    }
template <class T> int RingBuffer<T>::pending(void) {     return element_count;          }


template <class T> void RingBuffer<T>::clear(void) {
  for (int i = 0; i < (size_of_element * capacity)-1; i++) {
    *((unsigned char*) root + i) = 0x00;
  }
  first_element = 0;
  last_element  = 0;
  element_count = 0;
}


/*
* This template makes copies of whatever is passed into it. There is no reason
*   for the caller to maintain local copies of data (unless T is a pointer).
*/
template <class T> int RingBuffer<T>::insert(T d) {
  int return_value = 0;
	T *ref = &d;
	unsigned int offset = size_of_element * last_element;
	for (unsigned int i = 0; i < size_of_element; i++) {
		*((unsigned char*) root + offset + i) = *((unsigned char*)ref + i);
	}
	if (element_count == capacity) {
	  // We have wrapped our pointer because things have arrived faster than
	  //  they have been removed. We need to clobber the oldest data.
	  return_value = -1;
	  first_element = ((first_element + 1) % capacity);
	}
	else {
	  element_count++;
	}
	last_element = (last_element + 1) % capacity;
	return return_value;
}


template <class T> T RingBuffer<T>::get() {
  if (element_count == 0) {
    return (T)0;
  }
  T *return_value = (T*) (root + (first_element * size_of_element));
  first_element = ((first_element + 1) % capacity);
  element_count--;
  return *return_value;
}

template <class T> int RingBuffer<T>::get(T* d) {
  int return_value = 0;
  if (element_count == 0) {
    return -1;
  }
  unsigned int offset = size_of_element * first_element;
  for (unsigned int i = 0; i < size_of_element; i++) {
    *((unsigned char*)d + i) = *((unsigned char*) root + offset + i);
    return_value++;
  }
  first_element = ((first_element + 1) % capacity);
  element_count--;
  return return_value;
}


