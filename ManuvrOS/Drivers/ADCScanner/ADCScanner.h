#ifndef ADC_SCANNER_H
  #define ADC_SCANNER_H_H
  
  #include <ManuvrOS/Kernel.h>
  
  class ADCScanner : public EventReceiver {
    public:
      ADCScanner();
      ~ADCScanner();
      
      void addADCPin(int8_t);
      void delADCPin(int8_t);
      
      int8_t scan();
      
      uint16_t getSample(int8_t idx);
      
      /* Overrides from EventReceiver */
      void printDebug(StringBuilder*);
      const char* getReceiverName();
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);

      
    protected:
      int8_t bootComplete();
      
      
    private:
      uint32_t  pid_adc_check;
      
      int8_t   adc_list[16];
      uint16_t last_sample[16];
      uint16_t threshold[16];
  };
#endif
