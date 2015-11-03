#ifndef __MANUVR_KERNEL_H__
  #define __MANUVR_KERNEL_H__
  
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  
  #include "ManuvrMsg/ManuvrMsg.h"
  #include "Scheduler.h"
  #include "EventManager.h"

  #ifdef __cplusplus
   extern "C" {
  #endif

  
  class ManuvrRunnable {
  };
  
  /*
  * This class is the machinery that handles Events. It should probably only be instantiated once.
  */
  class ManuvrKernel : public EventReceiver {
    public:
      ManuvrKernel(void);
      ~ManuvrKernel(void);
      
      int8_t run(void);   // Called once in the main loop to start system running. Returns exit code.
      int8_t step(void);  // Called to state-step the kernel.
      
      // Fetch functions for specific resources.
      EventManager* fetchEventManager(void);
      Scheduler* fetchScheduler(void);
      
      //uint32_t at(uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback)

      /* Overrides from EventReceiver */
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);
      void procDirectDebugInstruction(StringBuilder *);
      const char* getReceiverName();
      void printDebug(StringBuilder*);


    protected:
      int8_t bootComplete();

  
    private:
      // Global system resource handles...
      EventManager __e_man;      // This is our asynchronous message queue. 
      Scheduler    __sched;      // The scheduler.
  };
  
  
  #ifdef __cplusplus
  }
  #endif 
#endif  // __MANUVR_KERNEL_H__

