/*
File:   LightSensor.h
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


This is a quick-and-dirty class to support reading a CdS cell from an analog
  pin, and then relaying the uncontrolled, unitless value out to the other
  modules in the system.
*/


#ifndef ANALOG_LIGHT_SENSOR_H
  #define ANALOG_LIGHT_SENSOR_H

  #include <EventReceiver.h>

  class LightSensor : public EventReceiver {
    public:
      LightSensor(int analog_pin);
      virtual ~LightSensor();

      /* Overrides from EventReceiver */
      void printDebug(StringBuilder*);
      int8_t notify(ManuvrRunnable*);
      int8_t callback_proc(ManuvrRunnable *);


    protected:
      int8_t bootComplete();


    private:
      int _analog_pin = -1;
      ManuvrRunnable _periodic_check;

      void light_check();
  };
#endif
