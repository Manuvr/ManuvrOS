/*
File:   ADP8866.h
Author: J. Ian Lindsay
Date:   2015.10.27


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef __ADP8866_LED_DRIVER_H__
#define __ADP8866_LED_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include "ManuvrOS/EventManager.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"



#define ADP8866_MANU_DEV_ID        0x00
#define ADP8866_MDCR               0x01
#define ADP8866_INT_STAT           0x02
#define ADP8866_INT_EN             0x03
#define ADP8866_ISCOFF_SEL_1       0x04
#define ADP8866_ISCOFF_SEL_2       0x05
#define ADP8866_GAIN_SEL           0x06
#define ADP8866_LVL_SEL_1          0x07
#define ADP8866_LVL_SEL_2          0x08
#define ADP8866_PWR_SEL_1          0x09
#define ADP8866_PWR_SEL_2          0x0A
#define ADP8866_CFGR               0x10
#define ADP8866_BLSEL              0x11
#define ADP8866_BLFR               0x12
#define ADP8866_BLMX               0x13
#define ADP8866_ISCC1              0x1A
#define ADP8866_ISCC2              0x1B
#define ADP8866_ISCT1              0x1C
#define ADP8866_ISCT2              0x1D
#define ADP8866_OFFTIMER6          0x1E
#define ADP8866_OFFTIMER7          0x1F
#define ADP8866_OFFTIMER8          0x20
#define ADP8866_OFFTIMER9          0x21
#define ADP8866_ISCF               0x22
#define ADP8866_ISC1               0x23
#define ADP8866_ISC2               0x24
#define ADP8866_ISC3               0x25
#define ADP8866_ISC4               0x26
#define ADP8866_ISC5               0x27
#define ADP8866_ISC6               0x28
#define ADP8866_ISC7               0x29
#define ADP8866_ISC8               0x2A
#define ADP8866_ISC9               0x2B
#define ADP8866_HB_SEL             0x2C
#define ADP8866_ISC6_HB            0x2D
#define ADP8866_ISC7_HB            0x2E
#define ADP8866_ISC8_HB            0x2F
#define ADP8866_ISC9_HB            0x30
#define ADP8866_OFFTIMER6_HB       0x31
#define ADP8866_OFFTIMER7_HB       0x32
#define ADP8866_OFFTIMER8_HB       0x33
#define ADP8866_OFFTIMER9_HB       0x34
#define ADP8866_ISCT_HB            0x35
#define ADP8866_DELAY6             0x3C
#define ADP8866_DELAY7             0x3D
#define ADP8866_DELAY8             0x3E
#define ADP8866_DELAY9             0x3F


#define ADP8866_I2CADDR            0x27



class ADP8866 : public I2CDeviceWithRegisters, public EventReceiver {
  public:
    ADP8866(uint8_t reset_pin, uint8_t irq_pin, uint8_t addr = ADP8866_I2CADDR);
    ~ADP8866(void);

    int8_t init(void);


    /* Overrides from I2CDeviceWithRegisters... */
    void operationCompleteCallback(I2CQueuedOperation*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);
    void procDirectDebugInstruction(StringBuilder *);

    /* Direct channel manipulation. */
    void enable_channel(uint8_t, bool);
    bool channel_enabled(uint8_t);

    /* Dimmer breakouts. */
    void set_brightness(uint8_t, uint8_t);
    void set_brightness(uint8_t);
    void toggle_brightness(void);
    
    void quell_all_timers();
    void set_led_mode(uint8_t num);


  protected:
    int8_t bootComplete();


  private:
    bool init_complete        = false;
    uint8_t power_mode        = 0;
    uint8_t class_mode        = 0;
    uint8_t stored_dimmer_val = 0;
    uint8_t reset_pin         = 0;
    uint8_t irq_pin           = 0;
    
    void reset();
    void set_power_mode(uint8_t);
};

#endif

