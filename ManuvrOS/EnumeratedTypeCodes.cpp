#include "EnumeratedTypeCodes.h"

/**
* Static Initializer for our type map.
*/
const TypeCodeDef type_codes[] = {
  {RSRVD_FM         , (TYPE_CODE_FLAG_RESERVED_TYPE | TYPE_CODE_FLAG_FITS_IN_POINTER), 0},
  {NOTYPE_FM        , (TYPE_CODE_FLAG_RESERVED_TYPE | TYPE_CODE_FLAG_FITS_IN_POINTER), 0},

  {INT8_FM          , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 1},
  {UINT8_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 1},
  {INT16_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 2},
  {UINT16_FM        , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 2},
  {INT32_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 4},
  {UINT32_FM        , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 4},
  {FLOAT_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 4},
  {BOOLEAN_FM       , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_FITS_IN_POINTER), 1},

  {UINT128_FM       , (TYPE_CODE_FLAG_EXPORTABLE), 16},
  {INT128_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 16},
  {UINT64_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 8},
  {INT64_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 8},
  {DOUBLE_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 8},

  {STR_FM           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {BINARY_FM        , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {AUDIO_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {IMAGE_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},

  {VECT_3_UINT16    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 6},
  {VECT_3_INT16     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 6},
  {VECT_3_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 12},
  {VECT_4_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 16},
  {MAP_FM           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 1},

  {UINT32_PTR_FM    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 4},
  {UINT16_PTR_FM    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 2},
  {UINT8_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 1},
  {INT32_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 4},
  {INT16_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 2},
  {INT8_PTR_FM      , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 1},
  {FLOAT_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 4},
  {STR_BUILDER_FM   , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_EXPORT_IMPLIES_CONV), 1},
  {SYS_SVC_MANIFEST      , 0, 0},
  {SYS_SVC_STATICHUB_FM  , 0, 0},
  {SYS_SVC_SCHEDULER_FM  , 0, 0},
  {EVENT_PTR_FM          , 0, 0},
};



/**
* Given type code, find size in bytes. Returns 1 for variable-length arguments, 
*   since this is their minimum size.
*
* @param  uint8_t the type code being asked about.
* @return The size of the type, or -1 on failure.
*/
int sizeOfArg(uint8_t typecode) {
  int return_value = -1;
  uint8_t idx = 0;
  while (idx < (sizeof(type_codes) / sizeof(TypeCodeDef))) {  // Search for the code...
    if (type_codes[idx].type_code == typecode) {
      return type_codes[idx].fixed_len;
    }
    idx++;
  }
  return return_value;
}


/**
* We cast the type-map into an unsigned char buffer because there is no need to
*   parse it, but it DOES need to be sent to another machine occasionally.
*
* @param  const unsigned char** A receptical for the type legend. 
* @return The size of the type legend, in bytes.
*/
int getTypemapSizeAndPointer(const unsigned char **pointer) {
  *(pointer) = (const unsigned char*) type_codes;
  return sizeof(type_codes);
}


/**
* Given a string of type codes, return the size required to carry it.
* Note: Variable length types will be assumed to have a length of 1.
*
* @param  char* The form of the message. NULL-terminated. 
* @return The minimum size required to carry the expressed form.
*/
int getTotalSizeByTypeString(char *str) {
  int return_value = 0;
  if (str != NULL) {
    while (*(str)) {
      return_value += sizeOfArg(*(str++));
    }
  }
  return return_value;
}



/**
* Given a type code, return the string representation for debug.
*
* @param  uint8_t the type code being asked about. 
* @return The string representation. Never NULL.
*/
const char* getTypeCodeString(uint8_t typecode) {
  switch (typecode) {
    // TODO: Fill this out.
    case NOTYPE_FM:             return "NOTYPE_FM";
    case RSRVD_FM:              return "RSRVD_FM";
    case VECT_4_FLOAT:          return "VECT_4_FLOAT";
    case VECT_3_FLOAT:          return "VECT_3_FLOAT";
    case VECT_3_INT16:          return "VECT_3_INT16";
    case VECT_3_UINT16:         return "VECT_3_UINT16";

    case INT8_FM:               return "INT8";
    case UINT8_FM:              return "UINT8";
    case INT16_FM:              return "INT16";
    case UINT16_FM:             return "UINT16";
    case INT32_FM:              return "INT32";
    case UINT32_FM:             return "UINT32";
    case FLOAT_FM:              return "FLOAT";
    case BOOLEAN_FM:            return "BOOLEAN";
    case STR_FM:                return "STR";
    case BINARY_FM:             return "BINARY";
    case STR_BUILDER_FM:        return "STR_BUILDER";
    case MAP_FM:                return "MAP";
    default:                    return "<NOT YET CODED FOR>";
  }
}


