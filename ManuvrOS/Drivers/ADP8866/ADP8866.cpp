/*
File:   ADP8866.cpp
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

#include <Kernel.h>
#include "ADP8866.h"
#include <Platform/Platform.h>


static const MessageTypeDef adp8866_message_defs[] = {
  {  MANUVR_MSG_ADP8866_IRQ,                       0x0000, "ADP8866_IRQ",      ManuvrMsg::MSG_ARGS_NONE },
  {  MANUVR_MSG_ADP8866_CHAN_ENABLED, MSG_FLAG_EXPORTABLE, "ADP8866_CHAN_EN",  ManuvrMsg::MSG_ARGS_NONE },
  {  MANUVR_MSG_ADP8866_CHAN_LEVEL,   MSG_FLAG_EXPORTABLE, "ADP8866_CHAN_LVL", ManuvrMsg::MSG_ARGS_NONE },
  {  MANUVR_MSG_ADP8866_RESET,                     0x0000, "ADP8866_RESET",    ManuvrMsg::MSG_ARGS_NONE }
};

ADP8866* ADP8866::INSTANCE = nullptr;


/*
* Static fxn to correct for the discontinuity in the timing deltas for the fade.
*/
uint8_t ADP8866::_fade_val_from_ms(uint16_t ms) {
  uint16_t adj_ms = strict_min((uint16_t) 1750, ms);
  if (adj_ms < 750) {
    return (adj_ms / 50);
  }
  else {
    return (0x0B + ((adj_ms - 750) / 250));
  }
}


/*
* This is the ISR for the interrupt pin (if provided).
*/
void ADP8866_ISR() {
  if (ADP8866::INSTANCE) {
    ADP8866::INSTANCE->_isr_fxn();
  }
}

// TODO: Bleh... I really dislike htis indirection...
void ADP8866::_isr_fxn() {
  if (_er_flag(ADP8866_FLAG_INIT_COMPLETE)) {
    readRegister((uint8_t) ADP8866_INT_STAT);
  }
}


/*
* TODO: This is a table of constants relating channel current to a register value.
*/


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/*
* Constructor. Takes pin numbers as arguments.
*/
ADP8866::ADP8866(const ADP8866Pins* p) : EventReceiver("ADP8866"), I2CDeviceWithRegisters(ADP8866_I2CADDR, 47), _pins(p) {
  _er_clear_flag(ADP8866_FLAG_INIT_COMPLETE);
  if (nullptr == ADP8866::INSTANCE) {
    ADP8866::INSTANCE = this;
    int mes_count = sizeof(adp8866_message_defs) / sizeof(MessageTypeDef);
    ManuvrMsg::registerMessages(adp8866_message_defs, mes_count);
  }

  if (255 != _pins.irq) {
    gpioDefine(_pins.irq, _pins.usePullup() ? GPIOMode::INPUT_PULLUP : GPIOMode::INPUT);
  }

  if (255 != _pins.rst) {
    gpioDefine(_pins.rst, GPIOMode::OUTPUT);
    setPin(_pins.rst, false);
  }

  defineRegister(ADP8866_MANU_DEV_ID,  (uint8_t) 0x00, false,  true, false);
  defineRegister(ADP8866_MDCR,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_INT_STAT,     (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_INT_EN,       (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCOFF_SEL_1, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCOFF_SEL_2, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_GAIN_SEL,     (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_LVL_SEL_1,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_LVL_SEL_2,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_PWR_SEL_1,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_PWR_SEL_2,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_CFGR,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_BLSEL,        (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_BLFR,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_BLMX,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCC1,        (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCC2,        (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCT1,        (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCT2,        (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER6,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER7,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER8,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER9,    (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCF,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC1,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC2,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC3,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC4,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC5,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC6,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC7,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC8,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC9,         (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_HB_SEL,       (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC6_HB,      (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC7_HB,      (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC8_HB,      (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISC9_HB,      (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER6_HB, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER7_HB, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER8_HB, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_OFFTIMER9_HB, (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_ISCT_HB,      (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_DELAY6,       (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_DELAY7,       (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_DELAY8,       (uint8_t) 0x00, false,  false, true);
  defineRegister(ADP8866_DELAY9,       (uint8_t) 0x00, false,  false, true);
}


/*
* Destructor.
*/
ADP8866::~ADP8866() {
}


int8_t ADP8866::init() {
  // Turn on charge pump. Limit it to 1.5x with autoscale.
  writeIndirect(ADP8866_MDCR,  0b01110101, true);

  // Maximum current of ~16mA.
  // All LED outputs are set with the level bits.
  writeIndirect(ADP8866_LVL_SEL_1,  0b01000100, true);
  writeIndirect(ADP8866_LVL_SEL_2,  0xFF, true);

  writeIndirect(ADP8866_GAIN_SEL, 0b00000100, true);

  // All LEDs are being driven by the charge pump.
  writeIndirect(ADP8866_PWR_SEL_1,  0x00, true);
  writeIndirect(ADP8866_PWR_SEL_2,  0x00, true);

  // All LED's independently sinkd. Backlight cubic transition.
  writeIndirect(ADP8866_CFGR,  0b00010100, true);
  writeIndirect(ADP8866_BLSEL, 0b11111111, true);

  // Backlight fade rates...
  writeIndirect(ADP8866_BLFR, 0b01100110, true);

  // No backlight current.
  writeIndirect(ADP8866_BLMX, 0b00000000, true);

  // Sink control. All on. Cubic transfer fxn.
  writeIndirect(ADP8866_ISCC1, 0b00000111, true);
  writeIndirect(ADP8866_ISCC2, 0b11111111, true);

  // SON/SOFF delays...
  writeIndirect(ADP8866_ISCT1, 0b11110000, true);
  writeIndirect(ADP8866_ISCT2, 0b00000000, true);

  // Enabled interrupts: Over-volt, short-circuit, thermal shutdown
  writeIndirect(ADP8866_INT_EN, 0x1C, true);

  // TODO: When the driver inits, we shouldn't have any LEDs on until the user sets
  //   some mandatory constraint so he doesn't fry his LEDs.
  writeIndirect(ADP8866_ISC1, regValue(ADP8866_ISC1), true);
  writeIndirect(ADP8866_ISC2, regValue(ADP8866_ISC2), true);
  writeIndirect(ADP8866_ISC3, regValue(ADP8866_ISC3), true);
  writeIndirect(ADP8866_ISC4, regValue(ADP8866_ISC4), true);
  writeIndirect(ADP8866_ISC5, regValue(ADP8866_ISC5), true);
  writeIndirect(ADP8866_ISC6, regValue(ADP8866_ISC6), true);
  writeIndirect(ADP8866_ISC7, regValue(ADP8866_ISC7), true);
  writeIndirect(ADP8866_ISC8, regValue(ADP8866_ISC8), true);
  writeIndirect(ADP8866_ISC9, regValue(ADP8866_ISC9), true);

  writeIndirect(ADP8866_ISCT_HB, 0x0A);
  _er_set_flag(ADP8866_FLAG_INIT_COMPLETE);
  return 0;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ADP8866::register_write_cb(DeviceRegister* reg) {
  uint8_t value = reg->getVal();
  switch (reg->addr) {
    case ADP8866_MDCR:
      break;
    case ADP8866_INT_STAT:      // Interrupt status.
      break;
    case ADP8866_INT_EN:
      break;
    case ADP8866_ISCOFF_SEL_1:
      break;
    case ADP8866_ISCOFF_SEL_2:
      break;
    case ADP8866_GAIN_SEL:
      break;
    case ADP8866_LVL_SEL_1:
      break;
    case ADP8866_LVL_SEL_2:
      break;
    case ADP8866_PWR_SEL_1:
      break;
    case ADP8866_PWR_SEL_2:
      break;
    case ADP8866_CFGR:
      break;
    case ADP8866_BLSEL:
      break;
    case ADP8866_BLFR:
      break;
    case ADP8866_ISCC1:
      break;
    case ADP8866_ISCC2:
      break;
    case ADP8866_ISCT1:
      break;
    case ADP8866_ISCT2:
      break;
    case ADP8866_OFFTIMER6:
      break;
    case ADP8866_OFFTIMER7:
      break;
    case ADP8866_OFFTIMER8:
      break;
    case ADP8866_OFFTIMER9:
      break;
    case ADP8866_ISCF:
      break;
    case ADP8866_BLMX:
      channels[9].present = value;
      break;
    case ADP8866_ISC1:
    case ADP8866_ISC2:
    case ADP8866_ISC3:
    case ADP8866_ISC4:
    case ADP8866_ISC5:
    case ADP8866_ISC6:
    case ADP8866_ISC7:
    case ADP8866_ISC8:
    case ADP8866_ISC9:
      channels[(reg->addr & 0x00FF) - ADP8866_ISC1].present = value;
      break;
    case ADP8866_HB_SEL:
      break;
    case ADP8866_ISC6_HB:
      break;
    case ADP8866_ISC7_HB:
      break;
    case ADP8866_ISC8_HB:
      break;
    case ADP8866_ISC9_HB:
      break;
    case ADP8866_OFFTIMER6_HB:
      break;
    case ADP8866_OFFTIMER7_HB:
      break;
    case ADP8866_OFFTIMER8_HB:
      break;
    case ADP8866_OFFTIMER9_HB:
      break;
    case ADP8866_ISCT_HB:
      break;
    case ADP8866_DELAY6:
      break;
    case ADP8866_DELAY7:
      break;
    case ADP8866_DELAY8:
      break;
    case ADP8866_DELAY9:
      break;

    case ADP8866_MANU_DEV_ID:
    default:
      // Illegal write target.
      break;
  }
  flushLocalLog();
  return 0;
}


int8_t ADP8866::register_read_cb(DeviceRegister* reg) {
  uint8_t value = reg->getVal();
  switch (reg->addr) {
    case ADP8866_MANU_DEV_ID:
      if (0x53 == value) {
        reg->unread = false;
        // Must be 0b01010011. If so, we init...
        init();
      }
      break;
    case ADP8866_MDCR:
      break;
    case ADP8866_INT_STAT:      // Interrupt status.
      if (value & 0x04) {
        Kernel::log("ADP8866 experienced an over-voltage fault.\n");
      }
      if (value & 0x08) {
        Kernel::log("ADP8866 experienced a thermal shutdown.\n");
      }
      if (value & 0x10) {
        Kernel::log("ADP8866 experienced a short-circuit fault.\n");
      }
      if (value & 0x1C) {
        // If we experienced a fault, we may as well shut off the device.
        writeIndirect(ADP8866_MDCR, (uint8_t)regValue(ADP8866_MDCR) & 0b01011010);
      }
      if (value & 0x20) {
        // Backlight off
      }
      if (value & 0x40) {
        // Independent sink off
      }
      // Whatever the interrupt was, clear it.
      writeIndirect((uint8_t) ADP8866_INT_STAT, value);
      break;
    case ADP8866_INT_EN:
      break;
    case ADP8866_ISCOFF_SEL_1:
      break;
    case ADP8866_ISCOFF_SEL_2:
      break;
    case ADP8866_GAIN_SEL:
      break;
    case ADP8866_LVL_SEL_1:
      break;
    case ADP8866_LVL_SEL_2:
      break;
    case ADP8866_PWR_SEL_1:
      break;
    case ADP8866_PWR_SEL_2:
      break;
    case ADP8866_CFGR:
      break;
    case ADP8866_BLSEL:
      break;
    case ADP8866_BLFR:
      break;
    case ADP8866_ISCC1:
      break;
    case ADP8866_ISCC2:
      break;
    case ADP8866_ISCT1:
      break;
    case ADP8866_ISCT2:
      break;
    case ADP8866_OFFTIMER6:
      break;
    case ADP8866_OFFTIMER7:
      break;
    case ADP8866_OFFTIMER8:
      break;
    case ADP8866_OFFTIMER9:
      break;
    case ADP8866_ISCF:
      break;
    case ADP8866_BLMX:
      channels[9].present = value;
      break;
    case ADP8866_ISC1:
    case ADP8866_ISC2:
    case ADP8866_ISC3:
    case ADP8866_ISC4:
    case ADP8866_ISC5:
    case ADP8866_ISC6:
    case ADP8866_ISC7:
    case ADP8866_ISC8:
    case ADP8866_ISC9:
      channels[(reg->addr & 0x00FF) - ADP8866_ISC1].present = value;
      break;
    case ADP8866_HB_SEL:
      break;
    case ADP8866_ISC6_HB:
      break;
    case ADP8866_ISC7_HB:
      break;
    case ADP8866_ISC8_HB:
      break;
    case ADP8866_ISC9_HB:
      break;
    case ADP8866_OFFTIMER6_HB:
      break;
    case ADP8866_OFFTIMER7_HB:
      break;
    case ADP8866_OFFTIMER8_HB:
      break;
    case ADP8866_OFFTIMER9_HB:
      break;
    case ADP8866_ISCT_HB:
      break;
    case ADP8866_DELAY6:
      break;
    case ADP8866_DELAY7:
      break;
    case ADP8866_DELAY8:
      break;
    case ADP8866_DELAY9:
      break;
    default:
      break;
  }
  reg->unread = false;
  flushLocalLog();
  return 0;
}


/*
* Dump this item to the dev log.
*/
void ADP8866::printDebug(StringBuilder* temp) {
  EventReceiver::printDebug(temp);
  temp->concatf("\tinit_complete:     %c\n", _er_flag(ADP8866_FLAG_INIT_COMPLETE) ? 'y' :'n');
  temp->concatf("\tpower_mode:        %d\n", power_mode);
  temp->concatf("\tReset pin:         %d\n", _pins.rst);
  temp->concatf("\tIRQ pin:           %d\n", _pins.irq);
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ADP8866::attached() {
  if (EventReceiver::attached()) {
    _pins.reset(true);   // Release the reset pin.

    // If this read comes back the way we think it should,
    //   we will init() the chip.
    readRegister((uint8_t) ADP8866_MANU_DEV_ID);

    if (255 != _pins.irq) {
      setPinFxn(_pins.irq, (_pins.usePullup() ? FALLING_PULL_UP : FALLING), ADP8866_ISR);
    }
    return 1;
  }
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ADP8866::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}



int8_t ADP8866::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_SYS_POWER_MODE:
      break;
    case MANUVR_MSG_ADP8866_RESET:
      // This is the reset callback
      if (_pins.reset(true)) {
        readRegister((uint8_t) ADP8866_MANU_DEV_ID);
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "r", "Reset" },
  { "b", "Set channel brightness" },
  { "L/l", "(En/Dis)able channel)" },
  { "g", "Sync registers" }
};


uint ADP8866::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void ADP8866::consoleCmdProc(StringBuilder* input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int channel  = 0;
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    channel = input->position_as_int(1);
  }
  if (input->count() > 2) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(2);
  }

  switch (c) {
    case 'i':   // Debug prints.
      switch (channel) {
        case 1:   // We want the channel stats.
          break;

        case 9:   // We want the i2c registers.
          I2CDeviceWithRegisters::printDebug(&local_log);
          break;

        default:
          printDebug(&local_log);
          break;
      }
      break;

    case 'r':
      reset();
      break;
    case 'g':
      syncRegisters();
      break;
    case 'x':
      if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ADP8866_MDCR)) {
        local_log.concat("Reading MDCR reg...\n");
      }
      else {
        local_log.concat("Can't read MDCR reg.\n");
      }
      break;
    case 'p':
      pulse_channel(channel, temp_int);
      break;
    case 'l':
    case 'L':
      enable_channel(channel, (c == 'L'));
      break;
    case 'b':
      set_brightness(channel, temp_int);
      if (getVerbosity() > 5) local_log.concatf("ADP8866: set_brightness(%u, %u)\n", channel, temp_int);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      channel = *str - 0x30;
      local_log.concatf("ADP8866: Channel %u is %sabled.\n", channel, channel_enabled(channel) ? "en" : "dis");
      break;
    default:
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/
/*
* This is a function that holds high-level macros for LED behavior that is generic.
* Example:
*   Some classes might wish to take direct control over the LEDs to do something fancy.
*   Other classes might not care about micromanaging LED behavior, but would like to use
*     them as indirectly as possible. This is the type of class this function is meant for.
*/
void ADP8866::set_led_mode(uint8_t num) {
}


/*
* Disables all on-chip timer influence on LEDs.
*/
void ADP8866::quell_all_timers() {
  uint8_t hb_sel = (uint8_t)regValue(ADP8866_HB_SEL);
  if (hb_sel) {
    if (hb_sel & 0x01) {
      writeIndirect(ADP8866_OFFTIMER6,    0x00, true);
      writeIndirect(ADP8866_OFFTIMER6_HB, 0x00, true);
    }
    if (hb_sel & 0x02) {
      writeIndirect(ADP8866_OFFTIMER7,    0x00, true);
      writeIndirect(ADP8866_OFFTIMER7_HB, 0x00, true);
    }
    if (hb_sel & 0x04) {
      writeIndirect(ADP8866_OFFTIMER8,    0x00, true);
      writeIndirect(ADP8866_OFFTIMER8_HB, 0x00, true);
    }
    if (hb_sel & 0x08) {
      writeIndirect(ADP8866_OFFTIMER9,    0x00, true);
      writeIndirect(ADP8866_OFFTIMER9_HB, 0x00, true);
    }
    writeIndirect(ADP8866_HB_SEL,       0x00);
  }
}


/*
* Set the global brightness for LEDs managed by this chip.
* Stores the previous value.
*/
void ADP8866::set_brightness(uint8_t nu_brightness) {
}


/*
* Given a channel and a brightness, pulse at the given rate.
* Only chip channels 6-9 are capable of this.
* Uses a default pair of timing values.
*/
void ADP8866::pulse_channel(uint8_t chan, uint8_t nu_brightness) {
  pulse_channel(chan, nu_brightness, 300, 1000);
}


/*
* Given a channel and a brightness, pulse at the given rate.
* Only chip channels 6-9 are capable of this.
* This function will clamp the timer input values to the maximum supported by
*   the hardware: 750ms for on time, and 12.5s for off time.
*/
void ADP8866::pulse_channel(uint8_t chan, uint8_t nu_brightness, uint16_t ms_on, uint16_t ms_off) {
  if ((chan > 8) || (chan < 5)) {
    // Channel outside of the valid range...
    return;
  }
  uint8_t fade_out_rate = (uint8_t) regValue(ADP8866_ISCF) >> 4;
  uint8_t adj_chan = (chan - 5);
  uint8_t adj_t_off = (uint8_t) strict_max((uint8_t) (strict_min((uint16_t) 12500, ms_off) / 100), (uint8_t) 1);
  uint8_t adj_t_on  = (uint8_t) strict_max((uint8_t) (strict_min((uint16_t) 750,   ms_on)  / 50),  (uint8_t) fade_out_rate);
  uint8_t hb_sel = ((uint8_t)regValue(ADP8866_HB_SEL)) | (0x01 << adj_chan);

  writeIndirect(ADP8866_HB_SEL, hb_sel, true);
  writeIndirect(ADP8866_ISC6_HB      + adj_chan, nu_brightness, true);
  writeIndirect(ADP8866_OFFTIMER6    + adj_chan, adj_t_off, true);
  writeIndirect(ADP8866_OFFTIMER6_HB + adj_chan, adj_t_on);
}


/*
* This function will clamp the timer input values to the maximum supported by
*   the hardware: 1750ms for both fade-in and fade-out.
*/
void ADP8866::set_fade(uint16_t ms_in, uint16_t ms_out) {
  uint8_t fade_out_rate = (uint8_t) regValue(ADP8866_ISCF) >> 4;
  uint8_t fade_in_rate  = (uint8_t) regValue(ADP8866_ISCF) & 0x0F;
  uint8_t adj_t_out = _fade_val_from_ms(ms_out);
  uint8_t adj_t_in  = _fade_val_from_ms(ms_in);

  if ((adj_t_in != fade_in_rate) || (adj_t_out != fade_out_rate)) {
    writeIndirect(ADP8866_ISCF, (adj_t_out << 4) + adj_t_in);
  }
}


void ADP8866::set_brightness(uint8_t chan, uint8_t nu_brightness) {
  if (chan > 9) {
    // Not that many channels....
    return;
  }
  // This is what happens when you javascript for too long...
  uint8_t reg_addr      = (chan > 8) ? ADP8866_BLMX : ADP8866_ISC1+chan;
  uint8_t present_state = (uint8_t)regValue(reg_addr);
  nu_brightness = nu_brightness & 0x7F;  // Only these bits matter.
  if (present_state != nu_brightness) {
    writeIndirect(reg_addr, nu_brightness);
  }
}


/*
* Set the global brightness for LEDs managed by this chip.
* Exchanges the current value and the previously-stored value.
*/
void ADP8866::toggle_brightness(void) {
  //writeIndirect(ADP8866_GLOBAL_PWM_DIM, stored_dimmer_val);
}


/*
* Enable or disable a specific LED. If something needs to be written to the
*   chip, will do that as well.
*/
void ADP8866::enable_channel(uint8_t chan, bool en) {
  if (chan > 8) {  // TODO: Should be 9, and treat the backlight channel as channel 9.
    // Not that many channels....
    return;
  }
  // This is what happens when you javascript for too long...
  uint8_t reg_addr      = (chan > 7) ? ADP8866_ISCC1 : ADP8866_ISCC2;
  uint8_t present_state = (uint8_t)regValue(reg_addr);
  uint8_t bitmask       = (chan > 7) ? 4 : (1 << chan);
  uint8_t desired_state = (en ? (present_state | bitmask) : (present_state & ~bitmask));

  if (present_state != desired_state) {
    // If the present state and the desired state differ, Set the register equal to the
    //   present state masked with the desired state.
    writeIndirect(reg_addr, desired_state);
  }
}


/*
* Returns the boolean answer to the question: Is the given channel enabled?
*/
bool ADP8866::channel_enabled(uint8_t chan) {
  uint8_t reg_addr      = (chan > 7) ? ADP8866_ISCC1 : ADP8866_ISCC2;
  uint8_t present_state = (uint8_t)regValue(reg_addr);
  uint8_t bitmask       = (chan > 7) ? 4 : (1 << chan);
  return (present_state & bitmask);
}


/*
* Perform a software reset.
*/
void ADP8866::reset() {
  _pins.reset(false);
  _er_clear_flag(ADP8866_FLAG_INIT_COMPLETE);
  raiseEvent(Kernel::returnEvent(MANUVR_MSG_ADP8866_RESET));
}

/*
* When a power mode broadcast is seen, this fxn will be called with the new
*   power profile identifier.
*/
void ADP8866::set_power_mode(uint8_t nu_power_mode) {
  power_mode = nu_power_mode;
  switch (power_mode) {
    case 0:
      //writeIndirect(ADP8866_BANK_A_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_BANK_B_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_BANK_C_CURRENT, 0x8E, true);
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x00);
      break;
    case 1:
      //writeIndirect(ADP8866_BANK_A_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_BANK_B_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_BANK_C_CURRENT, 0x50, true);
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x00);
      break;
    case 2:   // Enter standby.
      quell_all_timers();
      //writeIndirect(ADP8866_SOFTWARE_RESET, 0x40);
      break;
    default:
      break;
  }
  Kernel::log("ADP8866 Power mode set. \n");
}


void ADP8866::setMaxCurrents(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5, uint8_t c6, uint8_t c7, uint8_t c8) {
}
