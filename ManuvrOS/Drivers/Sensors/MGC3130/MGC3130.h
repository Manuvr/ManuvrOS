/*
File:   MGC3130.h
Author: J. Ian Lindsay
Date:   2015.06.01

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


This class is a driver for Microchip's MGC3130 e-field gesture sensor. It is meant
  to be used in ManuvrOS. This driver was derived from my prior derivation of the
  Arduino Hover library. Nothing remains of the original library. My original fork
  can be found here:
  https://github.com/jspark311/hover_arduino

*/

#ifndef __MGC3130_H__
#define __MGC3130_H__

#include <inttypes.h>
#include <stdint.h>
#include <Kernel.h>
#include "Platform/Peripherals/I2C/I2CAdapter.h"

#define MGC3130_ISR_MARKER_TS 0x01
#define MGC3130_ISR_MARKER_G0 0x80
#define MGC3130_ISR_MARKER_G1 0x40
#define MGC3130_ISR_MARKER_G2 0x20
#define MGC3130_ISR_MARKER_G3 0x10

/* These are the bit-masks for our internal state-machines. */
#define MGC3130_GESTURE_ASSERTION_AW   0x01
#define MGC3130_GESTURE_ASSERTION_POS  0x02
#define MGC3130_FLAGS_CLASS_READY      0x40   // Set if the class is initialized.
#define MGC3130_GESTURE_TS_HELD_BY_US  0x80   // Set if this class is holding the TS line.


#define MGC3130_MINIMUM_NUANCE_PERIOD  80     // How many milliseconds need to pass between repitions.


#if defined(_BOARD_FUBARINO_MINI_)
  #define BOARD_IRQS_AND_PINS_DISTINCT 1
#endif


class StringBuilder;

class MGC3130 : public I2CDevice, public EventReceiver {
  public:
    uint8_t last_swipe;
    uint8_t last_tap;
    uint8_t last_double_tap;
    uint8_t last_touch;
    uint8_t last_touch_noted;
    uint8_t touch_counter;
    uint8_t special;

    int32_t _pos_x;
    int32_t _pos_y;
    int32_t _pos_z;
    int32_t wheel_position;

    MGC3130(int ts, int mclr, uint8_t addr = 0x42);
    void init();


    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    void procDirectDebugInstruction(StringBuilder *);
    void printDebug(StringBuilder*);


    const char* getTouchTapString(uint8_t eventByte);
    const char* getSwipeString(uint8_t eventByte);

    int8_t setIRQPin(uint8_t, int);

    void enableApproachDetect(bool);
    void enableAirwheel(bool);

    void set_isr_mark(uint8_t mask);
    inline bool isPositionDirty() { return ((_pos_x + _pos_y + _pos_z) != -3); };
    inline bool isTouchDirty() {    return (last_touch_noted != last_touch);   };

    void dispatchGestureEvents();

    volatile static MGC3130* INSTANCE;


  protected:
    int8_t attached();


  private:
    uint32_t last_nuance_sent;
    uint32_t pid_read_timer;
    uint8_t _ts_pin;      // Pin number being used by the TS pin.
    uint8_t _reset_pin;   // Pin number being used by the MCLR pin.
    uint8_t _irq_pin_0;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_1;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_2;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_3;   // Pin number being used by optional IRQ pin.

    uint8_t last_event;
    uint8_t last_seq_num;

    uint8_t power_mode;

    uint8_t flags;

    uint8_t read_buffer[40];
    uint8_t write_buffer[20];


    /*
    * Accessor for assertion-gating state-machine. (AirWheel)
    */
    inline bool airwheel_asserted() {         return (flags & MGC3130_GESTURE_ASSERTION_AW);  }
    inline void airwheel_asserted(bool en) {
      flags = (en) ? (flags | MGC3130_GESTURE_ASSERTION_AW) : (flags & ~(MGC3130_GESTURE_ASSERTION_AW));
    }

    /*
    * Accessor for assertion-gating state-machine. (Position)
    */
    inline bool position_asserted() {         return (flags & MGC3130_GESTURE_ASSERTION_POS);  }
    inline void position_asserted(bool en) {
      flags = (en) ? (flags | MGC3130_GESTURE_ASSERTION_POS) : (flags & ~(MGC3130_GESTURE_ASSERTION_POS));
    }

    /*
    * Accessor for TS hold tracking.
    */
    inline bool are_we_holding_ts() {         return (flags & MGC3130_GESTURE_TS_HELD_BY_US);  }
    inline void are_we_holding_ts(bool en) {
      flags = (en) ? (flags | MGC3130_GESTURE_TS_HELD_BY_US) : (flags & ~(MGC3130_GESTURE_TS_HELD_BY_US));
    }

    /*
    * Accessor for TS hold tracking.
    */
    inline bool is_class_ready() {         return (flags & MGC3130_FLAGS_CLASS_READY);  }
    inline void is_class_ready(bool en) {
      flags = (en) ? (flags | MGC3130_FLAGS_CLASS_READY) : (flags & ~(MGC3130_FLAGS_CLASS_READY));
    }


#ifdef BOARD_IRQS_AND_PINS_DISTINCT
    int get_irq_num_by_pin(int _pin);
#endif
};




#endif  // __MGC3130_H__
