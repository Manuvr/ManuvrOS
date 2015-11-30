/*
File:   MGC3130.cpp
Author: J. Ian Lindsay
Date:   2015.06.01


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


This class is a driver for Microchip's MGC3130 e-field gesture sensor. It is meant
  to be used in ManuvrOS. This driver was derived from my prior derivation of the
  Arduino Hover library. Nothing remains of the original library. My original fork
  can be found here:
  https://github.com/jspark311/hover_arduino

*/

#include "StaticHub/StaticHub.h"
#include "MGC3130.h"
#include <StringBuilder/StringBuilder.h>


const MessageTypeDef mgc3130_message_defs[] = {
  {  MANUVR_MSG_SENSOR_MGC3130            , MSG_FLAG_IDEMPOTENT,  "MGC3130",                   ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_SENSOR_MGC3130_INIT       , MSG_FLAG_IDEMPOTENT,  "MGC3130_INIT",              ManuvrMsg::MSG_ARGS_NONE, NULL }, //

  /* 
    For messages that have arguments, we have the option of defining inline lables for each parameter.
    This is advantageous for debugging and writing front-ends. We case-off here to make this choice at
    compile time.
  */
  #if defined (__ENABLE_MSG_SEMANTICS)
  {  MANUVR_MSG_GESTURE_LEGEND            , MSG_FLAG_EXPORTABLE,  "GESTURE_LEGEND",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_DEFINITION        , MSG_FLAG_EXPORTABLE,  "GESTURE_DEFINITION",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_OBLITERATE        , MSG_FLAG_EXPORTABLE,  "GESTURE_OBLITERATE",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_LINK              , MSG_FLAG_EXPORTABLE,  "GESTURE_LINK",              ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_UNLINK            , MSG_FLAG_EXPORTABLE,  "GESTURE_UNLINK",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_RECOGNIZED        , MSG_FLAG_EXPORTABLE,  "GESTURE_RECOGNIZED",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_NUANCE            , MSG_FLAG_EXPORTABLE,  "GESTURE_NUANCE",            ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_DISASSERT         , MSG_FLAG_EXPORTABLE,  "GESTURE_DISASSERT",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_GESTURE_ONE_SHOT          , MSG_FLAG_EXPORTABLE,  "GESTURE_ONE_SHOT",          ManuvrMsg::MSG_ARGS_NONE }, //
  #else
  {  MANUVR_MSG_GESTURE_LEGEND            , MSG_FLAG_EXPORTABLE,  "GESTURE_LEGEND",            ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_DEFINITION        , MSG_FLAG_EXPORTABLE,  "GESTURE_DEFINITION",        ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_OBLITERATE        , MSG_FLAG_EXPORTABLE,  "GESTURE_OBLITERATE",        ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_LINK              , MSG_FLAG_EXPORTABLE,  "GESTURE_LINK",              ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_UNLINK            , MSG_FLAG_EXPORTABLE,  "GESTURE_UNLINK",            ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_RECOGNIZED        , MSG_FLAG_EXPORTABLE,  "GESTURE_RECOGNIZED",        ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_NUANCE            , MSG_FLAG_EXPORTABLE,  "GESTURE_NUANCE",            ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_DISASSERT         , MSG_FLAG_EXPORTABLE,  "GESTURE_DISASSERT",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_GESTURE_ONE_SHOT          , MSG_FLAG_EXPORTABLE,  "GESTURE_ONE_SHOT",          ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  #endif
};


ManuvrEvent _isr_read_event(MANUVR_MSG_SENSOR_MGC3130);
volatile int _isr_ts_pin = 0;


/*
* This is an ISR to initiate the read of the MGC3130.
*/
void mgc3130_isr_check() {
  if (_isr_ts_pin) { 
    detachInterrupt(_isr_ts_pin);
  }
  //EventManager::isrRaiseEvent(&_isr_read_event);
}



/*
* Some of you will recognize this design pattern for classes that are bound to an interrupt pin.
*   This is perfectly alright as long as we don't try to put more than one such chip in your project.
*/
volatile MGC3130* MGC3130::INSTANCE = NULL;


void gest_0() {
}


void gest_1() {
}


void gest_2() {
}


void gest_3() {
}




MGC3130::MGC3130(int ts, int rst, uint8_t addr) {
  _dev_addr  = addr;
  _ts_pin    = (uint8_t) ts;
  _isr_ts_pin = ts;
  _reset_pin = (uint8_t) rst;
  
  _isr_read_event.specific_target = this;
  _isr_read_event.callback        = this;
  _isr_read_event.priority        = 3;
  _isr_read_event.isManaged(true);

  // TODO: Formallize this for build targets other than Arduino. Use the abstracted Manuvr
  //       GPIO class instead.
  digitalWrite(_ts_pin, 0);
  pinMode(_ts_pin, INPUT_PULLUP);     //Used by TS line on MGC3130

  digitalWrite(_reset_pin, 0);
  pinMode(_reset_pin, OUTPUT);   //Used by reset line on MGC3130

  _irq_pin_0 = 0;
  _irq_pin_1 = 0;
  _irq_pin_2 = 0;
  _irq_pin_3 = 0;
  
  flags            = 0;
  _pos_x           = -1;
  _pos_y           = -1;
  _pos_z           = -1;
  wheel_position   = 0;
  last_tap         = 0;
  last_double_tap  = 0;
  last_swipe       = 0;
  special          = 0;
  last_event       = 0;
  last_touch       = 0;
  touch_counter    = 0;
  last_touch_noted = last_touch;
  last_seq_num     = 0; 
  last_nuance_sent = millis();
  INSTANCE = this;

  // Inform the EventManager of the codes we will be using...
  ManuvrMsg::registerMessages(mgc3130_message_defs, sizeof(mgc3130_message_defs) / sizeof(MessageTypeDef));
}


#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
/*
* If you are dealing with a board that has pins/IRQs that don't match, case it off
* in the fxn below.
* A return value of -1 means invalid IRQ/unhandled case.
*/
int MGC3130::get_irq_num_by_pin(int _pin) {
  #if defined(_BOARD_FUBARINO_MINI_)    // Fubarino has goofy IRQ/Pin relationships.
  switch (_pin) {
    case PIN_INT0: return 0;
    case PIN_INT1: return 1;
    case PIN_INT2: return 2;
    case PIN_INT3: return 3;
    case PIN_INT4: return 4;
  }
  #endif
  return -1;
}
#endif


void MGC3130::init() {
  if (_irq_pin_0) {
  pinMode(_irq_pin_0, INPUT_PULLUP);
  #if defined(BOARD_IRQS_AND_PINS_DISTINCT)
    int fubar_irq_number = get_irq_num_by_pin(_irq_pin_0);
    if (fubar_irq_number >= 0) {
    attachInterrupt(fubar_irq_number, gest_0, FALLING);
    }
  #else
    attachInterrupt(_irq_pin_0, gest_0, FALLING);
  #endif
  }
  
  if (_irq_pin_1) {
  pinMode(_irq_pin_1, INPUT_PULLUP);
  #if defined(BOARD_IRQS_AND_PINS_DISTINCT)
    int fubar_irq_number = get_irq_num_by_pin(_irq_pin_1);
    if (fubar_irq_number >= 0) {
    attachInterrupt(fubar_irq_number, gest_1, FALLING);
    }
  #else
    attachInterrupt(_irq_pin_1, gest_1, FALLING);
  #endif
  }
  
  if (_irq_pin_2) {
  pinMode(_irq_pin_2, INPUT_PULLUP);
  #if defined(BOARD_IRQS_AND_PINS_DISTINCT)
    int fubar_irq_number = get_irq_num_by_pin(_irq_pin_2);
    if (fubar_irq_number >= 0) {
    attachInterrupt(fubar_irq_number, gest_2, FALLING);
    }
  #else
    attachInterrupt(_irq_pin_2, gest_2, FALLING);
  #endif
  }
  
  if (_irq_pin_3) {
  pinMode(_irq_pin_3, INPUT_PULLUP);
  #if defined(BOARD_IRQS_AND_PINS_DISTINCT)
    int fubar_irq_number = get_irq_num_by_pin(_irq_pin_3);
    if (fubar_irq_number >= 0) {
    attachInterrupt(fubar_irq_number, gest_3, FALLING);
    }
  #else
    attachInterrupt(_irq_pin_3, gest_3, FALLING);
  #endif
  }

  //delay(400);   ManuvrOS does not use delay loops. Anywhere.
}


void MGC3130::enableApproachDetect(bool en) {
  write_buffer[0] = 0x10;
  write_buffer[1] = 0x00;
  write_buffer[2] = 0x00;
  write_buffer[3] = 0xA2;
  write_buffer[4] = 0x97;
  write_buffer[5] = 0x00;
  write_buffer[6] = 0x00;
  write_buffer[7] = 0x00;
  write_buffer[8] = (en ? 0x01 : 0x00);
  write_buffer[9] = 0x00;
  write_buffer[10] = 0x00;
  write_buffer[11] = 0x00;
  write_buffer[12] = 0x10;
  write_buffer[13] = 0x00;
  write_buffer[14] = 0x00;
  write_buffer[15] = 0x00;
  writeX(-1, 16, (uint8_t*) write_buffer);
}


void MGC3130::enableAirwheel(bool en) {
  write_buffer[0] = 0x10;
  write_buffer[1] = 0x00;
  write_buffer[2] = 0x00;
  write_buffer[3] = 0xA2;
  write_buffer[4] = 0x90;
  write_buffer[5] = 0x00;
  write_buffer[6] = 0x00;
  write_buffer[7] = 0x00;
  write_buffer[8] = (en ? 0x20 : 0x00);
  write_buffer[9] = 0x00;
  write_buffer[10] = 0x00;
  write_buffer[11] = 0x00;
  write_buffer[12] = 0x20;
  write_buffer[13] = 0x00;
  write_buffer[14] = 0x00;
  write_buffer[15] = 0x00;
  writeX(-1, 16, (uint8_t*) write_buffer);
}


// Bleh... needs rework.
int8_t MGC3130::setIRQPin(uint8_t _mask, int pin) {
  switch(_mask) {
  case MGC3130_ISR_MARKER_G0:
    _irq_pin_0 = pin;
    break;
    
  case MGC3130_ISR_MARKER_G1:
    _irq_pin_1 = pin;
    break;
    
  case MGC3130_ISR_MARKER_G2:
    _irq_pin_2 = pin;
    break;
    
  case MGC3130_ISR_MARKER_G3:
    _irq_pin_3 = pin;
    break;
    
  default:
    return -1;
  }

  return 0;
}


const char* MGC3130::getSwipeString(uint8_t eventByte) {
  switch (eventByte) {
    case B10000010:  return "Right Swipe";
    case B10000100:  return "Left Swipe";
    case B10001000:  return "Up Swipe";
    case B10010000:  return "Down Swipe";
    case B11000010:  return "Right Swipe (Edge)";
    case B11000100:  return "Left Swipe (Edge)";
    case B11001000:  return "Up Swipe (Edge)";
    case B11010000:  return "Down Swipe (Edge)";
    default:         return "<NONE>"; 
  }
}


const char* MGC3130::getTouchTapString(uint8_t eventByte) {
  switch (eventByte) {
    case B00100001:  return "Touch South";
    case B00100010:  return "Touch West";
    case B00100100:  return "Touch North";
    case B00101000:  return "Touch East";
    case B00110000:  return "Touch Center";
    case B01000001:  return "Tap South";
    case B01000010:  return "Tap West";
    case B01000100:  return "Tap North";
    case B01001000:  return "Tap East";
    case B01010000:  return "Tap Center";
    case B10000001:  return "DblTap South";
    case B10000010:  return "DblTap West";
    case B10000100:  return "DblTap North";
    case B10001000:  return "DblTap East";
    case B10010000:  return "DblTap Center";
    default:         return "<NONE>";
  } 
}


void MGC3130::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
  I2CDevice::printDebug(output);
  output->concatf("\t TS Pin:      %d \n\t MCLR Pin:   %d\n", _ts_pin, _reset_pin);
  output->concatf("\t Touch count  %d \n\t Flags:      0x%02x\n", touch_counter, flags);
  output->concatf("\t last_seq_num %d \n", last_seq_num);

  if (wheel_position)    output->concatf("\t Airwheel: 0x%08x\n", wheel_position);
  if (isPositionDirty()) output->concatf("\t Position: (0x%04x, 0x%04x, 0x%04x)\n", _pos_x, _pos_y, _pos_z);
  if (last_swipe)        output->concatf("\t Swipe     %s\n", getSwipeString(last_swipe));
  if (last_tap)          output->concatf("\t Tap       %s\n", getTouchTapString(last_tap));
  if (last_double_tap)   output->concatf("\t dbltap    %s\n", getTouchTapString(last_double_tap));
  if (isTouchDirty())    output->concatf("\t Touch     %s\n", getTouchTapString(last_touch));
  if (special)           output->concatf("\t Special condition %d\n", special);
}



/*
* This is the point at which our in-class data becomes abstract gesture codes.
* Ultimately, whatever strategy this evolves into will need to be extensible to
*   Digitabulum as well as many other kinds of gesture systems. I have no idea
*   if what I am doing is a good idea. Into the void....
*
* TODO: Some of this will have be fleshed out later once the strategy is more formallized.
*
* We are going to send two kinds of gesture events: One-shots, and assertions.
* A one-shot is a gesture that is recognized once, and that is all that matters.
*   examples of this in the MGC3130's case would be swipes and taps.
* An assertion is a gesture that locks in place and must then be disasserted later.
*   In the time intervening, aftertouch and ancillary data may be sent referencing
*   the locked gesture. In the MGC3130's case, this would be position and airwheel.
*
* In any case, the gestures are references by an integer position in a legend, which
*   we have hard-coded in this class, because there is no capability to define a gesture
*   independently of the MGC3130 firmware (which we wont touch). It may be the case that
*   later, we remove this restriction to implement one-shots such as glyphs by-way-of 
*   position. But this will have to come later.
*                              ---J. Ian Lindsay   Mon Jun 22 01:44:06 MST 2015
*/


/*
* For now, the hard-coded gesture map against which we will reference will be the mapping
*   of 32-bit pointer addresses for the strings that represent the gesture, for convenience 
*   sake. This mapping will therefore change from build-to-build.
* The exception is position (ID = 1) and AirWheel (ID = 2).
*/

void MGC3130::dispatchGestureEvents() {
  ManuvrEvent* event = NULL;
  
  if (isPositionDirty()) {
    // We don't want to spam the EventManager. We need to rate-limit.
    if ((millis() - MGC3130_MINIMUM_NUANCE_PERIOD) > last_nuance_sent) {
      if (!position_asserted()) {
        // If we haven't asserted the position gesture yet, do so now.
        event = EventManager::returnEvent(MANUVR_MSG_GESTURE_RECOGNIZED);
        event->addArg((uint32_t) 1);
        raiseEvent(event);
        position_asserted(true);
      }
      last_nuance_sent = millis();
      event = EventManager::returnEvent(MANUVR_MSG_GESTURE_NUANCE);
      event->addArg((uint32_t) 1);
      event->addArg((uint16_t) _pos_x);
      event->addArg((uint16_t) _pos_y);
      event->addArg((uint16_t) _pos_z);
      raiseEvent(event);
      _pos_x = -1;
      _pos_y = -1;
      _pos_z = -1;
    }
  }
  else if (position_asserted()) {
    // We need to disassert the position gesture.
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_DISASSERT);
    event->addArg((uint32_t) 1);
    raiseEvent(event);
    position_asserted(false);
    _pos_x = -1;
    _pos_y = -1;
    _pos_z = -1;
  }
  
  if (0 < wheel_position) {
    // We don't want to spam the EventManager. We need to rate-limit.
    if ((millis() - MGC3130_MINIMUM_NUANCE_PERIOD) > last_nuance_sent) {
      if (!airwheel_asserted()) {
        // If we haven't asserted the airwheel gesture yet, do so now.
        event = EventManager::returnEvent(MANUVR_MSG_GESTURE_RECOGNIZED);
        event->addArg((uint32_t) 2);
        raiseEvent(event);
        airwheel_asserted(true);
      }
      last_nuance_sent = millis();
      event = EventManager::returnEvent(MANUVR_MSG_GESTURE_NUANCE);
      event->addArg((uint32_t) 2);
      event->addArg((int32_t) wheel_position);
      raiseEvent(event);
      wheel_position = 0;
    }
  }
  else if (airwheel_asserted()) {
    // We need to disassert the airwheel gesture.
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_DISASSERT);
    event->addArg((uint32_t) 2);
    raiseEvent(event);
    airwheel_asserted(false);
    wheel_position = 0;
  }
  
  if (0 < last_tap) {
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_ONE_SHOT);
    event->addArg((uint32_t) getTouchTapString(last_tap));
    raiseEvent(event);
    last_tap = 0;
  }
  if (0 < last_double_tap) {
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_ONE_SHOT);
    event->addArg((uint32_t) getTouchTapString(last_double_tap));
    raiseEvent(event);
    last_double_tap = 0;
  }
  if (0 < last_swipe) {
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_ONE_SHOT);
    event->addArg((uint32_t) getTouchTapString(last_swipe));
    raiseEvent(event);
    last_swipe = 0;
  }
  if (isTouchDirty()) {
    event = EventManager::returnEvent(MANUVR_MSG_GESTURE_ONE_SHOT);
    event->addArg((uint32_t) getTouchTapString(last_touch));
    raiseEvent(event);
    last_touch_noted = last_touch;
  }
  if (special) {
    // TODO: Not sure how to deal with this yet...
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) {
      local_log.concatf("MGC3130 special code 0x08\n", special);
      StaticHub::log(&local_log);
    }
    #endif
    special = 0;
  }
  if (last_event) {
    // TODO: Not sure how to deal with this yet...
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) {
      local_log.concatf("MGC3130 last_event 0x08\n", last_event);
      StaticHub::log(&local_log);
    }
    #endif
    last_event = 0;
  }
}




/****************************************************************************************************
* These are overrides from I2CDevice.                                                               *
****************************************************************************************************/

void MGC3130::operationCompleteCallback(I2CQueuedOperation* completed) {
  pinMode(_ts_pin, INPUT_PULLUP);
  are_we_holding_ts(false);
  attachInterrupt(_ts_pin, mgc3130_isr_check, FALLING);

  if (completed->err_code == I2C_ERR_CODE_NO_ERROR) {
    if (completed->opcode == I2C_OPERATION_READ) {
      byte data;
      int c = 0;
      uint32_t temp_value = 0;   // Used to aggregate fields that span several bytes.
      uint16_t data_set = 0;
      uint8_t return_value = 0;
      
      bool pos_valid = false;
      bool wheel_valid = false;
      uint8_t byte_index = 0;
      
      int bytes_expected = completed->len;
      
      while(0 < bytes_expected) {
        data = *(read_buffer + byte_index++);
        bytes_expected--;
        switch (c++) {
          case 0:   // Length of the transfer by the sensor's reckoning.
            //if (bytes_expected < (data-1)) {
              bytes_expected = data - 1;  // Minus 1 because: we have already read one.
            //}
            break;
          case 1:   // Flags.
            last_event = (B00000001 << (data-1)) | B00100000;
            break;
          case 2:   // Sequence number
            if (data == last_seq_num) {
              return;
            }
            last_seq_num = data;
            break;
          case 3:   // Unique ID
            // data ought to always be 0x91 at this point.
            //if (0x91 != data) {
            //  local_log.concatf("UniqueID: 0x%02x\n", data);
            //  StaticHub::log(&local_log);
            //}
            break;
          case 4:   // Data output config mask is a 16-bit value.
            data_set = data;
            break;
          case 5:   // Data output config mask is a 16-bit value.
            data_set += data << 8;
            break;
          case 6:   // Timestamp (by the sensor's clock). Mostly useless unless we are paranoid about sample drop.
            break;
          case 7:   // System info. Tells us what data is valid.
            pos_valid   = (data & 0x01);  // Position
            wheel_valid = (data & 0x02);  // Air wheel
            break;
          
          /* Below this point, we enter the "variable-length" area. God speed.... */
          case 8:   //  DSP info
          case 9: 
            break;
    
          case 10:  // GestureInfo in the next 4 bytes.
            temp_value = data;
            break;
          case 11: 
            temp_value += data << 8;
            break;
          case 12:  break;   // These bits are reserved.
          case 13:
            temp_value += data << 24;
            if (0 == (temp_value & 0x80000000)) {   // Gesture recog completed?
              if (temp_value & 0x000060FC) {   // Swipe data
                last_swipe |= ((temp_value >> 2) & 0x000000FF) | 0b10000000;
                if (temp_value & 0x00010000) {
                  last_swipe |= 0b01000000;   // Classify as an edge-swipe.
                }
                return_value++;
              }
            }
            temp_value = 0;
            break;
    
          case 14:  // TouchInfo in the next 4 bytes.
            temp_value = data;
            break;
          case 15:
            temp_value += data << 8;
            if (temp_value & 0x0000001F) {
              last_touch = (temp_value & 0x0000001F) | 0x20;;
              return_value++;
            }
            else {
              last_touch = 0;
            }
              
            if (temp_value & 0x000003E0) {
              last_tap = ((temp_value & 0x000003E0) >> 5) | 0x40;
              return_value++;
            }
            if (temp_value & 0x00007C00) {
              last_double_tap = ((temp_value & 0x00007C00) >> 10) | 0x80;
              return_value++;
            }
            break;
    
          case 16:
            touch_counter = data;
            temp_value = 0;
            break;
      
          case 17:  break;   // These bits are reserved. 
    
          case 18:  // AirWheelInfo 
            if (wheel_valid) {
              wheel_position = (data%32)+1;
              return_value++;
            }
            break;
          case 19:  // AirWheelInfo, but the MSB is reserved.
            break;
    
          case 20:  // Position. This is a vector of 3 uint16's, but we store as 32-bit.
            if (pos_valid) _pos_x = data;
            break;
          case 21:
            if (pos_valid) _pos_x += data << 8;
            break;
          case 22: 
            if (pos_valid) _pos_y = data;
            break;
          case 23: 
            if (pos_valid) _pos_y += data << 8;
            break;
          case 24: 
            if (pos_valid) _pos_z = data;
            break;
          case 25:
            if (pos_valid) _pos_z += data << 8;
            break;
    
          case 26:   // NoisePower 
          case 27: 
          case 28: 
          case 29: 
            break;
            
          default:
            // There may be up to 40 more bytes after this, but we aren't dealing with them.
            break;
        }
      }
      
      dispatchGestureEvents();
    }
  }
  else{
    #ifdef __MANUVR_DEBUG
    if (verbosity > 3) {
      local_log.concat("An i2c operation requested by the MGC3130 came back failed.\n");
      completed->printDebug(&local_log);
      StaticHub::log(&local_log);
    }
    #endif
  }
}


/* If your device needs something to happen immediately prior to bus I/O... */
bool MGC3130::operationCallahead(I2CQueuedOperation* op) {
  if (!digitalRead(_ts_pin)) {   // Only initiate a read if there is something there.
    detachInterrupt(_ts_pin);
    pinMode(_ts_pin, OUTPUT);
    are_we_holding_ts(true);
    return true;
  }
  return false;
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* MGC3130::getReceiverName() {  return "MGC3130";  }



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
int8_t MGC3130::bootComplete() {
  EventReceiver::bootComplete();   // Call up to get scheduler ref and class init.
  init();
  ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_SENSOR_MGC3130_INIT);
  scheduler->createSchedule(1200, 0, true, this, event);
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
int8_t MGC3130::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SENSOR_MGC3130:
      break;
    default:
      break;
  }
  
  return return_value;
}



int8_t MGC3130::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_POWER_MODE:
      if (active_event->args.size() == 1) {
        uint8_t nu_power_mode;
        if (DIG_MSG_ERROR_NO_ERROR == active_event->getArgAs(&nu_power_mode)) {
          if (power_mode != nu_power_mode) {
            // TODO: We might use this later to put the MGC3130 into a low-power state.
            //set_power_mode(nu_power_mode);
            return_value++;
          }
        }
      }
      break;

    case MANUVR_MSG_SENSOR_MGC3130_INIT:
      is_class_ready(true);
      digitalWrite(_reset_pin, 1);
      enableAirwheel(false);
      attachInterrupt(_ts_pin, mgc3130_isr_check, FALLING);
      return_value++;
      break;

    case MANUVR_MSG_SENSOR_MGC3130:
      // Pick some safe number. We might limit this depending on what the sensor has to say.
      readX(-1, (uint8_t) 32, (uint8_t*)read_buffer);    // request bytes from slave device at 0x42
      return_value++;
      break;
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
      
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}



void MGC3130::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
#endif
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


