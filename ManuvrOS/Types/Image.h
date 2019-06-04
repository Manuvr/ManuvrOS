/*
File:   Image.h
Author: J. Ian Lindsay
Date:   2019.06.02

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


___________________________________
| 0, 1, 2...   x -->
| 1
| 2
|
| y
| |
| V
|

*/


#include <CommonConstants.h>
#include <EnumeratedTypeCodes.h>


#ifndef __MANUVR_TYPE_IMG_H
#define __MANUVR_TYPE_IMG_H

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(MANUVR_DEBUG)
  #include <DataStructures/StringBuilder.h>
#endif



#define MANUVR_IMG_FLAG_BUFFER_OURS     0x01  // We are responsible for freeing the buffer.
#define MANUVR_IMG_FLAG_BUFFER_LOCKED   0x02  // Buffer should not be modified when set.


enum class ImgBufferFormat : uint8_t {
  UNALLOCATED = 0x00,  // Buffer unallocated
  MONOCHROME  = 0x01,  // Monochrome
  GREY_24     = 0x02,  // 24-bit greyscale
  GREY_16     = 0x03,  // 16-bit greyscale
  GREY_8      = 0x04,  // 8-bit greyscale
  R8_G8_B8    = 0x05,  // 24-bit color
  R5_G6_B5    = 0x06,  // 16-bit color
  R2_G3_B3    = 0x07   // 8-bit color
};


class Image {
  public:
    Image(unsigned int x, unsigned int y, ImgBufferFormat, uint8_t*);
    Image(unsigned int x, unsigned int y, ImgBufferFormat);
    Image(unsigned int x, unsigned int y);
    Image();
    ~Image();

    bool setBuffer(uint8_t*);
    bool setBuffer(uint8_t*, ImgBufferFormat);
    bool setBufferByCopy(uint8_t*);
    bool setBufferByCopy(uint8_t*, ImgBufferFormat);

    bool isColor();

    uint32_t getColor(unsigned int x, unsigned int y);
    bool     setColor(unsigned int x, unsigned int y, uint32_t);
    bool     setColor(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);

    inline unsigned int    x() {          return _x;                     };
    inline unsigned int    y() {          return _y;                     };
    inline uint8_t*        buffer() {     return _buffer;                };
    inline ImgBufferFormat format() {     return _buf_fmt;               };
    inline bool            allocated() {  return (nullptr != _buffer);   };
    inline unsigned int    pixels() {     return (_x * _y);              };
    inline unsigned int    bytesUsed() {  return (_x * _y * _bits_per_pixel() >> 8);  };



  private:
    unsigned int    _x       = 0;
    unsigned int    _y       = 0;
    uint8_t*        _buffer  = nullptr;
    ImgBufferFormat _buf_fmt = ImgBufferFormat::UNALLOCATED;
    uint8_t         _flags   = 0;

    uint8_t _bits_per_pixel();

    /* Linearizes the X/y value in preparation for array indexing. */
    inline unsigned int _pixel_number(unsigned int x, unsigned int y) {
      return ((y * _x) + x);
    };

    /* Linearizes the X/y value accounting for color format. */
    inline unsigned int _calculate_offset(unsigned int x, unsigned int y) {
      return ((((y * _x) + x) * _bits_per_pixel()) >> 8);
    };

    inline bool  _is_ours() {     return _img_flag(MANUVR_IMG_FLAG_BUFFER_OURS);     };
    inline void  _ours(bool l) {  _img_set_flag(MANUVR_IMG_FLAG_BUFFER_OURS, l);     };
    inline bool  _locked() {      return _img_flag(MANUVR_IMG_FLAG_BUFFER_LOCKED);   };
    inline void  _lock(bool l) {  _img_set_flag(MANUVR_IMG_FLAG_BUFFER_LOCKED, l);   };

    inline uint8_t _img_flags() {                return _flags;            };
    inline bool _img_flag(uint8_t _flag) {       return (_flags & _flag);  };
    inline void _img_flip_flag(uint8_t _flag) {  _flags ^= _flag;          };
    inline void _img_clear_flag(uint8_t _flag) { _flags &= ~_flag;         };
    inline void _img_set_flag(uint8_t _flag) {   _flags |= _flag;          };
    inline void _img_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};

#endif   // __MANUVR_TYPE_IMG_H
