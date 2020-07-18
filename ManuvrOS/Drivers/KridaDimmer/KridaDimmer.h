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
#include <I2CAdapter.h>

#ifndef __KRIDA_4CH_DIMMER_DRIVER_H__
#define __KRIDA_4CH_DIMMER_DRIVER_H__



class KridaDimmer : public I2CDevice {
  public:
    KridaDimmer(uint8_t addr);
    ~KridaDimmer();

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);


    /* Direct channel manipulation. */
    virtual void enable_channel(uint8_t, bool);
    virtual bool channel_enabled(uint8_t);
    virtual void set_brightness(uint8_t, uint8_t);


  protected:
    int8_t attached();

    bool channel_has_outstanding_io(uint8_t chan);
    void printChannel(uint8_t chan, StringBuilder*);


  private:
    uint8_t    _channel_hw[4];      // Last-known value in the hardware.
    uint8_t    _channel_buffer[4];  // I/O operations are conducted against these values.
    uint8_t    _channel_values[4];  // Desired channel values.
    uint8_t    _channel_minima[4];  // Minimum values for each channel.
    uint8_t    _channel_maxima[4];  // Maximum values for each channel.

    inline bool is_channel_valid(uint8_t chan) {    return (chan < 4);   };
};


#endif   // __KRIDA_4CH_DIMMER_DRIVER_H__
