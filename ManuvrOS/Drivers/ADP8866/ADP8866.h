/*
File:   ADP8866.h
Author: J. Ian Lindsay
Date:   2015.10.27

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

*/

#ifndef __ADP8866_LED_DRIVER_H__
#define __ADP8866_LED_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include <Kernel.h>
#include "Drivers/i2c-adapter/i2c-adapter.h"


// These are the register definitions.
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

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define ADP8866_FLAG_INIT_COMPLETE 0x01    // Is the device initialized?


/*
* Internal register bitmask definitions.
*/
#define ADP8866_INT_STATUS_OV      0x04
#define ADP8866_INT_STATUS_OT      0x08
#define ADP8866_INT_STATUS_ICS_OFF 0x40
#define ADP8866_INT_STATUS_BL_OFF  0x20
#define ADP8866_INT_STATUS_SHORT   0x10


/*
* Used to aggregate channel information into a single place. Makes drive code more readable.
*/
typedef struct adp8866_led_chan {
  float          max_current;
  uint8_t        present;
  uint8_t        flags;
  uint16_t       reserved;     // Reserved for later use.
} ADPLEDChannel;



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
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    void procDirectDebugInstruction(StringBuilder *);

    /* Direct channel manipulation. */
    void enable_channel(uint8_t, bool);
    bool channel_enabled(uint8_t);

    /* Dimmer breakouts. */
    void set_brightness(uint8_t, uint8_t);
    void set_brightness(uint8_t);
    void pulse_channel(uint8_t, uint8_t);

    void toggle_brightness(void);

    void quell_all_timers();
    void set_led_mode(uint8_t num);

    void _isr_fxn(void);


    static ADP8866* INSTANCE;


  protected:
    int8_t bootComplete();
    void __class_initializer();


  private:
    uint8_t power_mode;
    uint8_t class_mode;
    uint8_t stored_dimmer_val;
    uint8_t reset_pin;
    uint8_t irq_pin;

    // The chip only has 9 outputs, but we make a synthetic tenth
    //   channel to represent the backlight.
    ADPLEDChannel channels[10];

    void reset();
    void set_power_mode(uint8_t);
};

#endif
