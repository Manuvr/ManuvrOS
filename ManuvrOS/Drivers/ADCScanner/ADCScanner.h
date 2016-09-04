/*
File:   ADCScanner.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


#ifndef ADC_SCANNER_H
#define ADC_SCANNER_H_H

  #define MANUVR_MSG_ADC_SCAN              0x9040

  #include <Kernel.h>

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
      int8_t notify(ManuvrRunnable*);
      int8_t callback_proc(ManuvrRunnable *);


    protected:
      int8_t bootComplete();


    private:
      int8_t   adc_list[16];
      uint16_t last_sample[16];
      uint16_t threshold[16];
      ManuvrRunnable _periodic_check;
  };
#endif
