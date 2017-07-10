/*
File:   SensorManager.h
Author: J. Ian Lindsay
Date:   2016.08.13

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


This is an EventReceiver that is meant to ease the implementation burden of
  sensor I/O and management. There should only ever be one of these instanced,
  and sensors ought to be added to it.

Design goals:
  1) Fully async
*/
#ifndef __SENSOR_AGGREGATION_DRIVER_H__
#define __SENSOR_AGGREGATION_DRIVER_H__

#include <Kernel.h>
#include "Sensors.h"


class SensorManagerOpts {
};


class SensorManager : public EventReceiver {
  public:
    SensorManager(const SensorManagerOpts*);
    virtual ~SensorManager();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT
    void printDebug(StringBuilder*);

    /* Means of accepting data from sensor classes. */
    int registerSensor(SensorDriver*);
    int unregisterSensor(SensorDriver*);

    /* Means of accepting data from non-sensor classes. */
    int registerDatum(SenseDatum*);
    int unregisterDatum(SenseDatum*);


  protected:
    int8_t attached();


  private:
    const SensorManagerOpts _opts;
    LinkedList<SensorDriver*> _s_list;    // A list of sensors we manage.
    LinkedList<SensorDatum*>  _d_list;    // A list of data we aggregate.

};
#endif   // __SENSOR_AGGREGATION_DRIVER_H__
