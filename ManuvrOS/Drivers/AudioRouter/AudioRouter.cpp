/*
File:   AudioRouter.cpp
Author: J. Ian Lindsay
Date:   2014.03.10


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

#include "AudioRouter.h"
#include <Kernel.h>
#include <string.h>


/*
* To facilitate cheap (2-layer) PCB layouts, the pots are not mapped in an ordered manner to the
* switch outputs. This lookup table allows us to forget that fact.
*/
static const uint8_t col_remap[8]  = {0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04};
static const uint8_t row_remap[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};


const MessageTypeDef message_defs_viam_sonus[] = {
  {  VIAM_SONUS_MSG_ENABLE_ROUTING,    MSG_FLAG_EXPORTABLE,     "ENABLE_ROUTING",    ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_DISABLE_ROUTING,   MSG_FLAG_EXPORTABLE,     "DISABLE_ROUTING",   ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_NAME_INPUT_CHAN,   MSG_FLAG_EXPORTABLE,     "NAME_INPUT_CHAN",   ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_NAME_OUTPUT_CHAN,  MSG_FLAG_EXPORTABLE,     "NAME_OUTPUT_CHAN",  ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_DUMP_ROUTER,       MSG_FLAG_EXPORTABLE,     "DUMP_ROUTER",       ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_OUTPUT_CHAN_VOL,   MSG_FLAG_EXPORTABLE,     "OUTPUT_CHAN_VOL",   ManuvrMsg::MSG_ARGS_NONE }, // Either takes a global volume, or a volume and a specific channel.
  {  VIAM_SONUS_MSG_UNROUTE,           MSG_FLAG_EXPORTABLE,     "UNROUTE",           ManuvrMsg::MSG_ARGS_NONE }, // Unroutes the given channel, or all channels.
  {  VIAM_SONUS_MSG_ROUTE,             MSG_FLAG_EXPORTABLE,     "ROUTE",             ManuvrMsg::MSG_ARGS_NONE }, // Routes the input to the output.
  {  VIAM_SONUS_MSG_PRESERVE_ROUTES,   MSG_FLAG_EXPORTABLE,     "PRESERVE_ROUTES",   ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_GROUP_CHANNELS,    MSG_FLAG_EXPORTABLE,     "GROUP_CHANNELS",    ManuvrMsg::MSG_ARGS_NONE }, // Pass two output channels to group them (stereo).
  {  VIAM_SONUS_MSG_UNGROUP_CHANNELS,  MSG_FLAG_EXPORTABLE,     "UNGROUP_CHANNELS",  ManuvrMsg::MSG_ARGS_NONE }  // Pass a group ID to free the channels it contains, or no args to ungroup everything.
};


/*
* Constructor. Here is all of the setup work. Takes the i2c addresses of the hardware as arguments.
*/
AudioRouter::AudioRouter(I2CAdapter* i2c, uint8_t cp_addr, uint8_t dp_lo_addr, uint8_t dp_hi_addr) : EventReceiver("AudioRouter") {
  const ADG2128Opts cp_opts(
    cp_addr,  // Device address
    255,      // No hard reset pin.
    true,     // Allow two sinks for a given source.
    false     // Disallow two sources from sharing a sink.
  );

  int mes_count = sizeof(message_defs_viam_sonus) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_viam_sonus, mes_count);

  cp_switch = new ADG2128(&cp_opts);
  dp_lo     = new ISL23345(dp_lo_addr);
  dp_hi     = new ISL23345(dp_hi_addr);

  i2c->addSlaveDevice(cp_switch);
  i2c->addSlaveDevice(dp_lo);
  i2c->addSlaveDevice(dp_hi);

  for (uint8_t i = 0; i < 12; i++) {   // Setup our input channels.
    inputs[i].cp_row   = i;
    inputs[i].name     = nullptr;
  }

  for (uint8_t i = 0; i < 8; i++) {    // Setup our output channels.
    outputs[i].cp_column = col_remap[i];
    outputs[i].i_chan    = nullptr;
    outputs[i].name      = nullptr;
    outputs[i].high_pot  = i < 4;
    outputs[i].dp_reg    = i % 4;
  }
}

AudioRouter::~AudioRouter() {
  for (uint8_t i = 0; i < 12; i++) {
    if (inputs[i].name) {
      free(inputs[i].name);
    }
  }
  for (uint8_t i = 0; i < 8; i++) {
    if (outputs[i].name) {
      free(outputs[i].name);
    }
  }

  // Destroy the objects that represeent our hardware. Downstream operations will
  //   put the hardware into an inert state, so that needn't be done here.
  if (dp_lo) {      delete dp_lo;      }
  if (dp_hi) {      delete dp_hi;      }
  if (cp_switch) {  delete cp_switch;  }
}


/*
* Do all the bus-related init.
*/
int8_t AudioRouter::init() {
  int8_t result = (int8_t) dp_lo->init();
  if (result != 0) {
    local_log.concatf("Failed to init() dp_lo with cause (%d).\n", result);
    Kernel::log(&local_log);
    return AUDIO_ROUTER_ERROR_BUS;
  }
  result = (int8_t) dp_hi->init();
  if (result != 0) {
    local_log.concatf("Failed to init() dp_hi with cause (%d).\n", result);
    Kernel::log(&local_log);
    return AUDIO_ROUTER_ERROR_BUS;
  }
  result = (int8_t) cp_switch->init();
  if (result != 0) {
    local_log.concatf("Failed to init() cp_switch with cause (%d).\n", result);
    Kernel::log(&local_log);
    return AUDIO_ROUTER_ERROR_BUS;
  }

  // If we are this far, it means we've successfully refreshed all the device classes
  //   to reflect the state of the hardware. Now to parse that data into structs that
  //   mean something to us at this level...
  for (int i = 0; i < 12; i++) {  // Routes...
    uint8_t temp_byte = cp_switch->getValue(inputs[i].cp_row);
    for (int j = 0; j < 8; j++) {
      if (0x01 & temp_byte) {
        CPOutputChannel* temp_output = getOutputByCol(j);
        if (temp_output) {
          temp_output->i_chan = &inputs[i];
        }
      }
      temp_byte = temp_byte >> 1;
    }
  }

  return AUDIO_ROUTER_ERROR_NO_ERROR;
}


CPOutputChannel* AudioRouter::getOutputByCol(uint8_t col) {
  if (col > 7)  return nullptr;
  for (int j = 0; j < 8; j++) {
    if (outputs[j].cp_column == col) {
      return &outputs[j];
    }
  }
  return nullptr;
}


void AudioRouter::preserveOnDestroy(bool x) {
  if (dp_lo) {      dp_lo->preserveOnDestroy(x);       }
  if (dp_hi) {      dp_hi->preserveOnDestroy(x);       }
  if (cp_switch) {  cp_switch->preserveOnDestroy(x);   }
}


/*
* Leak warning: There is no call to free(). Nor should there be unless we can
*   be assured that a const will not be passed in. In which case, we are probably
*   wasting precious RAM.
Fixed.
---J. Ian Lindsay   Mon Mar 09 03:35:40 MST 2015
*/
int8_t AudioRouter::nameInput(uint8_t row, char* name) {
  if (row > 11) return AUDIO_ROUTER_ERROR_BAD_ROW;
  int len = strlen(name);
  if (inputs[row].name) {
    free(inputs[row].name);
  }
  inputs[row].name = (char *) malloc(len + 1);
  for (int i = 0; i < len; i++) *(inputs[row].name + i) = *(name + i);
  return AUDIO_ROUTER_ERROR_NO_ERROR;
}


/*
* Leak warning: There is no call to free(). Nor should there be unless we can
*   be assured that a const will not be passed in. In which case, we are probably
*   wasting precious RAM.
Fixed.
---J. Ian Lindsay   Mon Mar 09 03:35:40 MST 2015
*/
int8_t AudioRouter::nameOutput(uint8_t col, char* name) {
  if (col > 7) return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  int len = strlen(name);
  if (outputs[col].name) {
    free(outputs[col].name);
  }
  outputs[col].name = (char *) malloc(len + 1);
  for (int i = 0; i < len; i++) *(outputs[col].name + i) = *(name + i);
  return AUDIO_ROUTER_ERROR_NO_ERROR;
}



int8_t AudioRouter::unroute(uint8_t col, uint8_t row) {
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  if (row > 11) return AUDIO_ROUTER_ERROR_BAD_ROW;
  bool remove_link = (outputs[col].i_chan == &inputs[row]) ? true : false;
  uint8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (0 > (int8_t) cp_switch->unsetRoute(outputs[col].cp_column, row)) {
    return AUDIO_ROUTER_ERROR_UNROUTE_FAILED;
  }
  if (remove_link) outputs[col].i_chan = nullptr;
  return return_value;
}


int8_t AudioRouter::unroute(uint8_t col) {
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  uint8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  for (int i = 0; i < 12; i++) {
    if (unroute(col, i) != AUDIO_ROUTER_ERROR_NO_ERROR) {
      return AUDIO_ROUTER_ERROR_UNROUTE_FAILED;
    }
  }
  return return_value;
}


/*
* Remember: This is the class responsible for ensuring that we don't cross-wire a circuit.
*   The crosspoint switch class is generalized, and knows nothing about the circuit it is
*   embedded within. It will follow whatever instructions it is given. So if we tell it
*   to route two inputs to the same output, it will oblige and possibly fry hastily-built
*   hardware. So under that condition, we unroute prior to routing and return a code to
*   indicate that we've done so.
*/
int8_t AudioRouter::route(uint8_t col, uint8_t row) {
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  if (row > 11) return AUDIO_ROUTER_ERROR_BAD_ROW;

  int8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (outputs[col].i_chan) {
    int8_t result = unroute(col);
    if (result == AUDIO_ROUTER_ERROR_NO_ERROR) {
      return_value = AUDIO_ROUTER_ERROR_INPUT_DISPLACED;
      outputs[col].i_chan = nullptr;
    }
    else {
      return_value = AUDIO_ROUTER_ERROR_UNROUTE_FAILED;
    }
  }

  if (return_value >= 0) {
    int8_t result = (int8_t) cp_switch->setRoute(outputs[col].cp_column, row);
    if (result != AUDIO_ROUTER_ERROR_NO_ERROR) {
      return_value = result;
    }
    else {
      outputs[col].i_chan = &inputs[row];
    }
  }

  return return_value;
}


int8_t AudioRouter::setVolume(uint8_t col, uint8_t vol) {
  int8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  return_value = (int8_t) _getPotRef(outputs[col].high_pot)->setValue(outputs[col].dp_reg, vol);
  return return_value;
}


int16_t AudioRouter::getVolume(uint8_t col) {
  if (col > 7) return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  return (int16_t) _getPotRef(outputs[col].high_pot)->getValue(outputs[col].dp_reg);
}



// Turn on the chips responsible for routing signals.
int8_t AudioRouter::enable() {
  int8_t result = (int8_t) dp_lo->enable();
  if (result != 0) {
    local_log.concatf("enable() failed to enable dp_lo. Cause: (%d).\n", result);
    Kernel::log(&local_log);
    return result;
  }
  result = (int8_t) dp_hi->enable();
  if (result != 0) {
    local_log.concatf("enable() failed to enable dp_hi. Cause: (%d).\n", result);
    Kernel::log(&local_log);
    return result;
  }
  return AudioRouter::AUDIO_ROUTER_ERROR_NO_ERROR;
}

// Turn off the chips responsible for routing signals.
int8_t AudioRouter::disable() {
  int8_t result = (int8_t) dp_lo->disable();
  if (result != 0) {
    local_log.concatf("disable() failed to disable dp_lo. Cause: (%d).\n", result);
    Kernel::log(&local_log);
    return result;
  }
  result = (int8_t) dp_hi->disable();
  if (result != 0) {
    local_log.concatf("disable() failed to disable dp_hi. Cause: (%d).\n", result);
    Kernel::log(&local_log);
    return result;
  }
  result = (int8_t) cp_switch->reset();
  if (result != 0) {
    local_log.concatf("disable() failed to reset cp_switch. Cause: (%d).\n", result);
    Kernel::log(&local_log);
    return result;
  }
  return AudioRouter::AUDIO_ROUTER_ERROR_NO_ERROR;
}


void AudioRouter::dumpOutputChannel(uint8_t chan, StringBuilder* output) {
  if (chan > 7)  {
    output->concat("dumpOutputChannel() was passed an out-of-bounds id.\n");
    return;
  }
  output->concatf("Output channel %d\n", chan);

  if (outputs[chan].name) {
    output->concatf("%s\n", outputs[chan].name);
  }
  output->concatf("Switch column %d\n", outputs[chan].cp_column);
  if (_getPotRef(outputs[chan].high_pot) == nullptr) {
    output->concatf("Potentiometer is NULL\n");
  }
  else {
    output->concatf("Potentiometer:            %u\n", (outputs[chan].high_pot) ? 1 : 0);
    output->concatf("Potentiometer register:   %u\n", outputs[chan].dp_reg);
    output->concatf("Potentiometer value:      %u\n", _getPotRef(outputs[chan].high_pot)->getValue(outputs[chan].dp_reg));
  }
  if (outputs[chan].i_chan == nullptr) {
    output->concatf("Output channel %d is presently unbound.\n", chan);
  }
  else {
    output->concatf("Output channel %d is presently bound to the following input...\n", chan);
    dumpInputChannel(outputs[chan].i_chan, output);
  }
}


void AudioRouter::dumpInputChannel(CPInputChannel *chan, StringBuilder* output) {
  if (chan == nullptr) {
    Kernel::log("dumpInputChannel() was passed an out-of-bounds id.\n");
    return;
  }

  if (chan->name) {
    output->concatf("%s\n", chan->name);
  }
  output->concatf("Switch row: %d\n", chan->cp_row);
}


void AudioRouter::dumpInputChannel(uint8_t chan, StringBuilder* output) {
  if (chan > 11) {
    output->concatf("dumpInputChannel() was passed an out-of-bounds id.\n");
    return;
  }
  output->concatf("Input channel %d\n", chan);
  if (inputs[chan].name) {
    output->concatf("%s\n", inputs[chan].name);
  }
  output->concatf("Switch row: %d\n", inputs[chan].cp_row);
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
int8_t AudioRouter::attached() {
  EventReceiver::attached();
  if (init() != AUDIO_ROUTER_ERROR_NO_ERROR) {
    Kernel::log("Tried to init AudioRouter and failed.\n");
  }
  return 1;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void AudioRouter::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  output->concat("\n");
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
int8_t AudioRouter::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


int8_t AudioRouter::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;
  uint8_t temp_uint8_0;
  uint8_t temp_uint8_1;
  StringBuilder *temp_sb;


  switch (active_event->eventCode()) {
    case VIAM_SONUS_MSG_DUMP_ROUTER:
      printDebug(&local_log);
      break;

    case VIAM_SONUS_MSG_NAME_INPUT_CHAN:
    case VIAM_SONUS_MSG_NAME_OUTPUT_CHAN:
      if (0 == active_event->getArgAs(&temp_uint8_0)) {
        if (0 == active_event->getArgAs(1, &temp_sb)) {
          int result = (VIAM_SONUS_MSG_NAME_OUTPUT_CHAN == active_event->eventCode()) ? nameOutput(temp_uint8_0, (char*) temp_sb->string()) : nameInput(temp_uint8_0, (char*) temp_sb->string());
          if (AUDIO_ROUTER_ERROR_NO_ERROR != result) {
            local_log.concat("Failed to set channel name.\n");
          }
        }
        else local_log.concat("Invalid parameter. Second arg needs to be a StringBuilder.\n");
      }
      else local_log.concat("Invalid parameter. First arg needs to be a uint8.\n");
      break;

    case VIAM_SONUS_MSG_OUTPUT_CHAN_VOL:
      if (0 == active_event->getArgAs(&temp_uint8_0)) {  // Volume param
        int result = 0;
        if (0 == active_event->getArgAs(1, &temp_uint8_1)) {  // Optional channel param
          // Setting the volume at a speciifc output channel.
          result = setVolume(temp_uint8_1, temp_uint8_0);
        }
        else {
          // Setting volume at all output channels.
          result = 0;
          for (int i = 0; i < 8; i++) result += setVolume(i, temp_uint8_0);
        }

        if (AUDIO_ROUTER_ERROR_NO_ERROR != result) {
          local_log.concat("Failed to set one or more channel volumes.\n");
        }
      }
      else local_log.concat("Invalid parameter. First arg needs to be a uint8.\n");
      break;

    case VIAM_SONUS_MSG_PRESERVE_ROUTES:
      preserveOnDestroy(true);
      break;
    case VIAM_SONUS_MSG_ENABLE_ROUTING:
      if (AUDIO_ROUTER_ERROR_NO_ERROR != enable()) local_log.concat("Failed to enable routing.\n");
      break;
    case VIAM_SONUS_MSG_DISABLE_ROUTING:
      if (AUDIO_ROUTER_ERROR_NO_ERROR != disable()) local_log.concat("Failed to disable routing.\n");
      break;

    case VIAM_SONUS_MSG_UNROUTE:
      if (0 == active_event->getArgAs(&temp_uint8_0)) {  // Output channel param
        int result = 0;
        if (0 == active_event->getArgAs(1, &temp_uint8_1)) {  // Optional input channel param
          // Unrouting a given output from a given input...
          result = unroute(temp_uint8_0, temp_uint8_1);
        }
        else {
          // Unrouting all channels...
          result = unroute(temp_uint8_0);
        }

        if (AUDIO_ROUTER_ERROR_NO_ERROR != result) {
          local_log.concat("Failed to unroute one or more channels.\n");
        }
      }
      else local_log.concat("Invalid parameter. First arg needs to be a uint8.\n");
      break;
    case VIAM_SONUS_MSG_ROUTE:
      if (0 == active_event->getArgAs(&temp_uint8_0)) {  // Output channel param
        if (0 == active_event->getArgAs(1, &temp_uint8_1)) {  // Input channel param
          // Routing a given output from a given input...
          if (AUDIO_ROUTER_ERROR_NO_ERROR != route(temp_uint8_0, temp_uint8_1)) {
            local_log.concat("Failed to route one or more channels.\n");
          }
        }
        else local_log.concat("Invalid parameter. Second arg needs to be a uint8.\n");
      }
      else local_log.concat("Invalid parameter. First arg needs to be a uint8.\n");
      break;

    case VIAM_SONUS_MSG_GROUP_CHANNELS:
    case VIAM_SONUS_MSG_UNGROUP_CHANNELS:
      #ifdef MANUVR_DEBUG
        local_log.concat("Was passed an event that we should be handling, but are not yet:\n");
        active_event->printDebug(&local_log);
      #endif
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
  { "r", "Route" },
  { "u", "Unroute" },
  { "V", "Volume" },
  { "I", "Dump input channel(s)" },
  { "O", "Dump output channel(s)" },
  { "e", "Enable routing" },
  { "d", "Disable routing" }
};


uint AudioRouter::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void AudioRouter::consoleCmdProc(StringBuilder *input) {
  char* str = input->position(0);
  char c = *(str);
  int temp_int = ((*(str) != 0) ? atoi((char*) str+1) : 0);
  StringBuilder parse_mule;
  ManuvrMsg* event;

  switch (c) {
    case 'i':
      switch (temp_int) {
        case 1:
          cp_switch->printDebug(&local_log);
          break;
        case 2:
          dp_lo->printDebug(&local_log);
          dp_hi->printDebug(&local_log);
          break;
        case 0:
        default:
          printDebug(&local_log);
          break;
      }
      break;

    case 'r':
    case 'u':
      input->drop_position(0);
      switch (input->count()) {
        case 2:
          event = new ManuvrMsg((c == 'r') ? VIAM_SONUS_MSG_ROUTE : VIAM_SONUS_MSG_UNROUTE);
          event->addArg((uint8_t) input->position_as_int(0));
          event->addArg((uint8_t) input->position_as_int(1));
          raiseEvent(event);
          break;
        default:
          local_log.concat("(Un)route <output> <input>\n");
          break;
      }
      break;

    case 'I':
    case 'O':
      input->drop_position(0);
      switch (input->count()) {
        case 0:
          if ('I' == c) {
            for (int i = 0; i < 12; i++) {
              dumpInputChannel(i, &local_log);
            }
          }
          else {
            for (int i = 0; i < 8; i++) {
              dumpOutputChannel(i, &local_log);
            }
          }
          break;
        case 1:
          if ('I' == c) {
            dumpInputChannel(input->position_as_int(0), &local_log);
          }
          else {
            dumpOutputChannel(input->position_as_int(0), &local_log);
          }
          break;
        default:
          local_log.concat("Need a channel parameter.\n");
          break;
      }
      break;

    case 'V':
      input->drop_position(0);
      event = new ManuvrMsg(VIAM_SONUS_MSG_OUTPUT_CHAN_VOL);
      switch (input->count()) {
        case 0:
          local_log.concat("CHAN | Volume\n-------------\n");
          for (int i = 0; i < 8; i++) {
            local_log.concatf("%d:  %d\n", i, getVolume(i));
          }
          break;
        case 2:
          event->addArg((uint8_t) input->position_as_int(0));
          input->drop_position(0);
          // NOTE: No break.
        case 1:
          event->addArg((uint8_t) input->position_as_int(0));
          raiseEvent(event);
          break;
        default:
          local_log.concat("v <output> <volume>\n");
          break;
      }
      break;

    case 'a':
      cp_switch->init();
      local_log.concat("v <output> <volume>\n");
      break;

    case 'e':
      if (AUDIO_ROUTER_ERROR_NO_ERROR != enable()) local_log.concat("Failed to enable routing.\n");
      break;
    case 'd':
      if (AUDIO_ROUTER_ERROR_NO_ERROR != disable()) local_log.concat("Failed to disable routing.\n");
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT
