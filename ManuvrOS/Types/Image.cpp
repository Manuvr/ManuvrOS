#include <stdlib.h>
#include "Image.h"

#if defined(CONFIG_MANUVR_IMG_SUPPORT)


Image::Image(uint32_t x, uint32_t y, ImgBufferFormat fmt, uint8_t* buf) {
  _x       = x;
  _y       = y;
  _buf_fmt = fmt;
  _flags   = 0;
  _buffer  = buf;
}


Image::Image(uint32_t x, uint32_t y, ImgBufferFormat fmt) {
  _x       = x;
  _y       = y;
  _buf_fmt = fmt;
  _flags   = MANUVR_IMG_FLAG_BUFFER_OURS;
  _buffer  = (uint8_t*) malloc(bytesUsed());
}


Image::Image(uint32_t x, uint32_t y) {
  _x = x;
  _y = y;
}


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
* Takes the given buffer and  format and assume it as our own.
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
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      uint32_t sz = bytesUsed();
      memcpy(_buffer, buf, sz);
      return true;
    }
  }
  return false;
}


/**
* TODO: Need to audit memory management here. Unit testing is clearly warranted.
*/
bool Image::setBufferByCopy(uint8_t* buf, ImgBufferFormat fmt) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (fmt == _buf_fmt) {
      if (allocated()) {
        uint32_t sz = bytesUsed();
        memcpy(_buffer, buf, sz);
        return true;
      }
    }
    else {
      if (_is_ours() && allocated()) {
        free(_buffer);
        _buf_fmt = fmt;
        uint32_t sz = bytesUsed();
        _buffer = (uint8_t*) malloc(sz);
        if (allocated()) {
          _ours(true);
          memcpy(_buffer, buf, sz);
          return true;
        }
      }
    }
  }
  else {
    _buf_fmt = fmt;
    uint32_t sz = bytesUsed();
    _buffer = (uint8_t*) malloc(bytesUsed());
    if (allocated()) {
      _ours(true);
      memcpy(_buffer, buf, sz);
      return true;
    }
  }
  return false;
}


/**
*
*/
void Image::wipe() {
  if (_is_ours() && allocated()) {
    free(_buffer);
  }
  _x       = 0;
  _y       = 0;
  _buffer  = nullptr;
  _buf_fmt = ImgBufferFormat::UNALLOCATED;
  _flags   = 0;
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
      buf[9] = _flags;
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
      *(buf + 9) = _flags;
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
      *(buf + 9) = _flags;
      *len = 10;
      return 0;
    }
  }
  return -1;
}


/**
* This function can only be run if there is not a buffer already allocated.
*/
int8_t Image::deserialize(uint8_t* buf, uint32_t len) {
  if (ImgBufferFormat::UNALLOCATED == _buf_fmt) {
    if (!allocated()) {
      _x = ((uint32_t) *(buf + 0) << 24) | ((uint32_t) *(buf + 1) << 16) | ((uint32_t) *(buf + 2) << 8) | ((uint32_t) *(buf + 3));
      _y = ((uint32_t) *(buf + 4) << 24) | ((uint32_t) *(buf + 5) << 16) | ((uint32_t) *(buf + 6) << 8) | ((uint32_t) *(buf + 7));
      _buf_fmt = (ImgBufferFormat) *(buf + 8);
      _flags   = *(buf + 9);
      uint32_t sz = bytesUsed();
      _buffer = (uint8_t*) malloc(sz);
      if (allocated()) {
        memcpy(_buffer, (buf + 10), sz);
        return 0;
      }
    }
  }
  return -1;
}


/**
*
*/
bool Image::isColor() {
  switch (_buf_fmt) {
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
    case ImgBufferFormat::R2_G3_B3:          // 8-bit color
      return true;
    default:
      break;
  }
  return false;
}


/**
*
*/
uint32_t Image::getColor(uint32_t x, uint32_t y) {
  switch (_buf_fmt) {
    case ImgBufferFormat::MONOCHROME:        // Monochrome
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
    case ImgBufferFormat::R2_G3_B3:          // 8-bit color
      break;
    case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
    default:
      return 0;
  }
  return 0;
}


/**
* Takes a color in 32-bit. Squeezes it into the buffer's format, discarding low bits as appropriate.
*/
bool Image::setColor(uint32_t x, uint32_t y, uint32_t c) {
  uint8_t r = (uint8_t) (c >> 16) & 0xFF;
  uint8_t g = (uint8_t) (c >> 8) & 0xFF;
  uint8_t b = (uint8_t) (c & 0xFF);
  switch (_buf_fmt) {
    case ImgBufferFormat::R2_G3_B3:          // 8-bit color
      r = r >> 3;
      g = g >> 3;
      b = b >> 2;
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
      r = r >> 3;
      g = g >> 2;
      b = b >> 3;
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
      return setColor(x, y, r, g, b);
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      return setColor(x, y, r, g, b);
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
bool Image::setColor(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t sz = bytesUsed();
  uint32_t offset = _calculate_offset(x, y);
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
        *(_buffer + offset + 0) = ((uint16_t) ((r >> 3) & 0xF1) << 8) | ((g >> 5) & 0x07);
        *(_buffer + offset + 1) = ((uint16_t) ((g >> 2) & 0xE0) << 8) | ((b >> 3) & 0x1F);
        break;
      case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      case ImgBufferFormat::R2_G3_B3:          // 8-bit color
        *(_buffer + offset) = ((uint8_t) (r >> 6) & 0x03) | ((uint8_t) (g >> 5) & 0x07) | ((uint8_t) (b >> 5) & 0x07);
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


/**
*
*/
uint8_t Image::_bits_per_pixel() {
  switch (_buf_fmt) {
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
      return 24;
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
      return 16;
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale
    case ImgBufferFormat::R2_G3_B3:          // 8-bit color
      return 8;
    case ImgBufferFormat::MONOCHROME:        // Monochrome
      return 1;
    case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
    default:
      return 0;
  }
}

#endif   // CONFIG_MANUVR_IMG_SUPPORT
