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

#include <ManuvrOS/Kernel.h>

// Drivers particular to this Manuvrable...
#include <ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h>

// Transports...
#include <ManuvrOS/Transports/ManuvrSerial/ManuvrSerial.h>
#include <ManuvrOS/Transports/ManuvrSocket/ManuvrSocket.h>


/****************************************************************************************************
* Globals and defines that make our life easier.                                                    *
****************************************************************************************************/
char *program_name  = NULL;
int __main_pid      = 0;
int __shell_pid     = 0;
Kernel* kernel      = NULL;

void kernelDebugDump() {
  StringBuilder output;
  kernel->printDebug(&output);
  Kernel::log(&output);
}



/****************************************************************************************************
* Functions that just print things.                                                                 *
****************************************************************************************************/
void printHelp() {
  printf("Help would ordinarily be displayed here.\n");
}


/****************************************************************************************************
* Code related to running a local stdio shell...                                                    *
****************************************************************************************************/
#define U_INPUT_BUFF_SIZE   255    // How big a buffer for user-input?

long unsigned int _thread_id = 0;;
StringBuilder user_input;
bool running = true;


void* spawnUIThread(void*) {
  char *input_text	= (char*) alloca(U_INPUT_BUFF_SIZE);	// Buffer to hold user-input.
  StringBuilder user_input;
  
  while (running) {
    printf("%c[36m%s> %c[39m", 0x1B, program_name, 0x1B);
    bzero(input_text, U_INPUT_BUFF_SIZE);
    if (fgets(input_text, U_INPUT_BUFF_SIZE, stdin) != NULL) {
      user_input.concat(input_text);
      user_input.trim();
      
      //while(*t_iterator++ = toupper(*t_iterator));        // Convert to uniform case...
      int arg_count = user_input.split(" \n");
      if (arg_count > 0) {  // We should have at least ONE argument. Right???
        // Begin the cases...
        if      (strcasestr(user_input.position(0), "QUIT"))  running = false;  // Exit
        else if (strcasestr(user_input.position(0), "HELP"))  printHelp();      // Show help.
        else {
          // This test is detined for input into the running kernel-> Send it back
          //   up the pipe.
          bool terminal = (user_input.split("\n") > 0);
          if (terminal) {
            kernel->accumulateConsoleInput((uint8_t*) user_input.position(0), strlen(user_input.position(0)), true);
            user_input.drop_position(0);
          }
        }
      }
      else {
        // User entered nothing.
        printHelp();
      }
    }
    else {
      // User insulted fgets()...
      printHelp();
    }
  }
  
  return NULL;
}




/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/

int main(int argc, char *argv[]) {
  program_name = argv[0];  // Name of running binary.
  __main_pid = getpid();
  
  kernel = new Kernel();  // Instance a kernel.

  #if defined(__MANUVR_DEBUG)
    // We want to see this data if we are a debug build.
    kernel->print_type_sizes();
    kernel->profiler(true);
    //kernel->createSchedule(1000, -1, false, kernelDebugDump);
  #endif


  /* 
  * At this point, we should instantiate whatever specific functionality we
  *   want this Manuvrable to have.
  */
  
  
  // We need at least ONE transport to be useful...
  #if defined (MANUVR_SUPPORT_TCPSOCKET)
    ManuvrTCP tcp_srv((const char*) "127.0.0.1", 2319);
    //ManuvrTCP tcp_cli((const char*) "127.0.0.1", 2319);
    //tcp_srv.nonSessionUsage(true);
    kernel->subscribe(&tcp_srv);
    //kernel->subscribe(&tcp_cli);
  #endif
  
  #if defined (MANUVR_SUPPORT_SERIAL)
    ManuvrSerial* ser = NULL;
  #endif

       
  #if defined(RASPI) || defined(RASPI2)
    // If we are running on a RasPi, let's try to fire up the i2c that is almost
    //   certainly present.
    I2CAdapter i2c(1);
    kernel->subscribe(&i2c);
  #endif


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
      createThread(&_thread_id, NULL, spawnUIThread, NULL);
    }
    if ((strcasestr(argv[i], "--serial")) && (argc > (i-2))) {
      // The user wants us to listen to the given serial port.
      #if defined (MANUVR_SUPPORT_SERIAL)
        ser = new ManuvrSerial((const char*) argv[++i], 9600);
        ser->nonSessionUsage(true);   // TODO: Hack to test. Remove and make dynmaic.
        kernel->subscribe(ser);
      #else
        printf("%s was compiled without serial port support. Exiting...\n", argv[0]);
        exit(1);
      #endif
    }
    if ((strcasestr(argv[i], "--quit")) || ((argv[i][0] == '-') && (argv[i][1] == 'q'))) {
      // Execute up-to-and-including boot. Then immediately shutdown.
      // This is how you can stack post-boot-operations into the kernel-> They will execute
      //   following the BOOT_COMPLETE message.
      Kernel::raiseEvent(MANUVR_MSG_SYS_SHUTDOWN, NULL);
    }
  }
  
  
  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr Kernel....\n", program_name);
  kernel->bootstrap();

  // TODO: Horrible hackishness to test TCP...
  #if defined (MANUVR_SUPPORT_TCPSOCKET)
    tcp_srv.listen();
    while (!tcp_srv.listening()) {}
    //tcp_cli.connect();
  #endif
  // TODO: End horrible hackishness.
  
  
  // The main loop. Run forever.
  // TODO: It would be nice to be able to ask the kernel if we should continue running.
  while (running) { 
    kernel->procIdleFlags();

    // Move the kernel log to stdout.
    if (Kernel::log_buffer.count()) {
      if (!kernel->getVerbosity()) {
        Kernel::log_buffer.clear();
      }
      else {
        printf("%s", Kernel::log_buffer.position(0));
        Kernel::log_buffer.drop_position(0);
      }
    }
  }
  
  // Pass a termination signal to the child proc (if we have one).
  if (__shell_pid > 0)  {
    kill(__shell_pid, SIGQUIT);
  }

  printf("\n\n");
  exit(0);
}

