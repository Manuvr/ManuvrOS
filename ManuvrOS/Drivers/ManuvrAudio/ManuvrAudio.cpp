/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/*
* This is a class that encompasses the functions provided by various filters
*   in PJRC's Audio library. Original commentary is preserved.
*
*         ---J. Ian Lindsay   Sun Jul 05 00:26:13 MST 2015
*/

#include "StaticHub/StaticHub.h"
#include "ManuvrAudio.h"
#include <StringBuilder/StringBuilder.h>



volatile ManuvrAudio* ManuvrAudio::INSTANCE = NULL;


ManuvrAudio::ManuvrAudio() {
  INSTANCE = this;
}


ManuvrAudio::~ManuvrAudio() {
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
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrAudio::getReceiverName() {  return "ManuvrAudio";  }


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ManuvrAudio::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
}



/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrAudio::bootComplete() {
  EventReceiver::bootComplete();   // Call up to get scheduler ref and class init.
  //scheduler->createSchedule(1200, 0, true, this, event);
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
int8_t ManuvrAudio::callback_proc(ManuvrEvent *event) {
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



int8_t ManuvrAudio::notify(ManuvrEvent *active_event) {
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



void ManuvrAudio::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    case 'f':  // CPU benchmark
      {
        // This is a test of black magic inverse-square-root efficiency.
        int      r = 0;
        long time_var2 = micros();
        for (int32_t x = 0; x < 1000; x++) {
          // No idea how to properly use this fxn. Doesn't matter yet...
          r += signed_saturate_rshift(x, 20, 2);
        }
        local_log.concatf("signed_saturate_rshift()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;

        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r += signed_multiply_32x16b(x, 0x0005000a);
        }
        local_log.concatf("signed_multiply_32x16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;

        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = signed_multiply_32x16t(0x00a20003, 0x000c0004);
        }
        local_log.concatf("signed_multiply_32x16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;

        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r += multiply_32x32_rshift32(0x12345a5a, x);
        }
        local_log.concatf("multiply_32x32_rshift32()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r += multiply_32x32_rshift32_rounded(0x12345a5a, x);
        }
        local_log.concatf("multiply_32x32_rshift32_rounded()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_accumulate_32x32_rshift32_rounded(r, 0x12345a5a, x);
        }
        local_log.concatf("multiply_accumulate_32x32_rshift32_rounded()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_subtract_32x32_rshift32_rounded(r, 0x12345a5a, x);
        }
        local_log.concatf("multiply_subtract_32x32_rshift32_rounded()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = pack_16t_16t(0x46465757, 0x24243535);
        }
        local_log.concatf("pack_16t_16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = pack_16t_16b(0x46465757, 0x24243535);
        }
        local_log.concatf("pack_16t_16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = pack_16b_16b(0x46465757, 0x24243535);
        }
        local_log.concatf("pack_16b_16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;

        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = pack_16x16(0x46465757, 0x24243535);
        }
        local_log.concatf("pack_16x16()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r += signed_add_16_and_16(0x55555555, x);
        }
        local_log.concatf("signed_add_16_and_16()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = signed_multiply_accumulate_32x16b(r, 0x00011831, x);
        }
        local_log.concatf("signed_multiply_accumulate_32x16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = signed_multiply_accumulate_32x16t(r, 0x00011831, 0x00200300);
        }
        local_log.concatf("signed_multiply_accumulate_32x16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = logical_and(0x10101010, 0x10010011);
        }
        local_log.concatf("logical_and()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16tx16t_add_16bx16b(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16tx16t_add_16bx16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16tx16b_add_16bx16t(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16tx16b_add_16bx16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;

        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16bx16b(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16bx16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16bx16t(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16bx16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16tx16b(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16tx16b()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
        
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = multiply_16tx16t(0x14231010, 0x00100011);
        }
        local_log.concatf("multiply_16tx16t()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        r = 0;
          
        time_var2 = micros();
        for (int x = 0; x < 1000; x++) {
          r = substract_32_saturate(r, x);
        }
        local_log.concatf("substract_32_saturate()\t 0x%08x \t %u us\n", r, (unsigned long) (micros() - time_var2));
        local_log.concat("CPU test concluded.\n\n");
      }
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
#endif
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


