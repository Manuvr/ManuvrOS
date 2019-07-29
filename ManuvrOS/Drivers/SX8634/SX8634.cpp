/*
File:   MCP3918.cpp
Author: J. Ian Lindsay
*/

#include "SX8634.h"
#if defined(CONFIG_MANUVR_SX8634)


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

volatile static SX8634* INSTANCE = nullptr;

static const uint8_t _reserved_spm_offsets[31] = {
  0x00, 0x01, 0x02, 0x03, 0x08, 0x20, 0x2A, 0x31, 0x32, 0x55, 0x6A, 0x6B, 0x6C,
  0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
  0x7B, 0x7C, 0x7D, 0x7E, 0x7F
};


const char* SX8634::getModeStr(SX8634OpMode x) {
  switch (x) {
    case SX8634OpMode::ACTIVE:   return "ACTIVE";
    case SX8634OpMode::DOZE:     return "DOZE";
    case SX8634OpMode::SLEEP:    return "SLEEP";
    default:                     return "UNDEF";
  }
}

const char* SX8634::getFSMStr(SX8634_FSM x) {
  switch (x) {
    case SX8634_FSM::NO_INIT:    return "NO_INIT";
    case SX8634_FSM::RESETTING:  return "RESETTING";
    case SX8634_FSM::SPM_READ:   return "SPM_READ";
    case SX8634_FSM::SPM_WRITE:  return "SPM_WRITE";
    case SX8634_FSM::READY:      return "READY";
    case SX8634_FSM::NVM_BURN:   return "NVM_BURN";
    case SX8634_FSM::NVM_VERIFY: return "NVM_VERIFY";
    default:                     return "UNDEF";
  }
}


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/



/*
* This is an ISR.
*/
void sx8634_isr() {
  ((SX8634*)INSTANCE)->read_irq_registers();
}


int8_t SX8634::read_irq_registers() {
  if (!_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT)) {
    // If IRQ service is not inhibited, read all the IRQ-related
    //   registers in one shot.
    I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = 0x00;
      nu->buf      = _registers;
      nu->buf_len  = 10;
      _bus->queue_io_job(nu);
      return 0;
    }
  }
  return -1;
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
* Constructor
*/
SX8634::SX8634(const SX8634Opts* o) : I2CDevice(o->i2c_addr), _opts{o} {
  INSTANCE = this;
  _clear_registers();
  _ll_pin_init();
}

/*
* Destructor
*/
SX8634::~SX8634() {
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t SX8634::io_op_callahead(BusOp* _op) {
  return 0;
}


int8_t SX8634::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  StringBuilder output;

  switch (completed->get_opcode()) {
    case BusOpcode::TX_CMD:
      _sx8634_clear_flag(SX8634_FLAG_PING_IN_FLIGHT);
      _sx8634_set_flag(SX8634_FLAG_DEV_FOUND, (!completed->hasFault()));
      if (!completed->hasFault()) {
        Kernel::log("SX8634 found\n");
        _read_full_spm();
      }
      else {
        Kernel::log("SX8634 not found\n");
      }
      break;

    case BusOpcode::TX:
      switch (completed->sub_addr) {
        case SX8634_REG_COMP_OP_MODE:
          _sx8634_set_flag(SX8634_FLAG_COMPENSATING, (*(completed->buf) & 0x04));
          break;
        case SX8634_REG_GPO_CTRL:
          break;
        case SX8634_REG_GPP_PIN_ID:
        case SX8634_REG_GPP_INTENSITY:
          // TODO: Update our local shadow to reflect the new GPP state.
          // SX8634_REG_GPP_INTENSITY always follows.
          break;
        case SX8634_REG_SPM_CONFIG:
          _sx8634_set_flag(SX8634_FLAG_SPM_WRITABLE, (0x00 == (*(completed->buf) & 0x08)));
          _sx8634_set_flag(SX8634_FLAG_SPM_OPEN, (0x10 == (*(completed->buf) & 0x30)));
          _set_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR, 0);
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            _write_register(SX8634_REG_SPM_BASE_ADDR, 0x00);
          }
          break;
        case SX8634_REG_SPM_BASE_ADDR:
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            // If the SPM is open, and this register was just written, we take
            //   the next step and read or write 8 bytes at the address.
            if (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE)) {
              // Write the next 8 bytes if needed.
              _write_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
            }
            else {
              // Read the next 8 bytes if needed.
              _read_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
            }
          }
          break;
        case SX8634_REG_SPM_KEY_MSB:
        case SX8634_REG_SPM_KEY_LSB:
          break;
        case SX8634_REG_SOFT_RESET:
          switch (*(completed->buf)) {
            case 0x00:
              if (0 == _wait_for_reset(300)) {
                _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
                _ping_device();
              }
              break;
            case 0xDE:
              _write_register(SX8634_REG_SOFT_RESET, 0x00);
              break;
            default:
              break;
          }
          break;

        case SX8634_REG_IRQ_SRC:     // These registers are all read-only.
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            int spm_base_addr = _get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR);
            if (spm_base_addr <= 0x78) {
              // We're writing our shadow of the SPM. Write the next base address.
              _wait_for_reset(30);
              _write_register(SX8634_REG_SPM_BASE_ADDR, spm_base_addr + 8);
            }
            else {
              _sx8634_clear_flag(SX8634_FLAG_SPM_DIRTY);
              _close_spm_access();
              _set_fsm_position(SX8634_FSM::READY);
            }
          }
        case SX8634_REG_CAP_STAT_MSB:
        case SX8634_REG_CAP_STAT_LSB:
        case SX8634_REG_SLIDER_POS_MSB:
        case SX8634_REG_SLIDER_POS_LSB:
        case SX8634_REG_RESERVED_0:
        case SX8634_REG_RESERVED_1:
        case SX8634_REG_GPI_STAT:
        case SX8634_REG_SPM_STAT:
        case SX8634_REG_RESERVED_2:
        default:
          break;
      }
      break;

    case BusOpcode::RX:
      switch (completed->sub_addr) {
        case SX8634_REG_IRQ_SRC:
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            int spm_base_addr = _get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR);
            _sx8634_set_flag(SX8634_FLAG_SPM_SHADOWED);
            if (spm_base_addr <= 0x78) {
              // We're shadowing the SPM. Write the next base address.
              _wait_for_reset(30);
              _write_register(SX8634_REG_SPM_BASE_ADDR, spm_base_addr + 8);
            }
            else {
              if (1 == _compare_config()) {
                // Since we apparently want to make changes to the SPM, we enter
                //   that state and start the write operation.
                _wait_for_reset(30);
                _write_full_spm();
              }
              else {
                // If we won't be writing SPM config
                _close_spm_access();
                _set_fsm_position(SX8634_FSM::READY);
              }
            }
          }
          else {
            // We're going to process an IRQ.
            /*  0x00:  IRQ_SRC
                0x01:  CapStatMSB
                0x02:  CapStatLSB
                0x03:  SliderPosMSB
                0x04:  SliderPosLSB
                0x05:  <reserved>
                0x06:  <reserved>
                0x07:  GPIStat
                0x08:  SPMStat
                0x09:  CompOpMode      */
            if (0x01 & _registers[0]) {  // Operating mode interrupt
              SX8634OpMode current = (SX8634OpMode) (_registers[9] & 0x04);
              _sx8634_set_flag(SX8634_FLAG_COMPENSATING, (_registers[9] & 0x04));
              if (current != _mode) {
                output.concatf("-- SX8634 is now in mode %s\n", getModeStr(current));
                _mode = current;
              }
            }
            if (0x02 & _registers[0]) {  // Compensation completed
              _compensations++;
            }
            if (0x04 & _registers[0]) {  // Button interrupt
              uint16_t current = (((uint16_t) (_registers[1] & 0x0F)) << 8) | ((uint16_t) _registers[2]);
              if (current != _buttons) {
                // TODO: Bitshift the button values into discrete messages.
                output.concatf("-- Buttons: %u\n", current);
                _buttons = current;
              }
            }
            if (0x08 & _registers[0]) {  // Slider interrupt
              _sx8634_set_flag(SX8634_FLAG_SLIDER_TOUCHED,   (_registers[1] & 0x10));
              _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_DOWN, (_registers[1] & 0x20));
              _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_UP,   (_registers[1] & 0x40));
              uint16_t current = (((uint16_t) _registers[3]) << 8) | ((uint16_t) _registers[4]);
              if (current != _slider_val) {
                // TODO: Send slider value.
                output.concatf("-- Slider: %u\n", current);
                _slider_val = current;
              }
            }
            if (0x10 & _registers[0]) {  // GPI interrupt
              uint8_t current = _registers[7];
              if (current != _gpi_levels) {
                // TODO: Bitshift the GPI values into discrete messages.
                _gpi_levels = current;
              }
            }
            if (0x20 & _registers[0]) {  // SPM stat interrupt
              _sx8634_set_flag(SX8634_FLAG_CONF_IS_NVM, (_registers[8] & 0x08));
              _nvm_burns = (_registers[8] & 0x07);
            }
            if (0x40 & _registers[0]) {  // NVM burn interrupt
              // Burn appears to have completed. Enter the verify phase.
              _set_fsm_position(SX8634_FSM::NVM_VERIFY);
              output.concat("-- SX8634 NVM burn completed.\n");
            }
          }
          break;

        case SX8634_REG_CAP_STAT_MSB:
        case SX8634_REG_CAP_STAT_LSB:
        case SX8634_REG_SLIDER_POS_MSB:
        case SX8634_REG_SLIDER_POS_LSB:
        case SX8634_REG_RESERVED_0:
        case SX8634_REG_RESERVED_1:
        case SX8634_REG_GPI_STAT:
        case SX8634_REG_SPM_STAT:
          // Right now, for simplicity's sake, the driver reads all 10 IRQ-related
          //   registers in one shot and updates everything.
          break;

        case SX8634_REG_COMP_OP_MODE:
        case SX8634_REG_GPO_CTRL:
          break;

        case SX8634_REG_GPP_PIN_ID:
        case SX8634_REG_GPP_INTENSITY:
          // TODO: Update our local shadow to reflect the new GPP state.
          // SX8634_REG_GPP_INTENSITY always follows.
          break;
        case SX8634_REG_SPM_BASE_ADDR:
        case SX8634_REG_SPM_KEY_MSB:
        case SX8634_REG_SPM_KEY_LSB:
          break;
        case SX8634_REG_RESERVED_2:
        case SX8634_REG_SOFT_RESET:
        default:
          break;
      }
      break;

    default:
      break;
  }

  if (output.length() > 0) Kernel::log(&output);
  return 0;
}



/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printDebug(StringBuilder* output) {
  output->concatf("Touch sensor (SX8634)\t%s%s-- Found:   %c\n-- Shadow:\n----------\n", getModeStr(operationalMode()), PRINT_DIVIDER_1_STR, (deviceFound() ? 'y':'n'));
  StringBuilder::printBuffer(output, _registers, sizeof(_registers), "\t");

  output->concatf("\tFSM Position:   %s\n", getFSMStr(_fsm));
  output->concatf("\tConf source:    %s\n", (_sx8634_flag(SX8634_FLAG_CONF_IS_NVM) ? "NVM" : "QSM"));
  output->concatf("\tIRQ Inhibit:    %c\n", (_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT) ? 'y': 'n'));
  output->concatf("\tSPM shadowed:   %c\n", (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED) ? 'y': 'n'));
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    StringBuilder::printBuffer(output, _spm_shadow, sizeof(_spm_shadow), "\t  ");
    output->concatf("\tSPM Dirty:      %c\n", (_sx8634_flag(SX8634_FLAG_SPM_DIRTY) ? 'y': 'n'));
  }
  output->concatf("\tSPM open:       %c\n", (_sx8634_flag(SX8634_FLAG_SPM_OPEN) ? 'y': 'n'));
  if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
    output->concatf("\tSPM writable:   %c\n", (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE) ? 'y': 'n'));
  }
  output->concatf("\tCompensations:  %u\n", _compensations);
  output->concatf("\tNVM burns:      %u\n", _nvm_burns);

  I2CDevice::printDebug(output);
}



/*******************************************************************************
* Shadow register access functions
*******************************************************************************/

int8_t SX8634::_get_shadow_reg_mem_addr(uint8_t addr) {
  uint8_t maximum_idx = 15;
  switch (addr) {
    case SX8634_REG_IRQ_SRC:         maximum_idx--;
    case SX8634_REG_CAP_STAT_MSB:    maximum_idx--;
    case SX8634_REG_CAP_STAT_LSB:    maximum_idx--;
    case SX8634_REG_SLIDER_POS_MSB:  maximum_idx--;
    case SX8634_REG_SLIDER_POS_LSB:  maximum_idx--;
    case SX8634_REG_GPI_STAT:        maximum_idx--;
    case SX8634_REG_SPM_STAT:        maximum_idx--;
    case SX8634_REG_COMP_OP_MODE:    maximum_idx--;
    case SX8634_REG_GPO_CTRL:        maximum_idx--;
    case SX8634_REG_GPP_PIN_ID:      maximum_idx--;
    case SX8634_REG_GPP_INTENSITY:   maximum_idx--;
    case SX8634_REG_SPM_CONFIG:      maximum_idx--;
    case SX8634_REG_SPM_BASE_ADDR:   maximum_idx--;
    case SX8634_REG_SPM_KEY_MSB:     maximum_idx--;
    case SX8634_REG_SPM_KEY_LSB:     maximum_idx--;
    case SX8634_REG_SOFT_RESET:
      return (maximum_idx & 0x0F);
    case SX8634_REG_RESERVED_0:
    case SX8634_REG_RESERVED_1:
    case SX8634_REG_RESERVED_2:
    default:
      break;
  }
  return -1;
}


uint8_t SX8634::_get_shadow_reg_val(uint8_t addr) {
  int idx = _get_shadow_reg_mem_addr(addr);
  return (idx >= 0) ? _registers[idx] : 0;
}


void SX8634::_set_shadow_reg_val(uint8_t addr, uint8_t val) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if (idx >= 0) {
    _registers[idx] = val;
  }
}


void SX8634::_set_shadow_reg_val(uint8_t addr, uint8_t* buf, uint8_t len) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if ((idx + len) <= sizeof(_registers)) {
    memcpy(&_registers[idx], buf, len);
  }
}


int8_t SX8634::_write_register(uint8_t addr, uint8_t val) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if (idx >= 0) {
    _registers[idx] = val;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = addr;
      nu->buf      = &_registers[idx];
      nu->buf_len  = 1;
      return _bus->queue_io_job(nu);
    }
  }
  return -1;
}


/*******************************************************************************
* Class-specific functions
*******************************************************************************/

/*
* Reset the chip. By hardware pin (if possible) or by software command.
* TODO: Gernealize delay.
*/
int8_t SX8634::reset() {
  int8_t ret = -1;
  if (_opts.haveIRQPin()) {
    _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  }
  _flags = 0;
  _clear_registers();
  _set_fsm_position(SX8634_FSM::RESETTING);

  if (_opts.haveResetPin()) {
    setPin(_opts.reset_pin, false);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    setPin(_opts.reset_pin, true);
    ret = _wait_for_reset(300);
    if (0 == ret) {
      _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
      ret = _ping_device();
    }
    return ret;
  }
  else {
    return _write_register(SX8634_REG_SOFT_RESET, 0xDE);
  }
}


int8_t SX8634::init() {
  return reset();
}


int8_t SX8634::setMode(SX8634OpMode m) {
  return _write_register(SX8634_REG_COMP_OP_MODE, (uint8_t) m);
}


int8_t SX8634::setGPIOState(uint8_t pin, uint8_t value) {
  uint8_t pin_mask  = 1 << pin;
  uint8_t gpo_value = pin_mask & _registers[SX8634_REG_GPO_CTRL];
  uint8_t w_val = (0 != value) ? (_registers[SX8634_REG_GPO_CTRL] | pin_mask) : (_registers[SX8634_REG_GPO_CTRL] & ~pin_mask);
  //StringBuilder output;
  //output.concatf("setGPIOState(%u, %u):  ", pin, value);
  //Kernel::log(&output);
  if (w_val != _registers[SX8634_REG_GPO_CTRL]) {
    return _write_register(SX8634_REG_GPO_CTRL, w_val);
  }
  return 0;
}


uint8_t SX8634::getGPIOState(uint8_t pin) {
  pin = pin & 0x07;
  uint8_t pin_mask = (1 << pin);
  return _gpo_levels & pin_mask;
}


int8_t SX8634::setPWMValue(uint8_t pin, uint8_t value) {
  int8_t ret = -1;
  int idx = _get_shadow_reg_mem_addr(SX8634_REG_GPP_PIN_ID);
  if (idx >= 0) {
    _registers[idx + 0] = pin;
    _registers[idx + 1] = value;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = SX8634_REG_GPP_PIN_ID;
      nu->buf      = &_registers[idx];
      nu->buf_len  = 2;
      ret = _bus->queue_io_job(nu);
    }
  }
  return ret;
}


uint8_t SX8634::getPWMValue(uint8_t pin) {
  return _pwm_levels[pin & 0x07];
}



/*******************************************************************************
* SPM related functions.
*******************************************************************************/

/*
* Calling this function sets off a long chain of async bus operations that
*   ought to end up with the SPM fully read, and the state machine back to
*   READY.
*/
int8_t SX8634::_read_full_spm() {
  _set_fsm_position(SX8634_FSM::SPM_READ);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & !_sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    return _open_spm_access_r();
  }
  return -1;
}

/*
* Calling this function sets off a long chain of async bus operations that
*   ought to end up with the SPM fully written, and the state machine back to
*   READY.
*/
int8_t SX8634::_write_full_spm() {
  _set_fsm_position(SX8634_FSM::SPM_WRITE);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & _sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    return _open_spm_access_w();
  }
  return -1;
}

/*
* Inhibits interrupt service.
* Opens the SPM for reading.
*/
int8_t SX8634::_open_spm_access_r() {
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  return _write_register(SX8634_REG_SPM_CONFIG, 0x18);
}

int8_t SX8634::_open_spm_access_w() {
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  return _write_register(SX8634_REG_SPM_CONFIG, 0x10);
}

int8_t SX8634::_close_spm_access() {
  _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
  setMode(SX8634OpMode::ACTIVE);
  return _write_register(SX8634_REG_SPM_CONFIG, 0x08);
}

/*
* Read 8-bytes of SPM into shadow from the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_read_block8(uint8_t idx) {
  StringBuilder output;
  output.concatf("_read_block8(%u)\n", idx);
  Kernel::log(&output);
  I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = 0;
    nu->buf      = (_spm_shadow + idx);
    nu->buf_len  = 8;
    return _bus->queue_io_job(nu);
  }
  return -1;
}

/*
* Write 8-bytes of SPM shadow back to the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_write_block8(uint8_t idx) {
  StringBuilder output;
  output.concatf("_write_block8(%u)\n", idx);
  Kernel::log(&output);
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = 0;
    nu->buf      = (_spm_shadow + idx);
    nu->buf_len  = 8;
    return _bus->queue_io_job(nu);
  }
  return -1;
}

/*
* Compares the existing SPM shadow against the desired config. Then copies the
*   differences into the SPM shadow in preparation for write.
* Returns...
*   1 on Valid and different. Pushes state machine into SPM_WRITE.
*   0 on equality
*   -1 on invalid provided
*   -2 SPM not shadowed
*   -3 no desired config to compare against
*/
int8_t SX8634::_compare_config() {
  int8_t ret = -3;
  if (nullptr != _opts.conf) {
    ret++;
    if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
      ret++;
      uint8_t rsvd_idx  = 0;
      uint8_t given_idx = 0;
      for (uint8_t i = 0; i < sizeof(_spm_shadow); i++) {
        if (i == _reserved_spm_offsets[rsvd_idx]) {
          // Skip the comparison. Increment the reserved pointer.
          rsvd_idx++;
        }
        else {
          // This is a comparable config byte.
          if (_spm_shadow[i] != *(_opts.conf + given_idx)) {
            _sx8634_set_flag(SX8634_FLAG_SPM_DIRTY);
            _spm_shadow[i] = *(_opts.conf + given_idx);
          }
          given_idx++;
        }
      }
      ret = (_sx8634_flag(SX8634_FLAG_SPM_DIRTY)) ? 1 : 0;
    }
  }
  return ret;
}


/*******************************************************************************
* Low-level stuff
*******************************************************************************/

int8_t SX8634::_wait_for_reset(uint timeout_ms) {
  int8_t ret = -1;
  //Kernel::log("Waiting for SX8634...\n");
  if (_opts.haveIRQPin()) {
    uint8_t tries = 40;
    while ((--tries > 0) && (0 == readPin(_opts.irq_pin))) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    if (0 < tries) {
      ret = 0;
    }
  }
  else {
    vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
    ret = 0;
  }
  return ret;
}


int8_t SX8634::_clear_registers() {
  _sx8634_clear_flag(SX8634_FLAG_SPM_SHADOWED);
  memset(_registers,  0, sizeof(_registers));
  memset(_spm_shadow, 0, sizeof(_spm_shadow));
  return 0;
}


int8_t SX8634::_start_compensation() {
  return _write_register(SX8634_REG_COMP_OP_MODE, 0x04);
}


int8_t SX8634::_ll_pin_init() {
  if (_opts.haveResetPin()) {
    gpioDefine(_opts.reset_pin, GPIOMode::OUTPUT);
  }
  if (_opts.haveIRQPin()) {
    _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
    gpioDefine(_opts.irq_pin, GPIOMode::INPUT_PULLUP);
    setPinFxn(_opts.irq_pin, FALLING, sx8634_isr);
  }
  return 0;  // Both pins are technically optional.
}


/*
* Pings the device.
* TODO: Promote this into I2CDevice driver.
*/
int8_t SX8634::_ping_device() {
  if (!_sx8634_flag(SX8634_FLAG_PING_IN_FLIGHT)) {
    _sx8634_set_flag(SX8634_FLAG_PING_IN_FLIGHT);
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = -1;
      nu->buf      = nullptr;
      nu->buf_len  = 0;
      _bus->queue_io_job(nu);
      return 0;
    }
  }
  return -1;
}



/*******************************************************************************
* These functions are optional at build time and relate to programming the NVM.
* Per the datasheet: Programming the NVM _must_ be done while the supply voltage
*   is [3.6, 3.7v].
* Also, the NVM can only be written 3 times. After which the chip will have to
*   be configured manually on every reset.
* Burning capability should probably only be built into binaries that are
*   dedicated to provisioning new hardware, and never into the end-user binary.
*******************************************************************************/

#if defined(CONFIG_SX8634_PROVISIONING)

int8_t SX8634::burn_nvm() {
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    // SPM is shadowed. Whatever changes we made to it will be propagated into
    //   the NVM so that they become the defaults after resst.
    _set_fsm_position(SX8634_FSM::NVM_BURN);
  }
  return -1;
}


int8_t SX8634::_write_nvm_keys() {
  int8_t ret = -1;
  int idx = _get_shadow_reg_mem_addr(SX8634_REG_SPM_KEY_MSB);
  if (idx >= 0) {
    _registers[idx + 0] = 0x62;
    _registers[idx + 1] = 0x9D;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = SX8634_REG_SPM_KEY_MSB;
      nu->buf      = &_registers[idx];
      nu->buf_len  = 2;
      ret = _bus->queue_io_job(nu);
    }
  }
  return ret;
}


#endif  // CONFIG_SX8634_PROVISIONING
#endif  // CONFIG_MANUVR_SX8634
