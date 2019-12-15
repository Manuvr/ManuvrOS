/*
File:   SX1503.cpp
Author: J. Ian Lindsay
Date:   2019.11.30

Copyright 2019 Manuvr, Inc

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

#include "SX1503.h"


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

volatile static bool isr_fired = false;


/*
* This is an ISR.
*/
void sx1503_isr() {
  isr_fired = true;
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/*
* Constructor.
*/
SX1503::SX1503(const uint8_t irq_pin, const uint8_t reset_pin) : I2CDeviceWithRegisters(SX1503_I2C_ADDR, 31, 31), _IRQ_PIN(irq_pin), _RESET_PIN(reset_pin) {
  for (uint8_t i = 0; i < 16; i++) {
    callbacks[i]  = nullptr;
    priorities[i] = 0;
  }
  defineRegister(SX1503_REG_DATA_B,       (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_DATA_A,       (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_DIR_B,        (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_DIR_A,        (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PULLUP_B,     (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PULLUP_A,     (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PULLDOWN_B,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PULLDOWN_A,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_IRQ_MASK_B,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_IRQ_MASK_A,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_SENSE_H_B,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_SENSE_H_A,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_SENSE_L_B,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_SENSE_L_A,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_IRQ_SRC_B,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_IRQ_SRC_A,    (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_EVENT_STAT_B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_EVENT_STAT_A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_MODE_B,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_MODE_A,   (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_0B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_0A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_1B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_1A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_2B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_2A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_3B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_3A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_4B, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_PLD_TABLE_4A, (uint8_t) 0x00, false, false, true);
  defineRegister(SX1503_REG_ADVANCED,     (uint8_t) 0x00, false, false, true);
}

/*
* Constructor.
*/
SX1503::SX1503(const uint8_t* buf, const unsigned int len) : SX1503(*(buf + 1), *(buf + 2)) {
  unserialize(buf, len);
}

/*
* Destructor.
*/
SX1503::~SX1503() {
  if (255 != _IRQ_PIN) {
    unsetPinIRQ(_IRQ_PIN);
  }
  if (!preserveOnDestroy() && (255 != _RESET_PIN)) {
    setPin(_RESET_PIN, 0);  // Leave the part in reset state.
  }
}


bool SX1503::isrFired() {
  return (255 != _IRQ_PIN) ? isr_fired : true;
}


int8_t SX1503::init() {
  int8_t ret = -1;
  _sx_clear_flag(SX1503_FLAG_INITIALIZED);
  if (!_sx_flag(SX1503_FLAG_PINS_CONFD)) {
    _ll_pin_init();
  }

  if (_from_blob()) {
    // Copy the blob-imparted values and clear the flag so we don't do this again.
    _sx_clear_flag(SX1503_FLAG_FROM_BLOB);
    if (0 != writeDirtyRegisters()) {
      return -3;
    }
    ret = 0;
  }
  else if (!preserveOnDestroy()) {
    ret = reset();
  }
  else {
    ret = refresh();
  }
  _sx_set_flag(SX1503_FLAG_INITIALIZED, (0 == ret));
  return ret;
}


int8_t SX1503::reset() {
  int8_t ret = -1;
  zeroRegisters();
  if (255 != _RESET_PIN) {
    setPin(_RESET_PIN, 0);
    sleep_millis(1);   // Datasheet says 300ns.
    setPin(_RESET_PIN, 1);
    if (255 != _IRQ_PIN) {
      // Wait on the IRQ pin to go high.
      uint32_t millis_abort = millis() + 15;
      while ((millis() < millis_abort) && (readPin(_IRQ_PIN) == LOW)) {}
      if (readPin(_IRQ_PIN) == HIGH) {
        ret = 0;
      }
    }
    else {
      sleep_millis(15);   // Datasheet says chip comes back in 7ms.
    }
    if (0 == ret) {  ret = refresh();   }
  }
  else {
    // TODO: Steamroll the registers with the default values.
  }

  // IRQ clears on data read.
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_ADVANCED, 0x02);   }
  // Set all event sensitivities to both edges. We use our sole IRQ line to
  //   read on ALL input changes. Even if the user hasn't asked for a callback
  //   function as our API understands it. All user calls to digitalRead() will
  //   be immediate and incur no I/O.
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_H_B, 0xFF);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_H_A, 0xFF);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_L_B, 0xFF);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_L_A, 0xFF);  }
  return ret;
}


int8_t SX1503::poll() {
  int8_t ret = 0;
  if (isr_fired) {
    _b_dat = regValue(SX1503_REG_DATA_B);
    _a_dat = regValue(SX1503_REG_DATA_A);
    if (0 == readRegister((uint8_t) SX1503_REG_DATA_B)) {
      if (0 == readRegister((uint8_t) SX1503_REG_DATA_A)) {
        ret = 1;
      }
    }
    isr_fired = false;
  }
  return ret;
}


int8_t SX1503::refresh() {
  return syncRegisters();
}


int8_t SX1503::attachInterrupt(uint8_t pin, SX1503Callback cb, uint8_t condition) {
  pin &= 0x0F;
  int8_t ret = -1;
  if (16 > pin) {
    callbacks[pin]  = cb;
    priorities[pin] = 0;
    ret = 0;
  }
  return ret;
}


/*
* Returns the number of interrupts removed.
*/
int8_t SX1503::detachInterrupt(uint8_t pin) {
  pin &= 0x0F;
  int8_t ret = (nullptr == callbacks[pin]) ? 0 : 1;
  callbacks[pin]  = nullptr;
  priorities[pin] = 0;
  return ret;
}


/*
* Returns the number of interrupts removed.
*/
int8_t SX1503::detachInterrupt(SX1503Callback cb) {
  int8_t ret = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if (cb == callbacks[i]) {
      callbacks[i] = nullptr;
      priorities[i] = 0;
      ret++;
    }
  }
  return ret;
}


/*
*/
int8_t SX1503::digitalWrite(uint8_t pin, uint8_t value) {
  int8_t ret = -2;
  if (pin < 16) {
    ret = 0;
    uint8_t reg0 = (pin < 8) ? SX1503_REG_DATA_A : SX1503_REG_DATA_B;
    uint8_t val0 = regValue(reg0);
    pin = pin & 0x07; // Restrict to 8-bits.
    uint8_t f = 1 << pin;
    val0 = (0 != value) ? (val0 | f) : (val0 & ~f);
    if ((0 == ret) & (regValue(reg0) != val0)) {  ret = writeIndirect(reg0, val0);   }
  }
  return ret;
}


/*
*/
uint8_t SX1503::digitalRead(uint8_t pin) {
  uint8_t ret = 0;
  if (pin < 8) {
    ret = (regValue(SX1503_REG_DATA_A) >> pin) & 0x01;
  }
  else if (pin < 16) {
    ret = (regValue(SX1503_REG_DATA_B) >> (pin & 0x07)) & 0x01;
  }
  return ret;
}


int8_t SX1503::gpioMode(uint8_t pin, GPIOMode mode) {
  uint8_t ret = -1;
  if (pin < 16) {
    ret = 0;
    bool in = true;
    bool pu = false;
    bool pd = false;
    switch (mode) {
      case GPIOMode::OUTPUT:
        in = false;
        break;
      case GPIOMode::INPUT_PULLUP:
        pu = true;
        break;
      case GPIOMode::INPUT_PULLDOWN:
        pd = true;
        break;
      case GPIOMode::INPUT:
      default:
        break;
    }

    uint8_t reg0 = (pin < 8) ? SX1503_REG_DIR_A : SX1503_REG_DIR_B;
    uint8_t reg1 = (pin < 8) ? SX1503_REG_PULLUP_A : SX1503_REG_PULLUP_B;
    uint8_t reg2 = (pin < 8) ? SX1503_REG_PULLDOWN_A : SX1503_REG_PULLDOWN_B;
    uint8_t reg3 = (pin < 8) ? SX1503_REG_IRQ_MASK_A : SX1503_REG_IRQ_MASK_B;
    uint8_t val0 = regValue(reg0);
    uint8_t val1 = regValue(reg1);
    uint8_t val2 = regValue(reg2);
    uint8_t val3 = regValue(reg3);
    pin = pin & 0x07; // Restrict to 8-bits.
    uint8_t f = 1 << pin;

    // Pin being set as an input means we need to unmask the interrupt.
    val0 = (in) ? (val0 | f) : (val0 & ~f);
    val1 = (pu) ? (val1 | f) : (val1 & ~f);
    val2 = (pd) ? (val2 | f) : (val2 & ~f);
    val3 = (in) ? (val3 & ~f) : (val3 | f);

    if ((0 == ret) & (regValue(reg0) != val0)) {  ret = writeIndirect(reg0, val0);   }
    if ((0 == ret) & (regValue(reg1) != val1)) {  ret = writeIndirect(reg1, val1);   }
    if ((0 == ret) & (regValue(reg2) != val2)) {  ret = writeIndirect(reg2, val2);   }
    if ((0 == ret) & (regValue(reg3) != val3)) {  ret = writeIndirect(reg3, val3);   }
  }
  return ret;
}


/*******************************************************************************
* Hidden machinery
*******************************************************************************/

int8_t SX1503::_invoke_pin_callback(uint8_t pin, bool value) {
  int8_t ret = -1;
  pin &= 0x0F;
  if (nullptr != callbacks[pin]) {
    callbacks[pin](pin, value?1:0);
    ret = 0;
  }
  return ret;
}


/*
* Setup the low-level pin details.
*/
int8_t SX1503::_ll_pin_init() {
  if (255 != _IRQ_PIN) {
    gpioDefine(_IRQ_PIN, GPIOMode::INPUT_PULLUP);
    setPinFxn(_IRQ_PIN, FALLING_PULL_UP, sx1503_isr);
  }
  if (255 != _RESET_PIN) {
    gpioDefine(_RESET_PIN, GPIOMode::OUTPUT);
  }
  _sx_set_flag(SX1503_FLAG_PINS_CONFD);
  return 0;
}



/*******************************************************************************
* Serialization fxns
*******************************************************************************/

/*
* Stores everything about the class in the provided buffer in this format...
*   Offset | Data
*   -------|----------------------
*   0      | Serializer version
*   1      | IRQ pin
*   2      | Reset pin
*   3      | Flags MSB
*   4      | Flags LSB
*   5-35   | Full register set
*
* Returns the number of bytes written to the buffer.
*/
uint8_t SX1503::serialize(uint8_t* buf, unsigned int len) {
  uint8_t offset = 0;
  if (len >= SX1503_SERIALIZE_SIZE) {
    if (_sx_flag(SX1503_FLAG_INITIALIZED)) {
      uint16_t f = _flags & SX1503_FLAG_SERIAL_MASK;
      *(buf + offset++) = SX1503_SERIALIZE_VERSION;
      *(buf + offset++) = _IRQ_PIN;
      *(buf + offset++) = _RESET_PIN;
      *(buf + offset++) = (uint8_t) 0xFF & (f >> 8);
      *(buf + offset++) = (uint8_t) 0xFF & f;
      for (uint8_t i = 0; i < 31; i++) {
        *(buf + offset++) = regValue(i);
      }
    }
  }
  return offset;
}


int8_t SX1503::unserialize(const uint8_t* buf, const unsigned int len) {
  uint8_t offset = 0;
  uint8_t expected_sz = 255;
  if (len >= SX1503_SERIALIZE_SIZE) {
    uint16_t f = 0;
    uint8_t vals[31];
    switch (*(buf + offset++)) {
      case SX1503_SERIALIZE_VERSION:
        expected_sz = SX1503_SERIALIZE_SIZE;
        f = (*(buf + 3) << 8) | *(buf + 4);
        _flags = (_flags & ~SX1503_FLAG_SERIAL_MASK) | (f & SX1503_FLAG_SERIAL_MASK);
        offset += 4;  // Skip to the register offset.
        for (uint8_t i = 0; i < 31; i++) {
          vals[i] = *(buf + offset++);
        }
        break;
      default:  // Unhandled serializer version.
        return -1;
    }
    if (0 != _clobber_all_registers(vals)) {
      return -2;
    }
    if (!_sx_flag(SX1503_FLAG_INITIALIZED)) {
      _sx_set_flag(SX1503_FLAG_FROM_BLOB);
    }
  }
  return (expected_sz == offset) ? 0 : -3;
}


int8_t SX1503::_clobber_all_registers(const uint8_t* buf) {
  bool defer = !_sx_flag(SX1503_FLAG_INITIALIZED);
  int8_t ret = writeIndirect(SX1503_REG_DATA_B, *(buf + 0), true);
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_DATA_A,       *(buf + 1),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_DIR_B,        *(buf + 2),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_DIR_A,        *(buf + 3),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PULLUP_B,     *(buf + 4),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PULLUP_A,     *(buf + 5),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PULLDOWN_B,   *(buf + 6),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PULLDOWN_A,   *(buf + 7),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_IRQ_MASK_B,   *(buf + 8),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_IRQ_MASK_A,   *(buf + 9),  true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_H_B,    *(buf + 10), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_H_A,    *(buf + 11), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_L_B,    *(buf + 12), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_SENSE_L_A,    *(buf + 13), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_IRQ_SRC_B,    *(buf + 14), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_IRQ_SRC_A,    *(buf + 15), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_EVENT_STAT_B, *(buf + 16), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_EVENT_STAT_A, *(buf + 17), defer); }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_MODE_B,   *(buf + 18), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_MODE_A,   *(buf + 19), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_0B, *(buf + 20), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_0A, *(buf + 21), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_1B, *(buf + 22), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_1A, *(buf + 23), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_2B, *(buf + 24), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_2A, *(buf + 25), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_3B, *(buf + 26), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_3A, *(buf + 27), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_4B, *(buf + 28), true);  }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_PLD_TABLE_4A, *(buf + 29), defer); }
  if (0 == ret) {  ret = writeIndirect(SX1503_REG_ADVANCED,     *(buf + 30), defer); }  // TODO: Might not work. Test.
  return ret;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t SX1503::register_write_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case SX1503_REG_DATA_B:
    case SX1503_REG_DATA_A:
    case SX1503_REG_DIR_B:
    case SX1503_REG_DIR_A:
    case SX1503_REG_PULLUP_B:
    case SX1503_REG_PULLUP_A:
    case SX1503_REG_PULLDOWN_B:
    case SX1503_REG_PULLDOWN_A:
    case SX1503_REG_IRQ_MASK_B:
    case SX1503_REG_IRQ_MASK_A:
    case SX1503_REG_SENSE_H_B:
    case SX1503_REG_SENSE_H_A:
    case SX1503_REG_SENSE_L_B:
    case SX1503_REG_SENSE_L_A:
    case SX1503_REG_IRQ_SRC_B:
    case SX1503_REG_IRQ_SRC_A:
    case SX1503_REG_EVENT_STAT_B:
    case SX1503_REG_EVENT_STAT_A:
    case SX1503_REG_PLD_MODE_B:
    case SX1503_REG_PLD_MODE_A:
    case SX1503_REG_PLD_TABLE_0B:
    case SX1503_REG_PLD_TABLE_0A:
    case SX1503_REG_PLD_TABLE_1B:
    case SX1503_REG_PLD_TABLE_1A:
    case SX1503_REG_PLD_TABLE_2B:
    case SX1503_REG_PLD_TABLE_2A:
    case SX1503_REG_PLD_TABLE_3B:
    case SX1503_REG_PLD_TABLE_3A:
    case SX1503_REG_PLD_TABLE_4B:
    case SX1503_REG_PLD_TABLE_4A:
    case SX1503_REG_ADVANCED:
    default:
      // Illegal register.
      break;
  }
  return 0;
}


int8_t SX1503::register_read_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case SX1503_REG_DATA_B:
      {
        uint8_t d = _b_dat ^ regValue(SX1503_REG_DATA_B);
        if (d) {
          for (uint8_t i = 0; i < 8; i++) {
            if ((d >> i) & 1) {
              _invoke_pin_callback((i+8), ((regValue(SX1503_REG_DATA_B) >> i) & 1));
            }
          }
        }
      }
      break;

    case SX1503_REG_DATA_A:
      {
        uint8_t d = _a_dat ^ regValue(SX1503_REG_DATA_A);
        if (d) {
          for (uint8_t i = 0; i < 8; i++) {
            if ((d >> i) & 1) {
              _invoke_pin_callback(i, ((regValue(SX1503_REG_DATA_A) >> i) & 1));
            }
          }
        }
      }
      break;

    case SX1503_REG_DIR_B:
    case SX1503_REG_DIR_A:
    case SX1503_REG_PULLUP_B:
    case SX1503_REG_PULLUP_A:
    case SX1503_REG_PULLDOWN_B:
    case SX1503_REG_PULLDOWN_A:
    case SX1503_REG_IRQ_MASK_B:
    case SX1503_REG_IRQ_MASK_A:
    case SX1503_REG_SENSE_H_B:
    case SX1503_REG_SENSE_H_A:
    case SX1503_REG_SENSE_L_B:
    case SX1503_REG_SENSE_L_A:
    case SX1503_REG_IRQ_SRC_B:
    case SX1503_REG_IRQ_SRC_A:
    case SX1503_REG_EVENT_STAT_B:
    case SX1503_REG_EVENT_STAT_A:
    case SX1503_REG_PLD_MODE_B:
    case SX1503_REG_PLD_MODE_A:
    case SX1503_REG_PLD_TABLE_0B:
    case SX1503_REG_PLD_TABLE_0A:
    case SX1503_REG_PLD_TABLE_1B:
    case SX1503_REG_PLD_TABLE_1A:
    case SX1503_REG_PLD_TABLE_2B:
    case SX1503_REG_PLD_TABLE_2A:
    case SX1503_REG_PLD_TABLE_3B:
    case SX1503_REG_PLD_TABLE_3A:
    case SX1503_REG_PLD_TABLE_4B:
    case SX1503_REG_PLD_TABLE_4A:
    case SX1503_REG_ADVANCED:
    default:
      break;
  }
  reg->unread = false;
  return 0;
}

/*
*/
void SX1503::printDebug(StringBuilder* output) {
  output->concat("---< SX1503 >---------------------------------------------------\n");
  output->concatf("RESET Pin:   %u\n", _RESET_PIN);
  output->concatf("IRQ Pin:     %u\n", _IRQ_PIN);
  output->concatf("Initialized: %c\n", (initialized() ? 'y' : 'n'));
  output->concatf("isr_fired:   %c\n", (isr_fired ? 'y' : 'n'));
  output->concatf("Preserve:    %c\n", (preserveOnDestroy() ? 'y' : 'n'));
  I2CDeviceWithRegisters::printDebug(output);
}
