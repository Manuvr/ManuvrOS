/***************************************************
  This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

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

#include "SSD1331.h"

#if defined(CONFIG_MANUVR_SSD1331)


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Constructor.
*/
SSD1331::SSD1331(const SSD1331Opts* o) : Image(o->width, o->height, ImgBufferFormat::R5_G6_B5), BusAdapter(4), _opts(o) {
}


/**
* Destructor. Should never be called.
*/
SSD1331::~SSD1331() {
}









/*!
  @brief   SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
void SSD1331::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {

  uint8_t x1 = x;
  uint8_t y1 = y;
  if (x1 > 95) x1 = 95;
  if (y1 > 63) y1 = 63;

  uint8_t x2 = (x+w-1);
  uint8_t y2 = (y+h-1);
  if (x2 > 95) x2 = 95;
  if (y2 > 63) y2 = 63;

  if (x1 > x2) {
    uint8_t t = x2;
    x2 = x1;
    x1 = t;
  }
  if (y1 > y2) {
    uint8_t t = y2;
    y2 = y1;
    y1 = t;
  }

  sendCommand(0x15); // Column addr set
  sendCommand(x1);
  sendCommand(x2);

  sendCommand(0x75); // Column addr set
  sendCommand(y1);
  sendCommand(y2);

  startWrite();
}


/**************************************************************************/
/*!
    @brief   Initialize SSD1331 chip
    Connects to the SSD1331 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/
void SSD1331::begin(uint32_t freq) {
    initSPI(freq);

    // Initialization Sequence
    sendCommand(SSD1331_CMD_DISPLAYOFF);    // 0xAE
    sendCommand(SSD1331_CMD_SETREMAP);   // 0xA0
    sendCommand(0x72);        // RGB Color
    sendCommand(SSD1331_CMD_STARTLINE);   // 0xA1
    sendCommand(0x0);
    sendCommand(SSD1331_CMD_DISPLAYOFFSET);   // 0xA2
    sendCommand(0x0);
    sendCommand(SSD1331_CMD_NORMALDISPLAY);    // 0xA4
    sendCommand(SSD1331_CMD_SETMULTIPLEX);   // 0xA8
    sendCommand(0x3F);        // 0x3F 1/64 duty
    sendCommand(SSD1331_CMD_SETMASTER);    // 0xAD
    sendCommand(0x8E);
    sendCommand(SSD1331_CMD_POWERMODE);    // 0xB0
    sendCommand(0x0B);
    sendCommand(SSD1331_CMD_PRECHARGE);    // 0xB1
    sendCommand(0x31);
    sendCommand(SSD1331_CMD_CLOCKDIV);    // 0xB3
    sendCommand(0xF0);  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    sendCommand(SSD1331_CMD_PRECHARGEA);    // 0x8A
    sendCommand(0x64);
    sendCommand(SSD1331_CMD_PRECHARGEB);    // 0x8B
    sendCommand(0x78);
    sendCommand(SSD1331_CMD_PRECHARGEC);    // 0x8C
    sendCommand(0x64);
    sendCommand(SSD1331_CMD_PRECHARGELEVEL);    // 0xBB
    sendCommand(0x3A);
    sendCommand(SSD1331_CMD_VCOMH);      // 0xBE
    sendCommand(0x3E);
    sendCommand(SSD1331_CMD_MASTERCURRENT);    // 0x87
    sendCommand(0x06);
    sendCommand(SSD1331_CMD_CONTRASTA);    // 0x81
    sendCommand(0x91);
    sendCommand(SSD1331_CMD_CONTRASTB);    // 0x82
    sendCommand(0x50);
    sendCommand(SSD1331_CMD_CONTRASTC);    // 0x83
    sendCommand(0x7D);
    sendCommand(SSD1331_CMD_DISPLAYON);  //--turn on oled panel
    _x  = TFTWIDTH;
    _y = TFTHEIGHT;
}



/*!
  @brief  Change whether display is on or off
  @param   enable True if you want the display ON, false OFF
*/
void SSD1331::enableDisplay(bool enable) {
  sendCommand(enable ? SSD1331_CMD_DISPLAYON : SSD1331_CMD_DISPLAYOFF);
}

#endif   // CONFIG_MANUVR_SSD1331
