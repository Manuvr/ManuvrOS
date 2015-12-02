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



/****************************************************************************************************
* Globals and defines that make our life easier.                                                    *
****************************************************************************************************/
char *program_name      = NULL;


/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/

/**
*  Takes one additional parameter at runtime that is not required: The path of the test vectors.
*/
int main(int argc, char *argv[]) {
  program_name = argv[0];  // Our name.

  Kernel kernel;

  // Parse through all the command line arguments and flags...
  // Please note that the order matters. Put all the most-general matches at the bottom of the loop.
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--version")) || ((argv[i][0] == '-') && (argv[i][1] == 'v'))) {
      printf("%s v%s\n\n", argv[0], VERSION_STRING);
    }
    if ((strcasestr(argv[i], "--info")) || ((argv[i][0] == '-') && (argv[i][1] == 'i'))) {
      // Cause the kernel to write a self-report to its own log.
      kernel.printDebug();
    }
    // TODO: Need to craft an example...
    // This is how you can stack post-boot-operations into the kernel. They will execute
    //   following the BOOT_COMPLETE message.
  }
 
  printf("%s: Booting Manuvr Kernel....\n", program_name);
  kernel.bootstrap();

  // The main loop. Run forever.
  // TODO: It would be nice to be able to ask the kernel if we should continue running.
  while (true) { 
    kernel.procIdleFlags();
    kernel.serviceScheduledEvents();

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

