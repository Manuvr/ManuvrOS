/*
File:   Peripheral.cpp
Author: J. Ian Lindsay
Date:   2017.05.20

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


This is the basal interface for a peripheral driver. Instances of these drivers
  will be loaded into Platform to provide the abstract interface to the
  represented hardware.
*/

#include "Peripheral.h"

/**
* Constructor.
*/
PeriphDrvr::PeriphDrvr(const char* nom, const uint32_t flags) :
  _driver_name(nom), _driver_flags(flags) {}
