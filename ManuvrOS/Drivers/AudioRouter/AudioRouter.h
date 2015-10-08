/*
File:   AudioRouter.h
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

#ifndef AUDIO_ROUTER_PCB_H
#define AUDIO_ROUTER_PCB_H

#include "ManuvrOS/Drivers/ISL23345/ISL23345.h"
#include "ManuvrOS/Drivers/ADG2128/ADG2128.h"



/*
* We must arbitrarilly assign inputs to rows or columns in the crosspoint switch. Since we don't want to allow
*   signal collision/mixing at the outputs, and since the crosspoint switch would allow us to do that on accident,
*   we must enforce those constraints in this class. To that end, we will consider the crosspoint's rows (12 pins)
*   to be the inputs.
*
* Given that, the class needs to ensure that two rows cannot both be attached to the same column. It should be
*   reiterated that this constraint is a software choice, and there is nothing preventing a user of this class
*   from re-tooling it to work in a different way entirely. If this is attempted, some other set of constraints
*   must be enforced to ensure that a program using this class cannot short-circuit the switch, otherwise magic smoke
*   might get released.
*
* Another consideration: There are 8 digital potentiometers on the PCB attached to the columns. The class is presently
*   organized to maintain the output attenuation regardless of input. It is, of course, possible to maintain the
*   attenuation settings such that they follow a given input, but this will require additions to this class to shift 
*   potentiometer values alongside crosspoint configurations.
* 
* Notes regarding hardware design....
* Normally, we would want to keep one input channel tied to ground so that otherwise unbound outputs can be tied to it
*   for improved noise suppression. But since we have the digital potentiometers on the side of the crosspoint that
*   we have designated as output, we can simply write a value to the channel's pot that achieves the same result.
* Some changes to the natural channel order were made to facilitate PCB layout. Which is the reason for the mapping
*   oddities between inputs on the switch and output channels (uint8_t pot_remap[8]).
*
* The digital potentiometers are linear across their range. Therefore, if you are going to use this class for audio,
*   you should adjust volume in a logrithmic manner. Additionally, the pots do not have zero-crossing detection. So
*   to avoid getting the "zipper" sound when changing volume, you should unroute() the channel prior to adjusting volume,
*   then route it again after the volume is set.
*/


// This struct defines an input pin on the PCB.
typedef struct cps_input_channel_t {
  uint8_t   cp_row;
  char*     name;                // A name for this input. Not required, but helpful for debug and output.
} CPInputChannel;


// This struct defines an output pin on the PCB.
typedef struct cps_output_channel_t {
  uint8_t         cp_column;
  char*           name;          // A name for this output. Not required, but helpful for debug and output.
  ISL23345*       dp_dev;        // Instance of the ISL23345 that is attached to this pin.
  uint8_t         dp_reg;        // The ISL23345 register that services this pin.
  uint8_t         dp_val;        // The value of the variable resistor for this pin (0-255 linear).
  CPInputChannel* cp_row;        // A pointer to the row that is presently bound to this column. If NULL, it is unbound.
} CPOutputChannel;



class AudioRouter : public EventReceiver {
  public:
    AudioRouter(I2CAdapter*, uint8_t, uint8_t, uint8_t); // Constructor needs the i2c-adapter, and addresses of the three chips on the PCB.
    ~AudioRouter(void);

    int8_t init(void);
    void preserveOnDestroy(bool);
    
    int8_t route(uint8_t col, uint8_t row);       // Establish a route to the given output from the given input.

    int8_t unroute(uint8_t col, uint8_t row);     // Disconnect the given output from the given input.
    int8_t unroute(uint8_t col);                  // Disconnect the given output from all inputs.

    int8_t nameInput(uint8_t row, char*);   // Name the input channel. 
    int8_t nameOutput(uint8_t col, char*);  // Name the output channel. 

    int8_t setVolume(uint8_t col, uint8_t vol);   // Set the volume coming out of a given output channel.

    int8_t enable(void);      // Turn on the chips responsible for routing signals.
    int8_t disable(void);     // Turn off the chips responsible for routing signals.

    int8_t status(StringBuilder*);     // Write some status about the routes to the provided char buffer.

    
    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    const char* getReceiverName();
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);
	void procDirectDebugInstruction(StringBuilder*);


    // TODO: These ought to be statics...
    void dumpInputChannel(CPInputChannel *chan, StringBuilder*);
    void dumpInputChannel(uint8_t chan, StringBuilder*);
    void dumpOutputChannel(uint8_t chan, StringBuilder*);

    
    static constexpr const int8_t AUDIO_ROUTER_ERROR_INPUT_DISPLACED = 1;    // There was no error, but a channel-routing operation has displaced a previously-routed input.
    static constexpr const int8_t AUDIO_ROUTER_ERROR_NO_ERROR        = 0;    // There was no error.
    static constexpr const int8_t AUDIO_ROUTER_ERROR_UNROUTE_FAILED  = -1;   // We tried to unroute a signal from an output and failed.
    static constexpr const int8_t AUDIO_ROUTER_ERROR_BUS             = -2;   // We tried to unroute a signal from an output and failed.
    static constexpr const int8_t AUDIO_ROUTER_ERROR_BAD_COLUMN      = -3;   // Column was out-of-bounds.
    static constexpr const int8_t AUDIO_ROUTER_ERROR_BAD_ROW         = -4;   // Row was out-of-bounds.

    
  protected:
    int8_t bootComplete();
    
    
  private:
    uint8_t i2c_addr_dp_lo;
    uint8_t i2c_addr_dp_hi;
    uint8_t i2c_addr_cp_switch;
    
    CPInputChannel*  inputs[12];
    CPOutputChannel* outputs[8];
    
    ADG2128 *cp_switch;
    ISL23345 *dp_lo;
    ISL23345 *dp_hi;
    
    CPOutputChannel* getOutputByCol(uint8_t);
    
    static const uint8_t col_remap[8];
};
#endif

