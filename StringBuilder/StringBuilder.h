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

#ifdef ARDUINO
  #include "Arduino.h"
#else
  #include <stdio.h>
  #include <stdlib.h>
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
*
* Note(0) regarding heap_ref():
* Sometimes, it may be desirable to return an instance of this class from a function. In that
*   case, the function doing the return should....
*      return str_builder.heap_ref();
*   This will cause the class to make a copy of itself on the heap. To avoid doubling the RAM usage,
*   the data-carrying elements are NOT copied, but the references are marked in the original instance
*   in such a way as to NOT be free()'d when the destructor is called. Therefore, the responsibility
*   of cleaning up the memory allocated by the class falls on the caller. It should be free()'d like so...
*      StringBuilder* obj = someFxn();   // someFxn() returns the result of heap_ref()...
*      // Do some things with the returned string....
*      obj.clear();       // Frees the allocated memory taken up by the strings.
*      free(obj);         // Frees the allocated memory taken up by the class itself.
* This is always safe to do, assuming that obj is not NULL. If the class contained a zero-length string,
*   the worst thing that will happen is wasted cycles. The class will do its own sanity-checks in the 
*   clear() function. 
*
* Note(1) regarding str_heap_ref():
* Like heap_ref(), str_heap_ref() will return a heap-allocated reference to the str member of this class
*   after collapsing it. This is done when the caller is expecting a simple (char*). This will not work
*   very well with unsigned character strings because no length can be returned. So the caller needs to
*   be sure that the StringBuilder wasn't built with unsigned character strings.
* If the caller is expecting (unsigned char*), it should do this instead...
*      int str_len = 0;
*      unsigned char* str = someFxn(&str_len);   // someFxn() returns the result of str_heap_ref()...
*                                                // str_len will contain the length of the string.
*      free(str);       // It is still on us to free the allocated memory taken up by the strings.
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
		
		StringBuilder* heap_ref(void);           // See Note0.
		char* str_heap_ref(void);                // See Note1.
		unsigned char* str_heap_ref(int len);    // See Note1.
		int str_heap_ref(unsigned char** callers_pntr);    // See Note2.

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
#ifdef ARDUINO
		void concat(String nu);
#endif
		void cull(int offset, int length);       // Use to throw away all but the specified range of this string.
		void cull(int length);                   // Use to discard the first X characters from the string.

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

#ifdef TEST_BENCH
		void printDebug();
#else
		void printDebug(StringBuilder*);
#endif

		
	private:
		int totalStrLen(StrLL *node);
		StrLL* stackStrOntoList(StrLL *current, StrLL *nu);
		StrLL* stackStrOntoList(StrLL *nu);
		void collapseIntoBuffer();
		void destroyStrLL(StrLL *r_node);
		void null_term_check(void);
		StrLL* promote_collapsed_into_ll(void);
};
#endif
