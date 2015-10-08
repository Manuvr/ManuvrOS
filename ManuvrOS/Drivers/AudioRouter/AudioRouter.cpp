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

#include "StaticHub/StaticHub.h"

#include <string.h>


/*
* To facilitate cheap (2-layer) PCB layouts, the pots are not mapped in an ordered manner to the
* switch outputs. This lookup table allows us to forget that fact.
*/
const uint8_t AudioRouter::col_remap[8] = {0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04};



/*
* Constructor. Here is all of the setup work. Takes the i2c addresses of the hardware as arguments.
*/
AudioRouter::AudioRouter(I2CAdapter* i2c, uint8_t cp_addr, uint8_t dp_lo_addr, uint8_t dp_hi_addr) {
  __class_initializer();
  i2c_addr_cp_switch = cp_addr;
  i2c_addr_dp_lo = dp_lo_addr;
  i2c_addr_dp_hi = dp_hi_addr;
  
    cp_switch = new ADG2128(cp_addr);
    dp_lo     = new ISL23345(dp_lo_addr);
    dp_hi     = new ISL23345(dp_hi_addr);
    
    i2c->addSlaveDevice(cp_switch);
    i2c->addSlaveDevice(dp_lo);
    i2c->addSlaveDevice(dp_hi);
    
    for (uint8_t i = 0; i < 12; i++) {   // Setup our input channels.
      inputs[i] = (CPInputChannel*) malloc(sizeof(CPInputChannel));
      inputs[i]->cp_row   = i;
      inputs[i]->name     = NULL;
    }

    for (uint8_t i = 0; i < 8; i++) {    // Setup our output channels.
      outputs[i] = (CPOutputChannel*) malloc(sizeof(CPOutputChannel));
      outputs[i]->cp_column = col_remap[i];
      outputs[i]->cp_row    = NULL;
      outputs[i]->name      = NULL;
      outputs[i]->dp_dev    = (i < 4) ? dp_lo : dp_hi;
      outputs[i]->dp_reg    = i % 4;
      outputs[i]->dp_val    = 128;
    }
}

AudioRouter::~AudioRouter() {
    for (uint8_t i = 0; i < 12; i++) {
      if (NULL != inputs[i]->name) free(inputs[i]->name);
      free(inputs[i]);
    }
    for (uint8_t i = 0; i < 8; i++) {
      if (NULL != outputs[i]->name) free(outputs[i]->name);
      free(outputs[i]);
    }

    // Destroy the objects that represeent our hardware. Downstream operations will
    //   put the hardware into an inert state, so that needn't be done here.
  if (dp_lo != NULL)     delete dp_lo;
  if (dp_hi != NULL)     delete dp_hi;
  if (cp_switch != NULL) delete cp_switch;
}


/*
* Do all the bus-related init.
*/
int8_t AudioRouter::init(void) {
  int8_t result = dp_lo->init();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to init() dp_lo (0x%02x) with cause (%d).", i2c_addr_dp_lo, result);
    return AUDIO_ROUTER_ERROR_BUS;
  }
  result = dp_hi->init();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to init() dp_hi (0x%02x) with cause (%d).", i2c_addr_dp_hi, result);
    return AUDIO_ROUTER_ERROR_BUS;
  }
  result = cp_switch->init();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to init() cp_switch (0x%02x) with cause (%d).", i2c_addr_cp_switch, result);
    return AUDIO_ROUTER_ERROR_BUS;
  }
  
  // If we are this far, it means we've successfully refreshed all the device classes
  //   to reflect the state of the hardware. Now to parse that data into structs that
  //   mean something to us at this level...
  for (int i = 0; i < 8; i++) {  // Volumes...
    outputs[i]->dp_val = outputs[i]->dp_dev->getValue(outputs[i]->dp_reg);
  }
  
  for (int i = 0; i < 12; i++) {  // Routes...
    uint8_t temp_byte = cp_switch->getValue(inputs[i]->cp_row);
    for (int j = 0; j < 8; j++) {
      if (0x01 & temp_byte) {
        CPOutputChannel* temp_output = getOutputByCol(j);
        if (temp_output != NULL) {
          temp_output->cp_row = inputs[i];
        }
      }
      temp_byte = temp_byte >> 1;
    }
  }
  
  return AUDIO_ROUTER_ERROR_NO_ERROR;
}


CPOutputChannel* AudioRouter::getOutputByCol(uint8_t col) {
  if (col > 7)  return NULL;
  for (int j = 0; j < 8; j++) {
    if (outputs[j]->cp_column == col) {
      return outputs[j];
    }
  }
  return NULL;
}


void AudioRouter::preserveOnDestroy(bool x) {
  if (dp_lo != NULL) dp_lo->preserveOnDestroy(x);
  if (dp_hi != NULL) dp_hi->preserveOnDestroy(x);
  if (cp_switch != NULL) cp_switch->preserveOnDestroy(x);
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
  if (NULL != inputs[row]->name) free(inputs[row]->name);
  inputs[row]->name = (char *) malloc(len + 1);
  for (int i = 0; i < len; i++) *(inputs[row]->name + i) = *(name + i);
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
  if (NULL != outputs[col]->name) free(outputs[col]->name);
  outputs[col]->name = (char *) malloc(len + 1);
  for (int i = 0; i < len; i++) *(outputs[col]->name + i) = *(name + i);
  return AUDIO_ROUTER_ERROR_NO_ERROR;
}



int8_t AudioRouter::unroute(uint8_t col, uint8_t row) {
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  if (row > 11) return AUDIO_ROUTER_ERROR_BAD_ROW;
  bool remove_link = (outputs[col]->cp_row == inputs[row]) ? true : false;
  uint8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (cp_switch->unsetRoute(outputs[col]->cp_column, row) < 0) {
    return AUDIO_ROUTER_ERROR_UNROUTE_FAILED;
  }
  if (remove_link) outputs[col]->cp_row = NULL;
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
  uint8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  if (row > 11) return AUDIO_ROUTER_ERROR_BAD_ROW;
  
  if (outputs[col]->cp_row != NULL) {
    int8_t result = unroute(col);
    if (result == AUDIO_ROUTER_ERROR_NO_ERROR) {
      return_value = AUDIO_ROUTER_ERROR_INPUT_DISPLACED;
      outputs[col]->cp_row = NULL;
    }
    else {
      return_value = AUDIO_ROUTER_ERROR_UNROUTE_FAILED;
    }
  }
  
  if (return_value >= 0) {
    int8_t result = cp_switch->setRoute(outputs[col]->cp_column, row);
    if (result != AUDIO_ROUTER_ERROR_NO_ERROR) {
      return_value = result;
    }
    else {
      outputs[col]->cp_row = inputs[row];
    }
  }
  
  return return_value;
}


int8_t AudioRouter::setVolume(uint8_t col, uint8_t vol) {
  int8_t return_value = AUDIO_ROUTER_ERROR_NO_ERROR;
  if (col > 7)  return AUDIO_ROUTER_ERROR_BAD_COLUMN;
  return_value = outputs[col]->dp_dev->setValue(outputs[col]->dp_reg, vol);
  return return_value;
}



// Turn on the chips responsible for routing signals.
int8_t AudioRouter::enable(void) {
  int8_t result = dp_lo->enable();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "enable() failed to enable dp_lo. Cause: (%d).\n", result);
    return result;
  }
  result = dp_hi->enable();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "enable() failed to enable dp_hi. Cause: (%d).\n", result);
    return result;
  }
  return AudioRouter::AUDIO_ROUTER_ERROR_NO_ERROR;
}

// Turn off the chips responsible for routing signals.
int8_t AudioRouter::disable(void) {
  int8_t result = dp_lo->disable();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "disable() failed to disable dp_lo. Cause: (%d).\n", result);
    return result;
  }
  result = dp_hi->disable();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "disable() failed to disable dp_hi. Cause: (%d).\n", result);
    return result;
  }
  result = cp_switch->reset();
  if (result != 0) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "disable() failed to reset cp_switch. Cause: (%d).\n", result);
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
  
  if (outputs[chan]->name != NULL) output->concatf("%s\n", outputs[chan]->name);
  output->concatf("Switch column %d\n", outputs[chan]->cp_column);
  if (outputs[chan]->dp_dev == NULL) {
    output->concatf("Potentiometer is NULL\n");
  }
  else {
    uint8_t temp_int = (outputs[chan]->dp_dev == dp_lo) ? 0 : 1;
    output->concatf("Potentiometer:            %d\n", temp_int);
    output->concatf("Potentiometer register:   %d\n", outputs[chan]->dp_reg);
    output->concatf("Potentiometer value:      %d\n", outputs[chan]->dp_val);
  }
  if (outputs[chan]->cp_row == NULL) {
    output->concatf("Output channel %d is presently unbound.\n", chan);
  }
  else {
    output->concatf("Output channel %d is presently bound to the following input...\n", chan);
    dumpInputChannel(outputs[chan]->cp_row, output);
  }
}


void AudioRouter::dumpInputChannel(CPInputChannel *chan, StringBuilder* output) {
  if (chan == NULL) {
    StaticHub::log("dumpInputChannel() was passed an out-of-bounds id.\n");
    return;
  }

  if (chan->name != NULL) output->concatf("%s\n", chan->name);
  output->concatf("Switch row: %d\n", chan->cp_row);
}


void AudioRouter::dumpInputChannel(uint8_t chan, StringBuilder* output) {
  if (chan > 11) {
    output->concatf("dumpInputChannel() was passed an out-of-bounds id.\n");
    return;
  }
  output->concatf("Input channel %d\n", chan);
  if (inputs[chan]->name != NULL) output->concatf("%s\n", inputs[chan]->name);
  output->concatf("Switch row: %d\n", inputs[chan]->cp_row);
}


// Write some status about the routes to the provided buffer.
int8_t AudioRouter::status(StringBuilder* output) {
  for (int i = 0; i < 8; i++) {
    dumpOutputChannel(i, output);
  }
  output->concat("\n");
  
  dp_lo->printDebug(output);
  dp_hi->printDebug(output);
  cp_switch->printDebug(output);
  
  StaticHub::log(output);
  
  return AudioRouter::AUDIO_ROUTER_ERROR_NO_ERROR;
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
int8_t AudioRouter::bootComplete() {
  EventReceiver::bootComplete();
  if (init() != AUDIO_ROUTER_ERROR_NO_ERROR) {
    StaticHub::log("Tried to init AudioRouter and failed.\n");
  }
  return 0;
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* AudioRouter::getReceiverName() {  return "AudioRouter";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void AudioRouter::printDebug(StringBuilder *output) {
  if (output == NULL) return;
  
  EventReceiver::printDebug(output);
  
  if (verbosity > 5) {
    status(output);
  }
  else {
    for (int i = 0; i < 8; i++) {
      dumpOutputChannel(i, output);
    }
  }
  output->concat("\n");
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
int8_t AudioRouter::callback_proc(ManuvrEvent *event) {
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


int8_t AudioRouter::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  uint8_t temp_uint8_0;
  uint8_t temp_uint8_1;
  StringBuilder *temp_sb;
  
  
  switch (active_event->event_code) {
    case VIAM_SONUS_MSG_DUMP_ROUTER:
      printDebug(&local_log);
      break;

    case VIAM_SONUS_MSG_NAME_INPUT_CHAN:
    case VIAM_SONUS_MSG_NAME_OUTPUT_CHAN:
      if (0 == active_event->consumeArgAs(&temp_uint8_0)) {
        if (0 == active_event->getArgAs(&temp_sb)) {
          int result = (VIAM_SONUS_MSG_NAME_OUTPUT_CHAN == active_event->event_code) ? nameOutput(temp_uint8_0, (char*) temp_sb->string()) : nameInput(temp_uint8_0, (char*) temp_sb->string());
          if (AUDIO_ROUTER_ERROR_NO_ERROR != result) {
            local_log.concat("Failed to set channel name.\n");
          }
        }
        else local_log.concat("Invalid parameter. Second arg needs to be a StringBuilder.\n");
      }
      else local_log.concat("Invalid parameter. First arg needs to be a uint8.\n");
      break;

    case VIAM_SONUS_MSG_OUTPUT_CHAN_VOL:
      if (0 == active_event->consumeArgAs(&temp_uint8_0)) {  // Volume param
        int result = 0;
        if (0 == active_event->consumeArgAs(&temp_uint8_1)) {  // Optional channel param
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
      if (0 == active_event->consumeArgAs(&temp_uint8_0)) {  // Output channel param
        int result = 0;
        if (0 == active_event->consumeArgAs(&temp_uint8_1)) {  // Optional input channel param
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
      if (0 == active_event->consumeArgAs(&temp_uint8_0)) {  // Output channel param
        if (0 == active_event->consumeArgAs(&temp_uint8_1)) {  // Input channel param
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
      local_log.concat("Was passed an event that we should be handling, but are not yet:\n");
      active_event->printDebug(&local_log);
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}




void AudioRouter::procDirectDebugInstruction(StringBuilder *input) {
  char* str = (char*) input->string();
  
  char c = *(str);
  uint8_t temp_byte = 0;        // Many commands here take a single integer argument.
  
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  StringBuilder parse_mule;
  ManuvrEvent* event;
  
  switch (c) {
    // AudioRouter debugging cases....
    case 'i':
      printDebug(&local_log);
      break;

	  
	case 'r':
	case 'u':
      parse_mule.concat(str);
      parse_mule.split(" ");
      parse_mule.drop_position(0);
      
      event = new ManuvrEvent((c == 'r') ? VIAM_SONUS_MSG_ROUTE : VIAM_SONUS_MSG_UNROUTE);
      switch (parse_mule.count()) {
        case 2:
          event->addArg((uint8_t) parse_mule.position_as_int(0));
          event->addArg((uint8_t) parse_mule.position_as_int(1));
          break;
        default:
		  local_log.concat("Not enough arguments to (un)route:\n");
          break;
      }
      raiseEvent(event);
	  break;
	  
	case 'V':
      parse_mule.concat(str);
      parse_mule.split(" ");
      parse_mule.drop_position(0);
      event = new ManuvrEvent(VIAM_SONUS_MSG_OUTPUT_CHAN_VOL);
      switch (parse_mule.count()) {
        case 2:
          event->addArg((uint8_t) parse_mule.position_as_int(0));
          event->addArg((uint8_t) parse_mule.position_as_int(1));
          break;
        case 1:
          event->addArg((uint8_t) parse_mule.position_as_int(0));
          break;
        default:
		  local_log.concat("Not enough arguments to set volume:\n");
          break;
      }
      raiseEvent(event);
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
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}
