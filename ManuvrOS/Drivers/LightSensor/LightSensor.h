#ifndef ANALOG_LIGHT_SENSOR_H
  #define ANALOG_LIGHT_SENSOR_H
  
  #include "StaticHub/StaticHub.h"
  
  class LightSensor : public EventReceiver {
    public:
      LightSensor();
      ~LightSensor();
      
      /* Overrides from EventReceiver */
      void printDebug(StringBuilder*);
      const char* getReceiverName();
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);
      

    protected:
      int8_t bootComplete();

  
    private:
      uint32_t  pid_light_level_check;
  };
#endif
