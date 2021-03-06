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

#include <PriorityQueue.h>
#if defined(CONFIG_MANUVR_IMG_SUPPORT)
  #include "Image/Image.h"
#endif   // CONFIG_MANUVR_IMG_SUPPORT

#include <Platform/Identity.h>

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
    if (nullptr != src->getKey()) {
      // This is a map.
      encoder.write_map(1);
      encoder.write_string(src->getKey());
    }
    switch(src->typeCode()) {
      case TCode::INT8:
        {
          int8_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT16:
        {
          int16_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT32:
        {
          int32_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((int) x);
          }
        }
        break;
      case TCode::INT64:
        {
          long long x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int(x);
          }
        }
        break;
      case TCode::UINT8:
        {
          uint8_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT16:
        {
          uint16_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT32:
        {
          uint32_t x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int((unsigned int) x);
          }
        }
        break;
      case TCode::UINT64:
        {
          unsigned long long x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_int(x);
          }
        }
        break;
      case TCode::FLOAT:
        {
          float x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_float(x);
          }
        }
        break;

      case TCode::DOUBLE:
        {
          double x = 0;
          if (0 == src->getValueAs(&x)) {
            encoder.write_double(x);
          }
        }
        break;

      case TCode::STR:
        {
          char* buf;
          if (0 == src->getValueAs(&buf)) {
            encoder.write_string(buf);
          }
        }
        break;
      case TCode::STR_BUILDER:
        {
          StringBuilder* buf;
          if (0 == src->getValueAs(&buf)) {
            encoder.write_string((char*) buf->string());
          }
        }
        break;

      case TCode::BINARY:
      case TCode::VECT_3_FLOAT:
      case TCode::VECT_4_FLOAT:
        // NOTE: This ought to work for any types retaining portability isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
        encoder.write_bytes((uint8_t*) src->pointer(), src->length());
        break;

      case TCode::IDENTITY:
        {
          Identity* ident;
          if (0 == src->getValueAs(&ident)) {
            uint16_t i_len = ident->length();
            uint8_t buf[i_len];
            if (ident->toBuffer(buf)) {
              encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
              encoder.write_bytes(buf, i_len);
            }
          }
        }
        break;
      //case TCode::ARGUMENT:
      //  {
      //    StringBuilder intermediary;
      //    Argument *subj;
      //    if (0 == src->getValueAs(&subj)) {
      //      // NOTE: Recursion.
      //      if (Argument::encodeToCBOR(subj, &intermediary)) {
      //        encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
      //        encoder.write_bytes(intermediary.string(), intermediary.length());
      //      }
      //    }
      //  }
      //  break;
      case TCode::IMAGE:
        #if defined(CONFIG_MANUVR_IMG_SUPPORT)
          {
            Image* img;
            if (0 == src->getValueAs(&img)) {
              uint32_t sz_buf = img->bytesUsed();
              if (sz_buf > 0) {
                uint32_t nb_buf = 0;
                uint8_t* intermediary = (uint8_t*) alloca(32);
                if (0 == img->serializeWithoutBuffer(intermediary, &nb_buf)) {
                  encoder.write_tag(MANUVR_CBOR_VENDOR_TYPE | TcodeToInt(src->typeCode()));
                  encoder.write_bytes(intermediary, nb_buf);   // TODO: This might cause two discrete CBOR objects.
                  encoder.write_bytes(img->buffer(), sz_buf);
                }
              }
            }
          }
        #endif   // CONFIG_MANUVR_IMG_SUPPORT
        break;
      //case TCode::SYS_PIPE_FXN_PTR:
      //case TCode::SYS_ARG_FXN_PTR:
      //case TCode::SYS_MANUVR_XPORT:
      //case TCode::SYS_FXN_PTR:
      //case TCode::SYS_THREAD_FXN_PTR:
      //case TCode::SYS_MANUVRMSG:
      case TCode::RESERVED:
        // Peacefully ignore the types we can't export.
        break;
      default:
        // TODO: Handle pointer types, bool
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



Argument* Argument::decodeFromCBOR(uint8_t* src, unsigned int len) {
  Argument* return_value = nullptr;
  CBORArgListener listener(&return_value);
  cbor::input input(src, len);
  cbor::decoder decoder(input, listener);
  decoder.run();
  return return_value;
}
#endif  // MANUVR_CBOR


#if defined(MANUVR_JSON)
#include "jansson/include/jansson.h"

int8_t Argument::encodeToJSON(Argument* src, StringBuilder* out) {
  return -1;
}

Argument* Argument::decodeFromJSON(StringBuilder* src) {
  return nullptr;
}
#endif //MANUVR_JSON


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Basal constructor.
*/
Argument::Argument() {
}

/**
* TCode-only constructor. Does no allocation, and assigns length if knowable.
*/
Argument::Argument(TCode code) : Argument() {
  _t_code  = code;
  switch (_t_code) {
    // TODO: This does not belong here, and I know it.
    case TCode::INT8:
    case TCode::UINT8:
    case TCode::INT16:
    case TCode::UINT16:
    case TCode::INT32:
    case TCode::UINT32:
    case TCode::FLOAT:
    case TCode::BOOLEAN:
      _alter_flags(true, MANUVR_ARG_FLAG_DIRECT_VALUE);
      break;
    default:
      break;
  }
  // If we can know the length with certainty, record it.
  if (typeIsFixedLength(_t_code)) {
    len = sizeOfType(_t_code);
  }
}

/*
* Protected delegate constructor.
*/
Argument::Argument(void* ptr, int l, TCode code) : Argument(code) {
  len        = l;
  target_mem = ptr;
}

Argument::Argument(double val) : Argument(malloc(sizeof(double)), sizeof(double), TCode::DOUBLE) {
  if (nullptr != target_mem) {
    *((double*) target_mem) = val;
    _alter_flags(true, MANUVR_ARG_FLAG_REAP_VALUE);
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
  if (nullptr != _key) {
    char* k = (char*) _key;
    _key = nullptr;
    if (reapKey()) free(k);
  }
  _t_code    = TCode::NONE;
  len        = 0;
  _flags     = 0;
}


/**
* Given an Argument pointer, finds that pointer and drops it from the list.
*
* @param  drop  The Argument to drop.
* @return       0 on success. 1 on warning, -1 on "not found".
*/
int8_t Argument::dropArg(Argument** root, Argument* drop) {
  if (*root == drop) {
    // Re-write the root parameter.
    root = &_next; // NOTE: may be null. Who cares.
    return 0;
  }
  else if (_next && _next == drop) {
    _next = _next->_next; // NOTE: may be null. Who cares.
    return 0;
  }
  return (_next) ? _next->dropArg(root, drop) : -1;
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
      //if (_key == k) {
      // TODO: Awful. Hash map? pointer-comparisons?
      if (0 == strcmp(_key, k)) {
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
  switch (typeCode()) {
    case TCode::INT8:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*) trg_buf) = *((uint8_t*)&target_mem);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*) trg_buf) = *((uint16_t*)&target_mem);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*) trg_buf) = *((uint32_t*)&target_mem);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*) trg_buf + 0) = *(((uint8_t*) &target_mem) + 0);
      *((uint8_t*) trg_buf + 1) = *(((uint8_t*) &target_mem) + 1);
      *((uint8_t*) trg_buf + 2) = *(((uint8_t*) &target_mem) + 2);
      *((uint8_t*) trg_buf + 3) = *(((uint8_t*) &target_mem) + 3);
      break;

    case TCode::DOUBLE:         // These are fixed-length allocated data.
    case TCode::VECT_4_FLOAT:   //
    case TCode::VECT_3_FLOAT:   //
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
      return_value = 0;
      memcpy(trg_buf, target_mem, len);
      break;

    //case TCode::UINT32_PTR:  // These are *pointers* to the indicated types. They
    //case TCode::UINT16_PTR:  //   therefore take the whole 4 bytes of memory allocated
    //case TCode::UINT8_PTR:   //   and can be returned as such.
    //case TCode::INT32_PTR:
    //case TCode::INT16_PTR:
    //case TCode::INT8_PTR:

    case TCode::STR_BUILDER:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::STR:                  // This is a pointer to a string constant. Presumably this is stored in flash.
    case TCode::IMAGE:                // This is a pointer to an Image.
    //case TCode::BUFFERPIPE:           // This is a pointer to a BufferPipe/.
    //case TCode::SYS_MANUVRMSG:        // This is a pointer to ManuvrMsg.
    //case TCode::SYS_EVENTRECEIVER:    // This is a pointer to an EventReceiver.
    //case TCode::SYS_MANUVR_XPORT:     // This is a pointer to a transport.
    default:
      return_value = 0;
      *((uintptr_t*) trg_buf) = *((uintptr_t*)&target_mem);
      break;
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
  unsigned char *temp_str = (unsigned char *) target_mem;   // Just a general use pointer.

  unsigned char *scratchpad = (unsigned char *) alloca(258);  // This is the maximum size for an argument.
  unsigned char *sp_index   = (scratchpad + 2);
  uint8_t arg_bin_len       = len;

  switch (_t_code) {
    /* These are hard types that we can send as-is. */
    case TCode::INT8:
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 1;
      *((uint8_t*) sp_index) = *((uint8_t*)& target_mem);
      break;
    case TCode::INT16:
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 2;
      *((uint16_t*) sp_index) = *((uint16_t*)& target_mem);
      break;
    case TCode::INT32:
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
    case TCode::FLOAT:   // This frightens the compiler. Its fears are unfounded.
      arg_bin_len = 4;
      *((uint32_t*) sp_index) = *((uint32_t*)& target_mem);
      break;

    /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
    //case TCode::INT8_PTR:
    //case TCode::UINT8_PTR:
    //  arg_bin_len = 1;
    //  *((uint8_t*) sp_index) = *((uint8_t*) target_mem);
    //  break;
    //case TCode::INT16_PTR:
    //case TCode::UINT16_PTR:
    //  arg_bin_len = 2;
    //  *((uint16_t*) sp_index) = *((uint16_t*) target_mem);
    //  break;
    //case TCode::INT32_PTR:
    //case TCode::UINT32_PTR:
    //case TCode::FLOAT_PTR:
    //  arg_bin_len = 4;
    //  //*((uint32_t*) sp_index) = *((uint32_t*) target_mem);
    //  for (int i = 0; i < 4; i++) {
    //    *((uint8_t*) sp_index + i) = *((uint8_t*) target_mem + i);
    //  }
    //  break;

    /* These are pointer types that require conversion. */
    case TCode::STR_BUILDER:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::URL:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
      temp_str    = ((StringBuilder*) target_mem)->string();
      arg_bin_len = ((StringBuilder*) target_mem)->length();
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_FLOAT:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_UINT16: // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::VECT_3_INT16:  // NOTE!!! This only works for Vectors because of the template layout. FRAGILE!!!
    case TCode::BINARY:
      for (int i = 0; i < arg_bin_len; i++) {
        *(sp_index + i) = *(temp_str + i);
      }
      break;

    /* Anything else should be dropped. */
    default:
      break;
  }

  *(scratchpad + 0) = (uint8_t) _t_code;
  *(scratchpad + 1) = arg_bin_len;
  out->concat(scratchpad, (arg_bin_len+2));
  if (_next) {
    _next->serialize(out);
  }
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
  if (out == nullptr) return -1;

  switch (_t_code) {
    /* These are hard types that we can send as-is. */
    case TCode::INT8:
    case TCode::UINT8:
      out->concat((unsigned char*) &target_mem, 1);
      break;
    case TCode::INT16:
    case TCode::UINT16:
      out->concat((unsigned char*) &target_mem, 2);
      break;
    case TCode::INT32:
    case TCode::UINT32:
    case TCode::FLOAT:
      out->concat((unsigned char*) &target_mem, 4);
      break;

    /* These are pointer types to data that can be sent as-is. Remember: LITTLE ENDIAN */
    //case TCode::INT8_PTR:
    //case TCode::UINT8_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 1);
    //  break;
    //case TCode::INT16_PTR:
    //case TCode::UINT16_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 2);
    //  break;
    //case TCode::INT32_PTR:
    //case TCode::UINT32_PTR:
    //case TCode::FLOAT_PTR:
    //  out->concat((unsigned char*) *((unsigned char**)target_mem), 4);
    //  break;

    /* These are pointer types that require conversion. */
    case TCode::STR_BUILDER:     // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::URL:             // This is a pointer to some StringBuilder. Presumably this is on the heap.
      out->concat((StringBuilder*) target_mem);
      break;
    case TCode::STR:
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
    case TCode::BINARY:     // This is a pointer to a big binary blob.
      out->concat((unsigned char*) target_mem, len);
      break;

    case TCode::IMAGE:      // This is a pointer to an Image.
      #if defined(CONFIG_MANUVR_IMG_SUPPORT)
        {
          Image* img = (Image*) target_mem;
          uint32_t sz_buf = img->bytesUsed();
          if (sz_buf > 0) {
            if (0 != img->serialize(out)) {
              // Failure
            }
          }
        }
      #endif   // CONFIG_MANUVR_IMG_SUPPORT
      break;

    /* Anything else should be dropped. */
    default:
      break;
  }

  if (_next) {
    _next->serialize_raw(out);
  }
  return 0;
}


/**
* @return A pointer to the linked argument.
*/
Argument* Argument::link(Argument* arg) {
  if (nullptr == _next) {
    _next = arg;
    return arg;
  }
  return _next->link(arg);
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
Argument* Argument::retrieveArgByIdx(unsigned int idx) {
  switch (idx) {
    case 0:
      return this;   // Special case.
    default:
      if (nullptr != _next) {
        return _next->retrieveArgByIdx(idx-1);
      }
      // NOTE: No break
      // Fall-through if the index is greater than the list's cardinality.
    case 1:
      return _next;  // Terminus of recursion for all args but 0.
  }
}


/*
* Does an Argument in our rank have the given key?
*
* Returns nullptr if the answer is 'no'. Otherwise, ptr to the first matching key.
*/
Argument* Argument::retrieveArgByKey(const char* k) {
  if (nullptr != k) {
    if (nullptr != _key) {
      //if (_key == k) {
      // TODO: Awful. Hash map? pointer-comparisons?
      if (0 == strcmp(_key, k)) {
        // If pointer comparison worked, we win.
        return this;
      }
    }

    if (nullptr != _next) {
      return _next->retrieveArgByKey(k);
    }
  }
  return nullptr;
}


void Argument::valToString(StringBuilder* out) {
  uint8_t* buf = (uint8_t*) pointer();
  switch (_t_code) {
    case TCode::INT8:
    case TCode::INT16:
    case TCode::INT32:
    case TCode::INT64:
    case TCode::INT128:
      out->concatf("%d", (uintptr_t) pointer());
      break;
    case TCode::UINT8:
    case TCode::UINT16:
    case TCode::UINT32:
    case TCode::UINT64:
    case TCode::UINT128:
      out->concatf("%u", (uintptr_t) pointer());
      break;
    case TCode::FLOAT:
      {
        float tmp;
        memcpy((void*) &tmp, &buf, 4);
        out->concatf("%.4f", (double) tmp);
      }
      break;
    case TCode::DOUBLE:
      {
        double tmp;
        getValueAs((void*) &tmp);
        out->concatf("%.6f", tmp);
      }
      break;

    case TCode::BOOLEAN:
      out->concatf("%s", ((uintptr_t) pointer() ? "true" : "false"));
      break;
    case TCode::VECT_3_FLOAT:
      {
        Vector3<float>* v = (Vector3<float>*) pointer();
        out->concatf("(%.4f, %.4f, %.4f)", (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    case TCode::VECT_4_FLOAT:
      {
        Vector4f* v = (Vector4f*) pointer();
        out->concatf("(%.4f, %.4f, %.4f, %.4f)", (double)(v->w), (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    default:
      {
        int l_ender = (len < 16) ? len : 16;
        for (int n = 0; n < l_ender; n++) {
          out->concatf("%02x ", *((uint8_t*) buf + n));
        }
      }
      break;
  }
}


/*
* Warning: call is propagated across entire list.
*/
void Argument::printDebug(StringBuilder* out) {
  out->concatf("\t%s\t%s\t%s",
    (nullptr == _key ? "" : _key),
    getTypeCodeString((TCode) typeCode()),
    (reapValue() ? "(reap)" : "\t")
  );
  valToString(out);
  out->concat("\n");

  if (nullptr != _next) _next->printDebug(out);
}


int8_t Argument::setValue(void* trg_buf, int len, TCode tc) {
  int8_t return_value = -1;
  if (typeCode() != tc) {
    return -2;
  }
  switch (tc) {
    case TCode::INT8:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT8:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint8_t*)&target_mem) = *((uint8_t*) trg_buf);
      break;
    case TCode::INT16:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT16:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint16_t*)&target_mem) = *((uint16_t*) trg_buf);
      break;
    case TCode::INT32:    // This frightens the compiler. Its fears are unfounded.
    case TCode::UINT32:   // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *((uint32_t*)&target_mem) = *((uint32_t*) trg_buf);
      break;
    case TCode::FLOAT:    // This frightens the compiler. Its fears are unfounded.
      return_value = 0;
      *(((uint8_t*) &target_mem) + 0) = *((uint8_t*) trg_buf + 0);
      *(((uint8_t*) &target_mem) + 1) = *((uint8_t*) trg_buf + 1);
      *(((uint8_t*) &target_mem) + 2) = *((uint8_t*) trg_buf + 2);
      *(((uint8_t*) &target_mem) + 3) = *((uint8_t*) trg_buf + 3);
      break;
    //case TCode::UINT32_PTR:  // These are *pointers* to the indicated types. They
    //case TCode::UINT16_PTR:  //   therefore take the whole 4 bytes of memory allocated
    //case TCode::UINT8_PTR:   //   and can be returned as such.
    //case TCode::INT32_PTR:
    //case TCode::INT16_PTR:
    //case TCode::INT8_PTR:
    //case TCode::FLOAT_PTR:
    case TCode::DOUBLE:
    case TCode::VECT_4_FLOAT:
    case TCode::VECT_3_FLOAT:
    case TCode::VECT_3_UINT16:
    case TCode::VECT_3_INT16:
      return_value = 0;
      for (int i = 0; i < len; i++) {
        *((uint8_t*) target_mem + i) = *((uint8_t*) trg_buf + i);
      }
      break;

    case TCode::STR_BUILDER:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
    case TCode::STR:                  // This is a pointer to a string constant. Presumably this is stored in flash.
    case TCode::IMAGE:                // This is a pointer to an Image.
    //case TCode::BUFFERPIPE:           // This is a pointer to a BufferPipe/.
    //case TCode::SYS_MANUVRMSG:        // This is a pointer to ManuvrMsg.
    //case TCode::SYS_EVENTRECEIVER:    // This is a pointer to an EventReceiver.
    //case TCode::SYS_MANUVR_XPORT:     // This is a pointer to a transport.
    default:
      return_value = 0;
      target_mem = trg_buf;  // TODO: Need to do an allocation check and possible cleanup.
      break;
  }
  return return_value;
}
