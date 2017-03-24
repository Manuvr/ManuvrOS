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
  (uint8_t) 0
);

const ADP8866Pins adp_opts(
  255,  // (Reset)
  255   // (IRQ)
);


/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int main(int argc, char *argv[]) {
  StringBuilder local_log;

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
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--version")) || ((argv[i][0] == '-') && (argv[i][1] == 'v'))) {
      // Print the version and quit.
      printf("%s v%s\n\n", argv[0], VERSION_STRING);
      exit(0);
    }
    if ((strcasestr(argv[i], "--info")) || ((argv[i][0] == '-') && (argv[i][1] == 'i'))) {
      // Cause the kernel to write a self-report to its own log.
      platform.printDebug(&local_log);
      printf("%s", local_log.string());
      local_log.clear();
    }
  }

  #if defined(MANUVR_CONSOLE_SUPPORT)
    /*
    * The user wants a local stdio "Shell".
    * StandardIO is a linux-specific thing, but the Console can be patched
    * to a serial port or TCP/UDP listener in the same fashion.
    */
    StandardIO* _console_xport = new StandardIO();
    ManuvrConsole* _console = new ManuvrConsole((BufferPipe*) _console_xport);
    kernel->subscribe((EventReceiver*) _console);
    kernel->subscribe((EventReceiver*) _console_xport);
  #endif

  I2CAdapter i2c(&i2c_opts);
  kernel->subscribe((EventReceiver*) &i2c);

  // Pins 58 and 63 are the reset and IRQ pin, respectively.
  // This is translated to pins 10 and 13 on PortD.
  ATECC508 atec(&atecc_opts);
  i2c.addSlaveDevice((I2CDeviceWithRegisters*) &atec);
  kernel->subscribe((EventReceiver*) &atec);

  // Pins 58 and 63 are the reset and IRQ pin, respectively.
  // This is translated to pins 10 and 13 on PortD.
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
