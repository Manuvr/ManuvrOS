/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.03.01


This is one of the files that the application author is required to provide. 
This is where definition of (application or device)-specific parameters ought to go.

This is an example file for building firmware on linux. Anticipated target is a Raspi.
  but that shouldn't matter too much. These settings are only typically relevant for
  reasons of memory-constraint, threading model (if any), or specific features that
  this hardware can support.
*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H

/****************************************************************************************************
* Required fields...                                                                                *
****************************************************************************************************/

/*
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


/****************************************************************************************************
* Optional fields...                                                                                *
****************************************************************************************************/

#define EXTENDED_DETAIL_STRING    "RasPiBuild"  // Optional. User-defined.


/*
* Kernel options.
*/
#define EVENT_MANAGER_PREALLOC_COUNT       32   // How large a preallocation buffer should we keep?

#endif
