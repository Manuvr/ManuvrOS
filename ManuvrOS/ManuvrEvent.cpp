#include "Kernel.h" 


/**
* Vanilla constructor.
*/
ManuvrEvent::ManuvrEvent() : ManuvrMsg(0) {
  __class_initializer();
}


/**
* Constructor that specifies the message code, and a callback.
*
* @param code  The message id code.
* @param cb    A pointer to the EventReceiver that should be notified about completion of this event.
*/
ManuvrEvent::ManuvrEvent(uint16_t code, EventReceiver* cb) : ManuvrMsg(code) {
  __class_initializer();
  callback   = cb;
}


/**
* Constructor that specifies the message code, but not a callback (which is set to NULL).
*
* @param code  The message id code.
*/
ManuvrEvent::ManuvrEvent(uint16_t code) : ManuvrMsg(code) {
  __class_initializer();
}


/**
* Destructor.
*/
ManuvrEvent::~ManuvrEvent(void) {
  clearArgs();
}


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*
* The nature of this class makes this a better design, anyhow. Since the same object is often
*   repurposed many times, doing any sort of init in the constructor should probably be avoided.
*/
void ManuvrEvent::__class_initializer() {
  flags           = 0x00;  // TODO: Optimistic about collapsing the bools into this. Or make gcc do it.
  callback        = NULL;
  specific_target = NULL;
  priority        = EVENT_PRIORITY_DEFAULT;
  
  // These things have implications for memory management, which is why repurpose() doesn't touch them.
  mem_managed  = false;
  scheduled    = false;
  preallocated = false;
}


/**
* This is called when the event is being repurposed for another run through the event queue.
*
* @param code The new message id code.
* @return 0 on success, or appropriate failure code.
*/
int8_t ManuvrEvent::repurpose(uint16_t code) {
  flags           = 0x00;
  callback        = NULL;
  specific_target = NULL;
  priority        = EVENT_PRIORITY_DEFAULT;
  return ManuvrMsg::repurpose(code);
}



/**
* Was this event preallocated?
* Preallocation implies no reap.
*
* @return true if the Kernel ought to return this event to its preallocation queue.
*/
bool ManuvrEvent::returnToPrealloc() {
  return preallocated;
}


/**
* Was this event preallocated?
* Preallocation implies no reap.
*
* @param  nu_val Pass true to cause this event to be marked as part of a preallocation pool.
* @return true if the Kernel ought to return this event to its preallocation queue.
*/
bool ManuvrEvent::returnToPrealloc(bool nu_val) {
  preallocated = nu_val;
  //if (preallocated) mem_managed = true;
  return preallocated;
}


/**
* If the memory isn't managed explicitly by some other class, this will tell the Kernel to delete
*   the completed event.
* Preallocation implies no reap.
*
* @return true if the Kernel ought to free() this Event. False otherwise.
*/
bool ManuvrEvent::eventManagerShouldReap() {
  if (mem_managed || preallocated || scheduled) {
    return false;
  }
  return true;
}


bool ManuvrEvent::isScheduled(bool nu) {
  scheduled = nu;
  return scheduled;   
}

bool ManuvrEvent::isManaged(bool nu) {
  mem_managed = nu;
  return mem_managed;   
}


bool ManuvrEvent::abort() {
  return Kernel::abortEvent(this);
}



/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrEvent::printDebug(StringBuilder *output) {
  if (output == NULL) return;
  StringBuilder msg_serial;
  ManuvrMsg::printDebug(output);
  int arg_count = serialize(&msg_serial);
  if (0 <= arg_count) {
  	  unsigned char* temp_buf = msg_serial.string();
  	  int temp_buf_len        = msg_serial.length();
  	  
  	  output->concatf("\t Preallocated          %s\n", (preallocated ? "yes" : "no"));
  	  output->concatf("\t Callback:             %s\n", (NULL == callback ? "NULL" : callback->getReceiverName()));
  	  output->concatf("\t specific_target:      %s\n", (NULL == specific_target ? "NULL" : specific_target->getReceiverName()));
  	  output->concatf("\t Argument count (ser): %d\n", arg_count);
  	  output->concatf("\t Bitstream length:     %d\n\t Buffer:  ", temp_buf_len);
  	  for (int i = 0; i < temp_buf_len; i++) {
  	    output->concatf("0x%02x ", *(temp_buf + i));
  	  }
  	  output->concat("\n\n");
  }
  //else {
  //  output->concatf("Failed to serialize message. Count was (%d).\n", arg_count);
  //}
}

