/*
File:   PlatformRaspi.cpp
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


This file is meant to contain a set of common functions that are typically platform-dependent.
  The goal is to make a class instance that is pre-processor-selectable for instantiation by the
  kernel, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)


Bits and pieces of low-level GPIO access and discovery code was taken from this library:
http://abyz.co.uk/rpi/pigpio/
*/


#include <Platform/Platform.h>
#include <Platform/Targets//Raspi/Raspi.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>

/* GPIO access macros... */
#define PI_BANK (pin >> 5)
#define PI_BIT  (1 << (pin & 0x1F))

#define GPSET0 7
#define GPSET1 8
#define GPCLR0 10
#define GPCLR1 11
#define GPLEV0 13
#define GPLEV1 14


/*******************************************************************************
* The code under this block is special on this platform,                       *
*******************************************************************************/
static volatile unsigned rev = 0;
static volatile uint32_t piModel      = 0;
static volatile uint32_t piPeriphBase = 0;
static volatile uint32_t piBusAddr    = 0;

#define GPIO_LEN  0xB4
#define SYST_LEN  0x1C

static volatile uint32_t  *gpioReg = (volatile uint32_t*) MAP_FAILED;
static volatile uint32_t  *systReg = (volatile uint32_t*) MAP_FAILED;



/**
 * Import an absolute register address so we can access it as-if we were on
 *   a microcontroller...
 *
 * Taken from...
 * http://abyz.co.uk/rpi/pigpio/
 */
static uint32_t* initMapMem(int fd, uint32_t addr, uint32_t len) {
  return (uint32_t *) mmap(0, len,
    PROT_READ|PROT_WRITE|PROT_EXEC,
    MAP_SHARED|MAP_LOCKED,
    fd, addr);
}


/**
 * Makes a guess at the version of the RasPi we are running on.
 *
 * Taken from...
 * http://abyz.co.uk/rpi/pigpio/
 * ...and adapted to fit this file.
 */
unsigned gpioHardwareRevision() {
  if (rev) return rev;
  const char MODEL_FILE[] = "/proc/device-tree/model";

  FILE * filp = fopen(MODEL_FILE, "r");
  if (nullptr != filp) {
    char buf[512];
    char term;
    int chars = 4; /* number of chars in revision string */

    while (fgets(buf, sizeof(buf), filp) != nullptr) {
      if (0 == piModel) {
        if (strstr (buf, "Raspberry Pi 1") != nullptr) {
          piModel = 1;
          chars = 4;
          piPeriphBase = 0x20000000;
          piBusAddr = 0x40000000;
          printf("Found a Raspberry Pi v1.\n");
        }
        else if (strstr (buf, "Raspberry Pi 2 Model B") != nullptr) {
          piModel = 2;
          chars = 6;
          piPeriphBase = 0x3F000000;
          piBusAddr = 0xC0000000;
          printf("Found a Raspberry Pi v2.\n");
        }
        else if (strstr (buf, "Raspberry Pi 3 Model B Plus") != nullptr) {
          piModel = 3;
          chars = 6;
          // TODO: Not likely correct...
          piPeriphBase = 0x7E000000;
          piBusAddr = 0xC0000000;
          printf("Found a Raspberry Pi v3 B+.\n");
        }
      }
      if (!strncasecmp("revision", buf, 8)) {
        if (sscanf(buf+strlen(buf)-(chars+1), "%x%c", &rev, &term) == 2) {
          if (term != '\n') {
            rev = 0;
          }
        }
      }
    }
    fclose(filp);
  }
  else {
    printf("Failed to open %s\n", MODEL_FILE);
  }
  return rev;
}


/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
int gpioSetup() {
  /* sets piModel, needed for peripherals address */
  if (0 < gpioHardwareRevision()) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC) ;
    if (fd < 0) {
      printf("Cannot access /dev/mem. Permissions?.\n");
      exit(-1);
    }

    gpioReg  = initMapMem(fd, (piPeriphBase + 0x200000),  GPIO_LEN);
    systReg  = initMapMem(fd, (piPeriphBase + 0x003000),  SYST_LEN);

    close(fd);

    if ((gpioReg == MAP_FAILED) || (systReg == MAP_FAILED)) {
      printf("mmap failed. No GPIO functions available.\n");
      exit(-1);
    }
  }
  else {
    printf("Could not determine raspi hardware revision.\n");
    exit(-1);
  }
  return 0;
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

Raspi::Raspi() : LinuxPlatform("Raspi") {
  gpioSetup();   // We must do this first if we want micros() to work.
}


void Raspi::printDebug(StringBuilder* output) {
  LinuxPlatform::printDebug(output);
  output->concatf("-- Raspi v%u\n", piModel);
  output->concatf("   piPeriphBase        %lu\n", piPeriphBase);
  output->concatf("   piBusAddr           %lu\n", piBusAddr);
}

/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/

/*
* Not provided elsewhere on a linux platform.
* Returns the number of microseconds after system boot. Wraps around
*   after 1 hour 11 minutes 35 seconds.
*
* Taken from:
* http://abyz.co.uk/rpi/pigpio/
*/
unsigned long micros() {
  return systReg[1];
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int platformSerialNumberSize() {
  return 8;
}


/**
* Writes the serial number to the indicated buffer.
* Thanks, Narishma!
* https://www.raspberrypi.org/forums/viewtopic.php?f=31&t=18936
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  long serial = 0;
  FILE *f = fopen("/proc/device-tree/system/linux,serial", "r");
  if (f) {
    char line[256];
    if (fgets(line, sizeof(long), f)) {
      serial = memcpy(&serial, line, sizeof(long));
    }
  }
  fclose(f);
  return serial;
}



/*******************************************************************************
* GPIO and change-notice                                                       *
*******************************************************************************/

int8_t pinMode(uint8_t pin, GPIOMode mode) {
  if (piModel) {
    int reg   = pin / 10;
    int shift = (pin % 10) * 3;

    gpioReg[reg] = (gpioReg[reg] & ~(7<<shift));

    switch (mode) {
      case GPIOMode::INPUT:
        break;
      case GPIOMode::INPUT_PULLUP:
        break;
      case GPIOMode::OUTPUT:
        gpioReg[reg] = (gpioReg[reg] | (1<<shift));
        break;
      default:
        break;
    }
    return 0;
  }
  return -1;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrMsg* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FxnPointer fxn) {
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  if (piModel) {
    *(gpioReg + (val ? GPSET0 : GPCLR0)) = 1 << pin;
    return 0;
  }
  return -1;
}


int8_t readPin(uint8_t pin) {
  if (piModel) {
    return (((*(gpioReg + GPLEV0 + PI_BANK) & PI_BIT) != 0) ? 1 : 0);
  }
  return -1;
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return -1;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}


/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t Raspi::platformPreInit(Argument* root_config) {
  LinuxPlatform::platformPreInit(root_config);
  _board_name = "Raspi";
  _alter_flags(true,  MANUVR_PLAT_FLAG_SERIALED);

  // These things are not true on the raspi.
  _alter_flags(false, MANUVR_PLAT_FLAG_INNATE_DATETIME);
  _alter_flags(false, MANUVR_PLAT_FLAG_RTC_READY);

  return 0;
}


/*
* Called before kernel instantiation. So do the minimum required to ensure
*   internal system sanity.
*/
int8_t Raspi::platformPostInit() {
  return LinuxPlatform::platformPostInit();
}
