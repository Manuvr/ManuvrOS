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


const char* SX8634::getModeStr(SX8634OpMode x) {
  switch (x) {
    case SX8634OpMode::ACTIVE:   return "ACTIVE";
    case SX8634OpMode::DOZE:     return "DOZE";
    case SX8634OpMode::SLEEP:    return "SLEEP";
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
  // Read all the IRQ-related registers in one shot.
  I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
  if (nullptr != nu) {
    nu->dev_addr = _opts.i2c_addr;
    nu->sub_addr = 0x00;
    nu->buf      = _irq_buffer;
    nu->buf_len  = 10;
    _bus->queue_io_job(nu);
    return 0;
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
  memset(_io_buffer,  0, 128);
  memset(_irq_buffer, 0, 10);
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
      _sx8634_set_flag(SX8634_FLAG_DEV_FOUND, (!completed->hasFault()));
      if (!completed->hasFault()) {
        Kernel::log("SX8634 found\n");
      }
      else {
        Kernel::log("SX8634 not found\n");
      }
      break;

    case BusOpcode::TX:
      switch (completed->sub_addr) {
        case SX8634_REG_IRQ_SRC:
        case SX8634_REG_CAP_STAT_MSB:
        case SX8634_REG_CAP_STAT_LSB:
        case SX8634_REG_SLIDER_POS_MSB:
        case SX8634_REG_SLIDER_POS_LSB:
        case SX8634_REG_RESERVED_0:
        case SX8634_REG_RESERVED_1:
        case SX8634_REG_GPI_STAT:
        case SX8634_REG_SPM_STAT:
          // TODO: remove irq_buffer check and migrate.
          break;

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
          break;
        case SX8634_REG_SPM_BASE_ADDR:
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            // If the SPM is open, and this register was just written, we take
            //   the next step and read or write 8 bytes at the address.
            if (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE)) {
              // Write the next 8 bytes if needed.
            }
            else {
              // Read the next 8 bytes if needed.
            }
          }
          break;
        case SX8634_REG_SPM_KEY_MSB:
        case SX8634_REG_SPM_KEY_LSB:
          break;
        case SX8634_REG_SOFT_RESET:
          break;
        case SX8634_REG_RESERVED_2:
        default:
          break;
      }
      break;

    case BusOpcode::RX:
      if (completed->buf == _irq_buffer) {
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
        if (0x01 & _irq_buffer[0]) {  // Operating mode interrupt
          SX8634OpMode current = (SX8634OpMode) (_irq_buffer[9] & 0x04);
          _sx8634_set_flag(SX8634_FLAG_COMPENSATING, (_irq_buffer[9] & 0x04));
          if (current != _mode) {
            output.concatf("-- SX8634 is now in mode %s\n", getModeStr(current));
            _mode = current;
          }
        }
        if (0x02 & _irq_buffer[0]) {  // Compensation completed
          _compensations++;
        }
        if (0x04 & _irq_buffer[0]) {  // Button interrupt
          uint16_t current = (((uint16_t) (_irq_buffer[1] & 0x0F)) << 8) | ((uint16_t) _irq_buffer[2]);
          if (current != _buttons) {
            // TODO: Bitshift the button values into discrete messages.
            output.concatf("-- Buttons: %u\n", current);
            _buttons = current;
          }
        }
        if (0x08 & _irq_buffer[0]) {  // Slider interrupt
          _sx8634_set_flag(SX8634_FLAG_SLIDER_TOUCHED,   (_irq_buffer[1] & 0x10));
          _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_DOWN, (_irq_buffer[1] & 0x20));
          _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_UP,   (_irq_buffer[1] & 0x40));
          uint16_t current = (((uint16_t) _irq_buffer[3]) << 8) | ((uint16_t) _irq_buffer[4]);
          if (current != _slider_val) {
            // TODO: Send slider value.
            output.concatf("-- Slider: %u\n", current);
            _slider_val = current;
          }
        }
        if (0x10 & _irq_buffer[0]) {  // GPI interrupt
          uint8_t current = _irq_buffer[7];
          if (current != _gpi_levels) {
            // TODO: Bitshift the GPI values into discrete messages.
            _gpi_levels = current;
          }
        }
        if (0x20 & _irq_buffer[0]) {  // SPM stat interrupt
          _sx8634_set_flag(SX8634_FLAG_CONF_IS_NVM, (_irq_buffer[8] & 0x08));
          _nvm_burns = (_irq_buffer[8] & 0x07);
        }
        if (0x40 & _irq_buffer[0]) {  // NVM burn interrupt
          // Burn appears to have completed. Enter the verify phase.
          _sx8634_clear_flag(SX8634_FLAG_SM_MASK);
          _sx8634_set_flag(SX8634_FLAG_SM_NVM_VERIFY);
          output.concat("-- SX8634 NVM burn completed.\n");
        }
      }
      else {
        switch (completed->sub_addr) {
          case SX8634_REG_IRQ_SRC:
          case SX8634_REG_CAP_STAT_MSB:
          case SX8634_REG_CAP_STAT_LSB:
          case SX8634_REG_SLIDER_POS_MSB:
          case SX8634_REG_SLIDER_POS_LSB:
          case SX8634_REG_RESERVED_0:
          case SX8634_REG_RESERVED_1:
          case SX8634_REG_GPI_STAT:
          case SX8634_REG_SPM_STAT:
            // TODO: remove irq_buffer check and migrate.
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
        memcpy(_registers, completed->buf, completed->buf_len);
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
  StringBuilder::printBuffer(output, _registers, 128, "\t");

  output->concatf("\tConf source:    %s\n", (_sx8634_flag(SX8634_FLAG_CONF_IS_NVM) ? "NVM" : "QSM"));
  output->concatf("\tSPM open:       %c\n", (_sx8634_flag(SX8634_FLAG_SPM_OPEN) ? 'y': 'n'));
  if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
    output->concatf("\tSPM writable:   %c\n", (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE) ? 'y': 'n'));
  }
  output->concatf("\tCompensations:  %u\n", _compensations);
  output->concatf("\tNVM burns:      %u\n", _nvm_burns);

  I2CDevice::printDebug(output);
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
    unsetPinIRQ(_opts.irq_pin);
  }

  if (_opts.haveResetPin()) {
    setPin(_opts.reset_pin, false);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    setPin(_opts.reset_pin, true);
  }
  else {
    *(_io_buffer + 0) = 0xDE;
    *(_io_buffer + 1) = 0x00;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = SX8634_REG_SOFT_RESET;
      nu->buf      = (_io_buffer + 0);
      nu->buf_len  = 1;
      _bus->queue_io_job(nu);

      nu = _bus->new_op(BusOpcode::TX, this);
      if (nullptr != nu) {
        nu->dev_addr = _opts.i2c_addr;
        nu->sub_addr = SX8634_REG_SOFT_RESET;
        nu->buf      = (_io_buffer + 1);
        nu->buf_len  = 1;
        _bus->queue_io_job(nu);
        return 0;
      }
    }
  }

  return _wait_for_reset();
}



int8_t SX8634::init() {
  if (0 == reset()) {
    return deviceFound() ? _read_full_spm() : _ping_device();
  }
  return -17;
}


int8_t SX8634::setMode(SX8634OpMode m) {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_COMP_OP_MODE) = (uint8_t) m;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_COMP_OP_MODE;
    nu->buf      = (_io_buffer + SX8634_REG_COMP_OP_MODE);
    nu->buf_len  = 1;
    _bus->queue_io_job(nu);
    return 0;
  }
  return -1;
}




int8_t SX8634::setGPIOState(uint8_t pin, uint8_t value) {
  return -1;
}


uint8_t SX8634::getGPIOState(uint8_t pin) {
  pin = pin & 0x07;
  uint8_t pin_mask = (1 << pin);
  return _gpo_levels & pin_mask;
}


int8_t SX8634::setPWMValue(uint8_t pin, uint8_t value) {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_GPP_PIN_ID + 0) = pin;
    *(_io_buffer + SX8634_REG_GPP_PIN_ID + 1) = value;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_COMP_OP_MODE;
    nu->buf      = (_io_buffer + SX8634_REG_COMP_OP_MODE);
    nu->buf_len  = 1;
    _bus->queue_io_job(nu);
    return 0;
  }
  return -1;
}


uint8_t SX8634::getPWMValue(uint8_t pin) {
  return _pwm_levels[pin & 0x07];
}




int8_t SX8634::_wait_for_reset() {
  int8_t ret = -1;
  if (_opts.haveIRQPin()) {
    uint8_t tries = 3;
    while ((--tries >= 0) && (0 == readPin(_opts.irq_pin))) {
      Kernel::log("Waiting for SX8634...\n");
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    setPinFxn(_opts.irq_pin, FALLING, sx8634_isr);
    ret = (0 >= tries) ? 0 : -2;
  }
  else {
    vTaskDelay(300 / portTICK_PERIOD_MS);
    ret = 0;
  }
  return ret;
}


int8_t SX8634::_clear_registers() {
  memset(_registers, 0, sizeof(_registers));
  _sx8634_clear_flag(SX8634_FLAG_REGS_SHADOWED);
  return 0;
}


int8_t SX8634::_read_full_spm() {
  _sx8634_clear_flag(SX8634_FLAG_SM_MASK);
  _sx8634_set_flag(SX8634_FLAG_SM_SPM_READ);
  int8_t ret = 0;
  for (uint8_t idx = 0; idx < 16; idx++) {
    ret -= _read_block8(idx);
  }
  return ret;
}


int8_t SX8634::_write_full_spm() {
  int8_t ret = -1;
  return ret;
}


int8_t SX8634::_start_compensation() {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_COMP_OP_MODE) = 0x04;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_COMP_OP_MODE;
    nu->buf      = (_io_buffer + SX8634_REG_COMP_OP_MODE);
    nu->buf_len  = 1;
    _bus->queue_io_job(nu);
    return 0;
  }
  return -1;
}


int8_t SX8634::_open_spm_access() {
  int8_t ret = -1;
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_SPM_CONFIG) = 0x18;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_SPM_CONFIG;
    nu->buf      = (_io_buffer + SX8634_REG_SPM_CONFIG);
    nu->buf_len  = 1;
    _bus->queue_io_job(nu);
    ret = 0;
  }
  return ret;
}


int8_t SX8634::_close_spm_access() {
  int8_t ret = -1;
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_SPM_CONFIG) = 0x08;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_SPM_CONFIG;
    nu->buf      = (_io_buffer + SX8634_REG_SPM_CONFIG);
    nu->buf_len  = 1;
    _bus->queue_io_job(nu);
    ret = 0;
  }
  return ret;
}


int8_t SX8634::_ll_pin_init() {
  if (_opts.haveResetPin()) {
    gpioDefine(_opts.reset_pin, GPIOMode::OUTPUT);
  }
  if (_opts.haveIRQPin()) {
    gpioDefine(_opts.irq_pin, GPIOMode::INPUT_PULLUP);
  }
  return 0;  // Both pins are technically optional.
}


int8_t SX8634::_read_block8(uint8_t idx) {
  I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (8 * idx);
    nu->buf      = (_io_buffer + (8 * idx));
    nu->buf_len  = 8;
    _bus->queue_io_job(nu);
    return 0;
  }
  return -1;
}


/*
* Pings the device.
* TODO: Promote this into I2CDevice driver.
*/
int8_t SX8634::_ping_device() {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
  if (nullptr != nu) {
    nu->dev_addr = _opts.i2c_addr;
    nu->sub_addr = -1;
    nu->buf      = nullptr;
    nu->buf_len  = 0;
    _bus->queue_io_job(nu);
    return 0;
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
  return -1;
}


int8_t SX8634::_write_nvm_keys() {
  int8_t ret = -1;
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    *(_io_buffer + SX8634_REG_SPM_KEY_MSB + 0) = 0x62;
    *(_io_buffer + SX8634_REG_SPM_KEY_LSB + 1) = 0x9D;
    nu->dev_addr = _dev_addr;
    nu->sub_addr = SX8634_REG_SPM_KEY_MSB;
    nu->buf      = (_io_buffer + SX8634_REG_SPM_KEY_MSB);
    nu->buf_len  = 2;
    _bus->queue_io_job(nu);
    ret = 0;
  }
  return ret;
}


#endif  // CONFIG_SX8634_PROVISIONING
#endif  // CONFIG_MANUVR_SX8634
