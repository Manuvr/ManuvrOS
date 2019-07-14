/*
File:   KridaDimmer.cpp
Author: J. Ian Lindsay
Date:   2016.04.09

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

#include "KridaDimmer.h"


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
KridaDimmer::KridaDimmer(uint8_t addr) : I2CDevice(addr) {
  for (uint8_t i = 0; i < 4; i++) {
    _channel_minima[i] = 33;
    _channel_maxima[i] = 100;
    _channel_values[i] = 0;
    _channel_buffer[i] = 0;
    _channel_hw[i]     = 0;
  }
}


/*
* Destructor.
*/
KridaDimmer::~KridaDimmer() {
}




/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t KridaDimmer::io_op_callahead(BusOp* _op) {
  return 0;
}


int8_t KridaDimmer::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  uint8_t addr = (completed->sub_addr & 0x0F);

  switch (completed->get_opcode()) {
    case BusOpcode::RX:
      switch (addr) {
        case 0:
        case 1:
        case 2:
        case 3:
          if (!completed->hasFault()) {
            for (uint8_t i = 0; i < completed->buf_len; i++) {
              _channel_hw[addr]     = completed->buf[i];
              _channel_values[addr] = 100 - completed->buf[i];
            }
          }
          break;
        default:
          break;
      }
      break;

    case BusOpcode::TX:
      switch (addr) {
        case 0:
        case 1:
        case 2:
        case 3:
          if (completed->hasFault()) {
            _channel_values[addr] = 100 - completed->buf[0];
          }
          else {
            _channel_hw[addr]     = completed->buf[0];
          }
          break;
        default:
          break;
      }
      break;

    case BusOpcode::TX_CMD:
      break;

    default:
      break;
  }

  return 0;
}


/*
* Dump this item to the dev log.
*/
void KridaDimmer::printDebug(StringBuilder* output) {
  output->concat("\n==< KridaDimmer >======================\n\tValue\tHW\tMin\tMax\n");
  for (uint8_t i = 0; i < 4; i++) {
    printChannel(i, output);
    output->concat("\n");
  }
}


/*
* Dump a channel to the dev log.
*/
void KridaDimmer::printChannel(uint8_t chan, StringBuilder* output) {
  if (is_channel_valid(chan)) {
    output->concatf("\t%u\t%u\t%u\t%u", _channel_values[chan], _channel_hw[chan], _channel_minima[chan], _channel_maxima[chan]);
  }
}


/*******************************************************************************
* Dimmer and channel control functions
*******************************************************************************/
void KridaDimmer::set_brightness(uint8_t chan, uint8_t nu_brightness) {
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      // Zero values are "off" in the hardware. So we always allow it. Otherwise,
      //   constrain the given value to the range defined for the circuit.
      {
        uint8_t safe_brightness = (0 == nu_brightness) ? 0 : strict_max(nu_brightness, _channel_minima[chan]);
        safe_brightness = strict_min(safe_brightness, (uint8_t) 100);
        if (safe_brightness != _channel_values[chan]) {
          _channel_values[chan] = safe_brightness;
          // Only generate bus activity if there is a change to be made.
          // TODO: Write register.
          I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
          if (nu) {
            _channel_buffer[chan] = 100 - safe_brightness;
            nu->dev_addr = _dev_addr;
            nu->sub_addr = (uint16_t) (0x80 | chan);
            nu->buf      = &_channel_buffer[chan];
            nu->buf_len  = 1;
            _bus->queue_io_job(nu);
          }
        }
      }
      break;

    default:
      break;
  }
}


/*
* Enable or disable a specific channel. If something needs to be written to the
*   dimmer board, will do that as well.
*/
void KridaDimmer::enable_channel(uint8_t chan, bool en) {
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      set_brightness(chan, en ? ((0 == _channel_values[chan]) ? _channel_maxima[chan] : _channel_values[chan]) : 0);
      break;
    default:
      break;
  }
}


/*
* Returns the boolean answer to the question: Is the given channel enabled?
*/
bool KridaDimmer::channel_enabled(uint8_t chan) {
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      return (0 != _channel_values[chan]);
    default:
      break;
  }
  return false;
}


/*
* If there is a difference between the i/o buffer and the register shadow,
*   we assume that there is an incomplete bus operation still outstanding.
*/
bool KridaDimmer::channel_has_outstanding_io(uint8_t chan) {
  bool ret = false;
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      ret |= (_channel_buffer[chan] != _channel_hw[chan]);
      break;
    default:
      break;
  }
  return ret;
}
