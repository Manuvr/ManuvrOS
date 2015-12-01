#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/signal.h>
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
const int INTERRUPT_PERIOD = 1;        // How many seconds between SIGALRM interrupts?

int parent_pid  = 0;            // The PID of the root process (always).
int looper_pid  = 0;            // This is the PID for the VM thread.

ManuvrComPort* com_port = NULL;

int maximum_field_print = 65;       // The maximum number of bytes we will print for sessions. Has no bearing on file output.

bool run_looper          = true;  // We will be running the looper in its own thread.
bool continue_listening  = true;
char *program_name       = NULL;
static Kernel* kernel    = NULL;


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
void sig_handler(int signo) {
    switch (signo) {
        case SIGINT:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGINT signal. Closing up shop...");
          continue_listening    = false;
          break;
        case SIGKILL:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGKILL signal. Something bad must have happened. Exiting hard....");
          exit(1);
          break;
        case SIGTERM:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGTERM signal. Closing up shop...");
          continue_listening    = false;
          break;
        case SIGQUIT:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGQUIT signal. Closing up shop...");
          continue_listening    = false;
          break;
        case SIGHUP:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGHUP signal. Closing up shop...");
          continue_listening    = false;
          break;
        case SIGSTOP:
           fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Received a SIGSTOP signal. Closing up shop...");
           continue_listening    = false;
           break;
        case SIGALRM:
           kernel->advanceScheduler();
           if (continue_listening) alarm(INTERRUPT_PERIOD);    // Fire again later...
           break;
        case SIGUSR1:      // Cause a configuration reload.
          break;
        case SIGUSR2:    // Cause a database reload.
          break;
        default:
          fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Unhandled signal: %d", signo);
          break;
    }

    // Echo whatever signals we receive to the child proc (if we are the parent).
    if ((looper_pid > 0) && (signo != SIGALRM) && (signo != SIGUSR2)) {
        kill(looper_pid, signo);
    }
}



// The parent process should call this function to set the callback address to its signal handlers.
//     Returns 1 on success, 0 on failure.
int initSigHandlers(){
    int return_value    = 1;
    // Try to open a binding to listen for signals from the OS...
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGINT to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGQUIT to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGHUP, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGHUP to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGTERM to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGUSR1 to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGUSR2 to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGALRM, sig_handler) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGALRM to the signal system. Failing...");
        return_value = 0;
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to bind SIGCHLD to the signal system. Failing...");
        return_value = 0;
    }
    return return_value;
}


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
  kernel = Kernel::getInstance();
  
  printf("\nSH pointer address: 0x%08x\n", (uint32_t)kernel);
  //watchdog_mark = 42;  // The period (in ms) of our clock punch. 

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

  initSigHandlers();
  alarm(INTERRUPT_PERIOD);                // Set a periodic interrupt.

  Kernel kernel0;
  //kernel0.run();
 
  // The main loop. Run forever.
  while (continue_listening) {
    int x = kernel0.procIdleFlags();
    if (x > 0) printf("\t kernel0.step() returns %d\n", x);
  }
  
  printf("\n\n");
  return 0;
}
