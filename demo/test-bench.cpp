
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

#include "demo/StaticHub.h"
#include "ManuvrOS/EventManager.h"
#include "ManuvrOS/Scheduler.h"
#include "ManuvrOS/Transports/ManuvrComPort/ManuvrComPort.h"
#include "ManuvrOS/XenoSession/XenoSession.h"
#include "ManuvrOS/Drivers/TMP006/TMP006.h"


/****************************************************************************************************
* Function prototypes.                                                                              *
****************************************************************************************************/
char *trim(char *str);

void run_one_idle_loop(EventManager* em, Scheduler* sch);

void printBinString(unsigned char * str, int len);
void troll(void);
void printHelp(void);


/****************************************************************************************************
* Globals and defines that make our life easier.                                                    *
****************************************************************************************************/
const int BUFFER_LEN       = 8192;     // This is the maximum size of any given packet we can handle.
const int INTERRUPT_PERIOD = 1;        // How many seconds between SIGALRM interrupts?



int parent_pid  = 0;            // The PID of the root process (always).
int looper_pid  = 0;            // This is the PID for the VM thread.

XenoSession* session     = NULL;
XenoSession* com_session = NULL;
ManuvrComPort* com_port = NULL;

int maximum_field_print = 65;       // The maximum number of bytes we will print for sessions. Has no bearing on file output.


bool run_looper          = true;  // We will be running the looper in its own thread.
bool continue_listening  = true;
char *program_name       = NULL;
static StaticHub* sh     = NULL;



// Log a message. Target is determined by the current_config.
//    If no logging target is specified, log to stdout.
void fp_log(const char *fxn_name, int severity, const char *str, ...) {
    va_list marker;
    printf("%d\t%s  ", severity, fxn_name);
    va_start(marker, str);
    vprintf(str, marker);
    va_end(marker);
}



/*  Trim the whitespace from the beginning and end of the input string.
*  Should not be used on malloc'd space, because it will eliminate the
*    reference to the start of an allocated range which must be freed.
*/
char* trim(char *str){
  char *end;
  while(isspace(*str)) str++;
  if(*str == 0) return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end+1) = '\0';
  return str;
}


/*
*  Will interpret the given str as a sequence of bytes in hex.
*  Function parses the bytes (after left-padding for inputs with odd character counts) and
*    will write the resulting bytes into result. Returns the number of bytes so written.
*/
int parseStringIntoBytes(char* str, unsigned char* result) {
  int str_len  = strlen(str);
  char *temp_str  = (char*)alloca(str_len + 2);
  char *str_idx  = temp_str;
  bzero(temp_str, str_len + 2);
  if (str_len % 2 == 1) {    // Are there an odd number of digits?
    // If so, left-pad the string with a zero and pray.
    *str_idx  = '0';
    str_idx++;
  }
  memcpy(str_idx, str, str_len);    // Now copy the string into a temp space on the stack...
  str_len  = strlen(temp_str);

  int return_value  = str_len/2;
  char *sub_buf    = (char*)alloca(3);
  int i = 0;
  int n = 0;
  for (i = 0; i < (return_value*2); i=i+2) {
    bzero(sub_buf, 3);
    memcpy(sub_buf, (temp_str+i), 2);
    *(result + n++) = 0xFF & strtoul(sub_buf, NULL, 16);
  }
  return return_value;
}



/*
*  Compares two binary strings on a byte-by-byte basis.
*  Returns 1 if the values match. 0 otherwise.
*/
int cmpBinString(unsigned char *unknown, unsigned char *known, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    if (*(unknown+i) != *(known+i)) return 0;
  }
  return 1;
}



/*
*  Writes the given bit string into a character buffer.
*/
char* printBinStringToBuffer(unsigned char *str, int len, char *buffer) {
  if (buffer != NULL) {
  int i = 0;
    unsigned int moo  = 0;
    if ((str != NULL) && (len > 0)) {
      for (i = 0; i < len; i++) {
        moo  = *(str + i);
        sprintf((buffer+(i*2)), "%02x", moo);
      }
    }
  }
  return buffer;
}


/*
*  Reads a string into bytes.
*  Returns the number of _bits_ read
*/
int bint2bin(const char *in, unsigned char *out) {
  int  n = 0;
  int  len  = strlen(in);
  if (len % 8 > 0) n = 1;
  bzero(out, len/8+n);
  for(n = 0; n < len; ++n) {
    if (*(in+n) == '1') out[n/8] |= (0x80 >> (n % 8));
    else out[n/8] &= ~(0x80 >> (n % 8));
  }
  return n;
}


///*
//* Perform a SHA256 digest.
//* It is the responsibility of the caller to ensure that the return buffer has enough space allocated
//*   to receive the digest.
//* Returns 1 on success and 0 on failure.
//*/
//int PROC_SHA256_MSG(unsigned char *msg, long msg_len, unsigned char *md, unsigned int md_len){
//  int return_value    = 0;
//  const EVP_MD *evp_md  = EVP_sha256();
//
//  memset(md, 0x00, md_len);
//
//  if (evp_md != NULL) {
//    EVP_MD_CTX *cntxt = (EVP_MD_CTX *)(intptr_t) EVP_MD_CTX_create();
//    EVP_DigestInit(cntxt, evp_md);
//    if (msg_len > 0) EVP_DigestUpdate(cntxt, msg, msg_len);
//    EVP_DigestFinal_ex(cntxt, md, &md_len);
//    EVP_MD_CTX_destroy(cntxt);
//    return_value  = 1;
//  }
//  else {
//    fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to load the digest algo SHA256.");
//  }
//  return return_value;
//}
//
//
///*
//* Function takes a path and a buffer as arguments. The binary is hashed and the ASCII representation is
//*   placed in the buffer. The number of bytes read is returned on success. 0 is returned on failure.
//*/
//long hashFileByPath(char *path, char *h_buf) {
//  long return_value    = 0;
//  ifstream self_file(path, ios::in | ios::binary | ios::ate);
//  if (self_file != NULL) {
//    long self_size = self_file.tellg();
//    
//    char *self_mass   = (char *) alloca(self_size);
//    int digest_size = 32;
//    unsigned char *self_digest = (unsigned char *) alloca(digest_size);
//    memset(self_digest, 0x00, digest_size);
//    memset(self_mass, 0x00, self_size);
//    self_file.seekg(0);   // After checking the file size, make sure to reset the read pointer...
//    self_file.read(self_mass, self_size);
//    fp_log(__PRETTY_FUNCTION__, LOG_INFO, "%s is %d bytes.", path, self_size);
//    
//    if (PROC_SHA256_MSG((unsigned char *) self_mass, self_size, self_digest, digest_size)) {
//      memset(h_buf, 0x00, 65);
//      printBinStringToBuffer(self_digest, 32, h_buf);
//      fp_log(__PRETTY_FUNCTION__, LOG_INFO, "This binary's SHA256 fingerprint is %s.", h_buf);
//      return_value = self_size;
//    }
//    else {
//      fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to run the hash on the input path.");
//    }
//  }
//  return return_value;
//}



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
           sh->advanceScheduler();
           run_one_idle_loop(sh->fetchEventManager(), sh->fetchScheduler());
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
* Simple tests and one-offs.                                                                        *
****************************************************************************************************/
int test_StringBuilder(void) {
  StringBuilder *heap_obj = new StringBuilder("This is datas we want to transfer.");
  StringBuilder stack_obj;
  
  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();
  
  
  
  printf("Heap obj before culling:   %s\n", heap_obj->string());
  
  while (heap_obj->length() > 10) {
    heap_obj->cull(5);
    printf("Heap obj during culling:   %s\n", heap_obj->string());
  }
  
  printf("Heap obj after culling:   %s\n", heap_obj->string());
  
  heap_obj->prepend("Meaningless data ");
  heap_obj->concat(" And stuff tackt onto the end.");

  stack_obj.concatHandoff(heap_obj);

  delete heap_obj;
  
  stack_obj.split(" ");
  
  printf("Final Stack obj:          %s\n", stack_obj.string());

  return 0;
}


int statistical_mode_test() {
  PriorityQueue<double> mode_bins;
  double dubs[20] = {0.54, 0.10, 0.68, 0.54, 0.54, \
                     0.10, 0.17, 0.67, 0.54, 0.09, \
                     0.57, 0.15, 0.68, 0.54, 0.67, \
                     0.11, 0.10, 0.64, 0.54, 0.09};
   
    
  for (int i = 0; i < 20; i++) {
      if (mode_bins.contains(dubs[i])) {
        mode_bins.incrementPriority(dubs[i]);
      }
      else {
        mode_bins.insert(dubs[i], 1);
      }
  }
  
  double temp_double = 0;
  double most_common = mode_bins.get();
  int stat_mode      = mode_bins.getPriority(most_common);
  printf("Most common:  %lf\n", most_common);
  printf("Mode:         %d\n\n",  stat_mode);
  
  // Now let's print a simple histogram...
  for (int i = 0; i < mode_bins.size(); i++) {
    temp_double = mode_bins.get(i);
    stat_mode = mode_bins.getPriority(temp_double);
    printf("%lf\t", temp_double);
    for (int n = 0; n < stat_mode; n++) {
      printf("*");
    }
    printf("  (%d)\n", stat_mode);
  }

  for (int i = 0; i < mode_bins.size(); i++) {
    printf("\nRecycling  (%lf)", mode_bins.recycle());
  }
  printf("\n");
  
  // Now let's print a simple histogram...
  while (mode_bins.hasNext()) {
    temp_double = mode_bins.get();
    stat_mode = mode_bins.getPriority(temp_double);
    printf("%lf\t", temp_double);
    for (int n = 0; n < stat_mode; n++) {
      printf("*");
    }
    printf("  (%d)\n", stat_mode);
    mode_bins.dequeue();
  }
  return 0;
}


int recycle_test() {
  PriorityQueue<double> recycle;
  double dubs[20] = {0.01, 0.02, 0.03, 0.04, 0.05, \
                     0.06, 0.07, 0.08, 0.09, 0.10, \
                     0.11, 0.12, 0.13, 0.14, 0.15, \
                     0.16, 0.17, 0.18, 0.19, 0.20};
   
  for (int i = 0; i < 20; i++) recycle.insert(dubs[19-i]);
    
  for (int i = 0; i < 200; i++) {
    printf("\nRecycling  (%lf)\n", recycle.recycle());
  }
  
  for (int i = 0; i < recycle.size(); i++) {
    printf("%lf\t", recycle.get(i));
  }

  for (int i = 0; i < recycle.size(); i++) {
    printf("\nReversing list  %d\n", i);
    recycle.insert(recycle.dequeue(), i);
  }
  
  for (int i = 0; i < recycle.size(); i++) {
    printf("%lf\t", recycle.get(i));
  }
  return 0;
}


int test_PriorityQueue(void) {
  statistical_mode_test();
  recycle_test();
  return 0;
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


/*
*  PrObLeM???
*/
void troll() {
  printf("\n");
  printf("777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777\n");
  printf("777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777\n");
  printf("777777777777777777777777777777777............................................7777777777777777777777\n");
  printf("7777777777777777777777777................7777777777777777777777777777777777.....7777777777777777777\n");
  printf("7777777777777777777...........7777777777.....777777777777777..7777777777777777...777777777777777777\n");
  printf("77777777777777777....77777777777777777777777777777777777777777777..7777777777777...7777777777777777\n");
  printf("77777777777777....7777.77......777777777777...77777777777777..777777.777777777777...777777777777777\n");
  printf("777777777777....7777777777777777777777777777777777777777777777777..7777.7777777777...77777777777777\n");
  printf("77777777777...777777777777.....77777777777777777777.7777777777...777.7777.777777777...7777777777777\n");
  printf("7777777777..77777777777.7777777777777777777777777.7777777777777777..777.7777.7777777...777777777777\n");
  printf("7777777777..777777777.777777777777.7777777777777777777777777777777777.77.7777.7777777..777777777777\n");
  printf("7777777777..77777777.77777777777777777777777777777777777777777777777777.77.77777777777..77777777777\n");
  printf("777777777..777777777777777777777777777777777777777777..............7777777777777777777...7777777777\n");
  printf("77777777...777777777777........7777777777777777777.....777............77777777777777777...777777777\n");
  printf("7777....77777777777..............7777777777777....777777........77...777777777777777777...777777777\n");
  printf("777...777.....7...7...............77777777777...77777.................7777.7777........7....7777777\n");
  printf("77..777.777777777777777777.............7777777.........77777777777..777.777777777777777777...777777\n");
  printf("7..77.777..777777777777777777777....77777777777.....777777...77777777777777..........7777777..77777\n");
  printf("7..7.77.777......7777777777777777..7777777777777777777777777....7777777......777777....7777.7..7777\n");
  printf("7...77777..........7777.777777777..777777777777777777777777777...........77777..77777...777.77...77\n");
  printf("7...77777.7777777........77777777..7777777777777777777777777777777777777777777..777777..777.77...77\n");
  printf("7...77.7777777..77....77777777....77777777777777777777777777777777777777777.....7777777..77.777..77\n");
  printf("7..7777.777777..77777777777....7777777777777777........7777777777777777.....777......77..77.777..77\n");
  printf("7....777..77....7777777777.....77777777777777777777..77777777777777......77777...7...7...77.77...77\n");
  printf("77..7..77777....7777777..77.....777777777.......777..7777777777.......77777777..777777..777777...77\n");
  printf("77...7777.77..7...777.77777777...77777777777777.7...777777........7..7777777....77777..777.77...777\n");
  printf("777...77777.........777777777777......777777777777777........777777..7777......777777777..77..77777\n");
  printf("7777...7777..7..7......77777777777...77777777777........7777777777...7........7777777.77777...77777\n");
  printf("77777..7777....77..77........77777777..............77..7777777777.......77...7777777777.....7777777\n");
  printf("77777..7777....77..7777....................7777777777..777777........7777...777777777777...77777777\n");
  printf("77777..7777....77..777...7777777..7777777..7777777777...7.........7..777...77777777777...7777777777\n");
  printf("77777..7777........777..77777777..7777777..777777777...........7777..77...77777777777...77777777777\n");
  printf("77777..7777.................................................7777777......777777777777..777777777777\n");
  printf("77777..7777...........................................7..77777777777...7777777777777...777777777777\n");
  printf("77777..77777....................................7777777..777777777...77777777777777...7777777777777\n");
  printf("77777..77777..7...........................7..7777777777...77777....777777777777777...77777777777777\n");
  printf("77777..777777..7..77..777..77777...77777777..77777777777..777....777777777777777...7777777777777777\n");
  printf("77777..777777......7...777..77777..77777777..777777777777......7777777777777777...77777777777777777\n");
  printf("77777..7777777....777...77...7777..77777777..777777777......7777777.77777.777...7777777777777777777\n");
  printf("77777..777777777.........77..7777...7777777..77.........77777777.77777..777....77777777777777777777\n");
  printf("77777..77777777777777...............................777777777..7777..7777....7777777777777777777777\n");
  printf("7777...77777777.777777777777777777777777777777777777777777.77777..7777.....777777777777777777777777\n");
  printf("7777..7777777777.777777777777777777777777777777777777..777777.77777.....777777777777777777777777777\n");
  printf("7777..777777777777.777777777777777777777777777777..777777..77777.....777777777777777777777777777777\n");
  printf("77..77777..77777777...77777777777..........777777..7777777......77777777777777777777777777777777777\n");
  printf("7..777777777.7777777777777777777777777...777777777777.....77777777777777777777777777777777777777777\n");
  printf("7...7777777777...............777777777777777777777.....77777777777777777777777777777777777777777777\n");
  printf("7...777777777777777777777777777777777777777777.....777777777777777777777777777777777777777777777777\n");
  printf("77...77777777777777777777777777777777777..7.....777777777777777777777777777777777777777777777777777\n");
  printf("777....7777777777777777777777777777..........777777777777777777777777777777777777777777777777777777\n");
  printf("7777.....7777777777777777777.........77777777777777777777777777777777777777777777777777777777777777\n");
  printf("7777777........................77777777777777777777777777777777777777777777777777777777777777777777\n");
  printf("777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777\n");
  printf("\n\n");
}


/*
*  Prints a list of valid commands.
*/
void printHelp() {
  printf("==< HELP >=========================================================================================\n");
  printf("%s v%s          Build date:  %s %s\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  printf("\n");
  printf("'help'\t\t\t\tThis.\n");
  printf("'max-width <arg>'\t\tSet the maximum width of field renders (bytes represented) in a session. Presently \033[01;32m%d\033[0m.\n", maximum_field_print);
  printf("'r'\t\t\t\tExecute a single idle loop.\n");
  printf("'s'\t\t\t\tAdvance the scheduler by a ms.\n");
  printf("'d'\t\t\t\tDump debug output.\n");
  printf("'pq'\t\t\t\tTest the PriorityQueue data structure.\n");
  printf("'sb'\t\t\t\tTest the StringBuilder class..\n");
  printf("'quit'\t\t\t\tBail? Bail.\n");
  printf("\n");
  printf("\n\n");
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
	printf("Options that operate on channels:\n");
	printf("==================================================================================\n");
	printf("-r  --route       Route the given input to the given output.\n");
	printf("-u  --unroute     Unroute the given input from the given output(s). If no output\n");
	printf("                   is specified, we will unroute the given input from all outputs.\n");
	printf("-v  --volume      Specify the value of the linear potentiometer (0-255).\n");
	printf("                   If no output is specified, we will adjust all outputs.\n");
	printf("-m  --mute        Mutes a specified output channel. If no output channel is\n");
	printf("                   specified, we will mute all outputs. Note that muting an output\n");
	printf("                   has no effect on the route.\n");
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




void run_one_idle_loop(EventManager* em, Scheduler* sch) {
  int x = em->procIdleFlags();
  int y = sch->serviceScheduledEvents();
  if (x && (x < EVENT_MANAGER_MAX_EVENTS_PER_LOOP-1)) printf("\t procIdleFlags() returns %d\n", x);
  if (y) printf("\t serviceScheduledEvents() returns %d\n", y);
}


void proc_until_finished() {
  EventManager* em = sh->fetchEventManager();
  Scheduler*    sch = sh->fetchScheduler();

  run_one_idle_loop(em, sch);
 
  /* TODO: This is awful. Need to thread to make this happen. Need to 
             get a better grip on time on the linux build. Does libuv have that? */
  for (int i = 0; i < 1000; i++) StaticHub::advanceScheduler();

  run_one_idle_loop(em, sch);
}





/****************************************************************************************************
* The work-area...                                                                                  *
****************************************************************************************************/




void event_battery() {
  printf("===================================================================================================\n");
  printf("|                                   Event System Battery                                          |\n");
  printf("====<  Leak test  >================================================================================\n");
  
  ManuvrEvent *event = NULL;
  
  proc_until_finished();
  for (int i = 0; i < 10000; i++) {
    // launch an event and run it a whole mess of times.
    event = EventManager::returnEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
    event->addArg((uint8_t) 2);
    EventManager::staticRaiseEvent(event);
    proc_until_finished();
  }
  
  for (int i = 0; i < 10; i++) {
    for (int i = 0; i < (EVENT_MANAGER_PREALLOC_COUNT*2); i++) {
      // Puposely flood the queue several times...
      event = EventManager::returnEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
      event->addArg((uint8_t) 2);
      EventManager::staticRaiseEvent(event);
    }
    proc_until_finished();
  }
  proc_until_finished();
}






void session_battery() {
//  StringBuilder transport_buffer;
//  
//  printf("===================================================================================================\n");
//  printf("|                                   XenoSession unit tests                                        |\n");
//  printf("====<  Leak test  >================================================================================\n");
//  proc_until_finished();
//  
//  EventManager::raiseEvent(MANUVR_MSG_LEGEND_MESSAGES, NULL);
//  proc_until_finished();
//
//  EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);
//  proc_until_finished();
//
//  EventManager::raiseEvent(MANUVR_MSG_SELF_DESCRIBE, NULL);
//  proc_until_finished();
//  
//  printf("nextMessage() returned %d as well as the following bytes:\n", session->nextMessage(&transport_buffer));
//  transport_buffer.printDebug();
//  printf("nextMessage() returned %d as well as the following bytes:\n", session->nextMessage(&transport_buffer));
//  transport_buffer.printDebug();
//
//  EventManager::raiseEvent(XENOSESSION_STATE_DISCONNECTED, NULL);
//  proc_until_finished();
//
//  printf("nextMessage() returned %d as well as the following bytes:\n", session->nextMessage(&transport_buffer));
//  transport_buffer.printDebug();
//
//  EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);
//  proc_until_finished();
//
//  
//
//  printf("====<  Sync test  >================================================================================\n");
//  establishSession();
//  proc_until_finished();
//
//  // Mark the session disconnected() to cause a desire for sync...
//  session->markSessionConnected(false);
//  proc_until_finished();
//  
//  session->markSessionConnected(true);
//  proc_until_finished();
//  
//  EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);
//  proc_until_finished();
//
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES[0], 2);
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES[2], 2);
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES[0], 1);
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES[1], 3);
//
//
//  EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);
//  proc_until_finished();
//
//  printf("Invalid test sequence sent (KMAP_INIT) bad checksum.\n");  {
//    unsigned char test_xeno_mes[] = {0x08, 0x00, 0x00, 0x21, 0x03, 0xa0, 0x20, 0x0a};
//    session->bin_stream_rx(test_xeno_mes, sizeof(test_xeno_mes));
//    EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);  // Raise an event
//    proc_until_finished();
//  }
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
//  session->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
//  proc_until_finished();
//
//  printf("Test sequence sent (KMAP_INIT).\n");  {
//    unsigned char test_xeno_mes[] = {0x08, 0x00, 0x00, 0x22, 0x03, 0xa0, 0x20, 0x0a};
//    session->bin_stream_rx(test_xeno_mes, sizeof(test_xeno_mes));
//    EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);  // Raise an event
//    proc_until_finished();
//  }


  //printf("====<  Sensible dialog test  >=====================================================================\n");
  //printf("====<  Sketchy transport quality  >================================================================\n");
  //printf("====<  Schizophrenic counterparty  >===============================================================\n");
}


void session_battery_1() {
  StringBuilder transport_buffer_in;
  StringBuilder output;
  XenoSession *sess = new XenoSession();
  
  printf("===================================================================================================\n");
  printf("|                                   XenoSession unit tests                                        |\n");
  printf("====<  Arg decomposition  >========================================================================\n");
  proc_until_finished();
  
  sess->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
  sess->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
  sess->bin_stream_rx((unsigned char*) &XenoSession::SYNC_PACKET_BYTES, 4);   // Pushes a sync packet into the Session.
  proc_until_finished();

  ManuvrEvent *test_event = EventManager::returnEvent(MANUVR_MSG_SELF_DESCRIBE);
  EventManager* em = sh->fetchEventManager();
  em->subscribe(sess);
  proc_until_finished();

  test_event->callback = sess;
  EventManager::staticRaiseEvent(test_event);
  proc_until_finished();


  //printf("Self-describe message generated. Packet takes %d bytes.\n", inbound_msg.buffer.length());
  //sess->bin_stream_rx(inbound_msg.buffer.string(), inbound_msg.buffer.length());
  proc_until_finished();

  sess->printDebug(&output);
  proc_until_finished();

  em->unsubscribe(sess);
  proc_until_finished();

  delete sess;
//  session->printDebug(&output);
//  printf("%s\n", (char*)output.string());
//  
//  proc_until_finished();
//  
//  EventManager::raiseEvent(MANUVR_MSG_SESS_DUMP_DEBUG, NULL);  // Raise an event
//  proc_until_finished();
//  
//  printf("nextMessage() returned %d as well as the following bytes:\n", session->nextMessage(&transport_buffer_in));
//  transport_buffer_in.printDebug();
//
}


void session_battery_2() {
  StringBuilder transport_buffer_in;
  StringBuilder output;
  
  printf("===================================================================================================\n");
  printf("|                                   XenoSession unit tests                                        |\n");
  printf("====<  Tied Together  >============================================================================\n");
  
  ManuvrEvent test_event_in(MANUVR_MSG_USER_BUTTON_PRESS);
  test_event_in.addArg((uint8_t) 9);
  
  if (NULL != com_port) {
    com_port->establishSession();
    com_port->getSession()->sendEvent(&test_event_in);
  }
  
  proc_until_finished();
  
  ManuvrEvent *test_event = EventManager::returnEvent(MANUVR_MSG_XPORT_DEBUG);
  test_event->addArg((uint8_t) 1);
  EventManager::staticRaiseEvent(test_event);
  proc_until_finished();
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

  printf("Booting StaticHub....\n");
  sh = StaticHub::getInstance();
  
  printf("\nSH pointer address: 0x%08x\n", (uint32_t)sh);
  StaticHub::watchdog_mark = 42;  // The period (in ms) of our clock punch. 

  EventManager* event_manager = sh->fetchEventManager();
  Scheduler*    scheduler     = sh->fetchScheduler();
  

    ///* INTERNAL INTEGRITY-CHECKS
    //*  Now... at this point, with our config complete and a database at our disposal, we do some administrative checks...
    //*  The first task is to look in the mirror and find our executable's full path. This will vary by OS, but for now we
    //*  assume that we are on a linux system.... */
    //char *exe_path = (char *) alloca(512);   // 512 bytes ought to be enough for our path info...
    //memset(exe_path, 0x00, 512);
    //int exe_path_len = readlink("/proc/self/exe", exe_path, 512);
    //if (!(exe_path_len > 0)) {
    //    fp_log(__PRETTY_FUNCTION__, LOG_ERR, "%s was unable to read its own path from /proc/self/exe. You may be running it on an unsupported operating system, or be running an old kernel. Please discover the cause and retry. Exiting...", exe_path);
    //    exit(1);
    //}
    //fp_log(__PRETTY_FUNCTION__, LOG_INFO, "This binary's path is %s", exe_path);
    //
    //// Now to hash ourselves...
    //char *h_buf = (char *)alloca(65);
    //memset(h_buf, 0x00, 65);
    //hashFileByPath(exe_path, h_buf);
    //
    //// If we've stored a hash for our binary, compare it with the hash we calculated. Make sure they match. Pitch a fit if they don't.
    //if (conf.configKeyExists("binary-hash")) {
    //    if (strcasestr(h_buf, conf.getConfigStringByKey("binary-hash")) == NULL) {
    //      fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Calculated hash value does not match what was stored in your config. Exiting...");
    //      exit(1);
    //    }
    //}
    ///* INTERNAL INTEGRITY-CHECKS */


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
		else {
			printf("Unhandled argument: %s\n", argv[i]);
			printUsage();
			exit(1);
		}
	}


//  // Now to bifurcate...
//  looper_pid      = fork();
//  if (looper_pid == -1) {        // Fork failure?
//      fp_log(__PRETTY_FUNCTION__, LOG_ERR, "%s failed to fork(). It may not be supported on this architecture.", program_name);
//      exit(1);
//  }
//  else if (looper_pid != 0) {        // Are we the child?
//    initSigHandlers();
//    alarm(1);                // Set a periodic interrupt.
//      while (continue_listening) {
//        run_one_idle_loop(event_manager, scheduler);
//      }
//      exit(0);
//  }
//  else {                // Are we the parent? Also executes if (pd_pid == -2).
//  }
    initSigHandlers();
    alarm(1);                // Set a periodic interrupt.
  
  proc_until_finished();

  printf("\n");
  
  char *input_text  = (char*)alloca(BUFFER_LEN);  // Buffer to hold user-input.
  char *trimmed;            // Temporary pointers for manipulating user-input.
  StringBuilder parsed;

  // The main loop. Run forever.
  while (continue_listening) {
    printf("%c[36m%s> %c[39m", 0x1B, argv[0], 0x1B);
    bzero(input_text, BUFFER_LEN);
    if (fgets(input_text, BUFFER_LEN, stdin) != NULL) {
      trimmed  = strchr(input_text, '\n');
      if (trimmed != NULL) *trimmed = '\0';            // Eliminate a possible newline character.
      trimmed  = trim(input_text);                // Nuke any excess whitespace the user might have entered.
      
      if (strlen(trimmed) > 0) {
          parsed.concat(trimmed);
          parsed.split(" ");
          
          char* tok = parsed.position(0);
        
          // Begin the cases...
          if (strlen(tok) == 0); //               printf("\n");            // User entered nothing.
          else if (strcasestr(tok, "QUIT"))    continue_listening = false;  // Exit
          else if (strcasestr(tok, "HELP"))    printHelp();            // Show help.
          else if (strcasestr(tok, "TROLL"))    troll();              // prOBleM?.
          else if (strcasestr(tok, "#PQ"))    test_PriorityQueue();  
          else if (strcasestr(tok, "#SB"))    test_StringBuilder();


          else if (strcasestr(tok, "#XB"))    session_battery();
          else if (strcasestr(tok, "#XA"))    session_battery_1();
          else if (strcasestr(tok, "#XC"))    session_battery_2();
          else if (strcasestr(tok, "#EB"))    event_battery();
          else if (strcasestr(tok, "#TS")) {
            TMP006* temp_sense = sh->fetchTMP006();
            temp_sense->readSensor();
            temp_sense->printDebug(&output);
          }
          else if (strcasestr(tok, "#B")) {
            printf("Running for a bit...\n");
            for (int i = 0; i < 100; i++) {
              for (int n = 0; n < 100; n++) run_one_idle_loop(event_manager, scheduler);
              
            }
          }
          else if (strcasestr(tok, "#R")) {
            int max_loops = (parsed.count() > 1) ? parsed.position_as_int(1) : 1;
            printf("Running %d idle_loop(s)...\n", max_loops);
            for (int i = 0; i < max_loops; i++) {
              run_one_idle_loop(event_manager, scheduler);
            }
          }
          else if (strcasestr(tok, "#S")) {
            int advance_count = (parsed.count() > 1) ? parsed.position_as_int(1) : 1;
            printf("Advancing scheduler by %dms...\n", advance_count);
            for (int i = 0; i < advance_count; i++) {
              StaticHub::advanceScheduler();
            }
          }
          else if (strcasestr(tok, "#D")) {
            scheduler->printDebug(&output);
            event_manager->printDebug(&output);
          }
          
          else if (strcasestr(tok, "#MAX-WIDTH"))  {
            if (parsed.count() > 1) {
              maximum_field_print = parsed.position_as_int(1);
                if (maximum_field_print <= 0) {
                  printf("You tried to set the output width as 0. This is a bad idea. Setting the value to 64 instead.\n");
                  maximum_field_print = 64;
                }
            }
            else {
              printf("max-width is presently set to %d.\n", maximum_field_print);
            }
          }
          else {                          // Any other input, we will assume the user is looking to dump a particular test.
            parsed.implode(" ");
            sh->feedUSBBuffer(parsed.string(), parsed.length(), true);
            proc_until_finished();
          }
          parsed.clear();
      }
    }
    else {
      printHelp();
    }
    
    if (output.length() > 0) {
      printf("%s", output.string());
      output.clear();
    }
  }
  
  kill(looper_pid, SIGQUIT);

  printf("\n\n");
  return 0;
}
