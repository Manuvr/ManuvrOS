#include "MCP4728.h"


/*
* Constructor.
*/
MCP4728::MCP4728(const float ext_vref, const uint8_t l_pin, const uint8_t b_pin, uint8_t address)
  : I2CDevice(address), _EXT_VREF(ext_vref), _LDAC_PIN(l_pin), _BUSY_PIN(b_pin) {}


/*
* Destructor
*/
MCP4728::~MCP4728() {}


/*
* Debug
*/
void MCP4728::printDebug(StringBuilder* output) {
  output->concat("-- MCP4728\n");
  output->concatf("--\tExternal Vref: %.4fv\n", _EXT_VREF);
  output->concatf("--\tPins conf'd:   %c\n", _mcp4728_flag(MCP4728_FLAG_PINS_CONFIGURED) ? 'y':'n');
  output->concatf("--\tFound:         %c\n", devFound() ? 'y':'n');
  output->concatf("--\tInitialized:   %c\n", initialized() ? 'y':'n');
  //output->concatf("--\tEnabled:       %c\n", enabled() ? 'y':'n');

  for (uint8_t i = 0; i < 4; i++) {
    output->concatf("--\tCHAN %u:  0x%03x  %.4fv\n", i, chanValue(i), _DAC_VOLTS[i]);
    output->concatf("--\t  Volatile: 0x%02x  0x%02x  0x%02x\n", _DAC_VALUES[(6 * i) + 0], _DAC_VALUES[(6 * i) + 1], _DAC_VALUES[(6 * i) + 2]);
    output->concatf("--\t  EEPROM:   0x%02x  0x%02x  0x%02x\n", _DAC_VALUES[(6 * i) + 3], _DAC_VALUES[(6 * i) + 4], _DAC_VALUES[(6 * i) + 5]);
  }
}


/*
* TODO: Unconfuse return values.
* Initialization function. Sets up pins, takes bus adapter reference.
* The real init work is handled on callback with a successful WHO_AM_I match.
*/
int8_t MCP4728::init(I2CAdapter* b) {
  _flags &= MCP4728_FLAG_RESET_MASK;
  int8_t ret = _ll_pin_init();
  assignBusInstance(b);
  for (uint8_t i = 0; i < 4; i++) {
    _DAC_VOLTS[i]  = 0;
  }
  for (uint8_t i = 0; i < 24; i++) {
    _DAC_VALUES[i] = 0;
  }

  if (0 == ret) {  // Pins are set up.
    if (nullptr != _bus) {
      ret = refresh();
    }
    else {
      ret = -2;
    }
  }
  return ret;
}


/*
*/
int8_t MCP4728::reset() {
  int8_t ret = -1;
  // TODO: Implement general call reset.
  return ret;
}


/*
* Refresh the local variables with the state of the hardware.
*/
int8_t MCP4728::refresh() {
  int8_t ret = -2;
  I2CBusOp* op = _bus->new_op(BusOpcode::RX, this);
  if (nullptr != op) {
    op->dev_addr = _dev_addr;
    op->setBuffer(_DAC_VALUES, 24);
    if (0 == _bus->queue_io_job(op)) {
      ret = 0;
    }
  }
  return ret;
}


/*
*/
int8_t MCP4728::enable(bool x) {
  int8_t ret = -1;
  return ret;
}


int8_t MCP4728::chanPowerState(uint8_t chan, MCP4728PwrState pwr_state) {
  int8_t ret = -1;
  if (4 > chan) {
    const uint8_t SHAD_IDX = 6 * chan;
    uint8_t old_pwr_state = _DAC_VALUES[SHAD_IDX+1] & 0x60;
    uint8_t new_pwr_state = ((uint8_t) pwr_state) << 5;
    ret--;
    if (old_pwr_state != new_pwr_state) {
      uint8_t* buf = (uint8_t*) malloc(4);
      ret--;
      if (nullptr != buf) {
        ret--;
        *(buf + 0)  = 0xA0 | ((_DAC_VALUES[1] & 0x60) >> 3);
        *(buf + 0) |= ((_DAC_VALUES[7]  & 0x60) >> 5);
        *(buf + 1) |= ((_DAC_VALUES[13] & 0x60) << 1);
        *(buf + 1) |= ((_DAC_VALUES[19] & 0x60) >> 1);
        I2CBusOp* op = _bus->new_op(BusOpcode::RX, this);
        if (nullptr != op) {
          ret--;
          op->dev_addr = _dev_addr;
          op->setBuffer(buf, 2);
          if (0 == _bus->queue_io_job(op)) {
            ret = 0;
          }
          else {
            // Clean up our allocation if we must bail out.
            op->setBuffer(nullptr, 0);
            free(buf);
          }
        }
      }
    }
  }
  return ret;
}


int8_t MCP4728::chanGain(uint8_t chan, uint8_t gain) {
  int8_t ret = -1;
  if (4 > chan) {
    const uint8_t SHAD_IDX = 6 * chan;
    uint8_t old_state = _DAC_VALUES[SHAD_IDX+1] & 0x10;
    uint8_t new_state = (gain & 0x01) << 5;
    ret--;
    if (old_state != new_state) {
      uint8_t* buf = (uint8_t*) malloc(4);
      ret--;
      if (nullptr != buf) {
        ret--;
        *(buf + 0)  = 0xC0 | ((_DAC_VALUES[1] & 0x10) >> 1);
        *(buf + 0) |= ((_DAC_VALUES[7]  & 0x10) >> 2);
        *(buf + 0) |= ((_DAC_VALUES[13] & 0x10) << 3);
        *(buf + 0) |= ((_DAC_VALUES[19] & 0x10) >> 4);
        I2CBusOp* op = _bus->new_op(BusOpcode::RX, this);
        if (nullptr != op) {
          ret--;
          op->dev_addr = _dev_addr;
          op->setBuffer(buf, 1);
          if (0 == _bus->queue_io_job(op)) {
            ret = 0;
          }
          else {
            // Clean up our allocation if we must bail out.
            op->setBuffer(nullptr, 0);
            free(buf);
          }
        }
      }
    }
  }
  return ret;
}


int8_t MCP4728::chanValue(uint8_t chan, uint16_t value) {
  int8_t ret = -1;
  if (4 > chan) {
    const uint8_t SHAD_IDX = 6 * chan;
    uint16_t current_val = _DAC_VALUES[SHAD_IDX + 2] | (((uint16_t) (_DAC_VALUES[SHAD_IDX + 1] & 0x0F)) << 8);
    ret--;
    if (value != current_val) {
      uint8_t* buf = (uint8_t*) malloc(4);
      ret--;
      if (nullptr != buf) {
        ret--;
        *(buf + 0)  = 0x40 | (chan << 1);
        *(buf + 1)  = (_DAC_VALUES[SHAD_IDX + 1] & 0xF0) | ((uint8_t) (value >> 8) & 0x0F);
        *(buf + 2)  = (uint8_t) (value & 0x00FF);
        I2CBusOp* op = _bus->new_op(BusOpcode::RX, this);
        if (nullptr != op) {
          ret--;
          op->dev_addr = _dev_addr;
          op->setBuffer(buf, 2);
          if (0 == _bus->queue_io_job(op)) {
            ret = 0;
          }
          else {
            // Clean up our allocation if we must bail out.
            op->setBuffer(nullptr, 0);
            free(buf);
          }
        }
      }
    }
  }
  return ret;
}


MCP4728PwrState MCP4728::chanPowerState(uint8_t chan) {
  const uint8_t SHAD_IDX = 6 * chan;
  return (MCP4728PwrState) ((_DAC_VALUES[SHAD_IDX+1] & 0x60) >> 5);
}


int8_t MCP4728::chanGain(uint8_t chan) {
  const uint8_t SHAD_IDX = 6 * chan;
  return (_DAC_VALUES[SHAD_IDX+1] & 0x10) ? 2 : 1;
}


uint16_t MCP4728::chanValue(uint8_t chan) {
  const uint8_t SHAD_IDX = 6 * chan;
  return _DAC_VALUES[SHAD_IDX + 2] | (((uint16_t) (_DAC_VALUES[SHAD_IDX + 1] & 0x0F)) << 8);
}


int8_t MCP4728::storeToNVM(uint8_t new_i2c_addr) {
  return -1;
}


/*
*
*/
int8_t MCP4728::_write_channel(uint8_t chan, uint16_t val) {
  const uint8_t SHAD_IDX = 6 * chan;
  int8_t ret = -2;
  I2CBusOp* op = _bus->new_op(BusOpcode::RX, this);
  if (nullptr != op) {
    _DAC_VALUES[SHAD_IDX+1] = (val >> 8) & 0x00FF;
    _DAC_VALUES[SHAD_IDX+2] = val & 0x00FF;
    op->dev_addr = _dev_addr;
    op->setBuffer(_DAC_VALUES, 24);
    if (0 == _bus->queue_io_job(op)) {
      ret = 0;
    }
  }
  return ret;
}


int8_t MCP4728::_recalculate_chan_voltage(uint8_t chan) {
  const uint8_t  IDX_BASE   = chan * 6;
  const uint8_t  CONF_BYTE  = _DAC_VALUES[IDX_BASE + 1];
  const uint16_t CHAN_COUNT = _DAC_VALUES[IDX_BASE + 2] | (((uint16_t) (CONF_BYTE & 0x0F)) << 8);
  float chan_volts = 0.0;
  int8_t ret = -2;
  if (0 == (CONF_BYTE & 0x60)) {
    float lsb_value  = 0.0;
    if (0 == (CONF_BYTE & 0x80)) {
      lsb_value = (_EXT_VREF / 4096.0);
    }
    else {
      lsb_value = (2.048 / 4096.0) * ((CONF_BYTE & 0x10) ? 2.0 : 1.0);
    }
    chan_volts = CHAN_COUNT * lsb_value;
  }
  _mcp4728_clear_flag(MCP4728_FLAG_CHAN_A_DIRTY << chan);
  _DAC_VOLTS[chan] = chan_volts;
  return ret;
}


/*
* Low-level pin intialization.
*/
int8_t MCP4728::_ll_pin_init() {
  int8_t ret = 0;
  if (!_mcp4728_flag(MCP4728_FLAG_PINS_CONFIGURED)) {
    if (255 != _BUSY_PIN) {  // Optional pin
      if (0 != pinMode(_BUSY_PIN, GPIOMode::INPUT_PULLUP)) {
        return -2;
      }
    }

    if (255 != _LDAC_PIN) {
      if (0 != pinMode(_LDAC_PIN, GPIOMode::OUTPUT)) {
        return -2;
      }
      setPin(_LDAC_PIN, false);
      _mcp4728_set_flag(MCP4728_FLAG_PINS_CONFIGURED);
    }
  }
  return ret;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t MCP4728::io_op_callahead(BusOp* _op) {
  return 0;   // Permit the transfer, always.
}

/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return BUSOP_CALLBACK_NOMINAL on success, or appropriate error code.
*/
int8_t MCP4728::io_op_callback(BusOp* _op) {
  int8_t ret = BUSOP_CALLBACK_NOMINAL;
  I2CBusOp* op = (I2CBusOp*) _op;

  if (0 == op->dev_addr) {
    // This was a general-call operation.
  }
  else {
    uint8_t* buf = op->buffer();
    if (!op->hasFault()) {
      switch (op->get_opcode()) {
        case BusOpcode::TX:
          switch (*buf & 0xE0) {  // Look at the command byte.
            case 0x00:    // Wrote volatile channel value.
            case 0x10:    // Wrote volatile channel value.
            case 0x20:    // Wrote volatile channel value.
            case 0x30:    // Wrote volatile channel value.
              // TODO: Update shadows for both register and EEPROM.
              // TODO: Recalculate channel voltages.
              free(op->buffer());
              break;
            case 0x40:    // Wrote channel value with EEPROM update.
              {
                const uint8_t chan = (*buf & 0x06) >> 1;
                const uint8_t SHAD_IDX = 6 * chan;
                switch (*buf & 0x18) {
                  case 0x10:   // Multi-write for input registers. With EEPROM change.
                    _DAC_VALUES[SHAD_IDX + 4] = (_DAC_VALUES[SHAD_IDX + 1] & 0xF0) | (*buf+1);
                    _DAC_VALUES[SHAD_IDX + 5] = (*buf+2);
                  case 0x00:   // Multi-write for input registers. No EEPROM change.
                    _DAC_VALUES[SHAD_IDX + 1] = (_DAC_VALUES[SHAD_IDX + 1] & 0xF0) | (*buf+1);
                    _DAC_VALUES[SHAD_IDX + 2] = (*buf+2);
                    break;
                  case 0x18:   // Single channel write with EEPROM change.
                    _DAC_VALUES[SHAD_IDX + 1] = (_DAC_VALUES[SHAD_IDX + 1] & 0xF0) | (*buf+1);
                    _DAC_VALUES[SHAD_IDX + 2] = (*buf+2);
                    _DAC_VALUES[SHAD_IDX + 4] = (_DAC_VALUES[SHAD_IDX + 1] & 0xF0) | (*buf+1);
                    _DAC_VALUES[SHAD_IDX + 5] = (*buf+2);
                    break;
                  default:  // Anything else is invalid.
                    break;
                }
                // TODO: Update shadows for both register and EEPROM.
                _recalculate_chan_voltage(chan);
              }
              free(op->buffer());
              break;

            case 0x60:    // Wrote new i2c address.
              // TODO: Anything?
              free(op->buffer());
              break;

            case 0x80:    // Wrote voltage reference bits for all channels.
            case 0xC0:    // Wrote gain bits for all channels.
              {
                const uint8_t pd_bits = *(buf + 0) & 0x0F;
                uint8_t shift_size    = (0x80 == (*buf & 0xE0)) ? 7:4;
                for (uint8_t i = 0; i < 4; i++) {
                  const uint8_t SHAD_IDX = 6 * i;
                  uint8_t scpd_bits = (pd_bits >> (3 - i)) & 0x01;
                  _DAC_VALUES[SHAD_IDX + 1] = (_DAC_VALUES[SHAD_IDX + 1] & ~(1 << shift_size)) | (scpd_bits << shift_size);
                  _recalculate_chan_voltage(i);
                }
              }
              free(op->buffer());
              break;

            case 0xA0:    // Wrote power-down bits for all channels.
              {
                const uint8_t pd_bits = (*(buf + 0) << 4) | (*(buf + 1) >> 4);
                for (uint8_t i = 0; i < 4; i++) {
                  const uint8_t SHAD_IDX = 6 * i;
                  uint8_t scpd_bits = (pd_bits >> (6 - (i << 1))) & 0x03;
                  _DAC_VALUES[SHAD_IDX + 1] = (_DAC_VALUES[SHAD_IDX + 1] & 0x9F) | (scpd_bits << 5);
                  _recalculate_chan_voltage(i);
                }
              }
              free(op->buffer());
              break;
          }
          break;

        case BusOpcode::RX:  // Only RX in driver is a full refresh.
          if (24 == op->bufferLen()) {
            for (uint8_t chan = 0; chan < 4; chan++) {
              _recalculate_chan_voltage(chan);
            }
            _mcp4728_set_flag(MCP4728_FLAG_INITIALIZED);
          }
          break;

        default:
          break;
      }
    }
    else {
      ret = BUSOP_CALLBACK_ERROR;
    }
  }
  return ret;
}
