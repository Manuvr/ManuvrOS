/*
File:   ManuvrOIC.cpp
Author: J. Ian Lindsay
Date:   2016.08.04

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


This is an attempt to wrap iotivity-constrained.
As of this date (2016-09-07), building against it has not provided sufficient
  functionality, and attempts to integrate it via Manuvr's Platform abstraction
  failed as well (but might in principle succeed).

Our goal here will be to provide all the platform-mandatory hooks required
  by iotivity, and implement them as shunts into our platform.

Much of this is copy-pasta from iotivity-constrained demo code.
See commentary in Platform/Linux.cpp
*/

#if defined(MANUVR_OPENINTERCONNECT)

// TODO: Until some platform gripes are resolved, this cannot be avoided.
#if defined(__MANUVR_LINUX)

#include "ManuvrOIC.h"
#include <Platform/Platform.h>

/*
* These are C functions that we must provide to iotivity-constrained
* for the sake of giving it a platform.
* Much of this was lifted from iotivity-constrained demo code until
*   the interface with that package can be formalized. Right now, I am
*   working by braile.            ---J. Ian Lindsay  2016.09.07
*/
extern "C" {
#include "oc_api.h"
#include "port/oc_signal_main_loop.h"
#include "port/oc_connectivity.h"


static pthread_cond_t cv;


int oc_storage_config(const char *store) {
  printf("oc_storage_config(%s)\n", store);
  // We already handle storage in Manuvr.
  // Do nothing.
  return 0;
}

long oc_storage_read(const char *store, uint8_t *buf, size_t size) {
  printf("oc_storage_read(%s, %u)\n", store, size);
  Argument *res = platform.getConfKey(store);
  if (nullptr != res) {
    int temp_len = res->length();
    if (temp_len > 0) {
      uint8_t* temp = (uint8_t*) alloca(temp_len);
      if (0 == res->getValueAs((uint8_t)0, &temp)) {
        memcpy(buf, temp, temp_len);
        return 1;
      }
    }
  }
  return 0;
}


long oc_storage_write(const char *store, uint8_t *buf, size_t size) {
  printf("oc_storage_write(%s, %u)\n", store, size);
  Argument *res = new Argument((void*) buf, size);
  res->setKey(store);
  res->reapValue(false);  // TODO: Probably wrong.
  if (nullptr != res) {
    return platform.setConfKey(res);
  }
  return 0;
}


void oc_signal_main_loop() {
  Kernel::log("oc_signal_main_loop()\n");
  pthread_cond_signal(&cv);
}


void oc_random_init(unsigned short seed) {
  // Manuvr has already handled the RNG. And we will not re-seed for
  // IoTivity's benefit. A TRNG would likely ignore this value anyhow.
  // Do nothing.
}

/*
 * Calculate a pseudo random number between 0 and 65535.
 *
 * \return A pseudo-random number between 0 and 65535.
 */
unsigned short oc_random_rand() {
  // Down-cast our innate random fxn's output...
  return (uint16_t)randomInt();
}

void oc_random_destroy() {
  // Manuvr never tears down it's RNG.
  // Do nothing.
}

//void oc_send_buffer(oc_message_t * message);

#ifdef OC_SECURITY
#endif /* OC_SECURITY */


  void oc_clock_init() {
    // Manuvr deals with this.
  }

  // super-basic stuff that should be inlined.
  oc_clock_time_t oc_clock_time(void) {    return millis();           }
  unsigned long oc_clock_seconds(void) {   return (millis() / 1000);  }
  void oc_clock_wait(oc_clock_time_t t) {  sleep_millis(t);           }


  pthread_mutex_t mutex;
  struct timespec ts;

  static void* main_OIC_loop(void* args) {
    printf("main_OIC_loop()\n");
    while (!platform.nominalState()) sleep_millis(80);  // ...Rush
    printf("main_OIC_loop() finds nominal state. Commencing...\n");

    int next_event;

    while (platform.nominalState()) {
      next_event = oc_main_poll();
      pthread_mutex_lock(&mutex);
      if (next_event == 0) {
        pthread_cond_wait(&cv, &mutex);
      }
      else {
        ts.tv_sec = (next_event / OC_CLOCK_SECOND);
        ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
        pthread_cond_timedwait(&cv, &mutex, &ts);
      }
      pthread_mutex_unlock(&mutex);
    }
    return nullptr;
  }

}  // extern "C"


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
static void app_init() {
  printf("OIC: app_init()\n");
  oc_init_platform("Apple", NULL, NULL);
  oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "1.0", "1.0", NULL, NULL);
}

#ifdef OC_SECURITY
static void fetch_credentials() {
  printf("OIC: fetch_credentials()\n");
  oc_storage_config("./creds");
}
#endif

#define MAX_URI_LENGTH  128
static char temp_uri[MAX_URI_LENGTH];

oc_discovery_flags_t discovery(const char *di, const char *uri,
              oc_string_array_t types, oc_interface_mask_t interfaces,
              oc_server_handle_t* server) {
  unsigned int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH)?MAX_URI_LENGTH-1:uri_len;
  printf("oc_discovery_flags_t(%s, %s)\n", di, uri);

  for (i = 0; i < oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strlen(t) == 10 && strncmp(t, "core.light", 10) == 0) {
      strncpy(temp_uri, uri, uri_len);
      temp_uri[uri_len] = '\0';
      Kernel::log("OIC: GET request\n\n");
      //return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}


static void issue_requests() {
  Kernel::log("OIC: issue_requests()\n");
  oc_do_ip_discovery("oic.r.light", &discovery);
}

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   BufferPipe* All sessions must have one (and only one) transport.
*/
ManuvrOIC::ManuvrOIC() {
  // NOTE: This is lower-cased due-to its usage in URIs.
  setReceiverName("oic");

  // We will have these, at minimum.
  _uri_map["/oic/p"]   = NULL;
  _uri_map["/oic/d"]   = NULL;
  _uri_map["/oic/res"] = NULL;
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
ManuvrOIC::~ManuvrOIC() {
  #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
    if (_thread_id > 0) {
      _thread_id = 0;
      pthread_cancel(_thread_id);
    }
  #endif
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀
*
* These are overrides from EventReceiver interface...
****************************************************************************************************/
/**
* The Kernel is booting, or has-booted prior to our being instanced.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrOIC::bootComplete() {
  EventReceiver::bootComplete();
  oc_handler_t handler = {
    .init = app_init,
    #ifdef OC_SECURITY
      .get_credentials = fetch_credentials,
    #endif /* OC_SECURITY */
    .requests_entry = issue_requests
  };

  int init = oc_main_init(&handler);
  if (init <= 0) {
    #if defined (__MANUVR_FREERTOS) | defined (__MANUVR_LINUX)
      if (0 == _thread_id) {
        // If we are in a threaded environment, we will want a thread if there isn't one already.
        if (createThread(&_thread_id, nullptr, main_OIC_loop, (void*) this)) {
          Kernel::log("Failed to create iotivity-constrained thread.\n");
        }
      }
    #endif
  }
  return 1;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ManuvrOIC::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


int8_t ManuvrOIC::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrOIC::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
}


#if defined(__MANUVR_CONSOLE_SUPPORT)
void ManuvrOIC::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif  // __MANUVR_CONSOLE_SUPPORT
#endif  // __MANUVR_LINUX
#endif  // MANUVR_OPENINTERCONNECT
