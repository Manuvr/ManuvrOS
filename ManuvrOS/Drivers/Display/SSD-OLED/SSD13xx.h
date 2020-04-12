/***************************************************
This is a library for the 0.96" 16-bit Color OLED with SSD13XX driver chip
Pick one up today in the adafruit shop!
------> http://www.adafruit.com/products/684
These displays use SPI to communicate, 4 or 5 pins are required to
interface
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, all text above must be included in any redistribution


Software License Agreement (BSD License)

Copyright (c) 2012, Adafruit Industries
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holders nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

****************************************************/


#include <Types/Image.h>
#include <Platform/Peripherals/SPI/SPIAdapter.h>
#include <Platform/Peripherals/SPI/SPIBusOp.h>

// Timing Delays
#define SSD13XX_DELAYS_HWFILL     (3)
#define SSD13XX_DELAYS_HWLINE     (1)

// SSD13XX Commands
#define SSD13XX_CMD_DRAWLINE       0x21
#define SSD13XX_CMD_DRAWRECT       0x22
#define SSD13XX_CMD_FILL           0x26
#define SSD13XX_CMD_SETCOLUMN      0x15
#define SSD13XX_CMD_SETROW         0x75
#define SSD13XX_CMD_CONTRASTA      0x81
#define SSD13XX_CMD_CONTRASTB      0x82
#define SSD13XX_CMD_CONTRASTC      0x83
#define SSD13XX_CMD_MASTERCURRENT  0x87
#define SSD13XX_CMD_SETREMAP       0xA0
#define SSD13XX_CMD_STARTLINE      0xA1
#define SSD13XX_CMD_DISPLAYOFFSET  0xA2
#define SSD13XX_CMD_NORMALDISPLAY  0xA4
#define SSD13XX_CMD_DISPLAYALLON   0xA5
#define SSD13XX_CMD_DISPLAYALLOFF  0xA6
#define SSD13XX_CMD_INVERTDISPLAY  0xA7
#define SSD13XX_CMD_SETMULTIPLEX   0xA8
#define SSD13XX_CMD_SETMASTER      0xAD
#define SSD13XX_CMD_DISPLAYOFF     0xAE
#define SSD13XX_CMD_DISPLAYON      0xAF
#define SSD13XX_CMD_POWERMODE      0xB0
#define SSD13XX_CMD_PRECHARGE      0xB1
#define SSD13XX_CMD_CLOCKDIV       0xB3
#define SSD13XX_CMD_PRECHARGEA     0x8A
#define SSD13XX_CMD_PRECHARGEB     0x8B
#define SSD13XX_CMD_PRECHARGEC     0x8C
#define SSD13XX_CMD_PRECHARGELEVEL 0xBB
#define SSD13XX_CMD_VCOMH          0xBE

/* Supported SSD chipsets */
enum class SSDModel : uint8_t {
  SSD1306  = 0,
  SSD1309  = 1,
  SSD1331  = 2,
  SSD1351  = 3
};


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class SSD13xxOpts {
  public:
    SSD13xxOpts(const SSD13xxOpts* p) :
      orientation(p->orientation),
      reset(p->reset),
      dc(p->dc),
      cs(p->cs),
      model(p->model)
    {};

    SSD13xxOpts(
      ImgOrientation _or,
      uint8_t _reset,
      uint8_t _dc,
      uint8_t _cs,
      SSDModel _m
    ) :
      orientation(_or),
      reset(_reset),
      dc(_dc),
      cs(_cs),
      model(_m)
    {};

    const ImgOrientation orientation;  //
    const uint8_t        reset;        //
    const uint8_t        dc;           //
    const uint8_t        cs;           //
    const SSDModel       model;        //
};



// Class to manage hardware interface with SSD13xx chipset
class SSD13xx : public Image, public BusOpCallback {
  public:
    SSD13xx(const SSD13xxOpts* opts);
    ~SSD13xx();

    // commands
    int8_t init(SPIAdapter*);
    inline bool enabled() {   return _enabled;  };

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    int8_t commitFrameBuffer();

    void enableDisplay(bool enable);
    void printDebug(StringBuilder*);

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);


  private:
    const SSD13xxOpts _opts;
    bool  _enabled  = false;
    SPIAdapter* _BUS = nullptr;
    SPIBusOp    _fb_data_op;

    int8_t _send_data(uint8_t* buf, uint16_t len);
    int8_t _ll_pin_init();

    int8_t _send_command(uint8_t commandByte, uint8_t* dataBytes, uint8_t numDataBytes);
    inline int8_t _send_command(uint8_t commandByte) {
      return _send_command(commandByte, nullptr, 0);
    };
};
