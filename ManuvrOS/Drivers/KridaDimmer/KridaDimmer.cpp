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


const MessageTypeDef message_defs_dimmer[] = {
  {  MANUVR_MSG_DIMMER_SET_LEVEL,  MSG_FLAG_EXPORTABLE,  "DIMMER_SET_LEVEL",  ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIMMER_TOGGLE,     MSG_FLAG_EXPORTABLE,  "DIMMER_TOGGLE",     ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIMMER_FADE_DOWN,  MSG_FLAG_EXPORTABLE,  "DIMMER_FADE_DOWN",  ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_DIMMER_FADE_UP,    MSG_FLAG_EXPORTABLE,  "DIMMER_FADE_UP",    ManuvrMsg::MSG_ARGS_NONE }  //
};



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
KridaDimmer::KridaDimmer(const KridaDimmerOpts* o) : EventReceiver("KridaDimmer"), I2CDevice(o->dimmer_addr), _opts(o) {
  int mes_count = sizeof(message_defs_dimmer) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_dimmer, mes_count);
  memset(_channel_values, 0, 4);
  memset(_channel_buffer, 0, 4);
  memset(_channel_hw, 0, 4);

  for (uint8_t i = 0; i < 4; i++) {
    _channel_timers[i] = nullptr;
    _channel_minima[i] = 33;
    _channel_maxima[i] = 100;
  }
}


/*
* Destructor.
*/
KridaDimmer::~KridaDimmer() {
  for (uint8_t i = 0; i < 4; i++) {
    if (nullptr != _channel_timers[i]) {
      _channel_timers[i]->enableSchedule(false);
      _channel_timers[i]->decRefs();
      platform.kernel()->removeSchedule(_channel_timers[i]);
      _channel_timers[i] = nullptr;
    }
  }
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

  flushLocalLog();
  return 0;
}


/*
* Dump this item to the dev log.
*/
void KridaDimmer::printDebug(StringBuilder* output) {
  output->concat("\n==< KridaDimmer >======================\n\tValue\tHW\tMin\tMax\tTimer\n");
  uint8_t hw  = 0;
  uint8_t val = 0;
  for (uint8_t i = 0; i < 4; i++) {
    hw  = _channel_hw[i];
    val = _channel_values[i];
    output->concatf("\t%u\t%u\t%u\t%u\t%c\n", val, hw, _channel_minima[i], _channel_maxima[i], ((nullptr != _channel_timers[i]) ? 'y' : 'n'));
  }
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
int8_t KridaDimmer::attached() {
  if (EventReceiver::attached()) {
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
int8_t KridaDimmer::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_DIMMER_TOGGLE:
    case MANUVR_MSG_DIMMER_FADE_UP:
    case MANUVR_MSG_DIMMER_FADE_DOWN:
      if (!event->willRunAgain()) {
        if (_channel_timers[0] == event) {  _channel_timers[0] = nullptr;   }
        if (_channel_timers[1] == event) {  _channel_timers[1] = nullptr;   }
        if (_channel_timers[2] == event) {  _channel_timers[2] = nullptr;   }
        if (_channel_timers[3] == event) {  _channel_timers[3] = nullptr;   }
      }
      break;
    default:
      break;
  }

  return return_value;
}



int8_t KridaDimmer::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_DIMMER_SET_LEVEL:
      if (active_event->argCount() == 2) {
        uint8_t chan;
        uint8_t val;
        if (0 == active_event->getArgAs(0, &chan)) {
          if (0 == active_event->getArgAs(1, &val)) {
            set_brightness(chan, val);
            return_value++;
          }
        }
      }
      break;

    case MANUVR_MSG_DIMMER_TOGGLE:
      if (active_event->argCount() == 2) {
        uint8_t chan;
        uint8_t val;
        if (0 == active_event->getArgAs(0, &chan)) {
          if (is_channel_valid(chan)) {
            if (0 == active_event->getArgAs(1, &val)) {
              if (channel_enabled(chan)) {
                enable_channel(chan, false);
              }
              else {
                set_brightness(chan, val);
              }
              return_value++;
            }
          }
        }
      }
      break;

    case MANUVR_MSG_DIMMER_FADE_DOWN:
      if (active_event->argCount() == 1) {
        uint8_t chan;
        if (0 == active_event->getArgAs(&chan)) {
          if (is_channel_valid(chan)) {
            if (!channel_has_outstanding_io(chan)) {
              if (_channel_minima[chan] < _channel_values[chan]) {
                set_brightness(chan, _channel_values[chan]-1);
              }
              else {
                // Fade completed. Clean up schedule.
                enable_channel(chan, false);
                if (_channel_timers[chan] == active_event) {
                  active_event->alterScheduleRecurrence(0);
                  active_event->decRefs();
                }
              }
            }
            return_value++;
          }
        }
      }
      break;

    case MANUVR_MSG_DIMMER_FADE_UP:
      if (active_event->argCount() == 1) {
        uint8_t chan;
        if (0 == active_event->getArgAs(&chan)) {
          if (is_channel_valid(chan)) {
            if (!channel_has_outstanding_io(chan)) {
              if (_channel_maxima[chan] > _channel_values[chan]) {
                set_brightness(chan, channel_enabled(chan) ? _channel_values[chan]+1 : _channel_minima[chan]);
              }
              else {
                // Fade completed. Clean up schedule.
                if (_channel_timers[chan] == active_event) {
                  active_event->alterScheduleRecurrence(0);
                  active_event->decRefs();
                }
              }
            }
            return_value++;
          }
        }
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}



#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i", "Info" },
  { "b", "Set channel brightness" },
  { "c", "Cycle color temperature" },
  { "L/l", "(En/Dis)able channel" },
  { "F/f", "Fade channel (up/down)" },
  { "i1", "I2CDevice debug" }
};


uint KridaDimmer::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void KridaDimmer::consoleCmdProc(StringBuilder* input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int channel  = 0;
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    channel = input->position_as_int(1);
  }
  if (input->count() > 2) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(2);
  }

  switch (c) {
    case 'i':   // Debug prints.
      switch (temp_int) {
        case 1:   // We want the slot stats.
          I2CDevice::printDebug(&local_log);
          break;
        default:
          printDebug(&local_log);
          break;
      }
      break;
    case 'p':
      pulse_channel(channel, 50, 100, 1);
      break;
    case 'l':
    case 'L':
      enable_channel(channel, (c == 'L'));
      break;
    case 'f':
    case 'F':
      fade_channel(channel, (c == 'F'));
      break;
    case 'b':
      set_brightness(channel, temp_int);
      if (getVerbosity() > 5) local_log.concatf("KridaDimmer: set_brightness(%u, %u)\n", channel, temp_int);
      break;

    default:
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT



/*******************************************************************************
* Dimmer and channel control functions
*******************************************************************************/

void KridaDimmer::pulse_channel(uint8_t chan, uint8_t nu_brightness, uint32_t period, int16_t recurrence) {
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      if (nullptr == _channel_timers[chan]) {
        _channel_timers[chan] = Kernel::returnEvent(MANUVR_MSG_DIMMER_TOGGLE, this);
        _channel_timers[chan]->incRefs();
        _channel_timers[chan]->alterSchedulePeriod(period);
        _channel_timers[chan]->alterScheduleRecurrence(recurrence);
        _channel_timers[chan]->autoClear(true);
        _channel_timers[chan]->enableSchedule(true);
        _channel_timers[chan]->addArg(chan);
        _channel_timers[chan]->addArg(nu_brightness);
        platform.kernel()->addSchedule(_channel_timers[chan]);
      }
      else {
        // There is already a timed operation on the channel.
        if (getVerbosity() > 4) local_log.concatf("KridaDimmer: Won't pulse channel %u because a timer is already operating on the channel.\n", chan);
      }
      break;

    default:
      if (getVerbosity() > 2) local_log.concatf("Channel %u is out of range.\n", chan);
      break;
  }
  flushLocalLog();
}


void KridaDimmer::fade_channel(uint8_t chan, bool up) {
  switch (chan) {
    case 0:
    case 1:
    case 2:
    case 3:
      if (nullptr == _channel_timers[chan]) {
        _channel_timers[chan] = Kernel::returnEvent((up ? MANUVR_MSG_DIMMER_FADE_UP : MANUVR_MSG_DIMMER_FADE_DOWN), this);
        _channel_timers[chan]->incRefs();
        _channel_timers[chan]->alterSchedulePeriod(50);
        _channel_timers[chan]->alterScheduleRecurrence(-1);  // Runs until dimmer boundary.
        _channel_timers[chan]->autoClear(true);
        _channel_timers[chan]->enableSchedule(true);
        _channel_timers[chan]->addArg(chan);
        platform.kernel()->addSchedule(_channel_timers[chan]);
      }
      else {
        // There is already a timed operation on the channel.
        if (getVerbosity() > 4) local_log.concatf("KridaDimmer: Won't pulse channel %u because a timer is already operating on the channel.\n", chan);
      }
      break;

    default:
      if (getVerbosity() > 2) local_log.concatf("Channel %u is out of range.\n", chan);
      break;
  }
  flushLocalLog();
}


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
