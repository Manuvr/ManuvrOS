/*
File:   LDS8160.cpp
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

#include "LDS8160.h"

/****************************************************************************************************
* These are callbacks from the scheduler. They are specific to this class.                          *
****************************************************************************************************/
/* TODO: This class was never ported to ManuvrOS's October 2015 refactor.
void lds8160_intensity(void) {
  if (NULL != ld) {
    ld->toggle_brightness();
  }
}

void lds8160_chan_0(void) {
  if (NULL != ld) {
    ld->enable_channel(0, !ld->channel_enabled(0));
  }
}

void lds8160_chan_1(void) {
  if (NULL != ld) {
    ld->enable_channel(1, !ld->channel_enabled(1));
  }
}

void lds8160_chan_2(void) {
  if (NULL != ld) {
    ld->enable_channel(2, !ld->channel_enabled(2));
  }
}

void lds8160_chan_3(void) {
  if (NULL != ld) {
    ld->enable_channel(3, !ld->channel_enabled(3));
  }
}

void lds8160_chan_4(void) {
  if (NULL != ld) {
    ld->enable_channel(4, !ld->channel_enabled(4));
  }
}

void lds8160_chan_5(void) {
  if (NULL != ld) {
    ld->enable_channel(5, !ld->channel_enabled(5));
  }
}
*/


/*
* Constructor. Takes i2c address as argument.
*/
LDS8160::LDS8160(uint8_t addr) : EventReceiver(), I2CDeviceWithRegisters() {
  setReceiverName("LDS8160");
  _dev_addr = addr;

  init_complete = false;
  defineRegister(LDS8160_BANK_A_CURRENT,     (uint8_t) 0x90, true,  false, true);
  defineRegister(LDS8160_BANK_B_CURRENT,     (uint8_t) 0x90, true,  false, true);
  defineRegister(LDS8160_BANK_C_CURRENT,     (uint8_t) 0x90, true,  false, true);
  defineRegister(LDS8160_CHANNEL_ENABLE,     (uint8_t) 0x3F, true, false, true);
  defineRegister(LDS8160_GLOBAL_PWM_DIM,     (uint8_t) 0xA0, true, false, true);
  defineRegister(LDS8160_BANK_A_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
  defineRegister(LDS8160_BANK_B_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
  defineRegister(LDS8160_BANK_C_PWM_DUTY,    (uint8_t) 0xDC, true,  false, true);
  defineRegister(LDS8160_TEST_MODE,          (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_LED_SHORT_GND,      (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_LED_FAULT,          (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_CONFIG_REG,         (uint8_t) 0x00, false, false, true);
  defineRegister(LDS8160_SOFTWARE_RESET,     (uint8_t) 0x00, false, false, true);
  defineRegister(LDS8160_TEMPERATURE_OFFSET, (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_LED_SHUTDOWN_TEMP,  (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_TABLE_ENABLE_BP,    (uint8_t) 0x00, false, false, false);
  defineRegister(LDS8160_SI_DIODE_DV_DT,     (uint8_t) 0x00, false, false, false);
}

/*
* Destructor.
*/
LDS8160::~LDS8160(void) {
}


int8_t LDS8160::init() {
  if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
    writeIndirect(LDS8160_BANK_A_CURRENT,  0x90, true);
    writeIndirect(LDS8160_BANK_B_CURRENT,  0x90, true);
    writeIndirect(LDS8160_BANK_C_CURRENT,  0x90, true);
    writeIndirect(LDS8160_CHANNEL_ENABLE,  0x3F, true);
    writeIndirect(LDS8160_GLOBAL_PWM_DIM,  0xB4, true);
    writeIndirect(LDS8160_BANK_A_PWM_DUTY, 0xDC, true);
    writeIndirect(LDS8160_BANK_B_PWM_DUTY, 0xDC, true);
    writeIndirect(LDS8160_BANK_C_PWM_DUTY, 0xDC);
    //writeIndirect(LDS8160_CONFIG_REG, 0x04);
    init_complete = true;
  }
  else {
    Kernel::log("LDS8160::init():\tFailed to sync registers. Init fails.\n");
    init_complete = false;
    return -1;
  }
  return 0;
}


/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void LDS8160::operationCompleteCallback(I2CBusOp* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);

  int i = 0;
  DeviceRegister *temp_reg = reg_defs.get(i++);
  while (temp_reg != NULL) {
    switch (temp_reg->addr) {
      default:
        temp_reg->unread = false;
        break;
    }
    temp_reg = reg_defs.get(i++);
  }
}

/*
* Dump this item to the dev log.
*/
void LDS8160::printDebug(StringBuilder* temp) {
  if (NULL == temp) return;
  EventReceiver::printDebug(temp);
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\tinit_complete:      %s\n", init_complete ? "yes" :"no");

  temp->concatf("\n\tpid_intensity: \t0x%08x\n", pid_intensity);
  temp->concatf("\tpid_channel_0:   \t0x%08x\n", pid_channel_0);
  temp->concatf("\tpid_channel_1:   \t0x%08x\n", pid_channel_1);
  temp->concatf("\tpid_channel_2:   \t0x%08x\n", pid_channel_2);
  temp->concatf("\tpid_channel_3:   \t0x%08x\n", pid_channel_3);
  temp->concatf("\tpid_channel_4:   \t0x%08x\n", pid_channel_4);
  temp->concatf("\tpid_channel_5:   \t0x%08x\n\n", pid_channel_5);

  temp->concatf("\tpower_mode:        %d\n", power_mode);
  temp->concatf("\tGlobal dimming:    %f%\n", 100*(regValue(LDS8160_GLOBAL_PWM_DIM)/255));

  uint8_t enable_mask = regValue(LDS8160_CHANNEL_ENABLE);
  temp->concat("\n\t           mA    Duty  Enabled\n");
  temp->concatf("\tChan_0:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_A_CURRENT), regValue(LDS8160_BANK_A_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_0) ? "yes" : "no"));
  temp->concatf("\tChan_1:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_A_CURRENT), regValue(LDS8160_BANK_A_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_1) ? "yes" : "no"));
  temp->concatf("\tChan_2:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_B_CURRENT), regValue(LDS8160_BANK_B_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_2) ? "yes" : "no"));
  temp->concatf("\tChan_3:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_B_CURRENT), regValue(LDS8160_BANK_B_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_3) ? "yes" : "no"));
  temp->concatf("\tChan_4:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_C_CURRENT), regValue(LDS8160_BANK_C_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_4) ? "yes" : "no"));
  temp->concatf("\tChan_5:   %d \t 0x%02x \t %s\n", regValue(LDS8160_BANK_C_CURRENT), regValue(LDS8160_BANK_C_PWM_DUTY), ((enable_mask & LDS8160_CHAN_MASK_5) ? "yes" : "no"));
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
int8_t LDS8160::attached() {
  EventReceiver::attached();

/* TODO: This class was never ported to ManuvrOS's October 2015 refactor.
  pid_channel_0 = scheduler->createSchedule(100,  0, false, lds8160_chan_0);
  pid_channel_1 = scheduler->createSchedule(100,  0, false, lds8160_chan_1);
  pid_channel_2 = scheduler->createSchedule(100,  0, false, lds8160_chan_2);
  pid_channel_3 = scheduler->createSchedule(100,  0, false, lds8160_chan_3);
  pid_channel_4 = scheduler->createSchedule(100,  0, false, lds8160_chan_4);
  pid_channel_5 = scheduler->createSchedule(100,  0, false, lds8160_chan_5);
  pid_intensity = scheduler->createSchedule(1500, 0, false, lds8160_intensity);
  scheduler->disableSchedule(pid_channel_0);
  scheduler->disableSchedule(pid_channel_1);
  scheduler->disableSchedule(pid_channel_2);
  scheduler->disableSchedule(pid_channel_3);
  scheduler->disableSchedule(pid_channel_4);
  scheduler->disableSchedule(pid_channel_5);
  scheduler->disableSchedule(pid_intensity);
*/
  //enable_channel(0, false);
  //enable_channel(1, false);
  //enable_channel(2, false);
  //enable_channel(3, false);
  //enable_channel(4, false);
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
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t LDS8160::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->KernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}



int8_t LDS8160::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {

    case MANUVR_MSG_SYS_POWER_MODE:
      if (active_event->argCount() == 1) {
        uint8_t nu_power_mode;
        if (0 == active_event->getArgAs(&nu_power_mode)) {
          if (power_mode != nu_power_mode) {
            set_power_mode(nu_power_mode);
            return_value++;
          }
        }
      }
      break;

    case DIGITABULUM_MSG_LED_MODE:
      if (active_event->argCount() >= 1) {
        if ((EventReceiver*) this != active_event->callback) {   // Only do this if we are not the callback.
          uint8_t  nu_mode;
          if (0 == active_event->getArgAs(&nu_mode)) {
            set_led_mode(nu_mode);
            return_value++;
          }
        }
      }
      else {
        active_event->addArg((uint8_t) class_mode);
        /* If the callback is NULL, make us the callback, as our class is the responsible party for
           this kind of message.  Follow your shadow. */
        if (NULL == active_event->callback) active_event->callback = (EventReceiver*) this;
        return_value++;
      }
      break;

    case DIGITABULUM_MSG_LED_PULSE:
      if (active_event->argCount() >= 3) {
        if ((EventReceiver*) this != active_event->callback) {   // Only do this if we are not the callback.
          uint8_t old_val = regValue(LDS8160_CHANNEL_ENABLE);
          uint8_t new_val = old_val;
          uint8_t chan;
          int16_t  pulse_count;
          uint32_t pulse_period;
          active_event->getArgAs(0, &pulse_period);
          active_event->getArgAs(1, &pulse_count);
          local_log.concatf("LDS8160: pulse_period/count\t %d / %d.  LEDs:  ", pulse_period, pulse_count);

          for (uint8_t i = 2; i < active_event->argCount(); i++) {
            active_event->getArgAs(i, &chan);
            local_log.concatf("%d ", chan);
            switch (chan) {
              case 0:
                scheduler->alterScheduleRecurrence(pid_channel_0, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_0, pulse_period);
                scheduler->delaySchedule(pid_channel_0, pulse_period);
                new_val = (LDS8160_CHAN_MASK_0 | new_val);
                break;
              case 1:
                scheduler->alterScheduleRecurrence(pid_channel_1, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_1, pulse_period);
                scheduler->delaySchedule(pid_channel_1, pulse_period);
                new_val = (LDS8160_CHAN_MASK_1 | new_val);
                break;
              case 2:
                scheduler->alterScheduleRecurrence(pid_channel_2, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_2, pulse_period);
                scheduler->delaySchedule(pid_channel_2, pulse_period);
                new_val = (LDS8160_CHAN_MASK_2 | new_val);
                break;
              case 3:
                scheduler->alterScheduleRecurrence(pid_channel_3, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_3, pulse_period);
                scheduler->delaySchedule(pid_channel_3, pulse_period);
                new_val = (LDS8160_CHAN_MASK_3 | new_val);
                break;
              case 4:
                scheduler->alterScheduleRecurrence(pid_channel_4, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_4, pulse_period);
                scheduler->delaySchedule(pid_channel_4, pulse_period);
                new_val = (LDS8160_CHAN_MASK_4 | new_val);
                break;
              case 5:
                scheduler->alterScheduleRecurrence(pid_channel_5, pulse_count);
                scheduler->alterSchedulePeriod(pid_channel_5, pulse_period);
                scheduler->delaySchedule(pid_channel_5, pulse_period);
                new_val = (LDS8160_CHAN_MASK_5 | new_val);
                break;
            }
          }
          if (new_val != old_val) {
            writeIndirect(LDS8160_CHANNEL_ENABLE, new_val);
          }
          /* If the callback is NULL, make us the callback, as our class is the responsible party for
             this kind of message.  Follow your shadow. */
          if (NULL == active_event->callback) active_event->callback = (EventReceiver*) this;
          return_value++;
        }
      }
      break;
    case DIGITABULUM_MSG_LED_DIGIT_LEVEL:   // Pertains to the global dimmer.
      switch (active_event->argCount()) {
        case 0:        // No args? Asking for dimmer value.
          active_event->addArg((uint8_t) regValue(LDS8160_GLOBAL_PWM_DIM));
          /* If the callback is NULL, make us the callback, as our class is the responsible party for
             this kind of message.  Follow your shadow. */
          if (NULL == active_event->callback) active_event->callback = (EventReceiver*) this;
          return_value++;
          break;
        case 3:        // Three args? Setting the global dimmer value, oscillation period, and oscillation count.
          if (NULL != scheduler) {
            uint8_t  pulse_count;
            active_event->getArgAs(2, &pulse_count);
            scheduler->alterScheduleRecurrence(pid_intensity, pulse_count);
          }
          // NO BREAK
        case 2:        // Two args? Setting the global dimmer value and an oscillation period.
          if (NULL != scheduler) {
            uint32_t pulse_period;
            active_event->getArgAs(1, &pulse_period);
            scheduler->alterSchedulePeriod(pid_intensity, pulse_period);
            scheduler->enableSchedule(pid_intensity);
          }
          // NO BREAK
        case 1:        // One arg? Setting the global dimmer value.
          if ((EventReceiver*) this != active_event->callback) {   // Only do this if we are not the callback.
            uint8_t brightness;
            if (0 == active_event->getArgAs(&brightness)) {
              set_brightness(brightness);
              /* If the callback is NULL, make us the callback, as our class is the responsible party for
                 this kind of message.  Follow your shadow. */
              if (NULL == active_event->callback) active_event->callback = (EventReceiver*) this;
              return_value++;
            }
          }
          break;
        default:
          local_log.concatf("LDS8160: Too many arguments.\n");
          break;
      }
      break;
    case DIGITABULUM_MSG_LED_DIGIT_ON:
    case DIGITABULUM_MSG_LED_DIGIT_OFF:
      switch (active_event->argCount()) {
        case 0:        // No args? Return all values.
          // TODO: need to check for initialization first. Will be a concealed bug from the counterparty perspective.
          active_event->addArg((uint8_t) channel_enabled(0));
          active_event->addArg((uint8_t) channel_enabled(1));
          active_event->addArg((uint8_t) channel_enabled(2));
          active_event->addArg((uint8_t) channel_enabled(3));
          active_event->addArg((uint8_t) channel_enabled(4));
          active_event->addArg((uint8_t) channel_enabled(5));
          /* If the callback is NULL, make us the callback, as our class is the responsible party for
             this kind of message.  Follow your shadow. */
          if (NULL == active_event->callback) active_event->callback = (EventReceiver*) this;
          return_value++;
          break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
          if ((EventReceiver*) this != active_event->callback) {   // Only do this if we are not the callback.
            uint8_t old_val = regValue(LDS8160_CHANNEL_ENABLE);
            uint8_t new_val = old_val;
            uint8_t chan;
            bool en = (active_event->eventCode() == DIGITABULUM_MSG_LED_DIGIT_ON);
            for (uint8_t i = 0; i < active_event->argCount(); i++) {
              active_event->getArgAs(i, &chan);
              new_val = en ? ((1 << chan) | new_val) : (((uint8_t) ~(1 << chan)) & new_val);
            }
            if (new_val != old_val) {
              writeIndirect(LDS8160_CHANNEL_ENABLE, new_val);
            }
            return_value++;
          }
          break;
        default:
          local_log.concatf("LDS8160: Too many arguments.\n");
          break;
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void LDS8160::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  ManuvrRunnable *event = NULL;  // Pitching events is a common thing in this fxn...

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'k':    // LED Driver testing: Global dimmer
      local_log.concatf("parse_mule split (%s) into %d positions.\n", str, input->split(","));
      input->drop_position(0);

      event = Kernel::returnEvent(DIGITABULUM_MSG_LED_DIGIT_LEVEL);
      event->specific_target = this;

      switch (input->count()) {
        case 1:
          event->addArg((uint8_t)  input->position_as_int(0));
          break;
        case 2:
          event->addArg((uint8_t)  input->position_as_int(0));
          event->addArg((uint32_t) input->position_as_int(1));
          break;
        case 3:
          event->addArg((uint8_t)  input->position_as_int(0));
          event->addArg((uint32_t) input->position_as_int(1));
          event->addArg((int16_t)  input->position_as_int(2));
          break;
        default:
          break;
      }
      Kernel::staticRaiseEvent(event);
      break;

    case 'j':    // LED Driver testing: Direct LED manipulation.
      local_log.concatf("parse_mule split (%s) into %d positions.\n", str, input->split(","));
      input->drop_position(0);

      event = Kernel::returnEvent(temp_byte ? DIGITABULUM_MSG_LED_DIGIT_ON : DIGITABULUM_MSG_LED_DIGIT_OFF);
      event->specific_target = this;

      switch (input->count()) {
        case 6:
        case 5:
        case 4:
        case 3:
        case 2:
        case 1:
          for (uint8_t i = 0; i < input->count(); i++) {
            event->addArg((uint8_t) input->position_as_int(i));
          }
          break;
        default:
          break;
      }
      Kernel::staticRaiseEvent(event);
      break;

    case ';':    // LED Driver testing: Modal behavior
      event = Kernel::returnEvent(DIGITABULUM_MSG_LED_MODE);
      event->specific_target = this;
      event->addArg((uint8_t) temp_byte);
      Kernel::staticRaiseEvent(event);
      break;


    case 'h':    // LED Driver testing: LED Pulse
      local_log.concatf("parse_mule split (%s) into %d positions.\n", str, input->split(","));
      input->drop_position(0);

      switch (input->count()) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          event = Kernel::returnEvent(DIGITABULUM_MSG_LED_PULSE);
          event->addArg((uint32_t) input->position_as_int(0));   // Pulse period
          event->addArg((int16_t) input->position_as_int(1));    // Pulse count
          for (uint8_t i = 1; i < input->count(); i++) {
            event->addArg((uint8_t) input->position_as_int(i));
          }
          Kernel::staticRaiseEvent(event);
          break;
        default:
          break;
      }
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}

#endif  // MANUVR_CONSOLE_SUPPORT


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
void LDS8160::set_led_mode(uint8_t num) {
  quell_all_timers();
  switch (num) {
    case 1:
      scheduler->alterScheduleRecurrence(pid_channel_0, -1);
      scheduler->alterScheduleRecurrence(pid_channel_1, -1);
      scheduler->alterScheduleRecurrence(pid_channel_2, -1);
      scheduler->alterScheduleRecurrence(pid_channel_3, -1);
      scheduler->alterScheduleRecurrence(pid_channel_4, -1);

      scheduler->alterSchedulePeriod(pid_channel_0, 500);
      scheduler->alterSchedulePeriod(pid_channel_1, 500);
      scheduler->alterSchedulePeriod(pid_channel_2, 500);
      scheduler->alterSchedulePeriod(pid_channel_3, 500);
      scheduler->alterSchedulePeriod(pid_channel_4, 500);

      scheduler->delaySchedule(pid_channel_3, 500);
      scheduler->delaySchedule(pid_channel_1, 600);
      scheduler->delaySchedule(pid_channel_0, 700);
      scheduler->delaySchedule(pid_channel_2, 800);
      scheduler->delaySchedule(pid_channel_4, 900);
      scheduler->disableSchedule(pid_channel_5);

      writeIndirect(LDS8160_CHANNEL_ENABLE, 0b00111001, true);
    case 2:
      set_brightness(0x00);  // Might be fragile hack.
      stored_dimmer_val = 190;
      scheduler->alterScheduleRecurrence(pid_intensity, -1);
      scheduler->alterScheduleRecurrence(pid_intensity, 3500);
      scheduler->enableSchedule(pid_intensity);

    case 0:
      class_mode = num;
      break;
  }
}


void LDS8160::quell_all_timers() {
  scheduler->disableSchedule(pid_channel_0);
  scheduler->disableSchedule(pid_channel_1);
  scheduler->disableSchedule(pid_channel_2);
  scheduler->disableSchedule(pid_channel_3);
  scheduler->disableSchedule(pid_channel_4);
  scheduler->disableSchedule(pid_channel_5);
  scheduler->disableSchedule(pid_intensity);
}


/*
* Set the global brightness for LEDs managed by this chip.
* Stores the previous value.
*/
void LDS8160::set_brightness(uint8_t nu_brightness) {
  stored_dimmer_val = regValue(LDS8160_GLOBAL_PWM_DIM);
  writeIndirect(LDS8160_GLOBAL_PWM_DIM, nu_brightness);
}

/*
* Set the global brightness for LEDs managed by this chip.
* Exchanges the current value and the previously-stored value.
*/
void LDS8160::toggle_brightness(void) {
  uint8_t old_val = regValue(LDS8160_GLOBAL_PWM_DIM);
  writeIndirect(LDS8160_GLOBAL_PWM_DIM, stored_dimmer_val);
  stored_dimmer_val = old_val;
}

/*
* Enable or disable a specific LED. If something needs to be written to the
*   chip, will do that as well.
*/
void LDS8160::enable_channel(uint8_t chan, bool en) {
  uint8_t old_val = regValue(LDS8160_CHANNEL_ENABLE);
  uint8_t new_val = en ? ((1 << chan) | old_val) : (((uint8_t) ~(1 << chan)) & old_val);
  if (new_val != old_val) {
    writeIndirect(LDS8160_CHANNEL_ENABLE, new_val);
  }
}

/*
* Returns the boolean answer to the question: Is the given channel enabled?
*/
bool LDS8160::channel_enabled(uint8_t chan) {
  return (regValue(LDS8160_CHANNEL_ENABLE) & (1 << chan));
}

/*
* Perform a software reset.
*/
void LDS8160::reset() {
  writeIndirect(LDS8160_SOFTWARE_RESET, 0x80);
}

/*
* When a power mode broadcast is seen, this fxn will be called with the new
*   power profile identifier.
*/
void LDS8160::set_power_mode(uint8_t nu_power_mode) {
  power_mode = nu_power_mode;
  switch (power_mode) {
    case 0:
      writeIndirect(LDS8160_BANK_A_CURRENT, 0x8E, true);
      writeIndirect(LDS8160_BANK_B_CURRENT, 0x8E, true);
      writeIndirect(LDS8160_BANK_C_CURRENT, 0x8E, true);
      writeIndirect(LDS8160_SOFTWARE_RESET, 0x00);
      break;
    case 1:
      writeIndirect(LDS8160_BANK_A_CURRENT, 0x50, true);
      writeIndirect(LDS8160_BANK_B_CURRENT, 0x50, true);
      writeIndirect(LDS8160_BANK_C_CURRENT, 0x50, true);
      writeIndirect(LDS8160_SOFTWARE_RESET, 0x00);
      break;
    case 2:   // Enter standby.
      quell_all_timers();
      writeIndirect(LDS8160_SOFTWARE_RESET, 0x40);
      break;
    default:
      break;
  }
  Kernel::log("LDS8160 Power mode set. \n");
}
