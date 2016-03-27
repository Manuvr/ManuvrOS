/*
File:   StringBuilder.h
Author: J. Ian Lindsay
Date:   2011.06.18

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

*/


#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <inttypes.h>
#include <stdarg.h>

#if defined(__MANUVR_LINUX)
  #include <pthread.h>
#elif defined(__MANUVR_FREERTOS)
  #include <FreeRTOS_ARM.h>
#endif


/*
*	This is a linked-list that is castable as a string.
*/
typedef struct str_ll_t {
	unsigned char    *str;   // The string.
	int              len;    // The length of this element.
	struct str_ll_t  *next;  // The next element.
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
	StrLL *root;         // The root of the linked-list.
	int col_length;      // The length of the collapsed string.
	unsigned char *str;  // The collapsed string.
	bool preserve_ll;    // If true, do not reap the linked list in the destructor.
	
	public:
		StringBuilder(void);
		StringBuilder(char *initial);
		StringBuilder(unsigned char *initial, int len);
		StringBuilder(const char *);
		~StringBuilder(void);

		int length(void);
		unsigned char* string(void);
		void prepend(unsigned char *nu, int len);
		void prepend(char *nu);
		void prepend(const char *nu);   // TODO: Mark as non-reapable and store the pointer.
		void concat(StringBuilder *nu);
		void concat(unsigned char *nu, int len);
		void concat(const char *nu);   // TODO: Mark as non-reapable and store the pointer.
		void concat(char *nu);
		void concat(char nu);
		void concat(unsigned char nu);

		void concat(int nu);
		void concat(unsigned int nu);

		/* These fxns allow for memory-tight hand-off of StrLL chains. Useful for merging 
		   StringBuilder instances. */
		void concatHandoff(StringBuilder *nu);
		void prependHandoff(StringBuilder *nu);
		
		// TODO: Badly need a variadic concat...
		// TODID: Got ir dun
		int concatf(const char *nu, ...);

		//inline void concat(uint16_t nu) { this->concat((unsigned int) nu); }
		//inline void concat(int16_t nu) { this->concat((int) nu); }

		void concat(double nu);
		void concat(float nu);
		void concat(bool nu);

		void cull(int offset, int length);       // Use to throw away all but the specified range of this string.
		void cull(int length);                   // Use to discard the first X characters from the string.

		void trim(void);                         // Trim whitespace off the ends of the string. 

		void clear(void);                        // Clears the string and frees the memory that was used to hold it.
		
		/* The functions below are meant to aid basic tokenization. They all consider the collapsed
		   root string (if present) to be index zero. This detail is concealed from client classes. */
		int split(const char*);                  // Split the string into tokens by the given string.
		int implode(const char*);                // Given a delimiter, form a single string from all StrLLs.
		char* position(int);                     // If the string has been split, get tokens with this.
		char* position_trimmed(int);             // Same as position(int), but trims whitespace from the return.
		int   position_as_int(int);              // Same as position(int), but uses atoi() to return an integer.
		unsigned char* position(int, int*);      // ...or this, if you need the length and a binary string.
		bool drop_position(unsigned int pos);    // And use this to reap the tokens that you've used.
		// Trim the whitespace from the end of the input string.

		unsigned short count(void);              // Count the tokens.

		int cmpBinString(unsigned char *unknown, int len);

#ifdef __MANUVR_DEBUG
		void printDebug();
#else
		void printDebug(StringBuilder*);
#endif

		
	private:
    #if defined(__MANUVR_LINUX)
      // If we are on linux, we control for concurrency with a mutex...
      pthread_mutex_t _mutex;
    #elif defined(__MANUVR_FREERTOS)
      SemaphoreHandle_t _mutex;
    #endif

		int totalStrLen(StrLL *node);
		StrLL* stackStrOntoList(StrLL *current, StrLL *nu);
		StrLL* stackStrOntoList(StrLL *nu);
		void collapseIntoBuffer();
		void destroyStrLL(StrLL *r_node);
		void null_term_check(void);
		StrLL* promote_collapsed_into_ll(void);
};
#endif