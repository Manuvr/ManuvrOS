/*
File:   PlatformLinux.cpp
Author: J. Ian Lindsay
Date:   2015.11.01

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


This file forms the catch-all for linux platforms that have no support.
*/

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include <Platform/Platform.h>

#if defined(MANUVR_STORAGE)
#include "LinuxStorage.h"
#include <fcntl.h>      // Needed for integrity checks.
#include <sys/stat.h>   // Needed for integrity checks.
#endif

#include <Transports/StandardIO/StandardIO.h>
#include <XenoSession/Console/ManuvrConsole.h>


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
volatile Kernel* __kernel = nullptr;


struct itimerval _interval              = {0};
struct sigaction _signal_action_SIGALRM = {0};

static unsigned long _last_millis = 0;

char* _binary_name = nullptr;
static int   _main_pid    = 0;

bool set_linux_interval_timer() {
  _last_millis = millis();
  _interval.it_value.tv_sec      = 0;
  _interval.it_value.tv_usec     = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;
  _interval.it_interval.tv_sec   = 0;
  _interval.it_interval.tv_usec  = MANUVR_PLATFORM_TIMER_PERIOD_MS * 1000;

  int err = setitimer(ITIMER_VIRTUAL, &_interval, nullptr);
  if (err) {
    Kernel::log("Failed to enable interval timer.");
  }
  return (0 == err);
}

bool unset_linux_interval_timer() {
  // TODO: We ultimately need to be retaining the values in the struct, as well as
  //   the current system time to calculate the delta in case we get re-enabled.
  _interval.it_value.tv_sec      = 0;
  _interval.it_value.tv_usec     = 0;
  _interval.it_interval.tv_sec   = 0;
  _interval.it_interval.tv_usec  = 0;
  return true;
}


void sig_handler(int signo) {
  switch (signo) {
    case SIGINT:
      Kernel::log("Received a SIGINT signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGKILL:
      Kernel::log("Received a SIGKILL signal. Something bad must have happened. Exiting hard....");
      platform.seppuku();
      break;
    case SIGTERM:
      Kernel::log("Received a SIGTERM signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGQUIT:
      Kernel::log("Received a SIGQUIT signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGHUP:
      Kernel::log("Received a SIGHUP signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGSTOP:
      Kernel::log("Received a SIGSTOP signal. Closing up shop...");
      platform.seppuku();
      break;
    case SIGUSR1:
      break;
    case SIGUSR2:
      break;
    default:
      #ifdef __MANUVR_DEBUG
      {
        StringBuilder _log;
        _log.concatf("Unhandled signal: %d", signo);
        Kernel::log(&_log);
      }
      #endif
      break;
  }

  // Echo whatever signals we receive to the child proc (if we are the parent).
  //if ((looper_pid > 0) && (signo != SIGALRM) && (signo != SIGUSR2)) {
  //  kill(looper_pid, signo);
  //}
}

void linux_timer_handler(int sig_num) {
  unsigned long _this_millis = millis();
  ((Kernel*)__kernel)->advanceScheduler(_this_millis - _last_millis);
  _last_millis = _this_millis;
}


// The parent process should call this function to set the callback address to its signal handlers.
//     Returns 1 on success, 0 on failure.
// TODO: Convert all other signals over to sigaction().
int initSigHandlers() {
  int return_value    = 1;
  // Try to open a binding to listen for signals from the OS...
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGINT to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGQUIT to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGHUP, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGHUP to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGTERM to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGUSR1 to the signal system. Failing...");
    return_value = 0;
  }
  if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
    Kernel::log("Failed to bind SIGUSR2 to the signal system. Failing...");
    return_value = 0;
  }

  _signal_action_SIGALRM.sa_handler   = &linux_timer_handler;
  if (sigaction(SIGVTALRM, &_signal_action_SIGALRM, nullptr)) {
    Kernel::log("Failed to bind to SIGVTALRM.");
    return_value = 0;
  }

  return return_value;
}



/*
* Parse through all the command line arguments and flags.
* Return an Argument object.
*/
Argument* parseFromArgCV(int argc, const char* argv[]) {
  Argument*   _args    = new Argument(argv[0]);
  _args->setKey("binary_name");
  const char* last_key = nullptr;
  for (int i = 1; i < argc; i++) {
    if ((strcasestr(argv[i], "--version")) || ((argv[i][0] == '-') && (argv[i][1] == 'v'))) {
      // Print the version and quit.
      printf("%s v%s\n\n", argv[0], VERSION_STRING);
      exit(0);
    }

    else if (((argv[i][0] == '-') && (argv[i][1] == '-'))) {
      if (last_key) {
        _args->link(new Argument((uint8_t) 1))->setKey(last_key);
      }
      last_key = (const char*) &argv[i][2];
    }
    else {
      Argument* nu = new Argument(argv[i]);
      if (last_key) {
        nu->setKey(last_key);
        last_key = nullptr;
      }
      _args->link(nu);
    }
  }

  if (last_key) {
    _args->link(new Argument((uint8_t) 1))->setKey(last_key);
  }
  return _args;
}


#if defined(__BUILD_HAS_DIGEST)
/*
* Function takes a path and a buffer as arguments. The binary is hashed and the ASCII representation is
*   placed in the buffer. The number of bytes read is returned on success. 0 is returned on failure.
*/
static int hashFileByPath(char* path, uint8_t* h_buf) {
  int8_t return_value = -1;
  StringBuilder log;
  if (nullptr != path) {
    struct stat st;
    if(-1 != stat(path, &st)) {
      size_t self_size = st.st_size;
      int digest_size = 32;
      StringBuilder buf;

      int fd = open(path, O_RDONLY);
      if (fd >= 0) {
        int r_buf_len = 2 << 15;
        uint8_t* buffer = (uint8_t*) alloca(r_buf_len);
        int r_len = read(fd, buffer, r_buf_len);
        while (0 < r_len) {
          buf.concat(buffer, r_len);
          r_len = read(fd, buffer, r_buf_len);
        }
        close(fd);

        uint8_t* self_mass = buf.string();
        log.concatf("%s is %ld bytes.\n", path, self_size);

        int ret = wrapped_hash(self_mass, self_size, h_buf, Hashes::SHA256);
        if (0 == ret) {
          return_value = digest_size;
        }
        else {
          log.concat("Failed to run the hash on the input path.\n");
        }
      }
      else {
        // Failure to read file.
        log.concat("Failed to read file.\n");
      }
    }
    else {
      // Failure to stat file.
      log.concat("Failed to stat file.\n");
    }
  }
  //Kernel::log(&log);
  printf((const char*)log.string());
  return return_value;
}
#endif  // __HAS_CRYPT_WRAPPER


/*******************************************************************************
* Watchdog                                                                     *
*******************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;


/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
volatile uint32_t randomness_pool[PLATFORM_RNG_CARRY_CAPACITY];
volatile unsigned int _random_pool_r_ptr = 0;
volatile unsigned int _random_pool_w_ptr = 0;

long unsigned int rng_thread_id = 0;

/**
* This is a thread to keep the randomness pool flush.
*/
static void* dev_urandom_reader(void*) {
  FILE* ur_file = fopen("/dev/urandom", "rb");
  unsigned int rng_level    = 0;
  unsigned int needed_count = 0;

  if (ur_file) {
    while (platform.platformState() <= MANUVR_INIT_STATE_NOMINAL) {
      rng_level = _random_pool_w_ptr - _random_pool_r_ptr;
      if (rng_level == PLATFORM_RNG_CARRY_CAPACITY) {
        // We have filled our entropy pool. Sleep.
        // TODO: Implement wakeThread() and this can go way higher.
        sleep_millis(10);
      }
      else {
        // We continue feeding the entropy pool as demanded until the platform
        //   leaves its nominal state. Don't write past the wrap-point.
        unsigned int _w_ptr = _random_pool_w_ptr % PLATFORM_RNG_CARRY_CAPACITY;
        unsigned int _delta_to_wrap = PLATFORM_RNG_CARRY_CAPACITY - _w_ptr;
        unsigned int _delta_to_fill = PLATFORM_RNG_CARRY_CAPACITY - rng_level;

        needed_count = ((_delta_to_fill < _delta_to_wrap) ? _delta_to_fill : _delta_to_wrap);
        size_t ret = fread((void*)&randomness_pool[_w_ptr], sizeof(uint32_t), needed_count, ur_file);
        //printf("Read %d uint32's from /dev/urandom.\n", ret);
        if((ret > 0) && !ferror(ur_file)) {
          _random_pool_w_ptr += ret;
        }
        else {
          fclose(ur_file);
          printf("Failed to read /dev/urandom.\n");
          return NULL;
        }
      }
    }
    fclose(ur_file);
  }
  else {
    printf("Failed to open /dev/urandom.\n");
  }
  printf("Exiting random thread.....\n");
  return NULL;
}



/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomInt() {
  // Preferably, we'd shunt to a PRNG at this point. For now we block.
  while (_random_pool_w_ptr <= _random_pool_r_ptr) {
  }
  return randomness_pool[_random_pool_r_ptr++ % PLATFORM_RNG_CARRY_CAPACITY];
}


/**
* Init the RNG. Short and sweet.
*/
void LinuxPlatform::init_rng() {
  srand(time(nullptr));          // Seed the PRNG...
  if (createThread(&rng_thread_id, nullptr, dev_urandom_reader, nullptr)) {
    printf("Failed to create RNG thread.\n");
    exit(-1);
  }
  int t_out = 30;
  while ((_random_pool_w_ptr < PLATFORM_RNG_CARRY_CAPACITY) && (t_out > 0)) {
    sleep_millis(20);
    t_out--;
  }
  if (0 == t_out) {
    printf("Failed to fill the RNG pool.\n");
    exit(-1);
  }
  _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);
}

/*******************************************************************************
*  ___   _           _      ___
* (  _`\(_ )        ( )_  /'___)
* | |_) )| |    _ _ | ,_)| (__   _    _ __   ___ ___
* | ,__/'| |  /'_` )| |  | ,__)/'_`\ ( '__)/' _ ` _ `\
* | |    | | ( (_| || |_ | |  ( (_) )| |   | ( ) ( ) |
* (_)   (___)`\__,_)`\__)(_)  `\___/'(_)   (_) (_) (_)
* These are overrides and additions to the platform class.
*******************************************************************************/
LinuxPlatform::~LinuxPlatform() {
  _close_open_threads();
}

void LinuxPlatform::printDebug(StringBuilder* output) {
  output->concatf(
    "==< %s Linux [%s] >=============================\n",
    _board_name,
    getPlatformStateStr(platformState())
  );
  ManuvrPlatform::printDebug(output);
  #if defined(__HAS_CRYPT_WRAPPER)
    output->concatf("-- Binary hash         %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
      _binary_hash[0],  _binary_hash[1],  _binary_hash[2],  _binary_hash[3],
      _binary_hash[4],  _binary_hash[5],  _binary_hash[6],  _binary_hash[7],
      _binary_hash[8],  _binary_hash[9],  _binary_hash[10], _binary_hash[11],
      _binary_hash[12], _binary_hash[13], _binary_hash[14], _binary_hash[15]
    );
    output->concatf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
      _binary_hash[16], _binary_hash[17], _binary_hash[18], _binary_hash[19],
      _binary_hash[20], _binary_hash[21], _binary_hash[22], _binary_hash[23],
      _binary_hash[24], _binary_hash[25], _binary_hash[26], _binary_hash[27],
      _binary_hash[28], _binary_hash[29], _binary_hash[30], _binary_hash[31]
    );
    randomArt(_binary_hash, 32, "SHA256", output);
  #endif
}



/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/
/*
*/
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s) {
  return false;
}


/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool setTimeAndDateStr(char* nu_date_time) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t currentTimestamp(void) {
  uint32_t return_value = (uint32_t) time(0);
  return return_value;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != nullptr) {
    time_t t = time(0);
    struct tm  tstruct;
    char       buf[64];
    tstruct = *localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    target->concat(buf);
  }
}


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}


/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/
#if defined(MANUVR_STORAGE)
  // Called during boot to load configuration.
  int8_t LinuxPlatform::_load_config() {
    if (_storage_device) {
      if (_storage_device->isMounted()) {
        uint8_t raw[2048];
        int len = _storage_device->persistentRead(NULL, raw, 2048, 0);
        _config = Argument::decodeFromCBOR(raw, len);
        if (_config) {
          return 0;
        }
      }
    }
    return -1;
  }
#endif


/*******************************************************************************
* Interrupt-masking                                                            *
*******************************************************************************/

// Ze interrupts! Zhey do nuhsing!
// TODO: Perhaps raise the nice value?
// At minimum, turn off the periodic timer, since this is what would happen on
//   other platforms.
void globalIRQEnable() {
  // TODO: Need to stack the time remaining.
  //set_linux_interval_timer();
}

void globalIRQDisable() {
  // TODO: Need to unstack the time remaining and fire any schedules.
  //unset_linux_interval_timer();
}



/*******************************************************************************
* Process control                                                              *
*******************************************************************************/
void LinuxPlatform::_close_open_threads() {
  _set_init_state(MANUVR_INIT_STATE_HALTED);
  if (rng_thread_id) {
    if (0 == deleteThread(&rng_thread_id)) {
    }
  }
  sleep_millis(100);
}


/*
* Terminate this running process, along with any children it may have forked() off.
*/
void LinuxPlatform::seppuku() {
  #if defined(MANUVR_STORAGE)
    if (_self && _self->isDirty()) {
      // Save the dirty identity.
      // TODO: int8_t persistentWrite(const char*, uint8_t*, int, uint16_t);
    }
  #endif
  _close_open_threads();
  // Whatever the kernel cared to clean up, it better have done so by this point,
  //   as no other platforms return from this function.
  printf("\nseppuku(): About to exit().\n\n");
  exit(0);
}


/*
* On linux, we take this to mean: scheule a program restart with the OS,
*   and then terminate this one.
*/
void LinuxPlatform::jumpToBootloader() {
  // TODO: Pull binary from a location of firmware's choice.
  // TODO: Install firmware after validation.
  // TODO: Schedule a program restart.
  _close_open_threads();
  printf("\njumpToBootloader(): About to exit().\n\n");
  exit(0);
}


/*******************************************************************************
* Underlying system control.                                                   *
*******************************************************************************/

void LinuxPlatform::hardwareShutdown() {
  // TODO: Actually shutdown the system.
  _close_open_threads();
  printf("\nhardwareShutdown(): About to exit().\n\n");
  exit(0);
}


void LinuxPlatform::reboot() {
  // TODO: Actually reboot the system.
  _close_open_threads();
  printf("\nreboot(): About to exit().\n\n");
  exit(0);
}


/*******************************************************************************
* INTERNAL INTEGRITY-CHECKS                                                    *
*******************************************************************************/
#if defined(__HAS_CRYPT_WRAPPER)
int8_t LinuxPlatform::internal_integrity_check(uint8_t* test_buf, int test_len) {
  if ((nullptr != test_buf) && (0 < test_len)) {
    for (int i = 0; i < test_len; i++) {
      if (*(test_buf+i) != _binary_hash[i]) {
        printf("Hashing %s yields a different value than expected. Exiting...\n", _binary_name);
        return -1;
      }
    }
    return 0;
  }
  else {
    // We have no idea what to expect. First boot?
  }
  return -1;
}


/**
* Look in the mirror and find our executable's full path.
* Then, read the full contents and hash them. Store the result in
*   local private member _binary_hash.
*
* @return 0 on success.
*/
int8_t LinuxPlatform::hash_self() {
  char *exe_path = (char *) alloca(300);   // 300 bytes ought to be enough for our path info...
  memset(exe_path, 0x00, 300);
  int exe_path_len = readlink("/proc/self/exe", exe_path, 300);
  if (!(exe_path_len > 0)) {
    printf("%s was unable to read its own path from /proc/self/exe. You may be running it on an unsupported operating system, or be running an old kernel. Please discover the cause and retry. Exiting...\n", _binary_name);
    return -1;
  }
  printf("This binary's path is %s\n", exe_path);
  memset(_binary_hash, 0x00, 32);
  int ret = hashFileByPath(exe_path, _binary_hash);
  if (0 < ret) {
    return 0;
  }
  else {
    printf("Failed to hash file: %s\n", exe_path);
  }
  return -1;
}
#endif  // __HAS_CRYPT_WRAPPER


/******************************************************************************
* Platform initialization.                                                    *
******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_INNATE_DATETIME | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t LinuxPlatform::platformPreInit(Argument* root_config) {
  ManuvrPlatform::platformPreInit(root_config);
  // Used for timer and signal callbacks.
  __kernel = (volatile Kernel*) &_kernel;

  uint32_t default_flags = DEFAULT_PLATFORM_FLAGS;
  _main_pid = getpid();  // Our PID.
  Argument* temp = nullptr;
  if (root_config) {
    // If 'binary_name' is absent, nothing will be done to _binary_name.
    root_config->getValueAs("binary_name", &_binary_name);
  }
  _alter_flags(true, default_flags);

  init_rng();
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);

  #if defined(MANUVR_STORAGE)
    LinuxStorage* sd = new LinuxStorage(root_config);
    _storage_device = (Storage*) sd;
    _kernel.subscribe((EventReceiver*) sd);
  #endif

  initSigHandlers();

  #if defined(__HAS_CRYPT_WRAPPER)
  hash_self();
  //internal_integrity_check(nullptr, 0);
  #endif

  #if defined(MANUVR_CONSOLE_SUPPORT)
    if (root_config && root_config->retrieveArgByKey("console")) {
      StandardIO* _console_xport = new StandardIO();
      ManuvrConsole* _console = new ManuvrConsole((BufferPipe*) _console_xport);
      _kernel.subscribe((EventReceiver*) _console);
      _kernel.subscribe((EventReceiver*) _console_xport);
    }
  #endif

  return 0;
}


/*
* Called before kernel instantiation. So do the minimum required to ensure
*   internal system sanity.
*/
int8_t LinuxPlatform::platformPostInit() {
  set_linux_interval_timer();
  return 0;
}
