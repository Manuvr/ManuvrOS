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
  //((SX8634*) INSTANCE)->isr_fired = true;
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
SX8634::SX8634(const SX8634Opts* o) : I2CDevice(o->i2c_addr),  _opts{o} {
  INSTANCE = this;
  _clear_registers();
  memset(_io_buffer, 0, 128);
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
  switch (completed->get_opcode()) {
    case BusOpcode::TX_CMD:
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
      break;

    case BusOpcode::RX:
      //switch () {
      {
        StringBuilder output;
        output.concat("-- io_buf:\n----------\n");
        StringBuilder::printBuffer(&output, completed->buf, completed->buf_len, "\t");
        //printDebug(&output);
        Kernel::log(&output);
      }
      break;

    default:
      break;
  }

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
  output->concat("-- io_buf:\n----------\n");
  StringBuilder::printBuffer(output, _io_buffer, 128, "\t");
  I2CDevice::printDebug(output);
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/*
* Reset the chip. By hardware pin (if possible) or by software command.
*/
int8_t SX8634::reset() {
  if (_opts.haveResetPin()) {
    // TODO: Gernealize delay.
    setPin(_opts.reset_pin, false);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    setPin(_opts.reset_pin, true);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return 0;
  }
  else {
    // TODO: Software reset
  }
  return -1;
}


int8_t SX8634::init() {
  if (0 == reset()) {
    return deviceFound() ? _read_full_spm() : _ping_device();
  }
  return -17;
}


bool SX8634::buttonPressed(uint8_t) {
  return false;
}


bool SX8634::buttonReleased(uint8_t) {
  return false;
}



int8_t  SX8634::setGPIOState(uint8_t pin, uint8_t value) {
  return -1;
}


uint8_t SX8634::getGPIOState(uint8_t pin) {
  return 0;
}


int8_t SX8634::_clear_registers() {
  memset(_registers, 0, 128);
  _sx8634_clear_flag(SX8634_FLAG_REGS_SHADOWED);
  return 0;
}


int8_t SX8634::_read_full_spm() {
  _clear_registers();
  _sx8634_clear_flag(SX8634_FLAG_SM_MASK);
  _sx8634_set_flag(SX8634_FLAG_SM_SPM_READ);
  int8_t ret = 0;
  for (uint8_t idx = 0; idx < 16; idx++) {
    ret -= _read_block8(idx);
  }
  return ret;
}


int8_t SX8634::_ll_pin_init() {
  if (_opts.haveResetPin()) {
    gpioDefine(_opts.reset_pin, GPIOMode::OUTPUT);
  }
  if (_opts.haveIRQPin()) {
    gpioDefine(_opts.irq_pin, GPIOMode::INPUT);   // TODO: Might use pullup?
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


#endif  // CONFIG_SX8634_PROVISIONING
#endif  // CONFIG_MANUVR_SX8634
