/*
File:   Image.h
Author: J. Ian Lindsay
Date:   2019.06.02

Certain drawing features of this class were lifted from Adafruit's GFX library.
  Rather than call out which specific functions, or isolate translation units
  for legal reasons, this class simply inherrits their license and attribution
  unless I rework it. License reproduced in the comment block below this one.
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


#include <CommonConstants.h>
#include <EnumeratedTypeCodes.h>


#ifndef __MANUVR_TYPE_IMG_H
#define __MANUVR_TYPE_IMG_H

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include <DataStructures/StringBuilder.h>


#define MANUVR_IMG_FLAG_BUFFER_OURS     0x01  // We are responsible for freeing the buffer.
#define MANUVR_IMG_FLAG_BUFFER_LOCKED   0x02  // Buffer should not be modified when set.
#define MANUVR_IMG_FLAG_IS_FRAMEBUFFER  0x04  // This class holds the framebuffer for some piece of hardware.
#define MANUVR_IMG_FLAG_IS_FB_DIRTY     0x08  // This image is dirty for the purposes of rendering.

#define MANUVR_IMG_FLAG_ROTATION_MASK   0xC0  // The bits that hold our orientation value.


// Font data stored PER GLYPH
// Taken from Adafruit GFX
typedef struct {
  uint16_t bitmapOffset; // Pointer into GFXfont->bitmap
  uint8_t  width;        // Bitmap dimensions in pixels
  uint8_t  height;       // Bitmap dimensions in pixels
  uint8_t  xAdvance;     // Distance to advance cursor (x axis)
  int8_t   xOffset;      // X dist from cursor pos to UL corner
  int8_t   yOffset;      // Y dist from cursor pos to UL corner
} GFXglyph;

// Data stored for FONT AS A WHOLE
// Taken from Adafruit GFX
typedef struct {
  uint8_t*  bitmap;      // Glyph bitmaps, concatenated
  GFXglyph* glyph;       // Glyph array
  uint8_t   first;       // ASCII extents (first char)
  uint8_t   last;        // ASCII extents (last char)
  uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;


enum class ImgBufferFormat : uint8_t {
  UNALLOCATED = 0x00,  // Buffer unallocated
  MONOCHROME  = 0x01,  // Monochrome
  GREY_24     = 0x02,  // 24-bit greyscale
  GREY_16     = 0x03,  // 16-bit greyscale
  GREY_8      = 0x04,  // 8-bit greyscale
  R8_G8_B8    = 0x05,  // 24-bit color
  R5_G6_B5    = 0x06,  // 16-bit color
  R3_G3_B2    = 0x07   // 8-bit color
};

enum class ImgOrientation : uint8_t {
  ROTATION_0   = 0x00,  // 0-degrees
  ROTATION_90  = 0x40,  // 90-degrees
  ROTATION_180 = 0x80,  // 180-degrees
  ROTATION_270 = 0xC0   // 270-degrees
};


class Image {
  public:
    Image(uint32_t x, uint32_t y, ImgBufferFormat, uint8_t*);
    Image(uint32_t x, uint32_t y, ImgBufferFormat);
    Image(uint32_t x, uint32_t y);
    Image();
    ~Image();

    bool setBuffer(uint8_t*);
    bool setBuffer(uint8_t*, ImgBufferFormat);
    bool setBufferByCopy(uint8_t*);
    bool setBufferByCopy(uint8_t*, ImgBufferFormat);

    void wipe();
    int8_t serialize(StringBuilder*);
    int8_t serialize(uint8_t*, uint32_t*);
    int8_t serializeWithoutBuffer(uint8_t*, uint32_t*);
    int8_t deserialize(uint8_t*, uint32_t);

    bool isColor();

    uint32_t getColor(uint32_t x, uint32_t y);
    bool     setColor(uint32_t x, uint32_t y, uint32_t);
    bool     setColor(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

    inline uint32_t        x() {            return _x;                     };
    inline uint32_t        y() {            return _y;                     };
    inline uint8_t*        buffer() {       return _buffer;                };
    inline ImgBufferFormat format() {       return _buf_fmt;               };
    inline bool            allocated() {    return (nullptr != _buffer);   };
    inline uint32_t        pixels() {       return (_x * _y);              };
    inline uint32_t        bytesUsed() {    return (_x * _y * (_bits_per_pixel() >> 3));  };
    inline ImgOrientation  orientation() {  return ((ImgOrientation) ((_imgflags & MANUVR_IMG_FLAG_ROTATION_MASK) >> 6));  };

    inline bool  isFrameBuffer() {        return _img_flag(MANUVR_IMG_FLAG_IS_FRAMEBUFFER);   };

    /* BEGIN ADAFRUIT GFX SPLICE */
    void writeFillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void writeFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t color);
    void writeFastVLine(uint32_t x, uint32_t y, uint32_t h, uint32_t color);
    void writeLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
    void fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

    //void drawElipse(uint32_t x0, uint32_t y0, uint32_t major_axis, uint32_t minor_axis, float rotation, uint32_t color);
    //void drawCircle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color);
    //void drawCircleHelper(uint32_t x0, uint32_t y0, uint32_t r, uint8_t cornername, uint32_t color);
    //void fillCircle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color);
    //void fillCircleHelper(uint32_t x0, uint32_t y0, uint32_t r, uint32_t cornername, uint32_t delta, uint32_t color);
    //void drawTriangle(uint32_t x0,  uint32_t y0, uint32_tt x1, uint32_t y1, uint32_t x2,     uint32_t y2, uint32_t color);
    //void fillTriangle(uint32_t x0,  uint32_t y0, uint32_tt x1, uint32_t y1, uint32_t x2,     uint32_t y2, uint32_t color);
    //void drawRoundRect(uint32_t x0, uint32_t y0, uint32_tt w,  uint32_t h,  uint32_t radius, uint32_t color);
    //void fillRoundRect(uint32_t x0, uint32_t y0, uint32_tt w,  uint32_t h,  uint32_t radius, uint32_t color);

    //drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color),
    //drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg),
    //drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color),
    //drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg),
    //drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color),
    //drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h),
    //drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h),
    //drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h),
    //drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h),
    //drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h),
    //drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h),
    //drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], const uint8_t mask[], int16_t w, int16_t h),
    //drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h),
    //drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y),

    void drawChar(uint32_t x, uint32_t y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size);
    void writeChar(uint8_t c);
    void setCursor(uint32_t x, uint32_t y);
    void getTextBounds(const char *string, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    void getTextBounds(const uint8_t* s, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    void getTextBounds(StringBuilder*, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    void setTextSize(uint8_t s);
    void setTextColor(uint32_t c, uint32_t bg);
    void setTextColor(uint32_t c);

    inline void     setFont(const GFXfont* f) {   _gfxFont = (GFXfont*) f;    };
    inline void     setTextWrap(bool w) {         _wrap = w;                  };
    inline void     cp437(bool x) {               _cp437 = x;                 };
    inline uint32_t getCursorX() const {          return _cursor_x;           };
    inline uint32_t getCursorY() const {          return _cursor_y;           };
    /* END ADAFRUIT GFX SPLICE */


  protected:
    uint32_t        _x        = 0;
    uint32_t        _y        = 0;
    uint8_t*        _buffer   = nullptr;
    ImgBufferFormat _buf_fmt  = ImgBufferFormat::UNALLOCATED;

    uint8_t _bits_per_pixel();

    inline bool  _is_dirty() {        return _img_flag(MANUVR_IMG_FLAG_IS_FB_DIRTY);   };
    inline void  _is_dirty(bool l) {  _img_set_flag(MANUVR_IMG_FLAG_IS_FB_DIRTY, l);   };
    inline void  _is_framebuffer(bool l) {  _img_set_flag(MANUVR_IMG_FLAG_IS_FRAMEBUFFER, l);   };
    inline bool  _locked() {          return _img_flag(MANUVR_IMG_FLAG_BUFFER_LOCKED); };
    inline void  _lock(bool l) {      _img_set_flag(MANUVR_IMG_FLAG_BUFFER_LOCKED, l); };


  private:
    uint8_t  _imgflags = 0;

    /* BEGIN ADAFRUIT GFX SPLICE */
    GFXfont* _gfxFont;        ///< Pointer to font
    uint32_t _cursor_x;       ///< x location to start print()ing text
    uint32_t _cursor_y;       ///< y location to start print()ing text
    uint32_t _textcolor;      ///< 16-bit background color for print()
    uint32_t _textbgcolor;    ///< 16-bit text color for print()
    uint8_t  _textsize;       ///< Desired magnification of text to print()
    bool     _wrap;           ///< TODO: MERGE INTO FLAGS.  If set, 'wrap' text at right edge of display
    bool     _cp437;          ///< TODO: MERGE INTO FLAGS.  If set, use correct CP437 charset (default is off)

    void charBounds(char c, uint32_t* x, uint32_t* y, uint32_t* minx, uint32_t* miny, uint32_t* maxx, uint32_t* maxy);
    /* END ADAFRUIT GFX SPLICE */


    /* Linearizes the X/y value in preparation for array indexing. */
    inline uint32_t _pixel_number(uint32_t x, uint32_t y) {
      return ((y * _x) + x);
    };

    /* Linearizes the X/y value accounting for color format. */
    inline uint32_t _calculate_offset(uint32_t x, uint32_t y) {
      return ((((y * _x) + x) * _bits_per_pixel()) >> 8);
    };

    inline bool  _is_ours() {     return _img_flag(MANUVR_IMG_FLAG_BUFFER_OURS);     };
    inline void  _ours(bool l) {  _img_set_flag(MANUVR_IMG_FLAG_BUFFER_OURS, l);     };

    inline uint8_t _img_flags() {                return _imgflags;            };
    inline bool _img_flag(uint8_t _flag) {       return (_imgflags & _flag);  };
    inline void _img_flip_flag(uint8_t _flag) {  _imgflags ^= _flag;          };
    inline void _img_clear_flag(uint8_t _flag) { _imgflags &= ~_flag;         };
    inline void _img_set_flag(uint8_t _flag) {   _imgflags |= _flag;          };
    inline void _img_set_flag(uint8_t _flag, bool nu) {
      if (nu) _imgflags |= _flag;
      else    _imgflags &= ~_flag;
    };
};

#endif   // __MANUVR_TYPE_IMG_H
