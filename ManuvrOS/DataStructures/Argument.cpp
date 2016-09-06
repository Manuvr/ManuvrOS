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


#include "Argument.h"
#include <string.h>

#include <DataStructures/PriorityQueue.h>

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
#if defined(MANUVR_CBOR)


int8_t Argument::encodeToCBOR(Argument* src, StringBuilder* out) {
	cbor::output_dynamic output;
	cbor::encoder encoder(output);
	while (nullptr != src) {
		switch(src->typeCode()) {
			case INT8_FM:
			case INT16_FM:
			case INT32_FM:
				{
					int32_t x = 0;
					if (0 == src->getValueAs(&x)) {
						encoder.write_int((int)x);
					}
				}
				break;
			case INT64_FM:
				{
					long long x = 0;
					if (0 == src->getValueAs(&x)) {
						encoder.write_int((long long)x);
					}
				}
				break;
			case UINT8_FM:
			case UINT16_FM:
			case UINT32_FM:
				{
					uint32_t x = 0;
					if (0 == src->getValueAs(&x)) {
						encoder.write_int((unsigned int)x);
					}
				}
				break;
			case UINT64_FM:
				{
					unsigned long long x = 0;
					if (0 == src->getValueAs(&x)) {
						encoder.write_int((unsigned long long int)x);
					}
				}
				break;
		}
		src = src->_next;
	}
	int final_size = output.size();
	if (final_size) {
		out->concat(output.data(), final_size);
	}
	return final_size;
}



CBORArgListener::CBORArgListener(Argument** target) {		built = target;		}

void CBORArgListener::_caaa(Argument* nu) {
	if ((nullptr != built) && (nullptr != *built)) {
		(*built)->append(nu);
	}
	else {
		*built = nu;
	}
}

void CBORArgListener::on_integer(int val) {		_caaa(new Argument((int32_t) val));   };
void CBORArgListener::on_bytes(unsigned char* data, int size) {		_caaa(new Argument(data, size));   };
void CBORArgListener::on_string(char* val) {		_caaa(new Argument(val));   };
void CBORArgListener::on_array(int size) {		_caaa(new Argument((int32_t) size));   };
void CBORArgListener::on_map(int size) {		_caaa(new Argument((int32_t) size));   };
void CBORArgListener::on_tag(unsigned int tag) {		_caaa(new Argument((uint32_t) tag));   };
void CBORArgListener::on_special(unsigned int code) {		_caaa(new Argument((uint32_t) code));   };
void CBORArgListener::on_error(const char* error) {		_caaa(new Argument(error));   };

void CBORArgListener::on_extra_integer(unsigned long long value, int sign) {}
void CBORArgListener::on_extra_tag(unsigned long long tag) {}
void CBORArgListener::on_extra_special(unsigned long long tag) {}


Argument* Argument::decodeFromCBOR(uint8_t* src, unsigned int len) {
	Argument* return_value = nullptr;
	CBORArgListener listener(&return_value);
  cbor::input input(src, len);
  cbor::decoder decoder(input, listener);
  decoder.run();
	return return_value;
}


#endif

#if defined(MANUVR_JSON)
#include "jansson/include/jansson.h"

int8_t Argument::encodeToJSON(Argument* src, StringBuilder* out) {
	return -1;
}

Argument* Argument::decodeFromJSON(StringBuilder* src) {
	return nullptr;
}
#endif

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
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

Argument::Argument(char* val) {
	len        = strlen(val)+1;
	type_code  = STR_FM;
	target_mem = malloc(len);
	if (nullptr != target_mem) {
		memcpy(target_mem, val, len);
		_alter_flags(true, MANUVR_ARG_FLAG_REAP_VALUE);
	}
};


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
* Takes all the keys in the provided argument chain and for any
*   that are ID'd by string keys, prints them to the provided buffer.
*
* @param  key_set A StringBuilder object that w e will write to.
* @return         The number of values written.
*/
int Argument::collectKeys(StringBuilder* key_set) {
	int return_value = 0;
	if (nullptr != _key) {
		key_set->concat(_key);
		return_value++;
	}
	if (nullptr != _next) {
		return_value += _next->collectKeys(key_set);
	}
	return return_value;
}


/**
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

/**
* Get a value by its key.
*
* @param  idx      The Argument position
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
int8_t Argument::getValueAs(const char* k, void* trg_buf) {
  if (nullptr != k) {
		if (nullptr != _key) {
			if (_key == k) {
				// If pointer comparison worked, return the value.
				return getValueAs(trg_buf);
			}
		}

		if (nullptr != _next) {
			return _next->getValueAs(k, trg_buf);
		}
	}
	return -1;
}


/**
* All of the type-specialized getValueAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
*
* @param  trg_buf  A pointer to the place where we should write the result.
* @return 0 on success or appropriate failure code.
*/
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
        return_value = 0;
        *((uint32_t*) trg_buf) = *((uint32_t*)&target_mem);
        break;
      case FLOAT_FM:    // This frightens the compiler. Its fears are unfounded.
        return_value = 0;
        *((float*) trg_buf) = *((float*)&target_mem);
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
* @return A pointer to the appended argument.
*/
Argument* Argument::append(Argument* arg) {
	if (nullptr == _next) {
		_next = arg;
		return arg;
	}
	return _next->append(arg);
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
  out->concatf("\t%s\t%s\t%s",
		(nullptr == _key ? "" : _key),
		getTypeCodeString(typeCode()),
		(reapValue() ? "(reap)" : "\t")
	);
	valToString(out);
  out->concat("\n");

  if (nullptr != _next) _next->printDebug(out);
}
