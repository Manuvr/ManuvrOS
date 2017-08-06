/*
File:   atecc508.cpp
Author: J. Ian Lindsay
Date:   2017.03.23

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

    __  ___                             ____  _____
   /  |/  /___ _____  __  ___   _______/ __ \/ ___/
  / /|_/ / __ `/ __ \/ / / / | / / ___/ / / /\__ \
 / /  / / /_/ / / / / /_/ /| |/ / /  / /_/ /___/ /
/_/  /_/\__,_/_/ /_/\__,_/ |___/_/   \____//____/

This is a demonstration program and utility for dealing with the ATECC508.

*/


/* Mandatory include for Manuvr Kernel.  */
#include <Kernel.h>
#include <Platform/Platform.h>

/*
* OPTIONAL:
* An application will likely want to talk to something in the outside world.
* For this, we need a transport, and typically a session of some sort.
*
* Note that each of these things is optional. Sessionless connections can be
*   handled with a BufferPipe (see ManuvrGPS) as well.
*
* StandardIO and the interactive Console are regarded as a Transport and
*   XenoSession, respectively.
*/
#include <Transports/StandardIO/StandardIO.h>
#include <XenoSession/Console/ManuvrConsole.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>
#include <Drivers/ATECC508/ATECC508.h>
#include <Drivers/ADP8866/ADP8866.h>


const I2CAdapterOptions i2c_opts(
  0,   // Device number
  255, // sda
  255  // scl
);

const ATECC508Opts atecc_opts(
  (uint8_t) 255
);

const ADP8866Pins adp_opts(
  255,  // (Reset)
  255   // (IRQ)
);


/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int main(int argc, const char* argv[]) {
  Argument* opts = parseFromArgCV(argc, argv);
  Argument* temp_arg = nullptr;
  StringBuilder local_log;

  /*
  * The platform object is created on the stack, but takes no action upon
  *   construction. The first thing that should be done is to call the preinit
  *   function to setup the defaults of the platform.
  */
  platform.platformPreInit(opts);

  // The platform comes with a messaging kernel. Get a ref to it.
  Kernel* kernel = platform.kernel();

  #if defined(MANUVR_DEBUG)
    // spend time and memory measuring performance.
    kernel->profiler(true);
  #endif

  /*
  * At this point, we should instantiate whatever specific functionality we
  *   want this Manuvrable to have.
  *
  * Parse through all the command line arguments and flags...
  * Please note that the order matters. Put all the most-general matches at the
  *   bottom of the loop.
  */
  if (opts->retrieveArgByKey("info")) {
    platform.printDebug(&local_log);
    printf("%s", local_log.string());
    local_log.clear();
  }

  I2CAdapter i2c(&i2c_opts);
  kernel->subscribe((EventReceiver*) &i2c);

  ATECC508 atec(&atecc_opts);
  i2c.addSlaveDevice((I2CDevice*) &atec);
  kernel->subscribe((EventReceiver*) &atec);

  ADP8866 leds(&adp_opts);
  i2c.addSlaveDevice((I2CDeviceWithRegisters*) &leds);
  kernel->subscribe((EventReceiver*) &leds);

  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  platform.bootstrap();

  /*
  * The main loop. Run forever, as a microcontroller would.
  * Program exit is handled by the Platform abstraction.
  */
  while (1) {
    kernel->procIdleFlags();
  }
}
