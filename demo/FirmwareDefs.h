/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.03.01

This is one of the files that the application author is required to provide. This is where definition of
  (application or device)-specific event codes ought to go. We also define some fields that will be used
  during communication with other devices, so some things here are mandatory.

*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H

/*
* Macros we will use in scattered places...
*/
#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
    #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

/*
* These are required fields.
*
* PROTOCOL_MTU is required for constraining communication length due to memory restrictions at
*   one-or-both sides. Since the protocol currently supports up to (2^24)-1 bytes in a single transaction,
*   a microcontroller would want to limit it's counterparty's use of precious RAM. PROTOCOL_MTU, therefore,
*   determines the effective maximum packet size for this device.
*/
#define PROTOCOL_MTU              20000                  // See MTU notes above....
#define VERSION_STRING            "0.0.0"                // We should be able to communicate version so broken behavior can be isolated.
#define HW_VERSION_STRING         "-1"                   // Because we are strictly-software, we report as such.
#define IDENTITY_STRING           "ManuvrOS-Testbench"   // Might also be a hash....
#define EXTENDED_DETAIL_STRING    ""                     // Optional. User-defined.



/* Codes that are specific to the testbench */




#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
#ifndef TEST_BENCH
  int main(void);
  volatile void jumpToBootloader(void);
  volatile void reboot(void);
#endif

unsigned long millis(void);
unsigned long micros(void);


#ifdef __cplusplus
}
#endif

#endif
