/*  ===========================================================================
#  This is the library for Hover.
#
#  Hover is a development kit that lets you control your hardware projects in a whole new way.
#  Wave goodbye to physical buttons. Hover detects hand movements in the air for touch-less interaction.
#  It also features five touch-sensitive regions for even more options.
#  Hover uses I2C and 2 digital pins. It is compatible with Arduino, Raspberry Pi and more.
#
#  Hover can be purchased here: http://www.justhover.com
#
#  Written by Emran Mahbub and Jonathan Li for Gearseven Studios.
#  Enhancement and extention by J. Ian Lindsay.
#  BSD license, all text above must be included in any redistribution
#  ===========================================================================
#
#  INSTALLATION
#  The 3 library files (Hover.cpp, Hover.h and keywords.txt) in the Hover folder should be placed in your Arduino Library folder.
#  Run the HoverDemo.ino file from your Arduino IDE.
#
#  SUPPORT
#  For questions and comments, email us at support@gearseven.com
#  v2.1
#  ===========================================================================*/


#ifndef __LDS8160_LED_DRIVER_H__
#define __LDS8160_LED_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include "ManuvrOS/EventManager.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"

#define MGC3130_ISR_MARKER_TS 0x01
#define MGC3130_ISR_MARKER_G0 0x80
#define MGC3130_ISR_MARKER_G1 0x40
#define MGC3130_ISR_MARKER_G2 0x20
#define MGC3130_ISR_MARKER_G3 0x10

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

    uint32_t events_received;

    MGC3130(int ts, int mclr, uint8_t addr = 0x42);
    void init();

    
    /* Overrides from I2CDevice... */
    void operationCompleteCallback(I2CQueuedOperation*);

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);
    void procDirectDebugInstruction(StringBuilder *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);


    int8_t service();
    const char* getTouchTapString(uint8_t eventByte);
    const char* getSwipeString(uint8_t eventByte);

    int8_t setIRQPin(uint8_t, int);

    // bool featureEnabled(uint8_t mask);

    void enableApproachDetect(bool);
    void enableAirwheel(bool);

    void set_isr_mark(uint8_t mask);
    inline bool isPositionDirty() { return ((_pos_x + _pos_y + _pos_z) != -3); };
    inline bool isTouchDirty() {    return (last_touch_noted != last_touch);   };

    bool isDirty();
    void markClean();

    void printBrief(StringBuilder* output);

    volatile static MGC3130* INSTANCE;


  protected:
    int8_t bootComplete();


  private:
    uint8_t _i2caddr;     // i2c address.

    uint8_t _ts_pin;      // Pin number being used by the TS pin.
    uint8_t _reset_pin;   // Pin number being used by the MCLR pin.
    uint8_t _irq_pin_0;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_1;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_2;   // Pin number being used by optional IRQ pin.
    uint8_t _irq_pin_3;   // Pin number being used by optional IRQ pin.

    uint8_t last_event;

    uint8_t service_flags;
    uint8_t class_state;

    uint8_t getEvent(void);



#ifdef BOARD_IRQS_AND_PINS_DISTINCT
    int get_irq_num_by_pin(int _pin);
#endif
};




#endif
