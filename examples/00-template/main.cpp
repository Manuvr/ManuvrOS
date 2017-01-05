/*
File:   main_template.cpp
Author: J. Ian Lindsay
Date:   2016.08.26

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

This is a template for a main.cpp file. This file was meant to be
  copy-pasta for beginners to create their own applications. It is
  also meant to serve as documentation for the boot process.

This is a demonstration program, and was meant to be compiled for a
  linux target.
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


/*******************************************************************************
* Functions that just print things.                                            *
*******************************************************************************/
void printHelp() {
  Kernel::log("Help would ordinarily be displayed here.\n");
}


/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int main(int argc, char *argv[]) {
  // The first thing we should do: Instance a kernel.
  Kernel* kernel = new Kernel();

  #if defined(__MANUVR_DEBUG)
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
      platform.printDebug();
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

  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr Kernel....\n", argv[0]);
  platform.bootstrap();

  /*
  * The main loop. Run forever, as a microcontroller would.
  * Program exit is handled by the Platform abstraction.
  */
  while (1) {
    kernel->procIdleFlags();
  }
}
