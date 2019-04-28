/*
File:   StringBuilder.h
Author: J. Ian Lindsay
Date:   2011.06.18

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


#ifndef __MANUVR_DS_STRING_BUILDER_H
#define __MANUVR_DS_STRING_BUILDER_H

#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#if defined(__BUILD_HAS_PTHREADS)
  #include <pthread.h>
#elif defined(__BUILD_HAS_FREERTOS)
  #include "freertos/FreeRTOS.h"
  #include "freertos/semphr.h"
#endif

#if !defined(_GNU_SOURCE)
int strcasestr(char *a, const char *b);
#endif

/*
* This is a linked-list that is castable as a string.
*/
typedef struct str_ll_t {
  unsigned char    *str;   // The string.
  struct str_ll_t  *next;  // The next element.
  int              len;    // The length of this element.
  bool             reap;   // Should this position be reaped?
} StrLL;



/*
* The point of this class is to ease some of the burden of doing string manipulations.
* This class uses lots of dynamic memory internally. Some of this is exposed as public
*   members, so care needs to be taken if heap references are to be used directly.
* There are two modes that this class uses to store strings: collapsed and tokenized.
* The collapsed mode stores the entire string in the str member with a NULL root.
* The tokenized mode stores the string as sequential elements in a linked-list.
* When the full string is requested, the class will collapse the linked-list into the str
*   member, respecting the fact that part of the string may already have been collapsed into
*   str, in which case, str will be prepended to the linked-list, and the entire linked-list
*   then collapsed. Needless to say, this shuffling act might cause the class to more-than
*   double its memory usage while the string is being reorganized. So be aware of your memory
*   usage.
*/
class StringBuilder {
  public:
    StringBuilder();
    StringBuilder(char *initial);
    StringBuilder(unsigned char *initial, int len);
    StringBuilder(const char *);
    ~StringBuilder();

    int length();
    uint8_t* string();
    void prepend(unsigned char *nu, int len);

    /**
    * Overrides to cleanly support C-style strings..
    */
    inline void concat(char* nu) {  concat((uint8_t*) nu, strlen(nu));  };
    inline void prepend(char* nu) { prepend((uint8_t*) nu, strlen(nu)); };

    void concat(const char *nu);
    void prepend(const char *nu);   // TODO: Mark as non-reapable and store the pointer.

    void concat(StringBuilder *nu);
    void concat(unsigned char *nu, int len);
    void concat(char nu);
    void concat(unsigned char nu);

    void concat(int nu);
    void concat(unsigned int nu);

    /* These fxns allow for memory-tight hand-off of StrLL chains. Useful for merging
       StringBuilder instances. */
    void concatHandoff(StringBuilder *nu);
    void prependHandoff(StringBuilder *nu);

    /* Same idea as above, but takes a malloc'd buffer, alleviating the caller
       of responsibility for managing it. */
    void concatHandoff(uint8_t* buf, int len);

    /* Variadic concat. Semantics are the same as printf. */
    int concatf(const char *nu, ...);

    //inline void concat(uint16_t nu) { this->concat((unsigned int) nu); }
    //inline void concat(int16_t nu) { this->concat((int) nu); }

    void concat(double nu);
    /* Floats are upgraded to doubles. */
    inline void concat(float nu) { concat((double) nu); };
    inline void concat(bool nu) {  concat(nu ? "T" : "F"); };

    void cull(int offset, int length);       // Use to throw away all but the specified range of this string.
    void cull(int length);                   // Use to discard the first X characters from the string.

    void trim();                             // Trim whitespace off the ends of the string.

    void clear();                            // Clears the string and frees the memory that was used to hold it.

    /* The functions below are meant to aid basic tokenization. They all consider the collapsed
       root string (if present) to be index zero. This detail is concealed from client classes. */
    int split(const char*);                  // Split the string into tokens by the given string.
    int implode(const char*);                // Given a delimiter, form a single string from all StrLLs.
    unsigned short count();                  // Count the tokens.

    char*  position(int);                    // If the string has been split, get tokens with this.
    char*  position_trimmed(int);            // Same as position(int), but trims whitespace from the return.
    int    position_as_int(int);             // Same as position(int), but uses atoi() to return an integer.
    double position_as_double(int);          // Same as position(int), but uses atof() to return a double.
    uint8_t* position(int, int*);      // ...or this, if you need the length and a binary string.
    bool drop_position(unsigned int pos);    // And use this to reap the tokens that you've used.

    /* Comparison and search. */
    bool contains(char);                     // Does the buffer contain the given character.
    int cmpBinString(uint8_t*, int);         // Compare byte-wise a given length.
    // int cmpCaseless(const char* unknown);

    void printDebug(StringBuilder*);

    /* Statics */
    static void printBuffer(StringBuilder* output, uint8_t* buf, unsigned int len, const char* indent);
    // Wrapper for high-level string functions that we may or may not have.
    static int strcasestr(const char*, const char*);


  private:
    StrLL *root;         // The root of the linked-list.
    int col_length;      // The length of the collapsed string.
    unsigned char* str;  // The collapsed string.
    bool preserve_ll;    // If true, do not reap the linked list in the destructor.

    #if defined(__BUILD_HAS_PTHREADS)
      // If we are on linux, we control for concurrency with a mutex...
      pthread_mutex_t _mutex;
    #elif defined(__BUILD_HAS_FREERTOS)
      SemaphoreHandle_t _mutex;
    #endif

    int totalStrLen(StrLL *node);
    StrLL* stackStrOntoList(StrLL *current, StrLL *nu);
    StrLL* stackStrOntoList(StrLL *nu);
    void collapseIntoBuffer();
    void destroyStrLL(StrLL *r_node);
    void null_term_check();
    StrLL* promote_collapsed_into_ll();
};
#endif  // __MANUVR_DS_STRING_BUILDER_H
