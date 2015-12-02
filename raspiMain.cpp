#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#include <sys/socket.h>
#include <fstream>
#include <iostream>

#include <netinet/in.h>
#include <fcntl.h>
#include <termios.h>

#include "FirmwareDefs.h"

#include <ManuvrOS/Kernel.h>
#include <ManuvrOS/Platform/Platform.h>
#include "ManuvrOS/Transports/ManuvrComPort/ManuvrComPort.h"


/****************************************************************************************************
* Function prototypes.                                                                              *
****************************************************************************************************/
void printBinString(unsigned char * str, int len);


/****************************************************************************************************
* Globals and defines that make our life easier.                                                    *
****************************************************************************************************/
int parent_pid  = 0;            // The PID of the root process (always).
int looper_pid  = 0;            // This is the PID for the VM thread.

ManuvrComPort* com_port = NULL;

int maximum_field_print = 65;       // The maximum number of bytes we will print for sessions. Has no bearing on file output.

bool run_looper          = true;  // We will be running the looper in its own thread.
bool continue_listening  = true;
char *program_name       = NULL;


// Log a message. Target is determined by the current_config.
//    If no logging target is specified, log to stdout.
void fp_log(const char *fxn_name, int severity, const char *str, ...) {
    va_list marker;
    printf("%d\t%s  ", severity, fxn_name);
    va_start(marker, str);
    vprintf(str, marker);
    va_end(marker);
}



/****************************************************************************************************
* Signal catching code.                                                                             *
****************************************************************************************************/



/****************************************************************************************************
* Functions that just print things.                                                                 *
****************************************************************************************************/

/*
*  An output function that prints the given number of integer values of a given binary string.
*/
void printBinString(unsigned char * str, int len) {
  int i = 0;
  unsigned int moo  = 0;
  int temp_len = min(len, maximum_field_print);
  if ((str != NULL) && (temp_len > 0)) {
    for (i = 0; i < temp_len; i++) {
      moo  = *(str + i);
      printf("%02x", moo);
    }
    if (len > temp_len) {
      printf(" \033[01;33m(truncated %d bytes)\033[0m", len - temp_len);
    }
  }
}


/**
* The help function. We use printf() because we are certain there is a user at the other end of STDOUT.
*/
void printUsage() {
  printf("==================================================================================\n");
  printf("Bus and channel selection:\n");
  printf("==================================================================================\n");
  printf("    --i2c-dev     Specify the i2c device to use.\n");
  printf("    --tty-dev     Specify the tty device to use.\n");
  printf("-i  --input       input pin (0-11)\n");
  printf("-o  --output      output pin (0-7)\n");
  printf("\n");

  printf("==================================================================================\n");
  printf("Meta:\n");
  printf("==================================================================================\n");
  printf("-v  --version     Print the version and exit.\n");
  printf("-h  --help        Print this output and exit.\n");
  printf("-s  --status      Read and print the present condition of the PCB.\n");
  printf("    --reset       Reset the PCB back to it's power-on state.\n");
  printf("    --enable      Enable a PCB that was previously disabled.\n");
  printf("    --disable     Disable the PCB. Mutes all outputs.\n");
  printf("\n\n");
}



/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/

/**
*  Takes one additional parameter at runtime that is not required: The path of the test vectors.
*/
int main(int argc, char *argv[]) {
  program_name = argv[0];  // Our name.
  StringBuilder output;
  
  printf("\n");
  printf("===================================================================================================\n");
  printf("|                                       Manuvr Test Apparatus                                     |\n");
  printf("===================================================================================================\n");

  printf("Booting Manuvr Kernel....\n");
  Kernel kernel;
  kernel.bootstrap();
  
  // Parse through all the command line arguments and flags...
  // Please note that the order matters. Put all the most-general matches at the bottom of the loop.
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--help")) || ((argv[i][0] == '-') && (argv[i][1] == 'h'))) {
      printUsage();
      exit(1);
    }
    else if ((strcasestr(argv[i], "--version")) || ((argv[i][0] == '-') && (argv[i][1] == 'v'))) {
      printf("%s v%s\n\n", argv[0], VERSION_STRING);
      exit(1);
    }
    else if (strcasestr(argv[i], "--tty-dev") && (i < argc-1)) {
      com_port = new ManuvrComPort(argv[i+1], 115200, O_RDWR | O_NOCTTY | O_NDELAY);
      i++;
    }
    else if (strcasestr(argv[i], "--ws-connect") && (i < argc-1)) {
      // Connect to another manuvrable at the given address.
    }
    else if (strcasestr(argv[i], "--ws-listen") && (i < argc-1)) {
      // Adding a websocket server at the given IPv4 address.
    }
    else if (strcasestr(argv[i], "--ws-port") && (i < argc-1)) {
      // Port modifier for websocket connections.
    }
    else {
      printf("Unhandled argument: %s\n", argv[i]);
      printUsage();
      exit(1);
    }
  }
 
  
  // The main loop. Run forever.
  while (continue_listening) {
    int x = 0;
    x = kernel.procIdleFlags();
    if (x > 0) printf("\t kernel0.procIdleFlags() returns %d\n", x);
    x = kernel.serviceScheduledEvents();
    if (x > 0) printf("\t kernel0.serviceScheduledEvents() returns %d\n", x);
    if (Kernel::log_buffer.count()) {
      if (!kernel.getVerbosity()) {
        Kernel::log_buffer.clear();
      }
      else {
        printf("%s", Kernel::log_buffer.position(0));
        Kernel::log_buffer.drop_position(0);
      }
    }
  }
  
  printf("\n\n");
  return 0;
}

