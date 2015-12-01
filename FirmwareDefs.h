/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.03.01

This is one of the files that the application author is required to provide. This is where definition of
  (application or device)-specific event codes ought to go. We also define some fields that will be used
  during communication with other devices, so some things here are mandatory.

This is an example file for building mock firmware on a Raspberry Pi.

*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H


/*
* These flags are meant to be sent during session setup. We need to 
*/
#define DEVICE_FLAG_AUTHORITATIVE_TIME          0x00000001  // Devices that can provide accurate time.
#define DEVICE_FLAG_INTERNET_ACCESS             0x00000002  // Can this firmware potentially provide net access?
#define DEVICE_FLAG_MESSAGE_RELAY               0x00000004  // Can we act as a message relay?


/*
* These are required fields.
*
* PROTOCOL_MTU is required for constraining communication length due to memory restrictions at
*   one-or-both sides. Since the protocol currently supports up to (2^24)-1 bytes in a single transaction,
*   a microcontroller would want to limit its counter-party's use of precious RAM. PROTOCOL_MTU, therefore,
*   determines the effective maximum packet size for this device, and by extension, the sessions in which
*   it participates.
*/
#define PROTOCOL_MTU              16777215      // See MTU notes above....
#define VERSION_STRING            "0.0.1"       // We should be able to communicate version so broken behavior can be isolated.
#define HW_VERSION_STRING         "1"           // We are strictly-software, but we will report as hardware.
#define IDENTITY_STRING           "MHBDebug"    // This will select Manuvr's debug engine in MHB.
#define PROTOCOL_VERSION          0x00000001    // The protocol version we are using.


#define EXTENDED_DETAIL_STRING    "RasPiBuild"  // Optional. User-defined.



/* Codes that are specific to the Raspi build. */




#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
int main(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
