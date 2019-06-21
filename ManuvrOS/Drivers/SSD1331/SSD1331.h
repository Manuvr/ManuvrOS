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


#include <Types/Image.h>
#include <Platform/Peripherals/SPI/SPIBusOp.h>

// Timing Delays
#define SSD1331_DELAYS_HWFILL     (3)
#define SSD1331_DELAYS_HWLINE     (1)

// SSD1331 Commands
#define SSD1331_CMD_DRAWLINE       0x21
#define SSD1331_CMD_DRAWRECT       0x22
#define SSD1331_CMD_FILL           0x26
#define SSD1331_CMD_SETCOLUMN      0x15
#define SSD1331_CMD_SETROW         0x75
#define SSD1331_CMD_CONTRASTA      0x81
#define SSD1331_CMD_CONTRASTB      0x82
#define SSD1331_CMD_CONTRASTC      0x83
#define SSD1331_CMD_MASTERCURRENT  0x87
#define SSD1331_CMD_SETREMAP       0xA0
#define SSD1331_CMD_STARTLINE      0xA1
#define SSD1331_CMD_DISPLAYOFFSET  0xA2
#define SSD1331_CMD_NORMALDISPLAY  0xA4
#define SSD1331_CMD_DISPLAYALLON   0xA5
#define SSD1331_CMD_DISPLAYALLOFF  0xA6
#define SSD1331_CMD_INVERTDISPLAY  0xA7
#define SSD1331_CMD_SETMULTIPLEX   0xA8
#define SSD1331_CMD_SETMASTER      0xAD
#define SSD1331_CMD_DISPLAYOFF     0xAE
#define SSD1331_CMD_DISPLAYON      0xAF
#define SSD1331_CMD_POWERMODE      0xB0
#define SSD1331_CMD_PRECHARGE      0xB1
#define SSD1331_CMD_CLOCKDIV       0xB3
#define SSD1331_CMD_PRECHARGEA     0x8A
#define SSD1331_CMD_PRECHARGEB     0x8B
#define SSD1331_CMD_PRECHARGEC     0x8C
#define SSD1331_CMD_PRECHARGELEVEL 0xBB
#define SSD1331_CMD_VCOMH          0xBE

#define SSD1331_MAX_Q_DEPTH   4

/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class SSD1331Opts {
  public:
    SSD1331Opts(const SSD1331Opts* p) :
      orientation(p->orientation),
      reset(p->reset),
      dc(p->dc),
      spi_cs(p->spi_cs),
      spi_clk(p->spi_clk),
      spi_mosi(p->spi_mosi),
      spi_miso(p->spi_miso)
    {};

    SSD1331Opts(
      ImgOrientation _or,
      uint8_t _reset,
      uint8_t _dc,
      uint8_t _spi_cs,
      uint8_t _spi_clk,
      uint8_t _spi_mosi,
      uint8_t _spi_miso
    ) :
      orientation(_or),
      reset(_reset),
      dc(_dc),
      spi_cs(_spi_cs),
      spi_clk(_spi_clk),
      spi_mosi(_spi_mosi),
      spi_miso(_spi_miso)
    {};

    const ImgOrientation orientation;  //
    const uint8_t        reset;        //
    const uint8_t        dc;           //
    const uint8_t        spi_cs;       //
    const uint8_t        spi_clk;      //
    const uint8_t        spi_mosi;     //
    const uint8_t        spi_miso;     //
};





/// Class to manage hardware interface with SSD1331 chipset
class SSD1331 : public Image, public BusAdapter<SPIBusOp> {
  public:
    SSD1331(const SSD1331Opts* opts);
    ~SSD1331();

    // commands
    void begin(uint32_t begin=8000000);

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    void enableDisplay(bool enable);

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);
    int8_t advance_work_queue();
    int8_t bus_init();
    int8_t bus_deinit();
    SPIBusOp* new_op();
    SPIBusOp* new_op(BusOpcode, BusOpCallback*);


  private:
    const SSD1331Opts _opts;

    void initSPI(uint8_t cpol, uint8_t cpha);

    int8_t _send_command(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes);
    inline int8_t _send_command(uint8_t commandByte) {
      return _send_command(commandByte, nullptr, 0);
    };

    void purge_queued_work();
    void purge_stalled_job();
    void reclaim_queue_item(SPIBusOp*);
};
