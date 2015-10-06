#include "ManuvrMsg.h"
#include <string.h>


/****************************************************************************************************
* Argument class constructors.                                                                      *
****************************************************************************************************/
// TODO: Need to devolve into this constructor. Forgot how.
Argument::Argument(){
	wipe();
}

Argument::Argument(uint8_t val) {
	wipe();
	len = sizeof(val);
	type_code = UINT8_FM;
	target_mem = (void*) (0x00000000 + val);
}

Argument::Argument(uint16_t val) {
	wipe();
	len = sizeof(val);
	type_code = UINT16_FM;
	target_mem = (void*) (0x00000000 + val);
}

Argument::Argument(uint32_t val) {
	wipe();
	len = sizeof(val);
	type_code = UINT32_FM;
	target_mem = (void*) (0x00000000 + val);
}


Argument::Argument(int8_t val) {
	wipe();
	len = sizeof(val);
	type_code = INT8_FM;
	target_mem = (void*) (0x00000000 + val);
}

Argument::Argument(int16_t val) {
	wipe();
	len = sizeof(val);
	type_code = INT16_FM;
	target_mem = (void*) (0x00000000 + val);
}

Argument::Argument(int32_t val) {
	wipe();
	len = sizeof(val);
	type_code = INT32_FM;
	target_mem = (void*) (0x00000000 + val);
}

Argument::Argument(float val) {
	wipe();
	len = sizeof(val);
	type_code = FLOAT_FM;
	target_mem = (void*) ((uint32_t) val);
}


Argument::Argument(uint8_t* val) {
	wipe();
	len = sizeof(val);
	type_code = UINT8_PTR_FM;
	target_mem = (void*) val;
}

Argument::Argument(uint16_t* val) {
	wipe();
	len = sizeof(val);
	type_code = UINT16_PTR_FM;
	target_mem = (void*) val;
}

Argument::Argument(uint32_t* val) {
	wipe();
	len = sizeof(val);
	type_code = UINT32_PTR_FM;
	target_mem = (void*) val;
}


Argument::Argument(int8_t* val) {
	wipe();
	len = sizeof(val);
	type_code = INT8_PTR_FM;
	target_mem = (void*) val;
}

Argument::Argument(int16_t* val) {
	wipe();
	len = sizeof(val);
	type_code = INT16_PTR_FM;
	target_mem = (void*) val;
}

Argument::Argument(int32_t* val) {
	wipe();
	len = sizeof(val);
	type_code = INT32_PTR_FM;
	target_mem = (void*) val;
}

Argument::Argument(float* val) {
	wipe();
	len = sizeof(val);
	type_code = FLOAT_PTR_FM;
	target_mem = (void*) val;
	reap       = false;
}

Argument::Argument(Vector4f* val) {
	wipe();
	len = 16;
	type_code = VECT_4_FLOAT;
	target_mem = (void*) val;
}

Argument::Argument(Vector3f* val) {
	wipe();
	len = 12;
	type_code = VECT_3_FLOAT;
	target_mem = (void*) val;
}

Argument::Argument(Vector3i16* val) {
	wipe();
	len = 8;
	type_code = VECT_3_INT16;
	target_mem = (void*) val;
}

Argument::Argument(Vector3ui16* val) {
	wipe();
	len = 6;
	type_code = VECT_3_UINT16;
	target_mem = (void*) val;
}

/*
* This is a constant character pointer. Do not reap it.
*/
Argument::Argument(const char* val) {
	wipe();
	len = strlen(val)+1;  // +1 because: NULL terminator.
	type_code = STR_FM;
	target_mem = (void*) val;
	reap       = false;
}



/*
* This constructor produces reapable Arguments.
* We typically want StringBuilder references to be reaped at the end of
*   the Argument's life cycle. We will specify otherwise when appropriate. 
*/
Argument::Argument(StringBuilder* val) {
	wipe();
	len = sizeof(val);
	type_code = STR_BUILDER_FM;
	target_mem = (void*) val;
	reap       = true;
}

/*
* We typically want references to typeless swaths of memory be left alone at the end of
*   the Argument's life cycle. We will specify otherwise when appropriate. 
*/
Argument::Argument(void* val, uint16_t buf_len) {
	wipe();
	len = buf_len;
	type_code  = BINARY_FM;
	target_mem = (void*) val;
	reap       = false;
}

/*
* We typically want ManuvrEvent references to be left alone at the end of
*   the Argument's life cycle. We will specify otherwise when appropriate. 
*/
Argument::Argument(ManuvrEvent* val) {
	wipe();
	len = sizeof(val);
	type_code = SYS_MANUVR_EVENT_PTR_FM;
	target_mem = (void*) val;
	reap       = false;
}

/*
* This is a system service pointer. Do not reap it.
*/
Argument::Argument(ManuvrXport* val) {
	wipe();
	len = sizeof(val);
	type_code = SYS_MANUVR_XPORT_FM;
	target_mem = (void*) val;
	reap       = false;
}

/*
* This is a system service pointer. Do not reap it.
*/
Argument::Argument(EventReceiver* val) {
	wipe();
	len = sizeof(val);
	type_code = SYS_EVENTRECEIVER_FM;
	target_mem = (void*) val;
	reap       = false;
}




Argument::~Argument() {
	if (reap && (NULL == target_mem)) {
		free(target_mem);
		target_mem = NULL;
	}
}


void Argument::wipe() {
	type_code = NOTYPE_FM;
	len = 0;
	reap = false;
	target_mem = NULL;
}


/*
* The prupose of this fxn is to pack up this Argument into something that can be stored or sent
*   over a wire.
* This is the point at which we will have to translate any pointer types into something concrete.
* 
* Returns DIG_MSG_ERROR_NO_ERROR (0) on success.
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
	    *(sp_index) = (uint8_t) (((uint32_t) target_mem) & 0x000000FF);
	    break;
	  case INT16_FM:
	  case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
	    arg_bin_len = 2;
	    *(sp_index) = (uint16_t) (((uint32_t) target_mem) & 0x0000FFFF);
	    break;
	  case INT32_FM:
	  case UINT32_FM:   // This frightens the compiler. Its fears are unfounded.
	  case FLOAT_FM:   // This frightens the compiler. Its fears are unfounded.
	    arg_bin_len = 4;
	    *(sp_index) = (uint32_t) target_mem;
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
	  case SYS_EVENTRECEIVER_FM:  // Host can't use our internal system services.
	  case SYS_MANUVR_XPORT_FM:   // Host can't use our internal system services.
	  default:
	    return DIG_MSG_ERROR_INVALID_TYPE;
	}
	
	*(scratchpad + 0) = type_code;
	*(scratchpad + 1) = arg_bin_len;
	out->concat(scratchpad, (arg_bin_len+2));
	return DIG_MSG_ERROR_NO_ERROR;
}


/*
* The purpose of this fxn is to pack up this Argument into something that can sent over a wire
*   with a minimum of overhead. We write only the bytes that *are* the data, and not the metadata
*   because we are relying on the parser at the other side to know what the type is.
* We still have to translate any pointer types into something concrete.
* 
* Returns DIG_MSG_ERROR_NO_ERROR (0) on success. Also updates the length of data in the offset argument.
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
	  case SYS_EVENTRECEIVER_FM:  // Host can't use our internal system services.
	  case SYS_MANUVR_XPORT_FM:   // Host can't use our internal system services.
	  default:
	    return DIG_MSG_ERROR_INVALID_TYPE;
	}
	
	return DIG_MSG_ERROR_NO_ERROR;
}



