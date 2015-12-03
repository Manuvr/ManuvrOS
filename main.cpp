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

// Transports...
#include <ManuvrOS/Transports/ManuvrSerial/ManuvrSerial.h>
#include <ManuvrOS/Transports/ManuvrSocket/ManuvrSocket.h>


/****************************************************************************************************
* Globals and defines that make our life easier.                                                    *
****************************************************************************************************/
char *program_name      = NULL;


/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/

int main(int argc, char *argv[]) {
  program_name = argv[0];  // Our name.

  Kernel kernel;

  #if defined(__MANUVR_DEBUG)
    kernel.print_type_sizes();
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
      kernel.printDebug();
    }
    if ((strcasestr(argv[i], "--quit")) || ((argv[i][0] == '-') && (argv[i][1] == 'q'))) {
      // Execute up-to-and-including boot. Then immediately shutdown.
      // This is how you can stack post-boot-operations into the kernel. They will execute
      //   following the BOOT_COMPLETE message.
      Kernel::raiseEvent(MANUVR_MSG_SYS_SHUTDOWN, NULL);
    }
  }
  
  
  // We need at least ONE transport to be useful...
  #if defined (MANUVR_SUPPORT_TCPSOCKET)
  ManuvrTCP tcp_srv();
  #endif
  
  #if defined (MANUVR_SUPPORT_SERIAL)
  #endif

  // At this point, we should instantiate whatever specific functionality we
  //   want this Manuvrable to have.
  #if defined(RASPI) || defined(RASPI2)
  
  #endif
  
  // Once we've loaded up all the goodies we want, we finalize everything thusly...
  printf("%s: Booting Manuvr Kernel....\n", program_name);
  kernel.bootstrap();

  // The main loop. Run forever.
  // TODO: It would be nice to be able to ask the kernel if we should continue running.
  while (true) { 
    kernel.procIdleFlags();
    kernel.serviceScheduledEvents();

    // Move the kernel log to stdout.
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

