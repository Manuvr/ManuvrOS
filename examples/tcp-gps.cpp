/*
File:   tcp-gps.cpp
Author: J. Ian Lindsay
Date:   2016.08.02

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


tcp-gps demo application.
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <ctype.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/socket.h>
#include <fstream>
#include <iostream>

#include <netinet/in.h>
#include <fcntl.h>
#include <termios.h>

#include "FirmwareDefs.h"

#include <Kernel.h>

// Transports...
#include <Transports/ManuvrSocket/ManuvrTCP.h>
#include <Transports/ManuvrSocket/ManuvrUDP.h>
#include <Transports/StandardIO/StandardIO.h>
#include <Transports/BufferPipes/ManuvrGPS/ManuvrGPS.h>

#include <XenoSession/Console/ManuvrConsole.h>



/*******************************************************************************
* Globals and defines that make our life easier.                               *
*******************************************************************************/
char *program_name  = NULL;
int __main_pid      = 0;
Kernel* kernel      = NULL;

BufferPipe* _pipe_factory_1(BufferPipe* _n, BufferPipe* _f) {
  return (BufferPipe*) new ManuvrGPS(_n);
}



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
  program_name = argv[0];  // Name of running binary.
  __main_pid = getpid();

  // The first thing we should do: Instance a kernel.
  kernel = new Kernel();

  if (0 != BufferPipe::registerPipe("ManuvrGPS", 1, _pipe_factory_1)) {
    printf("Failed to add ManuvrGPS to the pipe registry.\n");
    exit(1);
  }

  // Pipe strategy planning...
  const uint8_t pipe_plan_gps[] = {1, 0};

  #if defined(__MANUVR_DEBUG)
    // spend time and memory measuring performance.
    kernel->profiler(true);
  #endif

  /*
  * At this point, we should instantiate whatever specific functionality we
  *   want this Manuvrable to have.
  */
  // Parse through all the command line arguments and flags...
  // Please note that the order matters. Put all the most-general matches at the bottom of the loop.
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--console")) || ((argv[i][0] == '-') && (argv[i][1] == 'c'))) {
      // The user wants a local stdio "Shell".
      #if defined(__MANUVR_CONSOLE_SUPPORT)
        // TODO: Until smarter idea is finished, manually patch the transport
        //         into a console session.
        StandardIO* _console_xport = new StandardIO();
        ManuvrConsole* _console = new ManuvrConsole((BufferPipe*) _console_xport);
        kernel->subscribe((EventReceiver*) _console);
        kernel->subscribe((EventReceiver*) _console_xport);
      #else
        printf("%s was compiled without any console support. Ignoring directive...\n", argv[0]);
      #endif
    }
  }

  // We need at least ONE transport to be useful...
  #if defined(MANUVR_SUPPORT_TCPSOCKET)
    ManuvrTCP tcp_srv((const char*) "0.0.0.0", 2319);
    tcp_srv.setPipeStrategy(pipe_plan_gps);
    kernel->subscribe(&tcp_srv);
  #endif

  #if defined(MANUVR_SUPPORT_UDP)
    ManuvrUDP udp_srv((const char*) "0.0.0.0", 6053);
    udp_srv.setPipeStrategy(pipe_plan_gps);
    kernel->subscribe(&udp_srv);
  #endif

  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr Kernel....\n", program_name);
  kernel->bootstrap();

  #if defined(MANUVR_SUPPORT_TCPSOCKET)
    tcp_srv.listen();
  #endif

  // The main loop. Run forever, as a microcontroller would.
  // Program exit is handled in Platform.
  int events_procd = 0;
  while (1) {
    events_procd = kernel->procIdleFlags();
    if (0 == events_procd) {
      // This is a resource-saver. How this is handled on a particular platform
      //   is still out-of-scope. Since we are in a threaded environment, we can
      //   sleep. Other systems might use ISR hooks or RTC notifications.
      sleep_millis(20);
    }
  }
}
