/*
File:   main.cpp
Author: J. Ian Lindsay
Date:   2016.03.11

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

Main demo application.

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

// Drivers particular to this Manuvrable...
#include <Drivers/i2c-adapter/i2c-adapter.h>

// This is ONLY used to expose the GPIO pins to the outside world.
// It is not required for GPIO usage internally.
#include <Drivers/ManuvrableGPIO/ManuvrableGPIO.h>

// Transports...
#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <Transports/ManuvrSocket/ManuvrUDP.h>
#include <Transports/ManuvrSocket/ManuvrTCP.h>
#include <Transports/StandardIO/StandardIO.h>
#include <Transports/BufferPipes/ManuvrTLS/ManuvrTLS.h>

// We will use MQTT as our concept of "session"...
#include <XenoSession/MQTT/MQTTSession.h>
#include <XenoSession/CoAP/CoAPSession.h>
#include <XenoSession/Console/ManuvrConsole.h>

#include <Platform/Cryptographic.h>


/*******************************************************************************
* Globals and defines that make our life easier.                               *
*******************************************************************************/
char *program_name  = NULL;
int __main_pid      = 0;
Kernel* kernel      = NULL;

void kernelDebugDump() {
  StringBuilder output;
  kernel->printDebug(&output);
  Kernel::log(&output);
}

BufferPipe* _pipe_factory_1(BufferPipe* _n, BufferPipe* _f) {
  CoAPSession* coap_srv = new CoAPSession(_n);
  kernel->subscribe(coap_srv);
  return (BufferPipe*) coap_srv;
}

BufferPipe* _pipe_factory_2(BufferPipe* _n, BufferPipe* _f) {
  ManuvrConsole* _console = new ManuvrConsole(_n);
  kernel->subscribe(_console);
  return (BufferPipe*) _console;
}

BufferPipe* _pipe_factory_3(BufferPipe* _n, BufferPipe* _f) {
  ManuvrTLSServer* _tls_server = new ManuvrTLSServer(_n);
  /*
  * Until parameters can be passed to pipe's via a stretegy, we configure new
  *   pipes this way...
  */
  return (BufferPipe*) _tls_server;
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


  if (0 != BufferPipe::registerPipe(1, _pipe_factory_1)) {
    printf("Failed to add CoAP to the pipe registry.\n");
    exit(1);
  }

  if (0 != BufferPipe::registerPipe(2, _pipe_factory_2)) {
    printf("Failed to add console to the pipe registry.\n");
    exit(1);
  }

  if (0 != BufferPipe::registerPipe(3, _pipe_factory_3)) {
    printf("Failed to add TLSServer to the pipe registry.\n");
    exit(1);
  }

  // Pipe strategy planning...
  const uint8_t pipe_plan_coap[]    = {1, 0};
  const uint8_t pipe_plan_coaps[]   = {1, 3, 0};
  const uint8_t pipe_plan_console[] = {2, 0};

  #if defined(__MANUVR_DEBUG)
    // spend time and memory measuring performance.
    kernel->profiler(true);

    // Creating a simple schedule to a void*(void) function...
    //kernel->createSchedule(1000, -1, false, kernelDebugDump);
  #endif

  /*
  * At this point, we should instantiate whatever specific functionality we
  *   want this Manuvrable to have.
  */
  // Parse through all the command line arguments and flags...
  // Please note that the order matters. Put all the most-general matches at the bottom of the loop.
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--version")) || ((argv[i][0] == '-') && (argv[i][1] == 'v'))) {
      // Print the version and quit.
      printf("%s v%s\n\n", argv[0], VERSION_STRING);
      exit(0);
    }
    if ((strcasestr(argv[i], "--info")) || ((argv[i][0] == '-') && (argv[i][1] == 'i'))) {
      // Cause the kernel to write a self-report to its own log.
      kernel->printDebug();
    }
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
    if ((strcasestr(argv[i], "--serial")) && (argc > (i-2))) {
      // The user wants us to listen to the given serial port.
      #if defined (MANUVR_SUPPORT_SERIAL)
        ManuvrSerial* ser = new ManuvrSerial((const char*) argv[++i], 9600);
        kernel->subscribe(ser);
      #else
        printf("%s was compiled without serial port support. Exiting...\n", argv[0]);
        exit(1);
      #endif
    }
    if ((strcasestr(argv[i], "--quit")) || ((argv[i][0] == '-') && (argv[i][1] == 'q'))) {
      // Execute up-to-and-including boot. Then immediately shutdown.
      // This is how you can stack post-boot-operations into the kernel.
      // They will execute following the BOOT_COMPLETE message.
      Kernel::raiseEvent(MANUVR_MSG_SYS_SHUTDOWN, NULL);
    }
  }

  #if defined(RASPI) || defined(RASPI2)
    // If we are running on a RasPi, let's try to fire up the i2c that is almost
    //   certainly present.
    I2CAdapter i2c(1);
    kernel->subscribe(&i2c);

    ManuvrableGPIO gpio;
    kernel->subscribe(&gpio);
  #endif

  // We need at least ONE transport to be useful...
  #if defined(MANUVR_SUPPORT_TCPSOCKET)
    #if defined(MANUVR_SUPPORT_MQTT)
      ManuvrTCP tcp_cli((const char*) "127.0.0.1", 1883);
      MQTTSession mqtt(&tcp_cli);
      kernel->subscribe(&mqtt);

      #if defined(RASPI) || defined(RASPI2)
        ManuvrRunnable gpio_write(MANUVR_MSG_DIGITAL_WRITE);
        gpio_write.isManaged(true);
        gpio_write.specific_target = &gpio;
        mqtt.subscribe("gw", &gpio_write);

        mqtt.tapMessageType(MANUVR_MSG_DIGITAL_WRITE);
        mqtt.tapMessageType(MANUVR_MSG_DIGITAL_READ);
      #endif

      ManuvrRunnable debug_msg(MANUVR_MSG_USER_DEBUG_INPUT);
      debug_msg.isManaged(true);
      debug_msg.specific_target = (EventReceiver*) kernel;

      mqtt.subscribe("d", &debug_msg);
      kernel->subscribe(&tcp_cli);

    #else
      ManuvrTCP tcp_srv((const char*) "0.0.0.0", 2319);
      tcp_srv.setPipeStrategy(pipe_plan_console);
      kernel->subscribe(&tcp_srv);
    #endif
  #endif

  #if defined(MANUVR_SUPPORT_UDP)
    #if defined(MANUVR_SUPPORT_COAP)
      ManuvrUDP udp_srv((const char*) "0.0.0.0", 6053);
      kernel->subscribe(&udp_srv);
      udp_srv.setPipeStrategy(pipe_plan_coap);
    #else
      ManuvrUDP udp_srv((const char*) "0.0.0.0", 6053);
      kernel->subscribe(&udp_srv);
    #endif
  #endif

  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr Kernel....\n", program_name);
  kernel->bootstrap();

  // TODO: Horrible hackishness to test TCP...
  #if defined(MANUVR_SUPPORT_TCPSOCKET)
    #if defined(MANUVR_SUPPORT_MQTT)
      // Attempt to connect to the broker.
      tcp_cli.connect();
    #else
      tcp_srv.listen();
    #endif
    //tcp_cli.autoConnect(true);
  #endif
  // TODO: End horrible hackishness.

  #if defined(RASPI) || defined(RASPI2)
    gpioDefine(14, OUTPUT);
    gpioDefine(15, OUTPUT);
    gpioDefine(18, OUTPUT);
    bool pin_14_state = false;
  #endif


  // The main loop. Run forever, as a microcontroller would.
  // Program exit is handled in Platform.
  int events_procd = 0;
  while (1) {
    events_procd = kernel->procIdleFlags();
    #if defined(RASPI) || defined(RASPI2)
      // Combined with the sleep below, this will give a
      // visual indication of kernel activity.
      setPin(14, pin_14_state);
      pin_14_state = !pin_14_state;
    #endif

    if (0 == events_procd) {
      // This is a resource-saver. How this is handled on a particular platform
      //   is still out-of-scope. Since we are in a threaded environment, we can
      //   sleep. Other systems might use ISR hooks or RTC notifications.
      sleep_millis(20);
    }
  }
}
