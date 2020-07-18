/*
File:   LDS8160.h
Author: J. Ian Lindsay
Date:   2014.05.27

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

#ifndef __LDS8160_LED_DRIVER_H__
#define __LDS8160_LED_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include <Kernel.h>
#include <I2CAdapter.h>



#define LDS8160_BANK_A_CURRENT     0x00
#define LDS8160_BANK_B_CURRENT     0x01
#define LDS8160_BANK_C_CURRENT     0x02
#define LDS8160_CHANNEL_ENABLE     0x03
#define LDS8160_GLOBAL_PWM_DIM     0x04
#define LDS8160_BANK_A_PWM_DUTY    0x05
#define LDS8160_BANK_B_PWM_DUTY    0x06
#define LDS8160_BANK_C_PWM_DUTY    0x07
#define LDS8160_TEST_MODE          0x19
#define LDS8160_LED_SHORT_GND      0x1C
#define LDS8160_LED_FAULT          0x1D
#define LDS8160_CONFIG_REG         0x1E
#define LDS8160_SOFTWARE_RESET     0x1F
#define LDS8160_TEMPERATURE_OFFSET 0x49
#define LDS8160_LED_SHUTDOWN_TEMP  0x4A
#define LDS8160_TABLE_ENABLE_BP    0x4B
/* Skipped def of LUT registers */
#define LDS8160_SI_DIODE_DV_DT     0xA0
/* Skipped def of dV/dT registers */

#define LDS8160_CHAN_MASK_0        0x01
#define LDS8160_CHAN_MASK_1        0x02
#define LDS8160_CHAN_MASK_2        0x04
#define LDS8160_CHAN_MASK_3        0x08
#define LDS8160_CHAN_MASK_4        0x10
#define LDS8160_CHAN_MASK_5        0x20


#define LDS8160_I2CADDR        0x11



class LDS8160 : public I2CDeviceWithRegisters, public EventReceiver {
  public:
    LDS8160(uint8_t addr = LDS8160_I2CADDR);
    ~LDS8160();

    int8_t init();


    /* Overrides from I2CDeviceWithRegisters... */
    int8_t io_op_callback(I2CBusOp*);
    void printDebug(StringBuilder*);

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    void procDirectDebugInstruction(StringBuilder *);

    /* Direct channel manipulation. */
    void enable_channel(uint8_t, bool);
    bool channel_enabled(uint8_t);

    /* Dimmer breakouts. */
    void set_brightness(uint8_t);
    void toggle_brightness(void);

    void quell_all_timers();
    void set_led_mode(uint8_t num);


  protected:
    int8_t attached();


  private:
    bool init_complete        = false;
    uint8_t power_mode        = 0;
    uint8_t class_mode        = 0;
    uint8_t stored_dimmer_val = 0;


    uint32_t pid_intensity    = 0;
    uint32_t pid_channel_0    = 0;
    uint32_t pid_channel_1    = 0;
    uint32_t pid_channel_2    = 0;
    uint32_t pid_channel_3    = 0;
    uint32_t pid_channel_4    = 0;
    uint32_t pid_channel_5    = 0;



    void reset();
    void set_power_mode(uint8_t);
};

#endif
