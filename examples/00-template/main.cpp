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
* The main function.                                                           *
*******************************************************************************/
int main(int argc, const char* argv[]) {
  Argument* opts = parseFromArgCV(argc, argv);
  Argument* temp_arg = nullptr;
  StringBuilder local_log;

  if (opts) {  // Print the launch arguments.
    opts->printDebug(&local_log);
    printf("%s\n\n\n", (char*) local_log.string());
    local_log.clear();
  }

  /*
  * The platform object is created on the stack, but takes no action upon
  *   construction. The first thing that should be done is to call the preinit
  *   function to setup the defaults of the platform.
  */
  platform.platformPreInit(opts);

  // The platform comes with a messaging kernel. Get a ref to it.
  Kernel* kernel = platform.kernel();


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
