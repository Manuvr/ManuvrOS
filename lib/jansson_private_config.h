/**
* Here, we provide a configuration for Jansson appropriate for
*   Manuvr. I would rather this be done by the project with build-time
*   options.
*/

#include <inttypes.h>


#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H

#define JSON_INLINE inline
#define JSON_HAVE_LOCALECONV 0

#define JSON_INTEGER_IS_LONG_LONG 0
#define HAVE_UNISTD_H
#define HAVE_SYS_TYPES_H
#define HAVE_CONFIG_H

#endif
#define JSON_PARSER_MAX_DEPTH 2048
