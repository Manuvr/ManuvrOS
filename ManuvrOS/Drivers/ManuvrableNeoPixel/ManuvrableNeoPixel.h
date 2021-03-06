/**
* This class began life as AdaFruit's NeoPixel driver. I have adapted it
*   to ManuvrOS, changed the style, and removed things that don't need to
*   be supported.
* Eventually, this class will drive the pixels via the SPI.
*
*        ---J. Ian Lindsay   Fri Mar 13 14:32:52 MST 2015
*/

/*--------------------------------------------------------------------
  This file is part of the Adafruit NeoPixel library.

  NeoPixel is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  NeoPixel is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NeoPixel.  If not, see
  <http://www.gnu.org/licenses/>.
  --------------------------------------------------------------------*/

#ifndef MANUVRABLE_NEOPIXEL_H
#define MANUVRABLE_NEOPIXEL_H

#include <EventReceiver.h>

// 'type' flags for LED pixels (third parameter to constructor):
#define NEO_GRB     0x01 // Wired for GRB data order
#define NEO_COLMASK 0x01
#define NEO_KHZ800  0x02 // 800 KHz datastream
#define NEO_SPDMASK 0x02

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define NEOPIXEL_FLAG_AUTOBRIGHTNESS     0x01   // Is the device initialized?


class ManuvrableNeoPixel : public EventReceiver {
  public:
    // Constructor: number of LEDs, pin number, LED type
    ManuvrableNeoPixel(uint16_t n, uint8_t p=6, uint8_t t=NEO_GRB + NEO_KHZ800);
    ~ManuvrableNeoPixel();

    void colorWipe(uint32_t c, uint8_t wait);
    void rainbow(uint8_t wait);
    void rainbowCycle(uint8_t wait);
    uint32_t Wheel(uint8_t WheelPos);

    void begin(void);
    void show(void);
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint16_t n, uint32_t c);
    void setBrightness(uint8_t);
    uint8_t* getPixels() const;
    uint16_t numPixels(void) const;
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
    uint32_t getPixelColor(uint16_t n) const;

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT

    inline bool autoBrightness() {         return (_er_flag(NEOPIXEL_FLAG_AUTOBRIGHTNESS));           };
    inline void autoBrightness(bool nu) {  return (_er_set_flag(NEOPIXEL_FLAG_AUTOBRIGHTNESS, nu));   };


  protected:
    int8_t attached();


  private:
    uint32_t endTime;         // Latch timing reference
    uint16_t numLEDs;   // Number of RGB LEDs in strip
    uint16_t numBytes;  // Size of 'pixels' buffer below
    uint8_t  mode;
    uint8_t  pin;             // Output pin number
    uint8_t  brightness;
    uint8_t  *pixels;         // Holds LED color values (3 bytes each)
};

#endif // MANUVRABLE_NEOPIXEL_H
