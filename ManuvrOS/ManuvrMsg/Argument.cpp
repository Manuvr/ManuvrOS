/*
File:   Argument.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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


This class represents our type-abstraction layer. It is the means by which
  we parse from messages without copying.
*/


#include "ManuvrMsg.h"
#include <string.h>

/****************************************************************************************************
* Argument class constructors.                                                                      *
****************************************************************************************************/
/*
* Basal constructor.
*/
Argument::Argument() {
	wipe();
}

/*
* Protected delegate constructor.
*/
Argument::Argument(void* ptr, int l, uint8_t code) : Argument() {
	len        = l;
	type_code  = code;
	target_mem = ptr;
}



Argument::~Argument() {
	if (reapValue() && (nullptr != target_mem)) {
		free(target_mem);   // TODO: This is not good.
		target_mem = nullptr;
	}
}


void Argument::wipe() {
	type_code  = NOTYPE_FM;
	len        = 0;
	_flags     = 0;
	//_next      = nullptr;
	target_mem = nullptr;
}


///**
//* @return [description]
//*/
//int8_t Argument::add(Argument* nu) {
//  if (nullptr != _next) {
//    nu->_next = _next;
//  }
//  _next = nu;
//  return 0;
//}


/**
* All of the type-specialized getArgAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
*
* @param  idx      The Argument position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t Argument::getValueAs(uint8_t idx, void *trg_buf) {
  int8_t return_value = -1;

  if (0 < idx) {
    switch (arg->typeCode()) {
      case INT8_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT8_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint8_t*) trg_buf) = *((uint8_t*)&arg->target_mem);
        break;
      case INT16_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint16_t*) trg_buf) = *((uint16_t*)&arg->target_mem);
        break;
      case INT32_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT32_FM:   // This frightens the compiler. Its fears are unfounded.
      case FLOAT_FM:    // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint32_t*) trg_buf) = *((uint32_t*)&arg->target_mem);

      case UINT32_PTR_FM:  // These are *pointers* to the indicated types. They
      case UINT16_PTR_FM:  //   therefore take the whole 4 bytes of memory allocated
      case UINT8_PTR_FM:   //   and can be returned as such.
      case INT32_PTR_FM:
      case INT16_PTR_FM:
      case INT8_PTR_FM:

      case VECT_4_FLOAT:
      case VECT_3_FLOAT:
      case VECT_3_UINT16:
      case VECT_3_INT16:

      case STR_BUILDER_FM:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
      case STR_FM:                  // This is a pointer to a string constant. Presumably this is stored in flash.
      case BUFFERPIPE_PTR_FM:       // This is a pointer to a BufferPipe/.
      case SYS_RUNNABLE_PTR_FM:     // This is a pointer to ManuvrRunnable.
      case SYS_EVENTRECEIVER_FM:    // This is a pointer to an EventReceiver.
      case SYS_MANUVR_XPORT_FM:     // This is a pointer to a transport.
        return_value = 0;
        *((uintptr_t*) trg_buf) = *((uintptr_t*)&arg->target_mem);
        break;
      default:
        return_value = -2;
        break;
    }
  }
	else if (nullptr != _next) {
		return_value = _next->getValueAs(idx-1, trg_buf);
	}

  return return_value;
}


/*
* The purpose of this fxn is to pack up this Argument into something that can be stored or sent
*   over a wire.
* This is the point at which we will have to translate any pointer types into something concrete.
*
* Returns 0 (0) on success.
*/
int8_t Argument::serialize(StringBuilder *out) {
	if (out == NULL) return -1;

	unsigned char *temp_str = (unsigned char *) target_mem;   // Just a general use pointer.

	unsigned char *scratchpad = (unsigned char *) alloca(258);  // This is the maximum size for an argument.
	unsigned char *sp_index   = (scratchpad + 2);
	uint16_t arg_bin_len       = len;

	switch (type_code) {
	  /* These are hard types that we can send as-is. */
	  case INT8_FM:
	  case UINT8_FM:   // This frightens the compiler. Its fears are unfounded.
	    arg_bin_len = 1;
	    *((uint8_t*) sp_index) = *((uint8_t*)& target_mem);
	    break;
	  case INT16_FM:
	  case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
	    arg_bin_len = 2;
			*((uint16_t*) sp_index) = *((uint16_t*)& target_mem);
	    break;
	  case INT32_FM:
	  case UINT32_FM:   // This frightens the compiler. Its fears are unfounded.
	  case FLOAT_FM:   // This frightens the compiler. Its fears are unfounded.
	    arg_bin_len = 4;
			*((uint32_t*) sp_index) = *((uint32_t*)& target_mem);
	    break;

	  /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
	  case INT8_PTR_FM:
	  case UINT8_PTR_FM:
	    arg_bin_len = 1;
	    *((uint8_t*) sp_index) = *((uint8_t*) target_mem);
	    break;
	  case INT16_PTR_FM:
	  case UINT16_PTR_FM:
	    arg_bin_len = 2;
	    *((uint16_t*) sp_index) = *((uint16_t*) target_mem);
	    break;
	  case INT32_PTR_FM:
	  case UINT32_PTR_FM:
	  case FLOAT_PTR_FM:
	    arg_bin_len = 4;
	    //*((uint32_t*) sp_index) = *((uint32_t*) target_mem);
      for (int i = 0; i < 4; i++) {
        *((uint8_t*) sp_index + i) = *((uint8_t*) target_mem + i);
	    }
	    break;

	  /* These are pointer types that require conversion. */
	  case STR_BUILDER_FM:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
	  case URL_FM:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
	    temp_str    = ((StringBuilder*) target_mem)->string();
      arg_bin_len = ((StringBuilder*) target_mem)->length();
	  case VECT_4_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
	  case VECT_3_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
	  case VECT_3_UINT16: // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
	  case VECT_3_INT16:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
	  case BINARY_FM:
      for (int i = 0; i < arg_bin_len; i++) {
        *(sp_index + i) = *(temp_str + i);
	    }
	    break;

	  /* These are pointer types that will not make sense to the host. They should be dropped. */
	  case RSRVD_FM:              // Reserved code is meaningless to us. How did this happen?
	  case NOTYPE_FM:             // No type isn't valid ANYWHERE in this system. How did this happen?
		case BUFFERPIPE_PTR_FM:     // This would not be an actual buffer. Just it's pipe.
	  case SYS_EVENTRECEIVER_FM:  // Host can't use our internal system services.
	  case SYS_MANUVR_XPORT_FM:   // Host can't use our internal system services.
	  default:
	    return -2;
	}

	*(scratchpad + 0) = type_code;
	*(scratchpad + 1) = arg_bin_len;
	out->concat(scratchpad, (arg_bin_len+2));
	return 0;
}


/*
* The purpose of this fxn is to pack up this Argument into something that can sent over a wire
*   with a minimum of overhead. We write only the bytes that *are* the data, and not the metadata
*   because we are relying on the parser at the other side to know what the type is.
* We still have to translate any pointer types into something concrete.
*
* Returns 0 on success. Also updates the length of data in the offset argument.
*
*/
int8_t Argument::serialize_raw(StringBuilder *out) {
	if (out == NULL) return -1;

	switch (type_code) {
	  /* These are hard types that we can send as-is. */
	  case INT8_FM:
	  case UINT8_FM:
	    out->concat((unsigned char*) &target_mem, 1);
	    break;
	  case INT16_FM:
	  case UINT16_FM:
	    out->concat((unsigned char*) &target_mem, 2);
	    break;
	  case INT32_FM:
	  case UINT32_FM:
	  case FLOAT_FM:
	    out->concat((unsigned char*) &target_mem, 4);
	    break;

	  /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
	  case INT8_PTR_FM:
	  case UINT8_PTR_FM:
	    out->concat((unsigned char*) *((unsigned char**)target_mem), 1);
	    break;
	  case INT16_PTR_FM:
	  case UINT16_PTR_FM:
	    out->concat((unsigned char*) *((unsigned char**)target_mem), 2);
	    break;
	  case INT32_PTR_FM:
	  case UINT32_PTR_FM:
	  case FLOAT_PTR_FM:
	    out->concat((unsigned char*) *((unsigned char**)target_mem), 4);
	    break;

	  /* These are pointer types that require conversion. */
	  case STR_BUILDER_FM:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
	  case URL_FM:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
      out->concat((StringBuilder*) target_mem);
	    break;
	  case STR_FM:
	  case VECT_4_FLOAT:
	  case VECT_3_FLOAT:
	  case VECT_3_UINT16:
	  case VECT_3_INT16:
	  case BINARY_FM:     // This is a pointer to a big binary blob.
      out->concat((unsigned char*) target_mem, len);
	    break;

	  /* These are pointer types that will not make sense to the host. They should be dropped. */
	  case RSRVD_FM:              // Reserved code is meaningless to us. How did this happen?
	  case NOTYPE_FM:             // No type isn't valid ANYWHERE in this system. How did this happen?
		case BUFFERPIPE_PTR_FM:     // This would not be an actual buffer. Just it's pipe.
	  case SYS_EVENTRECEIVER_FM:  // Host can't use our internal system services.
	  case SYS_MANUVR_XPORT_FM:   // Host can't use our internal system services.
	  default:
	    return -2;
	}

	return 0;
}
