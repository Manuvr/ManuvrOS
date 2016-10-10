/**
* Here, we provide a configuration for Jansson appropriate for
*   Manuvr. I would rather this be done by the project with build-time
*   options.
*/

#include <stdint.h>
#include <sys/types.h>

#ifndef __PRIVATE_JANSSON_CONFIG_H__
#define __PRIVATE_JANSSON_CONFIG_H__

#define JSON_PARSER_MAX_DEPTH   2048
#define JSON_INLINE           inline
#define JSON_HAVE_LOCALECONV       0
#define JSON_INTEGER_IS_LONG_LONG  0
#define HAVE_UNISTD_H              1
#define HAVE_SYS_TYPES_H           1
#define HAVE_INTTYPES_H            1
#define HAVE_STDINT_H              1

#endif   //__PRIVATE_JANSSON_CONFIG_H__
