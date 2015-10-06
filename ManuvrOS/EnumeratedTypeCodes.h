#ifndef __ENUMERATED_TYPE_CODES_H__
#define __ENUMERATED_TYPE_CODES_H__

#include <inttypes.h>
#include <stddef.h>


/**
* This is the structure with which we define types. These types are used by a variety of
*   systems in this program, and need to be consistent. Sometimes, we might be talking to
*   another system that lacks support for some of these types.
* 
* This structrue conveys the type, it's size, and any special attributes of the type. 
*/
typedef struct typecode_def_t {
    uint8_t            type_code;   // This field identifies the type.
    uint8_t            type_flags;  // Flags that give us metadata about a type.
    uint16_t           fixed_len;   // If this type has a fixed length, it will be set here. 0 if no fixed length.
} TypeCodeDef;

/**
* These are the different flags that might be applied to a type. These should inform
*   parsing and packing for other systems, as well as inform client classes about nuances
*   of a specific type.
*/
#define TYPE_CODE_FLAG_EXPORTABLE          0b00000001   // This type is exportable to other systems, and we might receive it.
#define TYPE_CODE_FLAG_IS_POINTER          0b00000010   // Is this type a pointer type?
#define TYPE_CODE_FLAG_VARIABLE_LENGTH     0b00000100   // Some types do not have a fixed-length.
#define TYPE_CODE_FLAG_IS_NULL_DELIMITED   0b00001000   // Various string types are variable-length, yet self-delimiting.
#define TYPE_CODE_FLAG_RESERVED_3          0b00010000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_2          0b00100000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_1          0b01000000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_0          0b10000000   // Reserved for future use.




/**
* Special cases.
*/
#define RSRVD_FM       0xFF    // This type is reserved because it might be used for other things by other libraries.
#define NOTYPE_FM      0x00    // This is usually an indication of a failure to init.

/**
* Pointer types. These do not require a malloc() but they will not make sense to any
*   software running outside of our memory-scope. The values, therefore, will require
*   translation into one of the hard types before being sent.
*/
#define UINT32_PTR_FM     0xA0 // A pointer to a StringBuilder.
#define UINT16_PTR_FM     0xA1 // Unsigned 16-bit integer
#define UINT8_PTR_FM      0xA2 // Unsigned 8-bit integer
#define INT32_PTR_FM      0xA3 // 32-bit integer
#define INT16_PTR_FM      0xA4 // 16-bit integer
#define INT8_PTR_FM       0xA5 // 8-bit integer
#define FLOAT_PTR_FM      0xA6 // A float
#define CHAIN PTR_FM      0xAD // A pointer to a Chain.
#define STR_BUILDER_FM    0xAF // A pointer to a StringBuilder.

/**
* Type codes for pointers to system services and other types that will make no
*   sense whatever to a foriegn system. These are the non-exportable types that are
*   used for internal message interchange.
*/

#define SYS_EVENTRECEIVER_FM    0xE0 // A pointer to an EventReceiver.
#define SYS_MANUVR_XPORT_FM     0xE1 // A pointer to a transport.
#define SYS_MANUVR_EVENT_PTR_FM 0xE2 // A pointer to an Event.


/**
* These types are big enough to warrant a malloc() to pass them around internally, but
*   the data so passed is not itself a pointer, and therefore makes sense as-is to other 
*   devices.
*/
#define RELAYED_MSG_FM 0x19    // A serialized message that is being stored or relayed.
#define CHAIN_FM       0x18    // An event chain.
#define URL_FM         0x17    // An alias of string that carries the semantic 'URL'.
#define VECT_4_FLOAT   0x16    // A float vector in 4-space.
#define JSON_FM        0x15    // A JSON object. Export to other systems implies string conversion.
#define VECT_3_UINT16  0x14    // A vector of unsigned 16-bit integers in 3-space
#define VECT_3_INT16   0x13    // A vector of 16-bit integers in 3-space
#define VECT_3_FLOAT   0x12    // A vector of floats in 3-space
#define IMAGE_FM       0x11    // Image data
#define AUDIO_FM       0x10    // Audio stream
#define BINARY_FM      0x0F    // A collection of bytes
#define STR_FM         0x0E    // A null-terminated string
#define DOUBLE_FM      0x0D    // A double
#define INT64_FM       0x04    // 64-bit integer
#define INT128_FM      0x05    // 128-bit integer
#define UINT64_FM      0x09    // Unsigned 64-bit integer
#define UINT128_FM     0x0A    // Unsigned 128-bit integer


/**
* These are small enough to be cast into a pointer's space. They are therefore
*   "pass-by-value" for classes that interchange them. 
* Cheap because: no malloc()/free() cycle.
*/
#define BOOLEAN_FM     0x0B   // A boolean
#define FLOAT_FM       0x0C   // A float
#define INT8_FM        0x01   // 8-bit integer
#define INT16_FM       0x02   // 16-bit integer
#define INT32_FM       0x03   // 32-bit integer
#define UINT8_FM       0x06   // Unsigned 8-bit integer
#define UINT16_FM      0x07   // Unsigned 16-bit integer
#define UINT32_FM      0x08   // Unsigned 32-bit integer


/**
* The host sends binary data little-endian. This will convert bytes to the indicated type.
* It is the responsibility of the caller to check that these bytes actually exist in the buffer.
*/
inline double   parseDoubleFromchars(unsigned char *input) {  return ((double)   *((double*)   input)); }
inline float    parseFloatFromchars(unsigned char *input) {   return ((float)    *((float*)    input)); }
inline uint32_t parseUint32Fromchars(unsigned char *input) {  return ((uint32_t) *((uint32_t*) input)); }
inline uint16_t parseUint16Fromchars(unsigned char *input) {  return ((uint16_t) *((uint16_t*) input)); }


int sizeOfArg(uint8_t typecode);
int getTypemapSizeAndPointer(const unsigned char **pointer);
int getTotalSizeByTypeString(char *str);

const char* getTypeCodeString(uint8_t typecode);

#endif
