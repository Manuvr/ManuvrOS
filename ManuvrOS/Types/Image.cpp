/*
File:   Image.cpp
Author: J. Ian Lindsay
Date:   2019.06.02

Certain drawing features of this class were lifted from Adafruit's GFX library.
  Rather than call out which specific functions, or isolate translation units
  for legal reasons, this class simply inherrits their license and attribution
  unless I rework it. License reproduced in the comment block below this one.
I've converted the entire API to 32-bit coordinates and extended the color
  capabilities.
       ---J. Ian Lindsay 2019.06.14

                  Top
   ________________________________
   | 0, 1, 2...   x -->
   | 1
L  | 2
e  |
f  | y
t  | |
   | V
   |

*/

/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdlib.h>
#include "Image.h"
#include "../Utilities.h"

#if defined(CONFIG_MANUVR_IMG_SUPPORT)

/* If character support is enabled, we'll import fonts. */
#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeMono18pt7b.h"
#include "Fonts/FreeMono24pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMonoBold18pt7b.h"
#include "Fonts/FreeMonoBold24pt7b.h"
#include "Fonts/FreeMonoBold9pt7b.h"
#include "Fonts/FreeMonoBoldOblique12pt7b.h"
#include "Fonts/FreeMonoBoldOblique18pt7b.h"
#include "Fonts/FreeMonoBoldOblique24pt7b.h"
#include "Fonts/FreeMonoBoldOblique9pt7b.h"
#include "Fonts/FreeMonoOblique12pt7b.h"
#include "Fonts/FreeMonoOblique18pt7b.h"
#include "Fonts/FreeMonoOblique24pt7b.h"
#include "Fonts/FreeMonoOblique9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSansBold24pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeSansBoldOblique12pt7b.h"
#include "Fonts/FreeSansBoldOblique18pt7b.h"
#include "Fonts/FreeSansBoldOblique24pt7b.h"
#include "Fonts/FreeSansBoldOblique9pt7b.h"
#include "Fonts/FreeSansOblique12pt7b.h"
#include "Fonts/FreeSansOblique18pt7b.h"
#include "Fonts/FreeSansOblique24pt7b.h"
#include "Fonts/FreeSansOblique9pt7b.h"
#include "Fonts/FreeSerif12pt7b.h"
#include "Fonts/FreeSerif18pt7b.h"
#include "Fonts/FreeSerif24pt7b.h"
#include "Fonts/FreeSerif9pt7b.h"
#include "Fonts/FreeSerifBold12pt7b.h"
#include "Fonts/FreeSerifBold18pt7b.h"
#include "Fonts/FreeSerifBold24pt7b.h"
#include "Fonts/FreeSerifBold9pt7b.h"
#include "Fonts/FreeSerifBoldItalic12pt7b.h"
#include "Fonts/FreeSerifBoldItalic18pt7b.h"
#include "Fonts/FreeSerifBoldItalic24pt7b.h"
#include "Fonts/FreeSerifBoldItalic9pt7b.h"
#include "Fonts/FreeSerifItalic12pt7b.h"
#include "Fonts/FreeSerifItalic18pt7b.h"
#include "Fonts/FreeSerifItalic24pt7b.h"
#include "Fonts/FreeSerifItalic9pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
#include "Fonts/Tiny3x3a2pt7b.h"
#include "Fonts/TomThumb.h"
/* End of font import. */

// Adafruit GFX's Standard ASCII 5x7 font
static const unsigned char font[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
  0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
  0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
  0x18, 0x3C, 0x7E, 0x3C, 0x18,
  0x1C, 0x57, 0x7D, 0x57, 0x1C,
  0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
  0x00, 0x18, 0x3C, 0x18, 0x00,
  0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
  0x00, 0x18, 0x24, 0x18, 0x00,
  0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
  0x30, 0x48, 0x3A, 0x06, 0x0E,
  0x26, 0x29, 0x79, 0x29, 0x26,
  0x40, 0x7F, 0x05, 0x05, 0x07,
  0x40, 0x7F, 0x05, 0x25, 0x3F,
  0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
  0x7F, 0x3E, 0x1C, 0x1C, 0x08,
  0x08, 0x1C, 0x1C, 0x3E, 0x7F,
  0x14, 0x22, 0x7F, 0x22, 0x14,
  0x5F, 0x5F, 0x00, 0x5F, 0x5F,
  0x06, 0x09, 0x7F, 0x01, 0x7F,
  0x00, 0x66, 0x89, 0x95, 0x6A,
  0x60, 0x60, 0x60, 0x60, 0x60,
  0x94, 0xA2, 0xFF, 0xA2, 0x94,
  0x08, 0x04, 0x7E, 0x04, 0x08,
  0x10, 0x20, 0x7E, 0x20, 0x10,
  0x08, 0x08, 0x2A, 0x1C, 0x08,
  0x08, 0x1C, 0x2A, 0x08, 0x08,
  0x1E, 0x10, 0x10, 0x10, 0x10,
  0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
  0x30, 0x38, 0x3E, 0x38, 0x30,
  0x06, 0x0E, 0x3E, 0x0E, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x5F, 0x00, 0x00,
  0x00, 0x07, 0x00, 0x07, 0x00,
  0x14, 0x7F, 0x14, 0x7F, 0x14,
  0x24, 0x2A, 0x7F, 0x2A, 0x12,
  0x23, 0x13, 0x08, 0x64, 0x62,
  0x36, 0x49, 0x56, 0x20, 0x50,
  0x00, 0x08, 0x07, 0x03, 0x00,
  0x00, 0x1C, 0x22, 0x41, 0x00,
  0x00, 0x41, 0x22, 0x1C, 0x00,
  0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
  0x08, 0x08, 0x3E, 0x08, 0x08,
  0x00, 0x80, 0x70, 0x30, 0x00,
  0x08, 0x08, 0x08, 0x08, 0x08,
  0x00, 0x00, 0x60, 0x60, 0x00,
  0x20, 0x10, 0x08, 0x04, 0x02,
  0x3E, 0x51, 0x49, 0x45, 0x3E,
  0x00, 0x42, 0x7F, 0x40, 0x00,
  0x72, 0x49, 0x49, 0x49, 0x46,
  0x21, 0x41, 0x49, 0x4D, 0x33,
  0x18, 0x14, 0x12, 0x7F, 0x10,
  0x27, 0x45, 0x45, 0x45, 0x39,
  0x3C, 0x4A, 0x49, 0x49, 0x31,
  0x41, 0x21, 0x11, 0x09, 0x07,
  0x36, 0x49, 0x49, 0x49, 0x36,
  0x46, 0x49, 0x49, 0x29, 0x1E,
  0x00, 0x00, 0x14, 0x00, 0x00,
  0x00, 0x40, 0x34, 0x00, 0x00,
  0x00, 0x08, 0x14, 0x22, 0x41,
  0x14, 0x14, 0x14, 0x14, 0x14,
  0x00, 0x41, 0x22, 0x14, 0x08,
  0x02, 0x01, 0x59, 0x09, 0x06,
  0x3E, 0x41, 0x5D, 0x59, 0x4E,
  0x7C, 0x12, 0x11, 0x12, 0x7C,
  0x7F, 0x49, 0x49, 0x49, 0x36,
  0x3E, 0x41, 0x41, 0x41, 0x22,
  0x7F, 0x41, 0x41, 0x41, 0x3E,
  0x7F, 0x49, 0x49, 0x49, 0x41,
  0x7F, 0x09, 0x09, 0x09, 0x01,
  0x3E, 0x41, 0x41, 0x51, 0x73,
  0x7F, 0x08, 0x08, 0x08, 0x7F,
  0x00, 0x41, 0x7F, 0x41, 0x00,
  0x20, 0x40, 0x41, 0x3F, 0x01,
  0x7F, 0x08, 0x14, 0x22, 0x41,
  0x7F, 0x40, 0x40, 0x40, 0x40,
  0x7F, 0x02, 0x1C, 0x02, 0x7F,
  0x7F, 0x04, 0x08, 0x10, 0x7F,
  0x3E, 0x41, 0x41, 0x41, 0x3E,
  0x7F, 0x09, 0x09, 0x09, 0x06,
  0x3E, 0x41, 0x51, 0x21, 0x5E,
  0x7F, 0x09, 0x19, 0x29, 0x46,
  0x26, 0x49, 0x49, 0x49, 0x32,
  0x03, 0x01, 0x7F, 0x01, 0x03,
  0x3F, 0x40, 0x40, 0x40, 0x3F,
  0x1F, 0x20, 0x40, 0x20, 0x1F,
  0x3F, 0x40, 0x38, 0x40, 0x3F,
  0x63, 0x14, 0x08, 0x14, 0x63,
  0x03, 0x04, 0x78, 0x04, 0x03,
  0x61, 0x59, 0x49, 0x4D, 0x43,
  0x00, 0x7F, 0x41, 0x41, 0x41,
  0x02, 0x04, 0x08, 0x10, 0x20,
  0x00, 0x41, 0x41, 0x41, 0x7F,
  0x04, 0x02, 0x01, 0x02, 0x04,
  0x40, 0x40, 0x40, 0x40, 0x40,
  0x00, 0x03, 0x07, 0x08, 0x00,
  0x20, 0x54, 0x54, 0x78, 0x40,
  0x7F, 0x28, 0x44, 0x44, 0x38,
  0x38, 0x44, 0x44, 0x44, 0x28,
  0x38, 0x44, 0x44, 0x28, 0x7F,
  0x38, 0x54, 0x54, 0x54, 0x18,
  0x00, 0x08, 0x7E, 0x09, 0x02,
  0x18, 0xA4, 0xA4, 0x9C, 0x78,
  0x7F, 0x08, 0x04, 0x04, 0x78,
  0x00, 0x44, 0x7D, 0x40, 0x00,
  0x20, 0x40, 0x40, 0x3D, 0x00,
  0x7F, 0x10, 0x28, 0x44, 0x00,
  0x00, 0x41, 0x7F, 0x40, 0x00,
  0x7C, 0x04, 0x78, 0x04, 0x78,
  0x7C, 0x08, 0x04, 0x04, 0x78,
  0x38, 0x44, 0x44, 0x44, 0x38,
  0xFC, 0x18, 0x24, 0x24, 0x18,
  0x18, 0x24, 0x24, 0x18, 0xFC,
  0x7C, 0x08, 0x04, 0x04, 0x08,
  0x48, 0x54, 0x54, 0x54, 0x24,
  0x04, 0x04, 0x3F, 0x44, 0x24,
  0x3C, 0x40, 0x40, 0x20, 0x7C,
  0x1C, 0x20, 0x40, 0x20, 0x1C,
  0x3C, 0x40, 0x30, 0x40, 0x3C,
  0x44, 0x28, 0x10, 0x28, 0x44,
  0x4C, 0x90, 0x90, 0x90, 0x7C,
  0x44, 0x64, 0x54, 0x4C, 0x44,
  0x00, 0x08, 0x36, 0x41, 0x00,
  0x00, 0x00, 0x77, 0x00, 0x00,
  0x00, 0x41, 0x36, 0x08, 0x00,
  0x02, 0x01, 0x02, 0x04, 0x02,
  0x3C, 0x26, 0x23, 0x26, 0x3C,
  0x1E, 0xA1, 0xA1, 0x61, 0x12,
  0x3A, 0x40, 0x40, 0x20, 0x7A,
  0x38, 0x54, 0x54, 0x55, 0x59,
  0x21, 0x55, 0x55, 0x79, 0x41,
  0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
  0x21, 0x55, 0x54, 0x78, 0x40,
  0x20, 0x54, 0x55, 0x79, 0x40,
  0x0C, 0x1E, 0x52, 0x72, 0x12,
  0x39, 0x55, 0x55, 0x55, 0x59,
  0x39, 0x54, 0x54, 0x54, 0x59,
  0x39, 0x55, 0x54, 0x54, 0x58,
  0x00, 0x00, 0x45, 0x7C, 0x41,
  0x00, 0x02, 0x45, 0x7D, 0x42,
  0x00, 0x01, 0x45, 0x7C, 0x40,
  0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
  0xF0, 0x28, 0x25, 0x28, 0xF0,
  0x7C, 0x54, 0x55, 0x45, 0x00,
  0x20, 0x54, 0x54, 0x7C, 0x54,
  0x7C, 0x0A, 0x09, 0x7F, 0x49,
  0x32, 0x49, 0x49, 0x49, 0x32,
  0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
  0x32, 0x4A, 0x48, 0x48, 0x30,
  0x3A, 0x41, 0x41, 0x21, 0x7A,
  0x3A, 0x42, 0x40, 0x20, 0x78,
  0x00, 0x9D, 0xA0, 0xA0, 0x7D,
  0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
  0x3D, 0x40, 0x40, 0x40, 0x3D,
  0x3C, 0x24, 0xFF, 0x24, 0x24,
  0x48, 0x7E, 0x49, 0x43, 0x66,
  0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
  0xFF, 0x09, 0x29, 0xF6, 0x20,
  0xC0, 0x88, 0x7E, 0x09, 0x03,
  0x20, 0x54, 0x54, 0x79, 0x41,
  0x00, 0x00, 0x44, 0x7D, 0x41,
  0x30, 0x48, 0x48, 0x4A, 0x32,
  0x38, 0x40, 0x40, 0x22, 0x7A,
  0x00, 0x7A, 0x0A, 0x0A, 0x72,
  0x7D, 0x0D, 0x19, 0x31, 0x7D,
  0x26, 0x29, 0x29, 0x2F, 0x28,
  0x26, 0x29, 0x29, 0x29, 0x26,
  0x30, 0x48, 0x4D, 0x40, 0x20,
  0x38, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x38,
  0x2F, 0x10, 0xC8, 0xAC, 0xBA,
  0x2F, 0x10, 0x28, 0x34, 0xFA,
  0x00, 0x00, 0x7B, 0x00, 0x00,
  0x08, 0x14, 0x2A, 0x14, 0x22,
  0x22, 0x14, 0x2A, 0x14, 0x08,
  0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old code
  0xAA, 0x55, 0xAA, 0x55, 0xAA, // 50% block
  0xFF, 0x55, 0xFF, 0x55, 0xFF, // 75% block
  0x00, 0x00, 0x00, 0xFF, 0x00,
  0x10, 0x10, 0x10, 0xFF, 0x00,
  0x14, 0x14, 0x14, 0xFF, 0x00,
  0x10, 0x10, 0xFF, 0x00, 0xFF,
  0x10, 0x10, 0xF0, 0x10, 0xF0,
  0x14, 0x14, 0x14, 0xFC, 0x00,
  0x14, 0x14, 0xF7, 0x00, 0xFF,
  0x00, 0x00, 0xFF, 0x00, 0xFF,
  0x14, 0x14, 0xF4, 0x04, 0xFC,
  0x14, 0x14, 0x17, 0x10, 0x1F,
  0x10, 0x10, 0x1F, 0x10, 0x1F,
  0x14, 0x14, 0x14, 0x1F, 0x00,
  0x10, 0x10, 0x10, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x10,
  0x10, 0x10, 0x10, 0x1F, 0x10,
  0x10, 0x10, 0x10, 0xF0, 0x10,
  0x00, 0x00, 0x00, 0xFF, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0xFF, 0x10,
  0x00, 0x00, 0x00, 0xFF, 0x14,
  0x00, 0x00, 0xFF, 0x00, 0xFF,
  0x00, 0x00, 0x1F, 0x10, 0x17,
  0x00, 0x00, 0xFC, 0x04, 0xF4,
  0x14, 0x14, 0x17, 0x10, 0x17,
  0x14, 0x14, 0xF4, 0x04, 0xF4,
  0x00, 0x00, 0xFF, 0x00, 0xF7,
  0x14, 0x14, 0x14, 0x14, 0x14,
  0x14, 0x14, 0xF7, 0x00, 0xF7,
  0x14, 0x14, 0x14, 0x17, 0x14,
  0x10, 0x10, 0x1F, 0x10, 0x1F,
  0x14, 0x14, 0x14, 0xF4, 0x14,
  0x10, 0x10, 0xF0, 0x10, 0xF0,
  0x00, 0x00, 0x1F, 0x10, 0x1F,
  0x00, 0x00, 0x00, 0x1F, 0x14,
  0x00, 0x00, 0x00, 0xFC, 0x14,
  0x00, 0x00, 0xF0, 0x10, 0xF0,
  0x10, 0x10, 0xFF, 0x10, 0xFF,
  0x14, 0x14, 0x14, 0xFF, 0x14,
  0x10, 0x10, 0x10, 0x1F, 0x00,
  0x00, 0x00, 0x00, 0xF0, 0x10,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
  0xFF, 0xFF, 0xFF, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF,
  0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
  0x38, 0x44, 0x44, 0x38, 0x44,
  0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
  0x7E, 0x02, 0x02, 0x06, 0x06,
  0x02, 0x7E, 0x02, 0x7E, 0x02,
  0x63, 0x55, 0x49, 0x41, 0x63,
  0x38, 0x44, 0x44, 0x3C, 0x04,
  0x40, 0x7E, 0x20, 0x1E, 0x20,
  0x06, 0x02, 0x7E, 0x02, 0x02,
  0x99, 0xA5, 0xE7, 0xA5, 0x99,
  0x1C, 0x2A, 0x49, 0x2A, 0x1C,
  0x4C, 0x72, 0x01, 0x72, 0x4C,
  0x30, 0x4A, 0x4D, 0x4D, 0x30,
  0x30, 0x48, 0x78, 0x48, 0x30,
  0xBC, 0x62, 0x5A, 0x46, 0x3D,
  0x3E, 0x49, 0x49, 0x49, 0x00,
  0x7E, 0x01, 0x01, 0x01, 0x7E,
  0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
  0x44, 0x44, 0x5F, 0x44, 0x44,
  0x40, 0x51, 0x4A, 0x44, 0x40,
  0x40, 0x44, 0x4A, 0x51, 0x40,
  0x00, 0x00, 0xFF, 0x01, 0x03,
  0xE0, 0x80, 0xFF, 0x00, 0x00,
  0x08, 0x08, 0x6B, 0x6B, 0x08,
  0x36, 0x12, 0x36, 0x24, 0x36,
  0x06, 0x0F, 0x09, 0x0F, 0x06,
  0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x00, 0x10, 0x10, 0x00,
  0x30, 0x40, 0xFF, 0x01, 0x01,
  0x00, 0x1F, 0x01, 0x01, 0x1E,
  0x00, 0x19, 0x1D, 0x17, 0x12,
  0x00, 0x3C, 0x3C, 0x3C, 0x3C,
  0x00, 0x00, 0x00, 0x00, 0x00  // #255 NBSP
};

#include <Platform/Platform.h>

uint8_t Image::_bits_per_pixel(ImgBufferFormat x) {
  switch (x) {
    case ImgBufferFormat::MONOCHROME:
      return 1;
    case ImgBufferFormat::GREY_8:
    case ImgBufferFormat::R3_G3_B2:
      return 8;    // 8-bit color
    case ImgBufferFormat::GREY_16:
    case ImgBufferFormat::R5_G6_B5:
      return 16;   // 16-bit color
    case ImgBufferFormat::GREY_24:
    case ImgBufferFormat::R8_G8_B8:
      return 24;   // 24-bit color
    case ImgBufferFormat::UNALLOCATED:
    default:
      break;
  }
  return 0;
}


Image::Image(uint32_t x, uint32_t y, ImgBufferFormat fmt, uint8_t* buf) : Image(x, y) {
  _buf_fmt  = fmt;
  _imgflags = 0;
  _buffer   = buf;
}


Image::Image(uint32_t x, uint32_t y, ImgBufferFormat fmt) : Image(x, y) {
  _buf_fmt  = fmt;
}


Image::Image(uint32_t x, uint32_t y) : _x(x), _y(y) {}


Image::~Image() {
  if (_is_ours() && allocated()) {
    free(_buffer);
    _buffer = nullptr;
  }
}


/**
* Takes the given buffer and assume it as our own.
* Frees any existing buffer. It is the caller's responsibility to ensure that
*   the buffer is adequately sized.
*
* @return true if the buffer replacement succeeded. False if format is unknown.
*/
bool Image::setBuffer(uint8_t* buf) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (_is_ours() && allocated()) {
      free(_buffer);
      _buffer = nullptr;
      _ours(false);
    }
    _buffer = buf;
    return true;
  }
  return false;
}


/**
* Takes the given buffer and format and assume it as our own.
* Frees any existing buffer. It is the caller's responsibility to ensure that
*   the buffer is adequately sized.
*
* @return true always.
*/
bool Image::setBuffer(uint8_t* buf, ImgBufferFormat fmt) {
  if (_is_ours() && allocated()) {
    free(_buffer);
    _buffer = nullptr;
    _ours(false);
  }
  _buf_fmt = fmt;
  _buffer = buf;
  return true;
}


/**
*
*/
bool Image::setBufferByCopy(uint8_t* buf) {
  return setBufferByCopy(buf, _buf_fmt);
}


/**
* TODO: Need to audit memory management here. Unit testing is clearly warranted.
*/
bool Image::setBufferByCopy(uint8_t* buf, ImgBufferFormat fmt) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (fmt == _buf_fmt) {
      if (allocated()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
      else if (0 == _buffer_allocator()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
    }
    else {
      ImgBufferFormat old_fmt = _buf_fmt;
      _buf_fmt = fmt;
      if (allocated() && (_bits_per_pixel(old_fmt) == _bits_per_pixel(_buf_fmt))) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
      else if (0 == _buffer_allocator()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
    }
  }
  else {
    _buf_fmt = fmt;
    if (0 == _buffer_allocator()) {
      memcpy(_buffer, buf, bytesUsed());
      return true;
    }
  }
  return false;
}


/**
*
*/
bool Image::reallocate() {
  return (0 == _buffer_allocator());
}


/**
*
*/
void Image::wipe() {
  if (_is_ours() && allocated()) {
    free(_buffer);
  }
  _x        = 0;
  _y        = 0;
  _buffer   = nullptr;
  _buf_fmt  = ImgBufferFormat::UNALLOCATED;
  _imgflags = 0;
}


/**
*
*/
int8_t Image::serialize(StringBuilder* out) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      uint32_t sz = bytesUsed();
      uint8_t buf[10];
      buf[0] = (uint8_t) (_x >> 24) & 0xFF;
      buf[1] = (uint8_t) (_x >> 16) & 0xFF;
      buf[2] = (uint8_t) (_x >> 8) & 0xFF;
      buf[3] = (uint8_t) _x & 0xFF;
      buf[4] = (uint8_t) (_y >> 24) & 0xFF;
      buf[5] = (uint8_t) (_y >> 16) & 0xFF;
      buf[6] = (uint8_t) (_y >> 8) & 0xFF;
      buf[7] = (uint8_t) _y & 0xFF;
      buf[8] = (uint8_t) _buf_fmt;
      buf[9] = _imgflags;
      out->concat(buf, 10);
      out->concat(_buffer, sz);
      return 0;
    }
  }
  return -1;
}


/**
*
*/
int8_t Image::serialize(uint8_t* buf, uint32_t* len) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      uint32_t sz = bytesUsed();
      *(buf + 0) = (uint8_t) (_x >> 24) & 0xFF;
      *(buf + 1) = (uint8_t) (_x >> 16) & 0xFF;
      *(buf + 2) = (uint8_t) (_x >> 8) & 0xFF;
      *(buf + 3) = (uint8_t) _x & 0xFF;
      *(buf + 4) = (uint8_t) (_y >> 24) & 0xFF;
      *(buf + 5) = (uint8_t) (_y >> 16) & 0xFF;
      *(buf + 6) = (uint8_t) (_y >> 8) & 0xFF;
      *(buf + 7) = (uint8_t) _y & 0xFF;
      *(buf + 8) = (uint8_t) _buf_fmt;
      *(buf + 9) = _imgflags;
      memcpy((buf + 10), _buffer, sz);
      *len = sz;
      return 0;
    }
  }
  return -1;
}


/**
* Serializes the object without the bulk of the data (the image itself). This is
*   for the sake of avoiding memory overhead on an intermediate copy.
* The len parameter will be updated to reflect the actual written bytes. Not
*   including the langth of the full buffer.
*/
int8_t Image::serializeWithoutBuffer(uint8_t* buf, uint32_t* len) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      *(buf + 0) = (uint8_t) (_x >> 24) & 0xFF;
      *(buf + 1) = (uint8_t) (_x >> 16) & 0xFF;
      *(buf + 2) = (uint8_t) (_x >> 8) & 0xFF;
      *(buf + 3) = (uint8_t) _x & 0xFF;
      *(buf + 4) = (uint8_t) (_y >> 24) & 0xFF;
      *(buf + 5) = (uint8_t) (_y >> 16) & 0xFF;
      *(buf + 6) = (uint8_t) (_y >> 8) & 0xFF;
      *(buf + 7) = (uint8_t) _y & 0xFF;
      *(buf + 8) = (uint8_t) _buf_fmt;
      *(buf + 9) = _imgflags;
      *len = 10;
      return 0;
    }
  }
  return -1;
}


/**
*/
int8_t Image::deserialize(uint8_t* buf, uint32_t len) {
  if (ImgBufferFormat::UNALLOCATED == _buf_fmt) {
    if (len > 9) {
      uint32_t temp_x = ((uint32_t) *(buf + 0) << 24) | ((uint32_t) *(buf + 1) << 16) | ((uint32_t) *(buf + 2) << 8) | ((uint32_t) *(buf + 3));
      uint32_t temp_y = ((uint32_t) *(buf + 4) << 24) | ((uint32_t) *(buf + 5) << 16) | ((uint32_t) *(buf + 6) << 8) | ((uint32_t) *(buf + 7));
      ImgBufferFormat temp_fmt = (ImgBufferFormat) *(buf + 8);
      uint32_t temp_f = *(buf + 9);
      uint32_t proposed_size = (_bits_per_pixel(temp_fmt) * temp_x * temp_y) >> 3;
      if (proposed_size == (len - 10)) {
        // The derived and declared sizes match.
        _x = temp_x;
        _y = temp_y;
        _buf_fmt  = temp_fmt;
        _imgflags = temp_f;
        return setBufferByCopy(buf + 10) ? 0 : -2;
      }
    }
  }
  return -1;
}

/*
* Returns 0 on success, -1 on bad class state, -2 on free failure, -3 on malloc fail
*/
int8_t Image::_buffer_allocator() {
  int8_t ret = -1;
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    ret--;
    if (allocated()) {
      if (!_is_ours()) {
        return ret;
      }
      free(_buffer);
    }
    ret--;
    _buffer = (uint8_t*) malloc(bytesUsed());
    if (allocated()) {
      _ours(true);
      memset(_buffer, 0, bytesUsed());
      ret = 0;
    }
  }
  return ret;
}


/**
*
*/
bool Image::isColor() {
  switch (_buf_fmt) {
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
      return true;
    default:
      break;
  }
  return false;
}


/**
* Converts the color specified in the argument to the best-representation in
*   the buffer's native format.
* Color to grey conversion uses 30/59/11 scalar set with results averaged together.
*/
uint32_t Image::convertColor(uint32_t c, ImgBufferFormat src_fmt) {
  uint32_t ret = 0;
  uint8_t r = 0, g = 0, b = 0;
  uint8_t bits_per_src_channel;
  bool    src_is_color = true;
  if (src_fmt == _buf_fmt) {
    return c;
  }
  switch (src_fmt) {
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
      src_is_color = false;
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
      bits_per_src_channel = 8;
      r = (uint8_t) (c >> 16) & 0xFF;
      g = (uint8_t) (c >> 8) & 0xFF;
      b = (uint8_t) (c & 0xFF);
      break;
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
      src_is_color = false;
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
      bits_per_src_channel = 6;
      r = (uint8_t) (c >> 11) & 0x1F;
      g = (uint8_t) (c >> 5) & 0x3F;
      b = (uint8_t) (c & 0x1F);
      break;
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      src_is_color = false;
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
      bits_per_src_channel = 3;
      r = (uint8_t) (c >> 5) & 0x07;
      g = (uint8_t) (c >> 2) & 0x07;
      b = (uint8_t) (c & 0x03);
      break;
    case ImgBufferFormat::MONOCHROME:        // Monochrome   TODO: Unsupported.
      src_is_color = false;
      bits_per_src_channel = 1;
      // TODO:
      break;
    case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
    default:
      return ret;
  }

  if (src_is_color && !isColor()) {
    // Conversion from color to grey or monochrome.
    uint8_t bpp = _bits_per_pixel();
    float avg = ((r * 0.3) + (g * 0.59) + (b * 0.11)) / 3.0;
    float lum = avg / ((1 << bits_per_src_channel) - 1);
    ret = ((1 << bpp) - 1) * lum;
  }
  else {
  }
  return ret;
}


/**
*
*/
uint32_t Image::getPixel(uint32_t x, uint32_t y) {
  uint32_t ret = 0;
  uint32_t sz = bytesUsed();
  uint32_t offset = _pixel_offset(x, y);
  if (offset < sz) {
    switch (_buf_fmt) {
      case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R8_G8_B8:          // 24-bit color
        ret = ((uint32_t)*(_buffer + offset + 0) << 16) | ((uint32_t)*(_buffer + offset + 1) << 8) | (uint32_t)*(_buffer + offset + 2);
        break;
      case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R5_G6_B5:          // 16-bit color
        ret = ((uint32_t) *(_buffer + offset + 0) << 8) | (uint32_t) *(_buffer + offset + 1);
        break;
      case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      case ImgBufferFormat::R3_G3_B2:          // 8-bit color
        ret = (uint32_t) *(_buffer + offset);
        break;
      case ImgBufferFormat::MONOCHROME:        // Monochrome   TODO: Unsupported.
      case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
      default:
        break;
    }
  }
  else {
    // Addressed pixel is out-of-bounds
  }
  return ret;
}


/**
* Takes a color in 32-bit. Squeezes it into the buffer's format, discarding low bits as appropriate.
*/
bool Image::setPixel(uint32_t x, uint32_t y, uint32_t c) {
  switch (_buf_fmt) {
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale
      _set_pixel_8(x, y, c);
      break;
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale
      _set_pixel_16(x, y, c);
      break;
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale
      _set_pixel_32(x, y, c);
      break;
    case ImgBufferFormat::MONOCHROME:        // Monochrome   TODO: Unsupported.
    case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
    default:
      break;;
  }
  return false;
}


/**
* Takes a color in discrete RGB values. Squeezes it into the buffer's format, discarding low bits as appropriate.
*/
bool Image::setPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t sz = bytesUsed();
  uint32_t offset = _pixel_offset(x, y);
  if (offset < sz) {
    switch (_buf_fmt) {
      case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R8_G8_B8:          // 24-bit color
        *(_buffer + offset + 0) = r;
        *(_buffer + offset + 1) = g;
        *(_buffer + offset + 2) = b;
        break;
      case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R5_G6_B5:          // 16-bit color
        *(_buffer + offset + 0) = ((uint16_t) ((r << 3) & 0xF8) << 8) | ((g >> 3) & 0x07);
        *(_buffer + offset + 1) = ((uint16_t) ((g << 5) & 0xE0) << 8) | ((b >> 3) & 0x1F);
        break;
      case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      case ImgBufferFormat::R3_G3_B2:          // 8-bit color
        *(_buffer + offset) = ((uint8_t) (r << 6) & 0xE0) | ((uint8_t) (g << 2) & 0x1C) | (b & 0x07);
        break;
      case ImgBufferFormat::MONOCHROME:        // Monochrome   TODO: Unsupported.
      case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
      default:
        return false;
    }
    return true;
  }
  else {
    // Addressed pixel is out-of-bounds
  }
  return false;
}



/*******************************************************************************
* Code below this block was lifted from Adafruit's GFX library.
*******************************************************************************/

/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
    @param    size  Font magnification level, 1 is 'original' size
*/
void Image::drawChar(uint32_t x, uint32_t y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size) {
  if (!_gfxFont) { // 'Classic' built-in font
    if ((x >= _x)  || // Clip right
       (y >= _y)   || // Clip bottom
           ((int32_t)(x + 6 * size - 1) < 0) || // Clip left
           ((int32_t)(y + 8 * size - 1) < 0))   // Clip top
            return;

        if(!cp437() && (c >= 176)) c++; // Handle 'classic' charset behavior

        _lock(true);
        for (int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
          uint8_t line = font[c * 5 + i];
          for (int8_t j=0; j<8; j++, line >>= 1) {
            if(line & 1) {
              if(size == 1) {
                setPixel(x+i, y+j, color);
              }
              else {
                fillRect(x+i*size, y+j*size, size, size, color);
              }
            }
            else if(bg != color) {
              if (size == 1) {
                setPixel(x+i, y+j, bg);
              }
              else {
                fillRect(x+i*size, y+j*size, size, size, bg);
              }
            }
          }
        }
        if (bg != color) { // If opaque, draw vertical line for last column
            if(size == 1) drawFastVLine(x+5, y, 8, bg);
            else          fillRect(x+5*size, y, size, 8*size, bg);
        }
        _lock(false);
    }
    else { // Custom font
        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!
        c -= (uint8_t) _gfxFont->first;
        GFXglyph* glyph  = &(((GFXglyph*) _gfxFont->glyph))[c];
        uint8_t*  bitmap = (uint8_t*) _gfxFont->bitmap;
        uint16_t bo = glyph->bitmapOffset;
        uint8_t  w  = glyph->width;
        uint8_t  h  = glyph->height;
        int8_t   xo = glyph->xOffset;
        int8_t   yo = glyph->yOffset;
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if (size > 1) {
          xo16 = xo;
          yo16 = yo;
        }
        // Todo: Add character clipping here

        // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
        // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
        // has typically been used with the 'classic' font to overwrite old
        // screen contents with new data.  This ONLY works because the
        // characters are a uniform size; it's not a sensible thing to do with
        // proportionally-spaced fonts with glyphs of varying sizes (and that
        // may overlap).  To replace previously-drawn text when using a custom
        // font, use the getTextBounds() function to determine the smallest
        // rectangle encompassing a string, erase the area with fillRect(),
        // then draw new text.  This WILL infortunately 'blink' the text, but
        // is unavoidable.  Drawing 'background' pixels will NOT fix this,
        // only creates a new set of problems.  Have an idea to work around
        // this (a canvas object type for MCUs that can afford the RAM and
        // displays supporting setAddrWindow() and pushColors()), but haven't
        // implemented this yet.

    _lock(true);
    for (yy=0; yy<h; yy++) {
      for (xx=0; xx<w; xx++) {
        if (!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if (bits & 0x80) {
          if (size == 1) {
            setPixel(x+xo+xx, y+yo+yy, color);
          }
          else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        bits <<= 1;
      }
    }
    _lock(false);
  } // End classic vs custom font
}


/*!
    @brief  Print one byte/character of data, used to support print()
    @param  c  The 8-bit ascii character to write
*/
void Image::writeChar(uint8_t c) {
  if (!_gfxFont) { // 'Classic' built-in font
    if (c == '\n') {                        // Newline?
      _cursor_x  = 0;                     // Reset x to zero,
      _cursor_y += _textsize * 8;          // advance y one line
    }
    else if (c != '\r') {                   // Ignore carriage returns
      if (textWrap() && ((_cursor_x + _textsize * 6) > _x)) { // Off right?
        _cursor_x  = 0;                 // Reset x to zero,
        _cursor_y += _textsize * 8;      // advance y one line
      }
      drawChar(_cursor_x, _cursor_y, c, _textcolor, _textbgcolor, _textsize);
      _cursor_x += _textsize * 6;          // Advance x one char
    }
  }
  else { // Custom font
    if (c == '\n') {
      _cursor_x  = 0;
      _cursor_y += (int16_t) _textsize * (uint8_t) _gfxFont->yAdvance;
    }
    else if (c != '\r') {
      uint8_t first = _gfxFont->first;
      if ((c >= first) && (c <= _gfxFont->last)) {
        GFXglyph* glyph = &_gfxFont->glyph[c - first];
        uint8_t  w      = glyph->width;
        uint8_t  h      = glyph->height;
        if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = (int8_t) glyph->xOffset; // sic
          if (textWrap() && ((_cursor_x + _textsize * (xo + w)) > _x)) {
            _cursor_x  = 0;
            _cursor_y += (int16_t) _textsize * (uint8_t)_gfxFont->yAdvance;
          }
          drawChar(_cursor_x, _cursor_y, c, _textcolor, _textbgcolor, _textsize);
        }
        _cursor_x += (uint8_t) glyph->xAdvance * (int16_t) _textsize;
      }
    }
  }
}


void Image::writeString(StringBuilder* str) {
  writeString((const char*) str->string());
}


void Image::writeString(const char* str) {
  int len = strlen(str);
  for (int i = 0; i < len; i++) {
    writeChar((uint8_t) *(str + i));
  }
}


/*!
  @brief  Set text cursor location
  @param  x    X coordinate in pixels
  @param  y    Y coordinate in pixels
*/
void Image::setCursor(uint32_t x, uint32_t y) {
  _cursor_x = x;
  _cursor_y = y;
}

/*!
  @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
  @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
void Image::setTextSize(uint8_t s) {
  _textsize = (s > 0) ? s : 1;
}

/*!
  @brief   Set text font color with transparant background
  @param   c
*/
void Image::setTextColor(uint32_t c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  _textcolor = _textbgcolor = c;
}


/*!
  @brief   Set text font color and background
  @param   c
*/
void Image::setTextColor(uint32_t c, uint32_t bg) {
  _textcolor   = c;
  _textbgcolor = bg;
}


/*!
  @brief    Helper to determine size of a character with current font/size.
     Broke this out as it's used by both the PROGMEM- and RAM-resident getTextBounds() functions.
  @param    c     The ascii character in question
  @param    x     Pointer to x location of character
  @param    y     Pointer to y location of character
  @param    minx  Minimum clipping value for X
  @param    miny  Minimum clipping value for Y
  @param    maxx  Maximum clipping value for X
  @param    maxy  Maximum clipping value for Y
*/
void Image::charBounds(char c, uint32_t* x, uint32_t* y, uint32_t* minx, uint32_t* miny, uint32_t* maxx, uint32_t* maxy) {
  if (_gfxFont) {
    if (c == '\n') { // Newline?
      *x  = 0;    // Reset x to zero, advance y by one line
      *y += _textsize * (uint8_t)_gfxFont->yAdvance;
    }
    else if (c != '\r') { // Not a carriage return; is normal char
      uint8_t first = _gfxFont->first;
      uint8_t last  = _gfxFont->last;
      if((c >= first) && (c <= last)) { // Char present in this font?
        GFXglyph* glyph = &(((GFXglyph*) _gfxFont->glyph))[c - first];
        uint8_t gw = glyph->width;
        uint8_t gh = glyph->height;
        uint8_t xa = glyph->xAdvance;
        int8_t  xo = glyph->xOffset;
        int8_t  yo = glyph->yOffset;
        if(textWrap() && ((*x+(((int16_t)xo+gw) * _textsize)) > _x)) {
          *x  = 0; // Reset x to zero, advance y by one line
          *y += _textsize * (uint8_t) _gfxFont->yAdvance;
        }
        int16_t ts = (int16_t) _textsize;
        uint32_t x1 = *x + xo * ts;
        uint32_t y1 = *y + yo * ts;
        uint32_t x2 = x1 + gw * ts - 1;
        uint32_t y2 = y1 + gh * ts - 1;
        if(x1 < *minx) *minx = x1;
        if(y1 < *miny) *miny = y1;
        if(x2 > *maxx) *maxx = x2;
        if(y2 > *maxy) *maxy = y2;
        *x += xa * ts;
      }
    }
  }
  else { // Default font
    if(c == '\n') {                   // Newline?
      *x  = 0;                        // Reset x to zero,
      *y += _textsize * 8;             // advance y one line
      // min/max x/y unchaged -- that waits for next 'normal' character
    }
    else if (c != '\r') {  // Normal char; ignore carriage returns
      if (textWrap() && ((*x + _textsize * 6) > _x)) { // Off right?
        *x  = 0;                    // Reset x to zero,
        *y += _textsize * 8;         // advance y one line
      }
      int x2 = *x + _textsize * 6 - 1; // Lower-right pixel of char
      int y2 = *y + _textsize * 8 - 1;
      if(x2 > *maxx) {  *maxx = x2;  }      // Track max x, y
      if(y2 > *maxy) {  *maxy = y2;  }
      if(*x < *minx) {  *minx = *x;  }      // Track min x, y
      if(*y < *miny) {  *miny = *y;  }
      *x += _textsize * 6;             // Advance x one char
    }
  }
}


/*!
  @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str     The ascii string to measure
  @param    x       The current cursor X
  @param    y       The current cursor Y
  @param    x1      The boundary X coordinate, set by function
  @param    y1      The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(const char* str, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h) {
  uint8_t c; // Current character
  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  uint32_t minx = _x;
  uint32_t miny = _y;
  uint32_t maxx = -1;
  uint32_t maxy = -1;

  while((c = *str++)) {
    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if(maxx >= minx) {
    *x1 = minx;
    *w  = maxx - minx + 1;
  }
  if(maxy >= miny) {
    *y1 = miny;
    *h  = maxy - miny + 1;
  }
}


/*!
  @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str    The ascii string to measure (as an arduino String() class)
  @param    x      The current cursor X
  @param    y      The current cursor Y
  @param    x1     The boundary X coordinate, set by function
  @param    y1     The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(StringBuilder* str, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h) {
  if (str->length() != 0) {
    getTextBounds((const uint8_t*) str->string(), x, y, x1, y1, w, h);
  }
}


/*!
  @brief    Helper to determine size of a PROGMEM string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str     The flash-memory ascii string to measure
  @param    x       The current cursor X
  @param    y       The current cursor Y
  @param    x1      The boundary X coordinate, set by function
  @param    y1      The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(const uint8_t* str, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h) {
  uint8_t* s = (uint8_t*) str;
  uint8_t c;
  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  uint32_t minx = _x;
  uint32_t miny = _y;
  uint32_t maxx = -1;
  uint32_t maxy = -1;

  while((c = *s++)) {
    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if(maxx >= minx) {
    *x1 = minx;
    *w  = maxx - minx + 1;
  }
  if (maxy >= miny) {
    *y1 = miny;
    *h  = maxy - miny + 1;
  }
}



/*!
  @brief    Write a perfectly vertical line, overwrite in subclasses if startWrite is defined!
  @param    x   Top-most x coordinate
  @param    y   Top-most y coordinate
  @param    h   Height in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::drawFastVLine(uint32_t x, uint32_t y, uint32_t h, uint32_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Can be just drawLine(x, y, x, y+h-1, color);
  // or fillRect(x, y, 1, h, color);
  _lock(true);
  drawLine(x, y, x, y+h-1, color);
  _lock(false);
}


/*!
  @brief    Write a perfectly horizontal line, overwrite in subclasses if startWrite is defined!
  @param    x   Left-most x coordinate
  @param    y   Left-most y coordinate
  @param    w   Width in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::drawFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Example: drawLine(x, y, x+w-1, y, color);
  // or fillRect(x, y, w, 1, color);
  _lock(true);
  drawLine(x, y, x+w-1, y, color);
  _lock(false);
}


/*!
  @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
  @param    x   Top left corner x coordinate
  @param    y   Top left corner y coordinate
  @param    w   Width in pixels
  @param    h   Height in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
  _lock(true);
  for (uint32_t i=x; i<x+w; i++) {
    drawLine(i, y, i, y+h-1, color);
  }
  _lock(false);
}


/**
* @brief   Draw a rectangle with no fill color
*  @param    x   Top left corner x coordinate
*  @param    y   Top left corner y coordinate
*  @param    w   Width in pixels
*  @param    h   Height in pixels
*  @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

/**
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawRoundRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color) {
  uint32_t max_radius = ((w < h) ? w : h) >> 1; // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  drawFastHLine(x + r, y, w - 2 * r, color);         // Top
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
  drawFastVLine(x, y + r, h - 2 * r, color);         // Left
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}


/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
void Image::fillRoundRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color) {
  uint32_t max_radius = ((w < h) ? w : h) >> 1; // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  fillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}


/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawTriangle(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}


/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
void Image::fillTriangle(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
  uint32_t a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    strict_swap(&y0, &y1);
    strict_swap(&x0, &x1);
  }
  if (y1 > y2) {
    strict_swap(&y2, &y1);
    strict_swap(&x2, &x1);
  }
  if (y0 > y1) {
    strict_swap(&y0, &y1);
    strict_swap(&x0, &x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a) {       a = x1;   }
    else if (x1 > b) {  b = x1;   }
    if (x2 < a) {       a = x2;   }
    else if (x2 > b) {  b = x2;   }
    drawFastHLine(a, y0, b - a + 1, color);
    return;
  }

  uint32_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
           dx12 = x2 - x1, dy12 = y2 - y1;
  uint32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2) {    last = y1;       }  // Include y1 scanline
  else {             last = y1 - 1;   }  // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) {  strict_swap(&a, &b);   }
    drawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) {  strict_swap(&a, &b);   }
    drawFastHLine(a, y, b - a + 1, color);
  }
}


/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawCircle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;

  setPixel(x0, y0 + r, color);
  setPixel(x0, y0 - r, color);
  setPixel(x0 + r, y0, color);
  setPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    setPixel(x0 + x, y0 + y, color);
    setPixel(x0 - x, y0 + y, color);
    setPixel(x0 + x, y0 - y, color);
    setPixel(x0 - x, y0 - y, color);
    setPixel(x0 + y, y0 + x, color);
    setPixel(x0 - y, y0 + x, color);
    setPixel(x0 + y, y0 - x, color);
    setPixel(x0 - y, y0 - x, color);
  }
}


/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    quadrant  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawCircleHelper(uint32_t x0, uint32_t y0, uint32_t r, uint8_t quadrant, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (quadrant & 0x4) {
      setPixel(x0 + x, y0 + y, color);
      setPixel(x0 + y, y0 + x, color);
    }
    if (quadrant & 0x2) {
      setPixel(x0 + x, y0 - y, color);
      setPixel(x0 + y, y0 - x, color);
    }
    if (quadrant & 0x8) {
      setPixel(x0 - y, y0 + x, color);
      setPixel(x0 - x, y0 + y, color);
    }
    if (quadrant & 0x1) {
      setPixel(x0 - y, y0 - x, color);
      setPixel(x0 - x, y0 - y, color);
    }
  }
}


/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::fillCircle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
void Image::fillCircleHelper(uint32_t x0, uint32_t y0, uint32_t r, uint8_t corners, uint32_t delta, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;
  int32_t px = x;
  int32_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}


/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
   position. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Image::drawBitmap(uint32_t x, uint32_t y, const uint8_t* bitmap, uint32_t w, uint32_t h) {
  for (uint32_t j = 0; j < h; j++, y++) {
    for (uint32_t i = 0; i < w; i++) {
      setPixel(x + i, y, *(bitmap + (j * w + i)));
    }
  }
}


/**
*
*/
void Image::fill(uint32_t color) {
  //uint8_t bpp = _bits_per_pixel();
  _lock(true);
  switch (_bits_per_pixel()) {
    case 32:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *((uint32_t*) (_buffer + (i << 2))) = (uint32_t) color;
      }
      break;
    case 24:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *(_buffer + (i << 2) + 0) = (uint8_t) (color >> 16) & 0xFF;
        *(_buffer + (i << 2) + 1) = (uint8_t) (color >> 8) & 0xFF;
        *(_buffer + (i << 2) + 2) = (uint8_t) (color & 0xFF);
      }
      break;
    case 16:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *((uint16_t*) (_buffer + (i << 1))) = (uint16_t) color;
      }
      break;
    case 8:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *(_buffer + i) = (uint8_t) color;
      }
      break;
    case 1:   // NOTE: Assumes a pixel count of mod(8).
      for (uint32_t i=0; i < ((_x * _y) >> 3); i++) {
        *(_buffer + i) = (0 != color) ? 0xFF : 0x00;
      }
      break;
  }
  _lock(false);
}


/*!
  @brief    Write a line.  Bresenham's algorithm - thx wikpedia
  @param    x0  Start point x coordinate
  @param    y0  Start point y coordinate
  @param    x1  End point x coordinate
  @param    y1  End point y coordinate
  @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
  const bool steep = wrap_accounted_delta(y1, y0) > wrap_accounted_delta(x1, x0);
  if (steep) {
    strict_swap(&x0, &y0);
    strict_swap(&x1, &y1);
  }
  if (x0 > x1) {
    strict_swap(&x0, &x1);
    strict_swap(&y0, &y1);
  }

  const uint32_t dx    = x1 - x0;
  const uint32_t dy    = wrap_accounted_delta(y1, y0);
  const int8_t   ystep = (y0 < y1) ? 1 : -1;
  int32_t err = (int32_t) (dx >> 1);  // NOTE: Imposes width limit of 2,147,483,648 pixels.

  while (x0++ <= x1) {
    if (steep) {
      setPixel(y0, x0, color);
    }
    else {
      setPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

#endif   // CONFIG_MANUVR_IMG_SUPPORT
