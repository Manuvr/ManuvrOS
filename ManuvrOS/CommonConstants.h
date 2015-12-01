/*
File:   CommonConstants.h
Author: J. Ian Lindsay
Date:   2015.12.01

There ought to be no inclusion in this file. It is the bottom.
*/

#ifndef __MANUVR_COMMON_CONSTANTS_H__
  #define __MANUVR_COMMON_CONSTANTS_H__

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */

  // Function-pointer definitions
  typedef void (*FunctionPointer) ();


#endif
