/*
File:   BQ24155.cpp
Author: J. Ian Lindsay
Date:   2017.06.08

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


Support for the Texas Instruments li-ion charger.
*/


#include <Kernel.h>
#include "BQ24155.h"

#if defined(CONFIG_MANUVR_BQ24155)



/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

const char* BQ24155_STATE_STR[] = {
  "READY",
  "CHARGING",
  "CHARGED",
  "FAULT"
};


const char* BQ24155_FAULT_STR[] = {
  "NOMINAL",
  "VBUS_OVP",
  "SLEEP",
  "VBUS_SAG",
  "OUTPUT_OVP",
  "THERMAL",
  "TIMER",
  "NO_BATTERY"
};

BQ24155* BQ24155::INSTANCE = nullptr;

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
BQ24155::BQ24155(const BQ24155Opts* o) : EventReceiver("BQ24155"), I2CDeviceWithRegisters(BQ24155_I2CADDR), _opts(o) {
  if (nullptr == BQ24155::INSTANCE) {
    BQ24155::INSTANCE = this;
  }
  _er_clear_flag(BQ24155_FLAG_INIT_COMPLETE);

  defineRegister(BQ24155_REG_STATUS,    (uint8_t) 0x00, false, true,  true);
  defineRegister(BQ24155_REG_LIMITS,    (uint8_t) 0x30, false, false, true);
  defineRegister(BQ24155_REG_BATT_REGU, (uint8_t) 0x0A, false, false, true);
  defineRegister(BQ24155_REG_PART_REV,  (uint8_t) 0x00, false, true,  false);
  defineRegister(BQ24155_REG_FAST_CHRG, (uint8_t) 0x89, false, false, true);
}


/*
* Destructor.
*/
BQ24155::~BQ24155() {
}


int8_t BQ24155::init() {
  //writeIndirect(BQ24155_REG_STATUS,    0x00, true);
  //writeIndirect(BQ24155_REG_LIMITS,    0x00, true);
  //writeIndirect(BQ24155_REG_BATT_REGU, 0x00, true);
  //writeIndirect(BQ24155_REG_FAST_CHRG, 0x00);
  readRegister((uint8_t) BQ24155_REG_STATUS);
  readRegister((uint8_t) BQ24155_REG_LIMITS);
  readRegister((uint8_t) BQ24155_REG_BATT_REGU);
  readRegister((uint8_t) BQ24155_REG_FAST_CHRG);
  _er_set_flag(BQ24155_FLAG_INIT_COMPLETE);
  return 0;
}



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/

/**
* Returns the charger's current state.
*/
BQ24155State BQ24155::getChargerState() {
  return (BQ24155State) (0x03 & ((uint8_t) regValue(BQ24155_REG_STATUS)) >> 4);
}

/**
* Returns the fault condition.
*/
BQ24155Fault BQ24155::getFault() {
  return (BQ24155Fault) (0x07 & (uint8_t) regValue(BQ24155_REG_STATUS));
}

/**
*
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::punch_safety_timer() {
  int8_t return_val = -1;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    uint8_t int_val = regValue(BQ24155_REG_STATUS);
    writeIndirect(BQ24155_REG_STATUS, 0x80 | int_val);
    return_val++;
  }
  return return_val;
}

/**
* Reset the charger's internal state-machine.
*
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::put_charger_in_reset_mode() {
  int8_t return_val = -1;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    uint8_t int_val = regValue(BQ24155_REG_FAST_CHRG);
    writeIndirect(BQ24155_REG_FAST_CHRG, 0x80 | int_val);
    return_val++;
  }
  return return_val;
}

/**
* Set the battery voltage. It is very important to get this right.
*
* @param desired Valid range is [3.5 - 4.44]
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::batt_reg_voltage(float desired) {
  int8_t return_val = -2;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    return_val++;
    if ((desired >= BQ24155_VOREGU_OFFSET) && (desired < 4.44)) {   //
      uint8_t offset_val = (uint8_t) ((desired - BQ24155_VOREGU_OFFSET) / 0.02);
      writeIndirect(BQ24155_REG_BATT_REGU, offset_val << 2);
      return_val++;
    }
  }
  return return_val;
}

/**
* Get the battery voltage.
*
* @return The battery regulation voltage.
*/
float BQ24155::batt_reg_voltage() {
  uint8_t int_val = regValue(BQ24155_REG_BATT_REGU);
  return ((int_val >> 2) * 0.02f) + BQ24155_VOREGU_OFFSET;
}

/**
* Get the battery weakness threshold.
*
* @return The number of mV at which point the battery is considered weak.
*/
uint16_t BQ24155::batt_weak_voltage() {
  uint8_t int_val = regValue(BQ24155_REG_LIMITS);
  return BQ24155_VLOW_OFFSET + (100 * ((int_val >> 4) & 0x03));
}

/**
* Sets the voltage at which the battery is considered weak.
* Charger has 2-bits for this purpose, with 100mV increments.
*
* @param The number of mV at which point the battery is to be considered weak.
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::batt_weak_voltage(unsigned int mV) {
  int8_t return_val = -2;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    return_val++;
    if ((mV >= BQ24155_VLOW_OFFSET) && (mV <= 3700)) {
      uint8_t bw_step = (mV - BQ24155_VLOW_OFFSET) / 100;
      uint8_t int_val = regValue(BQ24155_REG_LIMITS);
      writeIndirect(BQ24155_REG_LIMITS, (int_val & 0xCF) | (bw_step << 4));
      return_val++;
    }
  }
  return return_val;
}

/**
* What is the host-imposed (or self-configured) current limit?
* Answer given in amps. For the sake of confining our concerns to finite
*   arithmetic, we will interpret the "unlimited" condition reported by the
*   charger to mean some very large (but limited) number, beyond the TDP of the
*   charger IC itself.
*
* @return The number of mV the USB host is able to supply. -1 if unlimited.
*/
int16_t BQ24155::usb_current_limit() {
  uint8_t int_val = regValue(BQ24155_REG_LIMITS);
  switch ((int_val >> 6) & 0x03) {
    case 3:  return -1;    // This is practically unlimited in this case.
    case 2:  return 800;
    case 1:  return 500;
  }
  return 100;  // This is the default condition.
}

/**
* We can artificially limit the draw from the USB port.
*
* @param The number of mV to draw from the USB host.
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::usb_current_limit(int16_t milliamps) {
  int8_t return_val = -2;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    return_val++;
    uint8_t c_step  = 0;
    uint8_t int_val = regValue(BQ24155_REG_LIMITS);
    switch (milliamps) {
      case -1:  c_step++;
      case 800: c_step++;
      case 500: c_step++;
      case 100: break;
      default:  return return_val;
    }
    return_val++;
    writeIndirect(BQ24155_REG_LIMITS, (int_val & 0xCF) | (c_step << 6));
  }
  return return_val;
}

/**
* Is the charger enabled?
*
* @return True if the charger is enabled.
*/
bool BQ24155::charger_enabled() {
  return (0x04 & (uint8_t) regValue(BQ24155_REG_LIMITS));
}

/**
* Enable or disable the charger.
*
* @param True to enable the charger.
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::charger_enabled(bool en) {
  int8_t return_val = -2;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    return_val++;
    uint8_t int_val = regValue(BQ24155_REG_LIMITS);
    if (en ^ (int_val & 0x04)) {
      return_val++;
      int_val = (en) ? (int_val | 0x04) : (int_val & 0xFB);
      writeIndirect(BQ24155_REG_LIMITS, int_val);
    }
  }
  return return_val;
}

/**
* Is the charge termination phase enabled?
*
* @return True if the charge termination phase is enabled.
*/
bool BQ24155::charge_current_termination_enabled() {
  return (0x08 & (uint8_t) regValue(BQ24155_REG_LIMITS));
}

/**
* Enable or disable the charge termination phase.
*
* @param True to enable the charge termination phase.
* @return 0 on success, non-zero otherwise.
*/
int8_t BQ24155::charge_current_termination_enabled(bool en) {
  int8_t return_val = -2;
  if (_er_flag(BQ24155_FLAG_INIT_COMPLETE)) {
    return_val++;
    uint8_t int_val = regValue(BQ24155_REG_LIMITS);
    if (en ^ (int_val & 0x08)) {
      return_val++;
      int_val = (en) ? (int_val | 0x08) : (int_val & 0xF7);
      writeIndirect(BQ24155_REG_LIMITS, int_val);
    }
  }
  return return_val;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t BQ24155::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  I2CDeviceWithRegisters::io_op_callback(_op);

  DeviceRegister *temp_reg = getRegisterByBaseAddress(completed->sub_addr);
  switch (completed->sub_addr) {
    case BQ24155_REG_PART_REV:
      // Must be 0b01001xxx. If so, we init...
      if (0x48 == (0xF8 & *(temp_reg->val))) {
        temp_reg->unread = false;
        init();
      }
      break;
    case BQ24155_REG_STATUS:
      temp_reg->unread = false;
      break;
    case BQ24155_REG_LIMITS:
      temp_reg->unread = false;
      break;
    case BQ24155_REG_BATT_REGU:
      temp_reg->unread = false;
      break;
    case BQ24155_REG_FAST_CHRG:
    temp_reg->unread = false;
      break;
    default:
      temp_reg->unread = false;
      break;
  }

  /* Null the buffer so the bus adapter isn't tempted to free it.
    TODO: This is silly. Fix this in the API. */
  _op->buf     = nullptr;
  _op->buf_len = 0;

  return 0;
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
int8_t BQ24155::attached() {
  if (EventReceiver::attached()) {
    if (255 != _opts.stat_pin) {
      gpioDefine(_opts.stat_pin, GPIOMode::INPUT_PULLUP);
    }

    if (255 != _opts.isel_pin) {
      gpioDefine(_opts.isel_pin, GPIOMode::OUTPUT);
      setPin(_opts.isel_pin, false);  // Defaults to 100mA charge current.
    }

    readRegister((uint8_t) BQ24155_REG_PART_REV);
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
int8_t BQ24155::callback_proc(ManuvrMsg* event) {
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

int8_t BQ24155::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_SYS_POWER_MODE:
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


/**
* Dump this item to the dev log.
*
* @param  output  The buffer to receive the output.
*/
void BQ24155::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
  output->concatf("\tInitialized:       %c\n", _er_flag(BQ24155_FLAG_INIT_COMPLETE) ? 'y' : 'n');
  output->concatf("\tSTAT pin:          %d\n", _opts.stat_pin);
  output->concatf("\tISEL pin:          %d\n", _opts.isel_pin);
  output->concatf("\tSense resistor:    %d mOhm\n", _opts.sense_milliohms);
  output->concatf("\tUSB current limit: %d mA\n", usb_current_limit());
  output->concatf("\tBatt reg voltage:  %.2f V\n", batt_reg_voltage());
  output->concatf("\tBattery weak at:   %u mV\n", batt_weak_voltage());
  output->concatf("\tCharger state:     %s\n", (const char*) BQ24155_STATE_STR[(uint8_t) getChargerState()]);
  output->concatf("\tFault:             %s\n", (const char*) BQ24155_FAULT_STR[(uint8_t) getFault()]);
  output->concat("\t  Bulk rates  Termination rates\n\t  ----------  -----------------\n");
  uint8_t c_btc_idx = (regValue(BQ24155_REG_FAST_CHRG) & 0x07);
  uint8_t c_bcc_idx = ((regValue(BQ24155_REG_FAST_CHRG) >> 4) & 0x07);
  for (int i = 0; i < 8; i++) {
    output->concatf("\t  [%c] %.3fA  [%c] %.3fA\n",
      (c_bcc_idx == i) ? '*' : ' ',
      bulkChargeCurrent(i),
      (c_btc_idx == i) ? '*' : ' ',
      terminateChargeCurrent(i)
    );
  }
}



#if defined(MANUVR_CONSOLE_SUPPORT)

void BQ24155::procDirectDebugInstruction(StringBuilder *input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(1);
  }

  switch (c) {
    case 'i':   // Debug prints.
      switch (temp_int) {
        case 9:   // We want the i2c registers.
          I2CDeviceWithRegisters::printDebug(&local_log);
          break;

        default:
          printDebug(&local_log);
          break;
      }
      break;

    case 'u':   // USB host current limit.
      if (temp_int) {
        usb_current_limit((unsigned int) temp_int);
      }
      else {
        local_log.concatf("USB current limit: %dmA\n", usb_current_limit());
      }
      break;

    case 'v':   // Battery voltage
      if (temp_int) {
        local_log.concatf("Setting battery voltage to %.2fV\n", input->position_as_double(1));
        batt_reg_voltage(input->position_as_double(1));
      }
      else {
        local_log.concatf("Batt reg voltage:  %.2fV\n", batt_reg_voltage());
      }
      break;
    case 'w':   // Battery weakness voltage
      if (temp_int) {
        local_log.concatf("Setting battery weakness voltage to %d mV\n", temp_int);
        batt_weak_voltage(temp_int);
      }
      else {
        local_log.concatf("Batt weakness voltage:  %d mV\n", batt_weak_voltage());
      }
      break;



    case 'g':
      switch (temp_int) {
        case 0:
          readRegister((uint8_t) BQ24155_REG_STATUS);
          break;
        case 1:
          readRegister((uint8_t) BQ24155_REG_LIMITS);
          break;
        case 2:
          readRegister((uint8_t) BQ24155_REG_BATT_REGU);
          break;
        case 3:
          readRegister((uint8_t) BQ24155_REG_PART_REV);
          break;
        case 4:
          readRegister((uint8_t) BQ24155_REG_FAST_CHRG);
          break;
        default:
          syncRegisters();
          break;
      }
      break;

    case 'E':
    case 'e':
      local_log.concatf("%sabling battery charge...\n", (*(str) == 'E' ? "En" : "Dis"));
      charger_enabled(*(str) == 'E');
      break;
    case 'T':
    case 't':
      local_log.concatf("%sabling charge current termination...\n", (*(str) == 'T' ? "En" : "Dis"));
      charger_enabled(*(str) == 'T');
      break;

    case 'P':
      local_log.concat("Punching safety timer...\n");
      punch_safety_timer();
      break;

    case 'R':
      local_log.concat("Resetting charger...\n");
      put_charger_in_reset_mode();
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT


#endif  // CONFIG_MANUVR_BQ24155
