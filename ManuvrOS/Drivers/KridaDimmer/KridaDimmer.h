/*
File:   KridaDimmer.h
Author: J. Ian Lindsay
Date:   2018.11.20

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


Driver for the Krida four-channel I2C dimmer.
*/

#include <inttypes.h>
#include <stdint.h>
#include <Platform/Platform.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#ifndef __KRIDA_4CH_DIMMER_DRIVER_H__
#define __KRIDA_4CH_DIMMER_DRIVER_H__

#define MANUVR_MSG_DIMMER_SET_LEVEL  0x79B0
#define MANUVR_MSG_DIMMER_TOGGLE     0x79B1
#define MANUVR_MSG_DIMMER_FADE_DOWN  0x79B2
#define MANUVR_MSG_DIMMER_FADE_UP    0x79B3


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class KridaDimmerOpts {
  public:
    const uint8_t ssr_a_pin;
    const uint8_t ssr_b_pin;
    const uint8_t dimmer_addr;

    KridaDimmerOpts(const KridaDimmerOpts* o) :
      ssr_a_pin(o->ssr_a_pin),
      ssr_b_pin(o->ssr_b_pin),
      dimmer_addr(o->dimmer_addr) {};

    KridaDimmerOpts(uint8_t _a_pin, uint8_t _b_pin, uint8_t _daddr) :
      ssr_a_pin(_a_pin),
      ssr_b_pin(_b_pin),
      dimmer_addr(_daddr) {};


  private:
};



class KridaDimmer : public EventReceiver,
  #ifdef MANUVR_CONSOLE_SUPPORT
    public ConsoleInterface,
  #endif
    I2CDevice {
  public:
    KridaDimmer(const KridaDimmerOpts* opts);
    ~KridaDimmer();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);

    #ifdef MANUVR_CONSOLE_SUPPORT
      /* Overrides from ConsoleInterface */
      uint consoleGetCmds(ConsoleCommand**);
      inline const char* consoleName() { return "KridaDimmer";  };
      void consoleCmdProc(StringBuilder* input);
    #endif  //MANUVR_CONSOLE_SUPPORT

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    /* Direct channel manipulation. */
    void enable_channel(uint8_t, bool);
    bool channel_enabled(uint8_t);

    /* Dimmer breakouts. */
    void set_brightness(uint8_t, uint8_t);
    void pulse_channel(uint8_t chan, uint8_t brightness, uint32_t period, int16_t recurrence);
    void fade_channel(uint8_t chan, bool up);


  protected:
    int8_t attached();


  private:
    const KridaDimmerOpts _opts;
    uint8_t    _channel_hw[4];      // Last-known value in the hardware.
    uint8_t    _channel_buffer[4];  // I/O operations are conducted against these values.
    uint8_t    _channel_values[4];  // Desired channel values.
    uint8_t    _channel_minima[4];  // Minimum values for each channel.
    uint8_t    _channel_maxima[4];  // Maximum values for each channel.
    ManuvrMsg* _channel_timers[4];  // Slots for schedules on each channel.
    bool _ssr_a_enabled = false;
    bool _ssr_b_enabled = false;


    bool channel_has_outstanding_io(uint8_t chan);

    inline bool is_channel_valid(uint8_t chan) {
      return (chan < 6);
    };
};


#endif   // __KRIDA_4CH_DIMMER_DRIVER_H__
