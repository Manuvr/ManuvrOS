#include "ADCScanner.h"

#if defined(ARDUiNO)
  #include "Arduino.h"
#endif

ADCScanner::ADCScanner() {
  __class_initializer();
  pid_adc_check = 0;
  for (int i = 0; i < 16; i++) {
    adc_list[i] = -1;
    last_sample[i] = 0;
    threshold[i] = 0;
  }
}


ADCScanner::~ADCScanner() {
}


uint16_t ADCScanner::getSample(int8_t idx) {
  if ((idx >= 0) && (idx < 16)) {
    return last_sample[idx];
  }
  return 0;
}


void ADCScanner::addADCPin(int8_t pin_no) {
  if (pin_no >= 0) {
    for (int i = 0; i < 16; i++) {
      if (-1 == adc_list[i]) {
        adc_list[i] = pin_no;
        return;
      }
    }
  }
}


void ADCScanner::delADCPin(int8_t pin_no) {
  if (pin_no >= 0) {
    for (int i = 0; i < 16; i++) {
      if (pin_no == adc_list[i]) {
        adc_list[i] = -1;
        return;
      }
    }
  }
}
      


int8_t ADCScanner::scan() {
  int8_t return_value = 0;
  uint16_t current_sample = 0;
  for (int i = 0; i < 16; i++) {
    if (-1 != adc_list[i]) {
      current_sample = analogRead(adc_list[i]);
      if (threshold[i] < current_sample) {
        return_value++;
      }
      last_sample[i] = current_sample;
    }
    else {
      last_sample[i] = 0;
    }
  }
  return return_value;
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
* Some peripherals and operations need a bit of time to complete. This function is called from a
*   one-shot schedule and performs all of the cleanup for latent consequences of bootstrap().
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ADCScanner::bootComplete() {
  EventReceiver::bootComplete();
  
  ManuvrEvent *event = new ManuvrEvent(VIAM_SONUS_MSG_ADC_SCAN);
  pid_adc_check = scheduler->createSchedule(50, -1, false, this, event);
  return 1;
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ADCScanner::getReceiverName() {  return "ADCScanner";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ADCScanner::printDebug(StringBuilder *output) {
  if (output == NULL) return;
  
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
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ADCScanner::callback_proc(ManuvrEvent *event) {
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


int8_t ADCScanner::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case VIAM_SONUS_MSG_ADC_SCAN:
      if (scan()) {
        
      }
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
  return return_value;
}

