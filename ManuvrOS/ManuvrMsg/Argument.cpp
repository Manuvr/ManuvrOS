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
Argument::Argument() {}

/*
* Protected delegate constructor.
*/
Argument::Argument(void* ptr, int l, uint8_t code) : Argument() {
	len        = l;
	type_code  = code;
	target_mem = ptr;
	switch (type_code) {
		// TODO: This does not belong here, and I know it.
		case INT8_FM:
		case UINT8_FM:
		case INT16_FM:
		case UINT16_FM:
		case INT32_FM:
		case UINT32_FM:
		case FLOAT_FM:
		case BOOLEAN_FM:
			_alter_flags(true, MANUVR_ARG_FLAG_DIRECT_VALUE);
			break;
		case UINT128_FM:
		case INT128_FM:
		case UINT64_FM:
		case INT64_FM:
		case DOUBLE_FM:
		default:
			break;
	}
}


Argument::~Argument() {
	wipe();
}


void Argument::wipe() {
	if (nullptr != _next) {
		Argument* a = _next;
		_next       = nullptr;
		delete a;
	}
	if (nullptr != target_mem) {
		void* p = target_mem;
		target_mem = nullptr;
		if (reapValue()) free(p);
	}
	_key       = nullptr;
	type_code  = NOTYPE_FM;
	len        = 0;
	_flags     = 0;
}


/**
* All of the type-specialized getArgAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
*
* @param  idx      The Argument position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t Argument::getValueAs(uint8_t idx, void* trg_buf) {
	int8_t return_value = -1;
  if (0 < idx) {
		if (nullptr != _next) {
			return_value = _next->getValueAs(--idx, trg_buf);
		}
	}
	else {
		return_value = getValueAs(trg_buf);
	}
	return return_value;
}


int8_t Argument::getValueAs(void* trg_buf) {
  int8_t return_value = -1;
  if (nullptr != pointer()) {
    switch (typeCode()) {
      case INT8_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT8_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint8_t*) trg_buf) = *((uint8_t*)&target_mem);
        break;
      case INT16_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint16_t*) trg_buf) = *((uint16_t*)&target_mem);
        break;
      case INT32_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT32_FM:   // This frightens the compiler. Its fears are unfounded.
      case FLOAT_FM:    // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((uint32_t*) trg_buf) = *((uint32_t*)&target_mem);
        break;
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
        *((uintptr_t*) trg_buf) = *((uintptr_t*)&target_mem);
        break;
      default:
        return_value = -2;
        break;
    }
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


/**
* @return The index of the appended argument.
*/
int Argument::append(Argument* arg) {
	if (nullptr == _next) {
		_next = arg;
		return 0;
	}
	return (1 + _next->append(arg));
}


/**
* @return The number of arguments in this list.
*/
int Argument::argCount() {
  return (1 + ((nullptr == _next) ? 0 : _next->argCount()));
}


/**
* @return [description]
*/
int Argument::sumAllLengths() {
  return (len + ((nullptr == _next) ? 0 : _next->sumAllLengths()));
}

/**
* @return [description]
*/
Argument* Argument::retrieveArgByIdx(int idx) {
	if ((0 < idx) && (nullptr != _next)) {
		return (1 == idx) ? _next : _next->retrieveArgByIdx(idx-1);
	}
	else {
		return this;
	}
}


void Argument::valToString(StringBuilder* out) {
	uint8_t* buf     = (uint8_t*) pointer();
	if (_check_flags(MANUVR_ARG_FLAG_DIRECT_VALUE)) {
		switch (type_code) {
			case INT8_FM:
			case UINT8_FM:
			case INT16_FM:
			case UINT16_FM:
			case INT32_FM:
			case UINT32_FM:
				out->concatf("%u", (uintptr_t) pointer());
				break;
			case FLOAT_FM:
				out->concatf("%f", (double)(uintptr_t) pointer());
				break;
			case BOOLEAN_FM:
				out->concatf("%s", ((uintptr_t) pointer() ? "true" : "false"));
				break;
			default:
				out->concatf("%p", pointer());
				break;
		}
	}
	else {
  	int l_ender = (len < 16) ? len : 16;
		for (int n = 0; n < l_ender; n++) {
			out->concatf("%02x ", *((uint8_t*) buf + n));
		}
	}
}


/*
* Warning: call is propagated across entire list.
*/
void Argument::printDebug(StringBuilder* out) {
  out->concatf("\t%s\t%s", getTypeCodeString(typeCode()), (reapValue() ? "(reap)" : "\t"));
	valToString(out);
  out->concat("\n");

  if (nullptr != _next) _next->printDebug(out);
}
