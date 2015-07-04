/*
File:   LightLinkedList.h
Author: J. Ian Lindsay
Date:   2014.03.28


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


Template for a linked list.

*/



#ifndef LIGHT_LINKED_LIST_H
#define LIGHT_LINKED_LIST_H

#include <inttypes.h>

//#ifdef ARDUINO
//  #include "Arduino.h"
//#else
  #include <stdlib.h>
//#endif


using namespace std;

/* This is the class that holds a datum in the list. */
template <class T> class Node{
  public:
    Node<T> *next;
    T data;
    bool reap;
};


/**
*	This is a linked-list element with a slot for data.
*/
template <class T> class LinkedList {

	public:
		LinkedList(void);
		~LinkedList(void);

		int insert(T);                   // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload. 
		int insertWithCopy(T);           // Returns the ID of the data, or -1 on failure. Makes a copy of the data.
		int insertWithCopy(T, size_t);   // Returns the ID of the data, or -1 on failure. Makes a copy of the data, with size specified.
		
		int size(void);                  // Returns the number of elements in this list.

		bool remove(void);               // Removes the first element of the list. Return true on success.
		bool remove(T);                  // Removes any elements with this data.
		bool remove(int position);       // Removes the element at the given position of the list. Return true on success.
		
		int clear(void);                 // Returns the number of elements purged from the list.

		//bool move(int old_position, int new_position);

		T get(void);                     // Returns the data from the first element.
		T get(int position);             // Returns the data from the element at the given position.
		bool hasNext(void);              // Returns false if this list is empty. True otherwise.
		bool contains(T);                // Returns true if this list contains the given data. False if not.


	private:
		Node<T> *root;
		Node<T>* getLast(void);          // Returns the last element in the list. Returns the WHOLE element, not just the data it holds.
		int element_count;               // Call this member if you are pressed for time and haven't changed the list recently.
		int count(void);                 // Counts the elements in the list without reliance on stored value. Slower...
};


/**
* Constructor.
*/
template <class T> LinkedList<T>::LinkedList() {
	root = NULL;
	element_count = 0;
}


/**
* Destructor. Empties the list.
*/
template <class T> LinkedList<T>::~LinkedList() {
	while (root != NULL) {
		remove();
	}
}


/**
* Empties the list.
*
* @return the number of elements removed by the call.
*/
template <class T> int LinkedList<T>::clear(void) {
	int return_value = 0;
	while (root != NULL) {
		if (remove()) return_value++;
	}
	return return_value;
};


/**
* Inserts the given dataum into the linked list, copying it.
* TODO: Might cut this member. Seems like a heap-driven nightmare. Not using it, AFAIK.
*
* @param  d   The data to copy and insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int LinkedList<T>::insertWithCopy(T d) {
	return insertWithCopy(d, sizeof(T));
}


/**
* Inserts the given dataum into the linked list.
*
* @param  d   The data to copy and insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int LinkedList<T>::insert(T d) {
	Node<T> *current;
	if (root == NULL) {
		root = (Node<T>*) malloc(sizeof(Node<T>));
		current = root;
	}
	else {
		current = getLast();
		current->next = (Node<T>*) malloc(sizeof(Node<T>));
		current = current->next;
	}
	current->next = NULL;
	current->reap = false;
	current->data = d;
	element_count++;
	return 1;
}


/**
* Inserts the given dataum into the linked list, copying it.
* TODO: Might cut this member. Seems like a heap-driven nightmare. Not using it, AFAIK.
*
* @param  d   The data to copy and insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int LinkedList<T>::insertWithCopy(T d, size_t len) {
	Node<T> *current;
	if (root == NULL) {
		root = (Node<T>*) malloc(sizeof(Node<T>));
		current = root;
	}
	else {
		current = getLast();
		current->next = (Node<T>*) malloc(sizeof(Node<T>));
		current = current->next;
	}
	current->next = NULL;
	current->data = (T) malloc(len);
	current->reap = true;
	memcpy(current->data, d, len);
	element_count++;
	return (size()-1);
}


/**
* How many elements in the list? Uses the cached value rather than traversing
*   the list for each call.
*
* @return the number of elements in the linked list.
*/
template <class T> int LinkedList<T>::size() {
  return element_count;
}


/**
* How many elements in the list? Actually traverses the list and updates the
*   cached count value.
*
* @return the number of elements in the linked list.
*/
template <class T> int LinkedList<T>::count() {
	Node<T>* current = root;
	int return_value = 0;
	while (current != NULL) {
		current = current->next;
		return_value++;
	}
	element_count = return_value;
	return return_value;
}


/**
* Convenience function for finding the tail of the list.
*
* @return a pointer to the last node in this linked list.
*/
template <class T> Node<T>* LinkedList<T>::getLast() {
	Node<T>* return_value = root;
	while ((return_value != NULL) && (return_value->next != NULL)) {
		return_value = return_value->next;
	}
	return return_value;
}


/**
* Removes the element at the head of the list. 
*
* @return true if something was removed. False otherwise.
*/
template <class T> bool LinkedList<T>::remove() {
	Node<T>* current = root;
	if (current != NULL) {
		root = current->next;
		free(current);
		element_count--;
		return true;
	}
	return false;
}


/**
* Removes the element at the given position within the list. 
*
* @param  pos The position to remove.
* @return true if something was removed. False otherwise.
*/
template <class T> bool LinkedList<T>::remove(int pos) {
	int i = 0;
	Node<T>* prior   = NULL;
	Node<T>* current = root;
	while (current != NULL) {
		if (i == pos) {
			if (prior != NULL) {
				prior->next = current->next;
			}
			else {
				root = current->next;
			}
			if (current->data != NULL) {
				if (current->reap) {
					free(current->data);
				}
			}
			free(current);
			element_count--;
			return true;
		}
		i++;
		prior = current;
		current = current->next;
	}
	return false;
}


/**
* Removes the element with the given data. 
*
* @param  test_data  The data to remove.
* @return true if something was removed. False otherwise.
*/
template <class T> bool LinkedList<T>::remove(T test_data) {
	Node<T>* prior   = NULL;
	Node<T>* current = root;
	bool return_value = false;
	while (current != NULL) {
		if ((current->data != NULL) && (current->data == test_data)) {
			if (prior != NULL) {
				prior->next = current->next;
			}
			else {
				root = current->next;
			}
			free(current);
			element_count--;
			return_value = true;
			current = (NULL == prior) ? NULL : prior->next;
		}
		else {
			prior = current;
			current = current->next;
		}
	}
	return return_value;
}


/**
* Get the first piece of data. 
*
* @return The data from the element at the head of the list, or NULL on empty list.
*/
template <class T> T LinkedList<T>::get() {
	Node<T>* current = root;
	if (current != NULL) {
	  return current->data;
	}
	return NULL;
}


/**
* Get the piece of data at the given position in the list. 
*
* @param  pos The position to fetch.
* @return The data from the requested element, or NULL if the list isn't that large.
*/
template <class T> T LinkedList<T>::get(int pos) {
	int i = 0;
	Node<T>* current = root;
	while (current != NULL) {
		if (i == pos) {
			return current->data;
		}
		i++;
		current = current->next;
	}
	return NULL;
}


/**
* Does this list have more elements in it?
*
* @return true if the list is not empty. False if it is.
*/
template <class T> bool LinkedList<T>::hasNext() {
	return (root != NULL);
}


/**
* Does this list contain the given data?
*
* @param  test_data The data to search for.
* @return true if the list has the given data. False otherwise.
*/
template <class T> bool LinkedList<T>::contains(T test_data) {
	Node<T>* current = root;
	while (current != NULL) {
		if ((current->data != NULL) && (current->data == test_data)) {
			return true;
		}
		current = current->next;
	}
	return false;
}


#endif
