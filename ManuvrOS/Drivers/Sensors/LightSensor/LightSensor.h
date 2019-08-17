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



#ifndef __ANALOG_LIGHT_SENSOR_H
#define __ANALOG_LIGHT_SENSOR_H

#include <Drivers/Sensors/SensorWrapper.h>
#include <EventReceiver.h>


#define ALS_FLAG_RANGE_INVERTED      1
#define ALS_FLAG_RANGE_LOG_RESPONSE  2


/*******************************************************************************
* Options object
*******************************************************************************/

/**
* Set pin def to 255 to mark it as unused.
*/
class LightSensorOpts {
  public:
    const uint8_t pin;    // Which pin for analog input?
    const int range_top;
    const int range_bottom;


    /** Copy constructor. */
    LightSensorOpts(const LightSensorOpts* o) :
      pin(o->pin),
      range_top(o->range_top),
      range_bottom(o->range_bottom),
      _flags(o->_flags) {};

    /**
    * Constructor.
    *
    * @param pin
    * @param Initial flags
    */
    LightSensorOpts(
      uint8_t _p,
      int _top,
      int _bottom,
      uint8_t _fi = 0
    ) :
      pin(_p),
      range_top(_top),
      range_bottom(_bottom),
      _flags(_fi) {};


  private:
    const uint8_t _flags;
};



class LightSensor : public SensorWrapper {
  public:
    LightSensor(const LightSensorOpts*);
    virtual ~LightSensor();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.
    void printDebug(StringBuilder*);


  protected:
    int8_t attached();


  private:
    const LightSensorOpts _opts;
    ManuvrMsg _periodic_check;
    uint16_t _current_value;

    void light_check();
};


#endif  // __ANALOG_LIGHT_SENSOR_H
