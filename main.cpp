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

This is a demonstration program, and was meant to be compiled for a
  linux target.
*/


/* Mandatory include for Manuvr Kernel.  */
#include <Kernel.h>

/* Drivers particular to this Manuvrable... */
#include <Drivers/i2c-adapter/i2c-adapter.h>

/*
* This is ONLY used to expose the GPIO pins to the outside world.
* It is NOT required for GPIO usage internally.
*/
#include <Drivers/ManuvrableGPIO/ManuvrableGPIO.h>

/* Transports... */
#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <Transports/ManuvrSocket/ManuvrUDP.h>
#include <Transports/ManuvrSocket/ManuvrTCP.h>
#include <Transports/StandardIO/StandardIO.h>
#include <Transports/BufferPipes/ManuvrTLS/ManuvrTLS.h>

/* Concepts of "session"... */
#include <XenoSession/MQTT/MQTTSession.h>
#include <XenoSession/CoAP/CoAPSession.h>
#include <XenoSession/Console/ManuvrConsole.h>


/* This global makes this source file read better. */
Kernel* kernel = nullptr;

/*******************************************************************************
* BufferPipe strategies particular to this firmware.                           *
*******************************************************************************/
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
  StringBuilder out;
  _tls_server->printDebug(&out);
  Kernel::log(&out);
  return (BufferPipe*) _tls_server;
}


/*******************************************************************************
* Functions that just print things.                                            *
*******************************************************************************/
void kernelDebugDump() {
  StringBuilder output;
  kernel->printDebug(&output);
  Kernel::log(&output);
}

/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int main(int argc, char *argv[]) {
  char* program_name = argv[0];   // Name of running binary.
  int   main_pid     = getpid();  // Our PID.

  /*
  * The platform object is created on the stack, but takes no action upon
  *   construction. The first thing that should be done is to call the preinit
  *   function to setup the defaults of the platform.
  */
  platform.platformPreInit();

  /*
  * Because our persona isn't yet fully-derived from config (and may not ever
  *   be in some projects), we pull the freshly-instanced (but unbooted) kernel
  *   from the platform object so we can augment and configure it.
  */
  kernel = platform.getKernel();

  /*
  * Absent a strategy for dynamically-loading strategies...
  */
  if (0 != BufferPipe::registerPipe(1, _pipe_factory_1)) {
    printf("Failed to add CoAP to the pipe registry.\n");
    exit(1);
  }

  if (0 != BufferPipe::registerPipe(2, _pipe_factory_2)) {
    printf("Failed to add console to the pipe registry.\n");
    exit(1);
  }

  // Pipe strategy planning...
  const uint8_t pipe_plan_console[] = {2, 0};
  const uint8_t pipe_plan_coap[]    = {1, 0};
  #if defined(__MANUVR_MBEDTLS)
    const uint8_t pipe_plan_coaps[]   = {1, 3, 0};
    if (0 != BufferPipe::registerPipe(3, _pipe_factory_3)) {
      printf("Failed to add TLSServer to the pipe registry.\n");
      exit(1);
    }
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
      platform.printDebug();
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
      /*
      * If we built support for an MQTT client, we can demo setting up a socket
      *   as a client.
      */
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

    #endif

    /*
    * Transports that listen need to be given instructions for building software
    *   pipes up to the application layer.
    * This is how to use pipe-strategies to instance a console session when a
    *   TCP client connects. Test by running without "--console" and then...
    *       nc -t 127.0.0.1 2319nc -t 127.0.0.1 2319
    */
    ManuvrTCP tcp_srv((const char*) "0.0.0.0", 2319);
    tcp_srv.setPipeStrategy(pipe_plan_console);
    kernel->subscribe(&tcp_srv);
  #endif

  #if defined(MANUVR_SUPPORT_UDP)
    #if defined(MANUVR_SUPPORT_COAP)
      /*
      * If we support CoAP, we establish a UDP server with a pipe-strategy to
      *   instantiate a CoAP session.
      */
      ManuvrUDP udp_srv((const char*) "0.0.0.0", 5683);
      kernel->subscribe(&udp_srv);
      udp_srv.setPipeStrategy(pipe_plan_coap);
      #if defined(__MANUVR_MBEDTLS)
        /**
        * If we have TLS support, open up a separate pipe strategy for secured
        *   connections.
        */
        ManuvrUDP udp_srv_secure((const char*) "0.0.0.0", 5684);
        kernel->subscribe(&udp_srv_secure);
        udp_srv.setPipeStrategy(pipe_plan_coaps);
      #endif
    #endif
  #endif

  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr (PID %u)....\n", program_name, main_pid);
  platform.bootstrap();

  /*
  * If that function returned 'nominal', we call postInit() to finalize.
  */
  platform.platformPostInit();


  #if defined(MANUVR_SUPPORT_TCPSOCKET)
    #if defined(MANUVR_SUPPORT_MQTT)
      /* Attempt to connect to the MQTT broker. */
      tcp_cli.connect();
      //tcp_cli.autoConnect(true);
    #endif
    /* Fire-up the listener for the console pipe-demo. */
    tcp_srv.listen();
  #endif


  #if defined(RASPI) || defined(RASPI2)
    gpioDefine(14, OUTPUT);
    gpioDefine(15, OUTPUT);
    gpioDefine(18, OUTPUT);
    bool pin_14_state = false;
  #endif


  // The main loop. Run forever, as a microcontroller would.
  // Program exit is handled in Platform.
  while (1) {
    kernel->procIdleFlags();
    #if defined(RASPI) || defined(RASPI2)
      // Combined with the sleep below, this will give a
      // visual indication of kernel activity.
      setPin(14, pin_14_state);
      pin_14_state = !pin_14_state;
    #endif
  }
}
