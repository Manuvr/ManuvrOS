/*
File:   StringBuilder.cpp
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

#include "StringBuilder.h"

#ifdef ARDUINO
  #include "Arduino.h"
#else
  #include <stdio.h>
  #include <stdlib.h>
  #include <ctype.h>
#endif


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
/*
* We might choose to roll-our-own so that we don't bring in enormous dependencies.
* TODO: This is buggy.
* Taken from
* http://c-for-dummies.com/blog/?p=1359
*/
int StringBuilder::strcasestr(const char *a, const char *b) {
  if ((nullptr != a) && (nullptr != b)) {
    char c = 0;
    while (*a && *b) {
      c = toupper(*a) - toupper(*b);
      if (c != 0) {
        return c;
      }
      a++;
      b++;
    }
    return c;
  }
  return -1;
}


/* Static utility function for dumping buffers for humans to read. */
void StringBuilder::printBuffer(StringBuilder* output, uint8_t* buf, unsigned int len, const char* indent) {
  if (buf) {
    const char* ind = (indent) ? indent : "\t";
    unsigned int i = 0;
    if (len >= 16) {
      for (i = 0; i < (len - 16); i+=16) {
        output->concatf("%s0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
          ind,
          i,
          *(buf + i +  0),
          *(buf + i +  1),
          *(buf + i +  2),
          *(buf + i +  3),
          *(buf + i +  4),
          *(buf + i +  5),
          *(buf + i +  6),
          *(buf + i +  7),
          *(buf + i +  8),
          *(buf + i +  9),
          *(buf + i + 10),
          *(buf + i + 11),
          *(buf + i + 12),
          *(buf + i + 13),
          *(buf + i + 14),
          *(buf + i + 15)
        );
      }
    }
    if (i < len) {
      output->concatf("%s0x%04x: ", ind, i);
      for (; i < len; i++) {
        output->concatf("%02x ", *(buf + i));
      }
      output->concat("\n");
    }
  }
  else {
    if (indent) {
      output->concat(indent);
    }
    output->concat("(NULL BUFFER)\n");
  }
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor.
*/
StringBuilder::StringBuilder() {
  root        = nullptr;
  str         = nullptr;
  col_length  = 0;
  preserve_ll = false;
  #if defined(__BUILD_HAS_PTHREADS)
    #if defined (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    #else
    _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
    #endif
  #elif defined(__BUILD_HAS_FREERTOS)
    //_mutex = xSemaphoreCreateRecursiveMutex();
  #endif
}

StringBuilder::StringBuilder(char* initial) : StringBuilder() {
  this->concat(initial);
}

StringBuilder::StringBuilder(uint8_t* initial, int len) : StringBuilder() {
  this->concat(initial, len);
}


StringBuilder::StringBuilder(const char* initial) : StringBuilder() {
  this->concat(initial);
}


/**
* Destructor.
*/
StringBuilder::~StringBuilder() {
  if (!this->preserve_ll) {
    if (this->root != nullptr) destroyStrLL(this->root);

    if (this->str != nullptr) {
      free(this->str);
      this->str = nullptr;
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_destroy(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //vSemaphoreDelete(&_mutex);
  #endif
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
  if (this->root != nullptr) {
    return_value  = return_value + this->totalStrLen(this->root);
  }
  return return_value;
}

/**
* How many discrete elements are in this object?
* Includes the base str member in the count.
*/
unsigned short StringBuilder::count() {
  unsigned short return_value = (nullptr != this->str) ? 1 : 0;
  StrLL *current = this->root;
  while (current != nullptr) {
    return_value++;
    current = current->next;
  }
  return return_value;
}



/**
* Public fxn to retrieve the flattened string as an uint8_t*.
*   Will never return nullptr. Will return a zero-length string in the worst-case.
*/
unsigned char* StringBuilder::string() {
  if ((this->str == nullptr) && (this->root == nullptr)) {
    // Nothing in this object. Return a zero-length string.
    this->str = (uint8_t*) malloc(1);
    this->str[0] = '\0';
    this->col_length  = 0;
  }
  else {
    this->collapseIntoBuffer();
  }
  return this->str;
}


/**
* Wipes the StringBuilder, free'ing memory as appropriate.
*/
void StringBuilder::clear() {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (this->root != nullptr) destroyStrLL(this->root);
  if (this->str != nullptr) {
    free(this->str);
  }
  this->root   = nullptr;
  this->str    = nullptr;
  this->col_length = 0;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Return a castable pointer for the string at position <pos>.
* Null on failure.
*/
char* StringBuilder::position(int pos) {
  if (this->str != nullptr) {
    if (pos == 0) {
      return (char*) this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) & (current != nullptr)){
    current = current->next;
    i++;
  }
  return (current == nullptr) ? nullptr : ((char *)current->str);
}


/**
* Convenience fxn for breaking up strings of integers using the tokenizer.
*/
int StringBuilder::position_as_int(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != nullptr) {
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
* Convenience fxn for breaking up strings of integers using the tokenizer.
*/
double StringBuilder::position_as_double(int pos) {
  const char* temp = (const char*) position(pos);
  if (temp != nullptr) {
    return atof(temp);
  }
  return 0.0;
}


/**
* Return a castable pointer for the string at position <pos>.
* Null on failure.
*/
unsigned char* StringBuilder::position(int pos, int *pos_len) {
  if (this->str != nullptr) {
    if (pos == 0) {
      *pos_len = this->col_length;
      return this->str;
    }
    pos--;
  }
  StrLL *current = this->root;
  int i = 0;
  while ((i != pos) && (current != nullptr)){
    current = current->next;
    i++;
  }
  *pos_len = (current != nullptr) ? current->len : 0;
  return ((current != nullptr) ? current->str : (uint8_t*)"");
}


char* StringBuilder::position_trimmed(int pos){
  char* str = position(pos);
  if (str == nullptr) {
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
  if (this->str != nullptr) {
    if (pos == 0) {
      this->col_length = 0;
      free(this->str);
      this->str = nullptr;
      return true;
    }
    pos--;
  }
  StrLL *current = this->root;
  StrLL *prior = nullptr;
  unsigned int i = 0;
  while ((i != pos) && (current != nullptr)){
    prior = current;
    current = current->next;
    i++;
  }
  if (current != nullptr) {
    if (prior == nullptr) {
      this->root = current->next;
      current->next = nullptr;
      destroyStrLL(current);
    }
    else {
      prior->next = current->next;
      current->next = nullptr;
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
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_lock(&_mutex);
    pthread_mutex_lock(&nu->_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreTakeRecursive(&_mutex, 0);
    //xSemaphoreTakeRecursive(&nu->_mutex, 0);
  #endif
  if ((nullptr != nu) && (nu->length() > 0)) {
    nu->promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    if (nullptr != nu->root) {
      this->stackStrOntoList(nu->root);
      nu->root = nullptr;  // Inform the origin instance...
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    pthread_mutex_unlock(&nu->_mutex);
    pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
    //xSemaphoreGiveRecursive(&nu->_mutex);
    //xSemaphoreGiveRecursive(&_mutex);
  #endif
}


/*
* Thank you N. Gortari for the most excellent tip:
*   Intention:       if (obj == nullptr)  // Null-checking is a common thing to do...
*   The risk:        if (obj = nullptr)   // Compiler allows this. Assignment always evals to 'true'.
*   The mitigation:  if (nullptr == obj)  // Equality is commutitive.
*
*     "(nullptr == obj)" and "(obj == nullptr)" mean exaclty the same thing, and are equally valid.
*
* Levarge assignment operator's non-commutivity, so if you derp and do this...
*           if (nullptr = obj)
* ...the mechanics of the language will prevent compilation, and thus, not allow you
*       to overlook it on accident.
*/
void StringBuilder::prependHandoff(StringBuilder *nu) {
  if (nullptr != nu) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
      //pthread_mutex_lock(&nu->_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    this->root = promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    // Promote the donor instance's previously-collapsed string so we don't have to worry about it.
    StrLL *current = nu->promote_collapsed_into_ll();

    if (nullptr != nu->root) {
      // Scan to the end of the donated LL...
      while (nullptr != current->next) {   current = current->next;  }

      current->next = this->root;  // ...and tack our existing list to the end of it.
      this->root = nu->root;       // ...replace our idea of the root.
      nu->root = nullptr;             // Inform the origin instance so it doesn't free what we just took.
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&nu->_mutex);
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/*
* This must be called with non-stack buffers,
* For safety's sake, the last byte should be a '\0'.
*/
void StringBuilder::concatHandoff(uint8_t* buf, int len) {
  if ((buf) && (len > 0)) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nu_element) {
      nu_element->reap = true;
      nu_element->next = nullptr;
      nu_element->len  = len;
      nu_element->str  = buf;
      this->stackStrOntoList(nu_element);
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/*
* Calling this fxn should take the collapsed string (if any) and prepend it to the list,
*   taking any measures necessary to convince the class that there is no longer a collapsed
*   string present.
* Always returns a pointer to the root of the LL. Changed or otherwise.
*/
StrLL* StringBuilder::promote_collapsed_into_ll(void) {
  if ((nullptr != str) && (col_length > 0)) {
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nullptr != nu_element) {   // This is going to grief us later...
      nu_element->reap = true;
      nu_element->next = this->root;
      this->root = nu_element;
      nu_element->str = this->str;
      nu_element->len = this->col_length;
      this->str = nullptr;
      this->col_length = 0;
    }
  }
  return this->root;
}


/*
*/
void StringBuilder::prepend(uint8_t*nu, int len) {
  if ((nullptr != nu) && (len > 0)) {
    this->root = promote_collapsed_into_ll();   // Promote the previously-collapsed string.

    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nullptr == nu_element) return;   // O no.
    nu_element->reap = true;
    nu_element->len  = len;
    nu_element->str  = (uint8_t*) malloc(len+1);
    if (nu_element->str != nullptr) {
      *(nu_element->str + len) = '\0';
      memcpy(nu_element->str, nu, len);
      nu_element->next = this->root;
      this->root = nu_element;
    }
    else {
      // We were able to malloc the slot for the string, but not the buffer for
      // the string itself. We should free() the slot before failing or we will
      // exacerbate an already-present memory crunch.
      free(nu_element);
    }
  }
}


void StringBuilder::prepend(const char *nu) {
  this->prepend((uint8_t*) nu, strlen(nu));
}


/**
* Private function to get the length of a string from any particular node.
* Does not consider the collapsed buffer.
*/
int StringBuilder::totalStrLen(StrLL *node) {
  int len  = 0;
  if (node != nullptr) {
    len  = this->totalStrLen(node->next);
    if (node->str != nullptr)  len  = len + node->len;
  }
  return len;
}


void StringBuilder::concat(uint8_t*nu, int len) {
  if ((nu != nullptr) && (len > 0)) {
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_lock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
    StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
    if (nu_element != nullptr) {
      nu_element->reap = true;
      nu_element->next = nullptr;
      nu_element->len  = len;
      nu_element->str  = (uint8_t*) malloc(len+1);
      if (nu_element->str != nullptr) {
        *(nu_element->str + len) = '\0';
        memcpy(nu_element->str, nu, len);
        this->stackStrOntoList(nu_element);
      }
    }
    #if defined(__BUILD_HAS_PTHREADS)
      //pthread_mutex_unlock(&_mutex);
    #elif defined(__BUILD_HAS_FREERTOS)
    #endif
  }
}


/**
* Override to make best use of memory for const strings...
*
* Crash warning: Any const char* concat'd to a StringBuilder will not be copied
*   until it is manipulated somehow. So be very careful if you cast to (const char*).
*/
void StringBuilder::concat(const char *nu) {
  if (nu != nullptr) {
    int len = strlen(nu);
    if (len > 0) {
      #if defined(__BUILD_HAS_PTHREADS)
        //pthread_mutex_lock(&_mutex);
      #elif defined(__BUILD_HAS_FREERTOS)
      #endif
      StrLL *nu_element = (StrLL *) malloc(sizeof(StrLL));
      if (nu_element != nullptr) {
        nu_element->reap = false;
        nu_element->next = nullptr;
        nu_element->len  = len;
        nu_element->str  = (uint8_t*) nu;
        this->stackStrOntoList(nu_element);
      }
      #if defined(__BUILD_HAS_PTHREADS)
        //pthread_mutex_unlock(&_mutex);
      #elif defined(__BUILD_HAS_FREERTOS)
      #endif
    }
  }
}


void StringBuilder::concat(unsigned char nu) {
  unsigned char* temp = (uint8_t*) alloca(1);
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

/**
* Override to cleanly support Our own type.
* This costs a great deal more than concatHandoff(StringBuilder *), but has
*   the advantage of making a copy of the argument.
*/
void StringBuilder::concat(StringBuilder *nu) {
  if (nu != nullptr) {
    this->concat(nu->string(), nu->length());
  }
}

/**
* Variadic. No mutex required because all working memory is confined to stack.
*/
int StringBuilder::concatf(const char *format, ...) {
  int len = strlen(format);
  unsigned short f_codes = 0;  // Count how many format codes are in use...
  for (unsigned short i = 0; i < len; i++) {  if (*(format+i) == '%') f_codes++; }
  va_list args;
  // Allocate (hopefully) more space than we will need....
  int est_len = len + 640 + (f_codes * 15);
  char *temp = (char *) alloca(est_len);
  memset(temp, 0, est_len);
  int ret = 0;
  va_start(args, format);
  ret = vsprintf(temp, format, args);
  va_end(args);
  if (ret > 0) this->concat((char *) temp);
  return ret;
}


//#ifdef ARDUINO
///**
//* Override to cleanly support Strings.
//*/
//void StringBuilder::concat(String str) {
//  int len = str.length()+1;
//  char *out  = (char *) alloca(len);
//  memset(out, 0, len);
//  str.toCharArray(out, (str.length()+1));
//  this->concat((uint8_t*) out, strlen(out));
//}
//#endif


/*
* Given an offset and a length, will throw away any part of the string that falls outside
*   of the given range.
*/
void StringBuilder::cull(int offset, int length) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (this->length() >= (length-offset)) {   // Does the given range exist?
    int remaining_length = length-offset;
    unsigned char* temp = (unsigned char*) malloc(remaining_length+1);  // + 1 for null-terminator.
    if (temp != nullptr) {
      *(temp + remaining_length) = '\0';
      this->collapseIntoBuffer();   // Room to optimize here...
      memcpy(temp, this->str, remaining_length);
      this->clear();         // Throw away all else.
      this->str = temp;      // Replace our ref.
      this->col_length = length;
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/*
* Given a character count (x), will throw away the first x characters and adjust the object appropriately.
*/
void StringBuilder::cull(int x) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (x == this->length()) {
    clear();
  }
  else if (this->length() > x) {   // Does the given range exist?
    int remaining_length = this->length()-x;
    unsigned char* temp = (unsigned char*) malloc(remaining_length+1);  // + 1 for null-terminator.
    if (temp != nullptr) {
      *(temp + remaining_length) = '\0';
      this->collapseIntoBuffer();   // Room to optimize here...
      memcpy(temp, (unsigned char*)(this->str + x), remaining_length);
      this->clear();         // Throw away all else.
      this->str = temp;      // Replace our ref.
      this->col_length = remaining_length;
    }
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Trims whitespace from the ends of the string and replaces it.
* Has heap consequences.
*/
void StringBuilder::trim() {
  this->collapseIntoBuffer();
  // TODO: How have I not needed this yet? Add it...
  //    ---J. Ian Lindsay   Thu Dec 17 03:22:01 MST 2015
}

bool StringBuilder::contains(char test) {
  this->collapseIntoBuffer();
  if (this->col_length == 0) return 0;

  for (int i = 0; i < this->col_length; i++) {
    if (test == *(this->str + i)) return true;
  }
  return false;
}


StrLL* StringBuilder::stackStrOntoList(StrLL *current, StrLL *nu) {
  if (current != nullptr) {
    if (current->next == nullptr) current->next = nu;
    else this->stackStrOntoList(current->next, nu);
  }
  return current;
}

/*
* Non-recursive override to make additions less cumbersome.
*/
StrLL* StringBuilder::stackStrOntoList(StrLL *nu) {
  StrLL* return_value = nullptr;
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (this->root == nullptr) {
    this->root  = nu;
    return_value = this->root;
  }
  else {
    return_value = this->stackStrOntoList(this->root, nu);
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  return return_value;
}



/**
* Traverse the list and keep appending strings to the buffer.
* Will prepend the str buffer if it is not nullptr.
* Updates the length.
*/
void StringBuilder::collapseIntoBuffer() {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  StrLL *current = promote_collapsed_into_ll();   // Promote the previously-collapsed string.
  if (current != nullptr) {
    this->col_length = this->totalStrLen(this->root);
    if (this->col_length > 0) {
      this->str = (uint8_t*) malloc(this->col_length + 1);
      if (this->str != nullptr) {
        *(this->str + this->col_length) = '\0';
        int tmp_len = 0;
        while (current != nullptr) {
          if (current->str != nullptr) {
            memcpy((void *)(this->str + tmp_len), (void *)(current->str), current->len);
            tmp_len = tmp_len + current->len;
          }
          current = current->next;
        }
      }
    }
    this->destroyStrLL(this->root);
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/**
* Clean up after ourselves. Assumes that everything has been malloc'd into existance.
*/
void StringBuilder::destroyStrLL(StrLL *r_node) {
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_lock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
  if (r_node != nullptr) {
    if (r_node->next != nullptr) {
      destroyStrLL(r_node->next);
      r_node->next  = nullptr;
    }
    if (r_node->reap) free(r_node->str);
    free(r_node);
    if (r_node == this->root) this->root = nullptr;
  }
  #if defined(__BUILD_HAS_PTHREADS)
    //pthread_mutex_unlock(&_mutex);
  #elif defined(__BUILD_HAS_FREERTOS)
  #endif
}


/*
* Compares two binary strings on a byte-by-byte basis.
* Returns 1 if the values match. 0 otherwise.
* Collapses the buffer prior to comparing.
* Will compare ONLY the first len bytes, or the length of out present string. Whichever is less.
*/
int StringBuilder::cmpBinString(uint8_t*unknown, int len) {
  this->collapseIntoBuffer();
  int minimum = (len > this->col_length) ? this->col_length : len;
  for (int i = 0; i < minimum; i++) {
    if (*(unknown+i) != *(this->str+i)) return 0;
  }
  return 1;
}


int StringBuilder::implode(const char *delim) {
  if (delim != nullptr) {
    if (str != nullptr) {
    }
  }
  return 0;
}


/**
* This function tokenizes the sum of this String according to the parameter.
*
* @return The number of tokens.
*/
int StringBuilder::split(const char *delims) {
  int return_value = 0;
  this->collapseIntoBuffer();
  if (this->col_length == 0) {
    return 0;
  }

  // If we are going to do something that requires a null-terminated string,
  //   make sure that we have one. If we do, this call does nothing.
  //   If we don't, we will add it.
  if (nullptr != this->str) {
    if (*(this->str + (this->col_length-1)) != '\0') {
      uint8_t* temp = (uint8_t*) malloc(this->col_length+1);
      if (nullptr != temp) {
        *(temp + this->col_length) = '\0';
        memcpy(temp, this->str, this->col_length);
        free(this->str);
        this->str = temp;
      }
    }
  }

  char *temp_str  = strtok((char *)this->str, delims);
  if (nullptr != temp_str) {
    while (nullptr != temp_str) {
      this->concat(temp_str);
      return_value++;
      temp_str  = strtok(nullptr, delims);
    }
    free(this->str);
    this->str = nullptr;
    this->col_length = 0;
  }
  else {
    this->concat("");   // Assure at least one token.
  }
  return return_value;
}


/**
* This method prints ASCII representations of the bytes this instance contains.
*
* @param output The StringBuilder object into which output is written.
*/
void StringBuilder::printDebug(StringBuilder* output) {
  unsigned char* temp = this->string();
  int temp_len  = this->length();

  if ((temp != nullptr) && (temp_len > 0)) {
    for (int i = 0; i < temp_len; i++) {
      output->concatf("%02x ", *(temp + i));
    }
    output->concat("\n");
  }
}
