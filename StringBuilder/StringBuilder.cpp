/*
File:   StringBuilder.cpp
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

// Needed so we can share libs between the host and the microcontroller:

#include "StringBuilder.h"

#ifdef ARDUINO
  #include "Arduino.h"
#else
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <cctype>
#endif


#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
    #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

/****************************************************************************************************
* Class management....                                                                              *
****************************************************************************************************/

/**
* Vanilla constructor.
*/
StringBuilder::StringBuilder() {
  this->root   = NULL;
  this->str    = NULL;
  this->col_length = 0;
  this->preserve_ll = false;
}

StringBuilder::StringBuilder(char *initial) {
  this->root   = NULL;
  this->str    = NULL;
  this->col_length = 0;
  this->preserve_ll = false;
  this->concat(initial);
}

StringBuilder::StringBuilder(unsigned char *initial, int len) {
  this->root   = NULL;
  this->str    = NULL;
  this->col_length = 0;
  this->preserve_ll = false;
  this->concat(initial, len);
}


StringBuilder::StringBuilder(const char *initial) {
  this->root   = NULL;
  this->str    = NULL;
  this->col_length = 0;
  this->preserve_ll = false;
  this->concat(initial);
}



/**
* Destructor.
*/
StringBuilder::~StringBuilder() {
  if (!this->preserve_ll) {
    if (this->root != NULL) destroyStrLL(this->root);

    if (this->str != NULL) {
      free(this->str);
      this->str = NULL;
    }
  }
}


/****************************************************************************************************
*                                                                                                   *
****************************************************************************************************/

/**
* Public fxn to get the length of the string represented by this class. This is called before allocating
*  mem to flatten the string into a single string.
*/
int StringBuilder::length() {
  int return_value = this->col_length;
  if (this->root != NULL) {
    return_value  = return_value + this->totalStrLen(this->root);
  }
  return return_value;
}

/**
* How many discrete elements are in this object?
* Includes the base str member in the count.
*/
unsigned short StringBuilder::count() {
  unsigned short return_value = 0;
  if (this->str != NULL) {
  return_value++;
  }
  StrLL *current = this->root;
  while (current != NULL) {
    return_value++;
    current = current->next;
  }
  return return_value;
}



/**
* Public fxn to retrieve the flattened string as an unsigned char *.
*   Will never return NULL. Will return a zero-length string in the worst-case.
*/
unsigned char* StringBuilder::string() {
  if ((this->str == NULL) && (this->root == NULL)) {
    // Nothing in this object. Return a zero-length string.
    this->str = (unsigned char *) malloc(1);
    this->str[0] = '\0';
    this->col_length  = 0;
  }
  else {
    this->collapseIntoBuffer();
  }
  return this->str;
}


void StringBuilder::clear(void) {
  if (this->root != NULL) destroyStrLL(this->root);
  if (this->str != NULL) {
    free(this->str);
  }
  this->root   = NULL;
  this->str    = NULL;
  this->col_length = 0;
}


/**
* Return a castable pointer for the string at position <pos>.
* Null on failure.
*/
char* StringBuilder::position(int pos) {
  if (this->str != NULL) {
    if (pos == 0) {
      return (char*) this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) & (current != NULL)){
    current = current->next;
    i++;
  }
  return (current == NULL) ? NULL : ((char *)current->str);
}


/**
* Convenience fxn for breaking up strings of integers using the tokenizer.
*/
int StringBuilder::position_as_int(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != NULL) {
    int len = strlen(temp);
    if ((len > 2) && (*(temp) == '0') && (*(temp + 1) == 'x')) {
      // Possibly dealing with a hex representation. Try to convert....
      unsigned int result = 0;
      len -= 2;
      temp += 2;
      // Only so big an int we can handle...
      int max_bytes = (len > 8) ? 8 : len;
      for (int i = 0; i < max_bytes; i++) {
        switch (*(temp + i)) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            result = (result << 4) + (*(temp + i) - 0x30);
            break;
          case 'a':
          case 'b':
          case 'c':
          case 'd':
          case 'e':
          case 'f':
            result = (result << 4) + 10 + (*(temp + i) - 0x61);
            break;
          case 'A':
          case 'B':
          case 'C':
          case 'D':
          case 'E':
          case 'F':
            result = (result << 4) + 10 + (*(temp + i) - 0x41);
            break;
          default: return result;
        }
      }
      return result;
    }
    else {
      return atoi(temp);
    }
  }
  return 0;
}


/**
* Return a castable pointer for the string at position <pos>.
* Null on failure.
*/
unsigned char* StringBuilder::position(int pos, int *pos_len) {
  if (this->str != NULL) {
    if (pos == 0) {
      *pos_len = this->col_length;
      return this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) & (current != NULL)){
    current = current->next;
    i++;
  }
  *pos_len = (current != NULL) ? current->len : 0;
  return ((current != NULL) ? current->str : (unsigned char *)"");
}


char* StringBuilder::position_trimmed(int pos){
  char* str = position(pos);
  if (str == NULL) {
    return (char*) "";
  }
  char *end;
  while(isspace(*str)) str++;
  if(*str == 0) return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end+1) = 0;
  return str;
}


/**
* Returns true on success, false on failure.
*/
bool StringBuilder::drop_position(unsigned int pos) {
  if (this->str != NULL) {
    if (pos == 0) {
      this->col_length = 0;
      free(this->str);
      this->str = NULL;
      return true;
    }
    pos--;
  }
  StrLL *current = this->root;
  StrLL *prior = NULL;
  unsigned int i = 0;
  while ((i != pos) & (current != NULL)){
    prior = current;
    current = current->next;
    i++;
  }
  if (current != NULL) {
    if (prior == NULL) {
      this->root = current->next;
      current->next = NULL;
      destroyStrLL(current);
    }
    else {
      prior->next = current->next;
      current->next = NULL;
      destroyStrLL(current);
    }
    return true;
  }
  return false;
}


/**
* This fxn is meant to hand off accumulated string data to us from the StringBuilder
*   that is passed in as the parameter. This is very fast, but doesn't actually copy
*   any data. So to eliminate the chance of dangling pointers and other 
*   difficult-to-trace bugs, we modify the donor SB to prevent its destruction from
*   taking the data we now have with it.
*/
void StringBuilder::concatHandoff(StringBuilder *nu) {
  if (NULL != nu) {
    nu->promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    if (NULL != nu->root) {
      this->stackStrOntoList(nu->root);
      nu->root = NULL;  // Inform the origin instance...
    }
  }
}


/*
* Thank you N. Gortari for the most excellent tip:
*   Intention:       if (obj == NULL)  // Null-checking is a common thing to do...
*   The risk:        if (obj = NULL)   // Compiler allows this. Assignment always evals to 'true'. 
*   The mitigation:  if (NULL == obj)  // Equality is commutitive.
*
*     "(NULL == obj)" and "(obj == NULL)" mean exaclty the same thing, and are equally valid.
*
* Levarge assignment operator's non-commutivity, so if you derp and do this...
*           if (NULL = obj) 
* ...the mechanics of the language will prevent compilation, and thus, not allow you
*       to overlook it on accident.
*/
void StringBuilder::prependHandoff(StringBuilder *nu) {
  if (NULL != nu) {
    this->root = promote_collapsed_into_ll();   // Promote the previously-collapsed string.
    
    // Promote the donor instance's previously-collapsed string so we don't have to worry about it.
    StrLL *current = nu->promote_collapsed_into_ll();
    
    if (NULL != nu->root) {
      // Scan to the end of the donated LL...
      while (NULL != current->next) {   current = current->next;  }
      
      current->next = this->root;  // ...and tack our existing list to the end of it.
      this->root = nu->root;       // ...replace our idea of the root. 
      nu->root = NULL;             // Inform the origin instance so it doesn't free what we just took.
    }
  }
}


/*
* Calling this fxn should take the collapsed string (if any) and prepend it to the list,
*   taking any measures necessary to convince the class that there is no longer a collapsed
*   string present.
* Always returns a pointer to the root of the LL. Changed or otherwise.
*/
StrLL* StringBuilder::promote_collapsed_into_ll(void) {
  if ((NULL != str) && (col_length > 0)) {
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (NULL != nu_element) {   // This is going to grief us later...
      nu_element->reap = true;
      nu_element->next = this->root;
      this->root = nu_element;
      nu_element->str = this->str;
      nu_element->len = this->col_length;
      this->str = NULL;
      this->col_length = 0;
    }
  }
  return this->root;
}


/*
*/
void StringBuilder::prepend(unsigned char *nu, int len) {
  if ((NULL != nu) && (len > 0)) {
    this->root = promote_collapsed_into_ll();   // Promote the previously-collapsed string.
    
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (NULL == nu_element) return;   // O no.
    nu_element->reap = true;
    nu_element->len  = len;
    nu_element->str  = (unsigned char *) malloc(len+1);
    if (nu_element->str != NULL) {
      *(nu_element->str + len) = '\0';
      memcpy(nu_element->str, nu, len);
      nu_element->next = this->root;
      this->root = nu_element;
    }
  }
}


/**
* Override to cleanly support signed characters.
*/
void StringBuilder::prepend(char *nu) {
  this->prepend((unsigned char *) nu, strlen(nu));
}

void StringBuilder::prepend(const char *nu) {
  this->prepend((unsigned char *) nu, strlen(nu));
}


/**
* Private function to get the length of a string from any particular node.
* Does not consider the collapsed buffer.
*/
int StringBuilder::totalStrLen(StrLL *node) {
  int len  = 0;
  if (node != NULL) {
    len  = this->totalStrLen(node->next);
    if (node->str != NULL)  len  = len + node->len;
  }
  return len;
}


void StringBuilder::concat(unsigned char *nu, int len) {
  if ((nu != NULL) && (len > 0)) {
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nu_element != NULL) {
      nu_element->reap = true;
      nu_element->next = NULL;
      nu_element->len  = len;
      nu_element->str  = (unsigned char *) malloc(len+1);
      if (nu_element->str != NULL) {
        *(nu_element->str + len) = '\0';   
        memcpy(nu_element->str, nu, len);
        this->stackStrOntoList(nu_element);
      }
    }
  }
}

/**
* Override to cleanly support signed characters.
*/
void StringBuilder::concat(char *nu) {
  this->concat((unsigned char *) nu, strlen(nu));
}

/**
* Override to make best use of memory for const strings...
*
* Crash warning: Any const char* concat'd to a StringBuilder will not be copied
*   until it is manipulated somehow. So be very careful if you cast to (const char*).
*/
void StringBuilder::concat(const char *nu) {
  if (nu != NULL) {
    int len = strlen(nu);
    if (len > 0) {
      StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
      if (nu_element != NULL) {
        nu_element->reap = false;
        nu_element->next = NULL;
        nu_element->len  = len;
        nu_element->str  = (unsigned char *) nu;
        this->stackStrOntoList(nu_element);
      }
    }
  }
}


void StringBuilder::concat(unsigned char nu) {
  unsigned char* temp = (unsigned char *) alloca(1);
  *(temp) = nu;
  this->concat(temp, 1);
}
void StringBuilder::concat(char nu) {
  char* temp = (char *) alloca(2);
  *(temp) = nu;
  *(temp+1) = 0;
  this->concat(temp);
}
void StringBuilder::concat(int nu) {
  char * temp = (char *) alloca(12);
  memset(temp, 0x00, 12);
  sprintf(temp, "%d", nu);
  this->concat(temp);
}
void StringBuilder::concat(unsigned int nu) { 
  char * temp = (char *) alloca(12);
  memset(temp, 0x00, 12);
  sprintf(temp, "%u", nu);
  this->concat(temp);
}
void StringBuilder::concat(double nu) {
  char * temp = (char *) alloca(16);
  memset(temp, 0x00, 16);
  sprintf(temp, "%f", nu);
  this->concat(temp);
}

void StringBuilder::concat(float nu) {
  this->concat((double) nu);
}

void StringBuilder::concat(bool nu) {
  if (nu) this->concat("1");
  else this->concat("0");
}

/**
* Override to cleanly support Our own type.
* This costs a great deal more than concatHandoff(StringBuilder *), but has
*   the advantage of making a copy of the argument.
*/
void StringBuilder::concat(StringBuilder *nu) {
  if (nu != NULL) {
    this->concat(nu->string(), nu->length());
  }
}

/*
* Variadic
*/
int StringBuilder::concatf(const char *format, ...) {
  int len = strlen(format);
  unsigned short f_codes = 0;  // Count how many format codes are in use...
  for (unsigned short i = 0; i < len; i++) {  if (*(format+i) == '%') f_codes++; }
  va_list args;
  // Allocate (hopefully) more space than we will need....
  int est_len = strlen(format) + 64 + (f_codes * 15);
  char *temp = (char *) alloca(est_len);
  memset(temp, 0, est_len);
  va_start(args, format);
  int ret = vsprintf(temp, format, args);
  va_end(args);
  if (ret > 0) this->concat((char *) temp);
  return ret;
}




#ifdef ARDUINO
/**
* Override to cleanly support Strings.
*/
void StringBuilder::concat(String str) {
  int len = str.length()+1;
  char *out  = (char *) alloca(len);
  memset(out, 0, len);
  str.toCharArray(out, (str.length()+1));
  this->concat((unsigned char *) out, strlen(out));
}
#endif


/*
* Given an offset and a length, will throw away any part of the string that falls outside
*   of the given range.
*/
void StringBuilder::cull(int offset, int length) {
  if (this->length() >= (length-offset)) {   // Does the given range exist?
    int remaining_length = length-offset;
    unsigned char* temp = (unsigned char*) malloc(remaining_length+1);  // + 1 for null-terminator.
    if (temp != NULL) {
      *(temp + remaining_length) = '\0';
      this->collapseIntoBuffer();   // Room to optimize here...
      memcpy(temp, this->str, remaining_length);
      this->clear();         // Throw away all else.
      this->str = temp;      // Replace our ref.
      this->col_length = length;
    }
  }
}


/*
* Given a character count (x), will throw away the first x characters and adjust the object appropriately.
*/
void StringBuilder::cull(int x) {
  if (this->length() >= x) {   // Does the given range exist?
    int remaining_length = this->length()-x;
    unsigned char* temp = (unsigned char*) malloc(remaining_length+1);  // + 1 for null-terminator.
    if (temp != NULL) {
      *(temp + remaining_length) = '\0';
      this->collapseIntoBuffer();   // Room to optimize here...
      memcpy(temp, (unsigned char*)(this->str + x), remaining_length);
      this->clear();         // Throw away all else.
      this->str = temp;      // Replace our ref.
      this->col_length = remaining_length;
    }
  }
}


StrLL* StringBuilder::stackStrOntoList(StrLL *current, StrLL *nu) {
  if (current != NULL) {
    if (current->next == NULL) current->next = nu;
    else this->stackStrOntoList(current->next, nu);
  }
  return current;
}

/*
* Non-recursive override to make additions less cumbersome.
*/
StrLL* StringBuilder::stackStrOntoList(StrLL *nu) {
  if (this->root == NULL) {
    this->root  = nu;
    return this->root;
  }
  else return this->stackStrOntoList(this->root, nu);
}



/**
* Traverse the list and keep appending strings to the buffer.
* Will prepend the str buffer if it is not NULL.
* Updates the length.
*/
void StringBuilder::collapseIntoBuffer() {
  StrLL *current = promote_collapsed_into_ll();   // Promote the previously-collapsed string.
  if (current != NULL) {
    this->col_length = this->totalStrLen(this->root);
    if (this->col_length > 0) {
      this->str = (unsigned char *) malloc(this->col_length + 1);
      if (this->str != NULL) {
        *(this->str + this->col_length) = '\0';
        int tmp_len = 0;
        while (current != NULL) {
          if (current->str != NULL) {
            memcpy((void *)(this->str + tmp_len), (void *)(current->str), current->len);
            tmp_len = tmp_len + current->len;
          }
          current = current->next;
        }
      }
    }
    this->destroyStrLL(this->root);
  }
}


/**
* Clean up after ourselves. Assumes that everything has been malloc'd into existance.
*/
void StringBuilder::destroyStrLL(StrLL *r_node) {
  if (r_node != NULL) {
    if (r_node->next != NULL) {
      destroyStrLL(r_node->next);
      r_node->next  = NULL;
    }
    if (r_node->reap) free(r_node->str);
    free(r_node);
    if (r_node == this->root) this->root = NULL;
  }
}


/*
* Compares two binary strings on a byte-by-byte basis.
* Returns 1 if the values match. 0 otherwise.
* Collapses the buffer prior to comparing.
* Will compare ONLY the first len bytes, or the length of out present string. Whichever is less.
*/
int StringBuilder::cmpBinString(unsigned char *unknown, int len) {
  int i = 0;
  this->collapseIntoBuffer();
  for (i = 0; i < min(len, this->col_length); i++) {
    if (*(unknown+i) != *(this->str+i)) return 0;
  }
  return 1;
}



int StringBuilder::implode(const char *delim) {
  if (delim != NULL) {
    if (str != NULL) {
      
    }
  }
  return 0;
}



/**
* This function tokenizes the sum of this String according to the parameter.
*/
int StringBuilder::split(const char *delims) {
  int return_value = 0;
  this->collapseIntoBuffer();
  if (this->col_length == 0) return 0;
  this->null_term_check();
  char *temp_str  = strtok((char *)this->str, delims);
  if (temp_str != NULL) {
  while (temp_str != NULL) {
    this->concat(temp_str);
    temp_str  = strtok(NULL, delims);
    return_value++;
  }
  free(this->str);
  this->str = NULL;
  this->col_length = 0;
  }
  else {
  this->concat("");
  }
  return return_value;
}



/****************************************************************************************************
* These functions are awful. Don't use them if you can avoid it. They were a crime of desperation.  *
* A future commit will see these disappear into the trash can they should have been consigned to.   *
*    ---J. Ian Lindsay   Fri Nov 28 17:57:45 MST 2014                                               *
****************************************************************************************************/

/**
* Calling this will make a copy of this object on the heap, and return the pointer.
* The goal is to let this object expire on the stack on which it was likely created.
*/
StringBuilder* StringBuilder::heap_ref() {
  StringBuilder* return_value = (StringBuilder*) malloc(sizeof(StringBuilder));
  memcpy(return_value, this, sizeof(StringBuilder));
  // It is important that this be done AFTER the copy, or we will see leaks.
  this->preserve_ll = true;
  return return_value;
}


/**
* Returns the length.
*/
int StringBuilder::str_heap_ref(unsigned char** callers_pntr) {
  this->collapseIntoBuffer();
  *(callers_pntr) = this->str;
  return this->col_length;
}


unsigned char* StringBuilder::str_heap_ref(int len) {
  this->collapseIntoBuffer();
  unsigned char* return_value = this->str;
  this->preserve_ll = true;
  len = this->col_length;
  return return_value;                
}


char* StringBuilder::str_heap_ref() {
  this->collapseIntoBuffer();
  char* return_value = (char*) this->str;
  this->preserve_ll = true;
  return return_value;
}


/**
* If we are going to do something that requires a null-terminated string, make
*   sure that we have one. If we do, this call does nothing.
*   If we don't, we will add it.
* NOTE: This only operates on the collapsed buffer.
*/
void StringBuilder::null_term_check() {
  if (this->str != NULL) {
    if (*(this->str + (this->col_length-1)) != '\0') {
      unsigned char *temp = (unsigned char *) malloc(this->col_length+1);
      if (temp != NULL) {
        *(temp + this->col_length) = '\0';
        memcpy(temp, this->str, this->col_length);
        free(this->str);
        this->str = temp;
      }
    }
  }
}



#ifdef TEST_BENCH
void StringBuilder::printDebug() {
  unsigned char* temp = this->string();
  int temp_len  = this->length();
  printf("\nStringBuilder\t Total bytes: %d\n", temp_len);
  
  if ((temp != NULL) && (temp_len > 0)) {
    for (int i = 0; i < temp_len; i++) {
      printf("%02x ", *(temp + i));
    }
    printf("\n\n");
  }
}

#else
void StringBuilder::printDebug(StringBuilder* output) {
  unsigned char* temp = this->string();
  int temp_len  = this->length();
  output->concatf("\nStringBuilder\t Total bytes: %d\n", temp_len);
  
  if ((temp != NULL) && (temp_len > 0)) {
    for (int i = 0; i < temp_len; i++) {
      output->concatf("%02x ", *(temp + i));
    }
    output->concat("\n\n");
  }
}
#endif
