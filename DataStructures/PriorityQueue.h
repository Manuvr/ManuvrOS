/*
File:   PriorityQueue.h
Author: J. Ian Lindsay
Date:   2014.06.30


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


Template for a priority queue implemented on top of a linked-list.
Highest-priority nodes are stored closest to the beginning of the list.

Some functions are #pragma'd to stop the compiler from complaining about NULL being
  interpreted as a non-pointer. This is to enable the use of this template to carry
  (doubles, floats, ints, etc) without polluting the log.
*/


#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <inttypes.h>

//#ifdef ARDUINO
//  #include "Arduino.h"
//#else
  #include <stdlib.h>
//#endif


using namespace std;

/* This is the class that holds a datum in the list. */
template <class T> class PriorityNode{
  public:
    PriorityNode<T> *next;
    T data;
    int priority;
};


/*
* The class that should be instantiated.
*/
template <class T> class PriorityQueue {

  public:
    PriorityQueue(void);
    ~PriorityQueue(void);

    int insert(T, int priority);  // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
    int insert(T);                // Same as above, but assumes lowest priority.
    int insertIfAbsent(T);        // Same as above, but only if the queue doesn't already have the argument.
    int insertIfAbsent(T, int);   // Same as above, but also specifies the priority if successful.
    
    int size(void);               // Returns the number of elements in this list.

    T dequeue(void);              // Removes the first element of the list. Return the node's data on success, or NULL on empty queue.
    T recycle(void);              // Reycle this element. Return the node's data on success, or NULL on empty queue.
    bool remove(int position);    // Removes the element at the given position of the list. Return true on success.
    bool remove(T);               // Removes any elements with this data.
    int clear(void);              // Purges the whole queue. Returns the number of things purged or -1 on failure.

    T get(void);                  // Returns the data from the first element.
    T get(int position);          // Returns the data from the element at the given position.
    int getPosition(T);           // Returns the position in the queue for the given element.
    int getPriority(T);           // Returns the priority in the queue for the given element.
    int getPriority(int position);           // Returns the priority in the queue for the given element.

    bool hasNext(void);           // Returns false if this list is empty. True otherwise.
    bool contains(T);             // Returns true if this list contains the given data. False if not.

    bool incrementPriority(T);    // Finds the given T and increments its priority by one.
    bool decrementPriority(T);    // Finds the given T and decrements its priority by one.

    
  private:
    PriorityNode<T> *root;        // The root of the queue. Is also the highest-priority.
    int element_count;
    
    /*
    * Returns the last element in the list. 
    * Returns the WHOLE element, not just the data it holds.
    */
    PriorityNode<T>* getLast(void);
    
    /* 
    * Returns the last element in the list with the given priority, or the highest priority
    *   item in the list if the argument is greater than all nodes. 
    */
    PriorityNode<T>* getLastWithPriority(int);

    PriorityNode<T>* get_node_by_element(T);    // Return a priority node by the element it contains.
    int insert(PriorityNode<T>*);    // Returns the ID of the data, or -1 on failure.
    int count(void);                 // Counts the elements in the list without reliance on stored value. Slower...
    void enforce_priorities(void);   // Need to call this after changing a priority to ensure position reflects priority.
};


/**
* Constructor.
*/
template <class T> PriorityQueue<T>::PriorityQueue() {
  root = NULL;
  element_count = 0;
}

/**
* Destructor. Empties the queue.
*/
template <class T> PriorityQueue<T>::~PriorityQueue() {
  while (root != NULL) {
    dequeue();
  }
}


/**
* Inserts the given dataum into the queue.
*
* @param  d   The data to insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int PriorityQueue<T>::insert(T d) {
  return insert(d, 0);
}


/**
* Inserts the given dataum into the queue.
*
* @param  d       The data to insert.
* @param  nu_pri  The priority of the node.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int PriorityQueue<T>::insertIfAbsent(T d, int nu_pri) {
  // Note that we are going to replicate alot of code here for speed reasons.
  // We could just as well have done with....
  // return (contains(d)) ? false : insert(d, 0);
  
  if (NULL == root) {
    // If root is null, we just insert and call it done. That way we
    // don't have to worry about it later.
    return insert(d, nu_pri);
  }
  
  int return_value = -1;
  PriorityNode<T>* insert_pointer = NULL;
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (current->data == d) {
      return -1;
    }
    if (current->priority >= nu_pri) {
      insert_pointer = current;
      return_value++;
    }
    current = current->next;
  }
  
  PriorityNode<T> *nu = (PriorityNode<T>*) malloc(sizeof(PriorityNode<T>));
  if (nu == NULL) {
    return -1;      // Failed to allocate memory.
  }
  nu->data     = d;
  nu->priority = nu_pri;

  if (NULL == insert_pointer) {
    nu->next = root;
    root = nu;
    return_value = 0;
  }
  else {
    nu->next = insert_pointer->next;
    insert_pointer->next = nu;
  }
  element_count++;
  return return_value;
}


/**
* Inserts the given dataum into the queue.
*
* @param  d   The data to insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int PriorityQueue<T>::insertIfAbsent(T d) {
  return insertIfAbsent(d, 0);
}


/*
* Returns the position in the list that the data was inserted, or -1 on failure.
* Inserts at the first position that has a lower priority than the one given.
*/
template <class T> int PriorityQueue<T>::insert(T d, int nu_pri) {
  int return_value = -1;
  PriorityNode<T> *nu = (PriorityNode<T>*) malloc(sizeof(PriorityNode<T>));
  if (nu == NULL) {
    return return_value;      // Failed to allocate memory.
  }
  nu->data     = d;
  nu->priority = nu_pri;

  return_value = insert(nu);
  element_count++;
  return 1;
}


// TODO: This doesn't return something sensible...
template <class T> int PriorityQueue<T>::insert(PriorityNode<T>* nu) {
  if (nu == NULL) {
    return -1;
  }

  PriorityNode<T> *current = getLastWithPriority(nu->priority);

  if (current == NULL) {
    nu->next = root;
    root     = nu;
  }
  else {
    nu->next      = current->next;
    current->next = nu;
  }
  return 1;
}



template <class T> int PriorityQueue<T>::size() {
  return element_count;
}


template <class T> int PriorityQueue<T>::count() {
  PriorityNode<T>* current = root;
  int return_value = 0;
  while (current != NULL) {
    current = current->next;
    return_value++;
  }
  element_count = return_value;
  return return_value;
}


/*
* Returns a pointer to the last PriorityNode in this linked list.
*/
template <class T> PriorityNode<T>* PriorityQueue<T>::getLast() {
  PriorityNode<T>* return_value = root;
  while ((return_value != NULL) && (return_value->next != NULL)) {
    return_value = return_value->next;
  }
  return return_value;
}


/*
* Returns a pointer to the first PriorityNode in this linked list with a matching element.
*/
template <class T> PriorityNode<T>* PriorityQueue<T>::get_node_by_element(T test_data) {
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (current->data == test_data) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}


/*
* Returns a pointer to the last PriorityNode in this linked list.
* Returns NULL if there is not a node that has a priority at-least matching what we provided.
*/
template <class T> PriorityNode<T>* PriorityQueue<T>::getLastWithPriority(int nu_pri) {
  PriorityNode<T>* current      = root;
  PriorityNode<T>* last         = NULL;
  while (current != NULL) {
    if (nu_pri > current->priority) {
      return last;
    }
    last    = current;
    current = current->next;
  }
  return last;
}


template <class T> void PriorityQueue<T>::enforce_priorities(void) {
  PriorityNode<T>* current  = root;
  PriorityNode<T>* prior    = NULL;
  while (current != NULL) {
    if (prior == NULL) {              // If current is the root node...
      if (current->next != NULL) {  // ...and there are at least two items in the queue...
        if (current->next->priority > current->priority) {  // ...and they are out of order...
          root = current->next;         // Swap them.
          current->next = root->next;
          root->next = current;
          
          current = root;               // Restart the test.
        }
      }
    }
    else {
      if (prior->priority < current->priority) {
        // We need to promote current to come before prior in the queue.
        // We won't bother juggling this here. Instead, we'll simply remove
        // current and re-insert it.
        prior->next = current->next;   // Effectively removes current.
        insert(current);
        
        // Restart the test.
        current = root;
      }
    }
    
    prior = current;
    current = current->next;
  }
}


template <class T> bool PriorityQueue<T>::incrementPriority(T test_data) {
  PriorityNode<T>* current = get_node_by_element(test_data);
  if (current != NULL) {
    current->priority++;
    enforce_priorities();
    return true;
  }
  return false;
}

template <class T> bool PriorityQueue<T>::decrementPriority(T test_data) {
  PriorityNode<T>* current = get_node_by_element(test_data);
  if (current != NULL) {
    current->priority--;
    enforce_priorities();
    return true;
  }
  return false;
}


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion-null"
template <class T> T PriorityQueue<T>::dequeue() {
  PriorityNode<T>* current = root;
  if (current != NULL) {
    // This is safe because we only store references. Not the actual data.
    T return_value = current->data;
    root = current->next;
    free(current);
    element_count--;
    return return_value;
  }
  return NULL;
}
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion-null"
template <class T> T PriorityQueue<T>::recycle() {
  PriorityNode<T>* current = root;
  if (current != NULL) {
    T return_value = current->data;

    if (element_count > 1) {
      root = current->next;
      current->next = NULL;

      PriorityNode<T>* temp = getLast();
      current->priority = temp->priority;  // Demote to the rear of the queue.
      temp->next = current;
    }

    return return_value;
  }
  return NULL;
}
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif


template <class T> int PriorityQueue<T>::clear(void) {
  int return_value = 0;
  while (remove(0)) {
    return_value++;
  }
  return return_value;
}


template <class T> bool PriorityQueue<T>::remove(int pos) {
  int i = 0;
  PriorityNode<T>* prior   = NULL;
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (i == pos) {
      if (prior != NULL) {
        prior->next = current->next;
      }
      else {
        root = current->next;
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


template <class T> bool PriorityQueue<T>::remove(T test_data) {
  PriorityNode<T>* prior   = NULL;
  PriorityNode<T>* current = root;
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

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion-null"
template <class T> T PriorityQueue<T>::get() {
  PriorityNode<T>* current = root;
  if (current != NULL) {
    return current->data;
  }
  return NULL;
}
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion-null"
template <class T> T PriorityQueue<T>::get(int pos) {
  int i = 0;
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (i == pos) {
      return current->data;
    }
    i++;
    current = current->next;
  }
  return NULL;
}
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif


template <class T> bool PriorityQueue<T>::contains(T test_data) {
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (current->data == test_data) {
      return true;
    }
    current = current->next;
  }
  return false;
}


template <class T> int PriorityQueue<T>::getPosition(T test_data) {
  int i = 0;
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (current->data == test_data) {
      return i;
    }
    i++;
    current = current->next;
  }
  return -1;
}


template <class T> int PriorityQueue<T>::getPriority(T test_data) {
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (current->data == test_data) {
      return current->priority;
    }
    current = current->next;
  }
  return -1;
}


template <class T> int PriorityQueue<T>::getPriority(int position) {
  int i = 0;
  PriorityNode<T>* current = root;
  while (current != NULL) {
    if (position == i++) return current->priority;
    current = current->next;
  }
  return -1;
}


template <class T> bool PriorityQueue<T>::hasNext() {
  return (root != NULL);
}


#endif
