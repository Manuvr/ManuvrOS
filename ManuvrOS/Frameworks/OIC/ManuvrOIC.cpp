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
  static pthread_mutex_t mutex;
  struct timespec ts;

  // These are things that are already handled by Manuvr's platform, and we
  // should either do nothing, or provide a shunt.
  void oc_random_destroy() {}
  void oc_random_init(unsigned short seed) {}
  unsigned short oc_random_rand() {
    // Down-cast our innate random fxn's output...
    // Hopefully it isn't important that the number be pseudorandom
    //   because we are ignoring the seed value.
    return (uint16_t) randomInt();
  }

  int oc_storage_config(const char *store) {
    return 0;   // Return no error.
  }

  void oc_clock_init() {}
  oc_clock_time_t oc_clock_time(void) {    return millis();           }
  unsigned long oc_clock_seconds(void) {   return (millis() / 1000);  }
  void oc_clock_wait(oc_clock_time_t t) {  sleep_millis(t);           }


  // Now we start to get into territory where we take non-trivial steps
  //   to either wrap iotivities threading-assumption, or fill in gaps
  //   that it doesn't address.
  void oc_signal_main_loop() {
    Kernel::log("oc_signal_main_loop()\n");
    #if defined(__MANUVR_LINUX)
      pthread_cond_signal(&cv);
    #else
      #error No support for unthreadedness yet.
    #endif
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


  static void* main_OIC_loop(void* args) {
    printf("main_OIC_loop()\n");
    while (!platform.nominalState()) sleep_millis(80);  // ...Rush
    Kernel::log("main_OIC_loop() finds nominal state. Commencing...\n");

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

ManuvrOIC* ManuvrOIC::INSTANCE = nullptr;


static void set_device_custom_property(void *data) {
  // TODO: There *has* to be a better way.
  setPin(14, !readPin(14));
}

void ManuvrOIC::app_init_hook() {
  Kernel::log("OIC: app_init()\n");
  oc_init_platform(IDENTITY_STRING, NULL, NULL);
  #if defined(OC_CLIENT)
    oc_add_device("/oic/d", "oic.d.phone", "ManuvrClient", "1.0", "1.0", NULL, NULL);
  #elif defined(OC_SERVER)
    oc_add_device("/oic/d", "oic.d.light", "ManuvrServer", "1.0", "1.0", set_device_custom_property, NULL);
  #endif
  if (nullptr != INSTANCE) INSTANCE->frameworkReady(true);
}

#ifdef OC_SECURITY
static void fetch_credentials() {
  printf("OIC: fetch_credentials()\n");
  oc_storage_config("./creds");
}
#endif

static char temp_uri[OIC_MAX_URI_LENGTH];
static bool light_state = false;

oc_discovery_flags_t discovery(const char *di, const char *uri,
              oc_string_array_t types, oc_interface_mask_t interfaces,
              oc_server_handle_t* server) {
  unsigned int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= OIC_MAX_URI_LENGTH)?OIC_MAX_URI_LENGTH-1:uri_len;
  printf("oc_discovery_flags_t(%s, %s)\n", di, uri);

  for (i = 0; i < oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strlen(t) == 10 && strncmp(t, "core.light", 10) == 0) {
      strncpy(temp_uri, uri, uri_len);
      temp_uri[uri_len] = '\0';
      Kernel::log("OIC: GET request\n\n");
    }
  }
  return ManuvrOIC::INSTANCE->isDiscovering() ? OC_CONTINUE_DISCOVERY : OC_STOP_DISCOVERY;
}


static void get_light(oc_request_t *request, oc_interface_mask_t interface) {
  Kernel::log("GET_light:\n");
  // TODO: Strip and re-write all use of macros to executable code.
  //       Debugging preprocessor macros is a waste of life.
  CborEncoder g_encoder, _rmap;
  CborError g_err;
  cbor_encoder_create_map(&g_encoder, &_rmap, CborIndefiniteLength);
  switch (interface) {
    case OC_IF_BASELINE:
      oc_process_baseline_interface(request->resource);
    case OC_IF_RW:
      cbor_encode_text_string(&_rmap, "state", strlen("state"));
      cbor_encode_boolean(&_rmap, light_state);
      break;
    default:
      break;
  }
  oc_send_response(request, OC_STATUS_OK);
  Kernel::log(light_state ? "Light: ON\n" : "Light: OFF\n");
}

static void put_light(oc_request_t *request, oc_interface_mask_t interface) {
  Kernel::log("PUT_light:\n");
  bool state = false;
  oc_rep_t *rep = request->request_payload;
  while(rep != NULL) {
    PRINT("key: %s ", oc_string(rep->name));
    switch(rep->type) {
      case BOOL:
        state = rep->value_boolean;
        break;
      default:
        oc_send_response(request, OC_STATUS_BAD_REQUEST);
        return;
        break;
    }
    rep = rep->next;
  }
  light_state = state;
  Kernel::log(light_state ? "value: ON\n" : "value: OFF\n");
  setPin(14, light_state);
  oc_send_response(request, OC_STATUS_CHANGED);
}

#if defined(OC_SERVER)
static void register_resources() {
  oc_resource_t *res = oc_new_resource("/light/1", 1, 0);
  oc_resource_bind_resource_type(res, "oic.r.light");
  oc_resource_bind_resource_interface(res, OC_IF_RW);
  oc_resource_set_default_interface(res, OC_IF_RW);

  #ifdef OC_SECURITY
    oc_resource_make_secure(res);
  #endif

  oc_resource_set_discoverable(res);
  oc_resource_set_periodic_observable(res, 1);
  oc_resource_set_request_handler(res, OC_GET, get_light);
  oc_resource_set_request_handler(res, OC_PUT, put_light);
  oc_add_resource(res);
}
#endif

void ManuvrOIC::issue_requests_hook() {
  Kernel::log("OIC: issue_requests()\n");
  #if defined(OC_CLIENT)
  oc_do_ip_discovery("oic.r.light", &discovery);
  #endif
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
  INSTANCE = this;
  // NOTE: This is lower-cased due-to its usage in URIs.
  setReceiverName("OIC");
  uint8_t _default_flags = 0;

  #if defined(OC_CLIENT)
    _default_flags |= OIC_FLAG_SUPPORT_CLIENT;
  #endif

  #if defined(OC_SERVER)
    _default_flags |= OIC_FLAG_SUPPORT_SERVER;
  #endif
  _er_set_flag(_default_flags, true);

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


/*******************************************************************************
* iotivity-constrained wrapper.
*******************************************************************************/

/**
* Make this device (presumably a server) discoverable or not.
*
* @param  Discoverable?.
* @return 0 on success, -1 on failure.
*/
int8_t ManuvrOIC::makeDiscoverable(bool en) {
  if (frameworkReady()) {
    #if defined(OC_SERVER)
    #endif
  }
  return -1;
}


/**
* Make this device (presumably a server) discoverable or not.
*
* @param  Discoverable?.
* @return 0 on success, -1 on failure.
*/
int8_t ManuvrOIC::discoverOthers(bool en) {
  if (frameworkReady()) {
    #if defined(OC_CLIENT)
    #endif
  }
  return -1;
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
    .init = app_init_hook,
    #ifdef OC_SECURITY
      .get_credentials = fetch_credentials,
    #endif /* OC_SECURITY */

    #if defined(OC_SERVER)
      .register_resources = register_resources
    #elif defined(OC_CLIENT)
      .requests_entry = issue_requests_hook
    #endif
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
  output->concatf("-- IoTivity status:     %s\n", (frameworkReady() ? "ready" : "stalled"));
  output->concatf("-- Discoverable:        %s\n", (isDiscoverable() ? "yes" : "no"));
  output->concatf("-- Discovering:         %s\n", (isDiscovering() ? "yes" : "no"));

  #if defined(OC_CLIENT)
    output->concatf("-- Client support:\n");
  #endif

  #if defined(OC_SERVER)
    output->concatf("-- Server support:\n");
  #endif
}


#if defined(__MANUVR_CONSOLE_SUPPORT)
void ManuvrOIC::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  char c    = *(str);

  switch (c) {
    case 'D':   // Turn discovery on and off.
    case 'd':
      #if defined(OC_SERVER)
        local_log.concatf("makeDiscoverable(%d) returns %s\n", ('D' == c ? 1:0), makeDiscoverable('D' == c));
      #elif defined(OC_CLIENT)
        local_log.concatf("discoverOthers(%d) returns %s\n", ('D' == c ? 1:0), discoverOthers('D' == c));
      #endif
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // __MANUVR_CONSOLE_SUPPORT
#endif  // __MANUVR_LINUX
#endif  // MANUVR_OPENINTERCONNECT
