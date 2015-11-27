/*
File:   ADP8866.cpp
Author: J. Ian Lindsay
Date:   2014.05.27


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "StaticHub/StaticHub.h"
#include "ADP8866.h"

/*
* TODO: This is a table of constants relating channel current to a register value.
*/


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ADP8866::__class_initializer() {
  EventReceiver::__class_initializer();
  reset_pin         = 0;
  irq_pin           = 0;
  stored_dimmer_val = 0;
  class_mode        = 0;
  power_mode        = 0;
  init_complete     = false;
}


/*
* Constructor. Takes i2c address as argument.
*/
ADP8866::ADP8866(uint8_t _reset_pin, uint8_t _irq_pin, uint8_t addr) : I2CDeviceWithRegisters() {
  __class_initializer();
  _dev_addr = addr;
  
  reset_pin = _reset_pin;
  irq_pin   = _irq_pin;
  pinMode(_irq_pin, INPUT_PULLUP); 
  pinMode(_reset_pin, OUTPUT);
  
  digitalWrite(_reset_pin, 0);
  
  init_complete = false;
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
ADP8866::~ADP8866(void) {
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
  
  
  // TODO: When the driver inits, we shouldn't have any LEDs on until the user sets
  //   some mandatory constraint so ve doesn't fry vis LEDs.
  writeIndirect(ADP8866_ISC1, 0x10, true);
  writeIndirect(ADP8866_ISC2, 0x10, true);
  writeIndirect(ADP8866_ISC3, 0x10, true);
  writeIndirect(ADP8866_ISC4, 0x10, true);
  writeIndirect(ADP8866_ISC5, 0x10, true);
  writeIndirect(ADP8866_ISC6, 0x10, true);
  writeIndirect(ADP8866_ISC7, 0x10, true);
  writeIndirect(ADP8866_ISC8, 0x10, true);
  writeIndirect(ADP8866_ISC9, 0x10);
  init_complete = true;
  return 0;
}


/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void ADP8866::operationCompleteCallback(I2CQueuedOperation* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  
  DeviceRegister *temp_reg = getRegisterByBaseAddress(completed->sub_addr);
  switch (completed->sub_addr) {
      case ADP8866_MANU_DEV_ID:
        if (0x53 == *(temp_reg->val)) {
          temp_reg->unread = false;
          // Must be 0b01010011. If so, we init...
          init();
        }
        break;
      case ADP8866_MDCR:
        break;
      case ADP8866_INT_STAT:
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
      case ADP8866_BLMX:
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
      case ADP8866_ISC1:
        break;
      case ADP8866_ISC2:
        break;
      case ADP8866_ISC3:
        break;
      case ADP8866_ISC4:
        break;
      case ADP8866_ISC5:
        break;
      case ADP8866_ISC6:
        break;
      case ADP8866_ISC7:
        break;
      case ADP8866_ISC8:
        break;
      case ADP8866_ISC9:
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
        temp_reg->unread = false;
        break;
  }
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}



/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ADP8866::getReceiverName() {  return "ADP8866";  }


/*
* Dump this item to the dev log.
*/
void ADP8866::printDebug(StringBuilder* temp) {
  if (NULL == temp) return;
  EventReceiver::printDebug(temp);
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\tinit_complete:      %s\n", init_complete ? "yes" :"no");
  temp->concatf("\tpower_mode:        %d\n", power_mode);
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 
* 
* These are overrides from EventReceiver interface...
****************************************************************************************************/
/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ADP8866::bootComplete() {
  EventReceiver::bootComplete();
  digitalWrite(reset_pin, 1);   // Release the reset pin.

  readRegister((uint8_t) ADP8866_MANU_DEV_ID);
  //writeDirtyRegisters();  // If i2c is broken, this will hang the boot process...
  return 1;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ADP8866::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }
  
  return return_value;
}



int8_t ADP8866::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_POWER_MODE:
      break;
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
      
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}



void ADP8866::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'r':
      reset();
      break;
    case 'g':
      syncRegisters();
      break;
    case 'l':
    case 'L':
      enable_channel(temp_byte, (*(str) == 'L'));
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
      set_brightness(*(str) - 0x30, temp_byte);
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}



/****************************************************************************************************
* Functions specific to this class....                                                              *
****************************************************************************************************/
/*
* This is a function that holds high-level macros for LED behavior that is generic.
* Example:
*   Some classes might wish to take direct control over the LEDs to do something fancy.
*   Other classes might not care about micromanaging LED behavior, but would like to use
*     them as indirectly as possible. This is the type of class this function is meant for. 
*/
void ADP8866::set_led_mode(uint8_t num) {
}


void ADP8866::quell_all_timers() {
}


/*
* Set the global brightness for LEDs managed by this chip.
* Stores the previous value.
*/
void ADP8866::set_brightness(uint8_t nu_brightness) {
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
  uint8_t desired_state = (en ? bitmask : 0);
  
  if ((present_state & bitmask) ^ desired_state) {
    // If the present state and the desired state differ, Set the register equal to the
    //   present state masked with the desired state.
    writeIndirect(reg_addr, desired_state ? (present_state | desired_state) : (present_state & desired_state));
  }
}

/*
* Returns the boolean answer to the question: Is the given channel enabled?
*/
bool ADP8866::channel_enabled(uint8_t chan) {
  //return (regValue(ADP8866_CHANNEL_ENABLE) & (1 << chan));
  return false;
}

/*
* Perform a software reset.
*/
void ADP8866::reset() {
  //digitalWrite(reset_pin, 0);
  digitalWrite(reset_pin, !digitalRead(reset_pin));
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
  StaticHub::log("ADP8866 Power mode set. \n");
}




