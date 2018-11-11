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
#include "port/oc_connectivity.h"


  static pthread_cond_t cv;
  static pthread_mutex_t mutex;
  struct timespec ts;

  // These are things that are already handled by Manuvr's platform, and we
  // should either do nothing, or provide a shunt.
  void oc_random_destroy() {}
  void oc_random_init() {}

  unsigned int oc_random_value() {
    return randomInt();
  }

  void oc_clock_init() {}
  oc_clock_time_t oc_clock_time(void) {    return millis();           }
  unsigned long oc_clock_seconds(void) {   return (millis() / 1000);  }
  void oc_clock_wait(oc_clock_time_t t) {  sleep_millis(t);           }


  // Now we start to get into territory where we take non-trivial steps
  //   to either wrap iotivities threading-assumption, or fill in gaps
  //   that it doesn't address.
  static void signal_event_loop() {
    Kernel::log("signal_event_loop()\n");
    #if defined(__MANUVR_LINUX)
      pthread_cond_signal(&cv);
    #else
      #error No support for unthreadedness yet.
    #endif
  }

// TODO: Eventually, we will wrap this into a map cordoning-off the framework's
//       options in a map that can be re-configured and persisted independently.
  int oc_storage_config(const char *store) {
    return 0;   // Return no error.
  }


  long oc_storage_read(const char *store, uint8_t *buf, size_t size) {
    printf("oc_storage_read(%s, %u)\n", store, size);
    Argument *res = platform.getConfKey(store);
    if (res) {
      int temp_len = res->length();
      if (temp_len > 0) {
        uint8_t* temp = (uint8_t*) alloca(temp_len);
        if (0 == res->getValueAs((uint8_t)0, &temp)) {
          memcpy(buf, temp, temp_len);
          return temp_len;
        }
      }
    }
    printf("oc_storage_read() returns 0\n");
    return 0;
  }


  long oc_storage_write(const char *store, uint8_t *buf, size_t size) {
    printf("oc_storage_write(%s, %u)\n", store, size);
    //Argument *res = new Argument((void*) buf, size);
    //res->setKey(store);
    //res->reapValue(false);  // TODO: Probably wrong.
    //if (nullptr != res) {
    //  return platform.setConfKey(res);
    //}
    return 0;
  }

  /*
  * This is the worker thread that holds the iotivity-constrained main-loop.
  */
  static void* main_OIC_loop(void* args) {
    printf("main_OIC_loop()\n");
    while (!platform.nominalState()) sleep_millis(80);  // ...Rush
    Kernel::log("main_OIC_loop() finds nominal state. Commencing...\n");

    int next_event;

    while (platform.nominalState()) {
      next_event = oc_main_poll();
      pthread_mutex_lock(&mutex);
      if (0 == next_event) {
        pthread_cond_wait(&cv, &mutex);
      }
      else {
        ts.tv_sec  = (next_event / OC_CLOCK_SECOND);
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

extern "C" {

static oc_handler_t handler;

void app_init_hook() {
  oc_init_platform(FIRMWARE_NAME, NULL, NULL);
  ManuvrOIC::INSTANCE->frameworkReady(true);
  Kernel::raiseEvent(MANUVR_MSG_OIC_READY, (EventReceiver*) ManuvrOIC::INSTANCE);
}


oc_discovery_flags_t discovery(const char *di, const char *uri,
              oc_string_array_t types, oc_interface_mask_t interfaces,
              oc_server_handle_t* server, void*) {
  unsigned int i;
  ManuvrMsg* disc_ev = Kernel::returnEvent(MANUVR_MSG_OIC_DISCOVERY);

  if (server->endpoint.flags & 0x10) {  // TODO: Should use their namespace.
    disc_ev->addArg((uint8_t) 1)->setKey("secure");
  }

  int srv_len = sizeof(oc_server_handle_t);
  uint8_t* server_ep = (uint8_t*) malloc(srv_len);
  memcpy(server_ep, server, srv_len);

  Argument* ep_arg = new Argument(server_ep, srv_len);
  ep_arg->setKey("server");
  ep_arg->reapValue(true);
  disc_ev->addArg(ep_arg);

  Argument* di_arg  = disc_ev->addArg(new StringBuilder((char*) di));
  di_arg->setKey("di");
  di_arg->reapValue(true);

  Argument* uri_arg  = disc_ev->addArg(new StringBuilder((char*) uri));
  uri_arg->setKey("uri");
  uri_arg->reapValue(true);

  for (i = 0; i < oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    Argument* t_arg = disc_ev->addArg(new StringBuilder(t));
    t_arg->setKey("r_type");
    t_arg->reapValue(true);
  }
  ManuvrOIC::INSTANCE->raiseEvent(disc_ev);
  return ManuvrOIC::INSTANCE->isDiscovering() ? OC_CONTINUE_DISCOVERY : OC_STOP_DISCOVERY;
}


#if defined(OC_SERVER)
static void register_resources() {
  Kernel::raiseEvent(MANUVR_MSG_OIC_REG_RESOURCES, (EventReceiver*) ManuvrOIC::INSTANCE);
}
#endif

void ManuvrOIC::issue_requests_hook() {
  Kernel::log("OIC: issue_requests()\n");
  #if defined(OC_CLIENT)
    oc_do_ip_discovery("oic.r.light", &discovery, NULL);
  #endif
}

}  // extern "C"


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Vanilla constructor.
*/
ManuvrOIC::ManuvrOIC() : EventReceiver("OIC") {
  INSTANCE = this;
  uint8_t _default_flags = 0;

  #if defined(OC_CLIENT)
    _default_flags |= OIC_FLAG_SUPPORT_CLIENT;
  #endif

  #if defined(OC_SERVER)
    _default_flags |= OIC_FLAG_SUPPORT_SERVER;
  #endif

  _er_set_flag(_default_flags, true);
}


/**
* Constructor. Takes arguments.
*
* @param   Argument* root_config
*/
ManuvrOIC::ManuvrOIC(Argument* root_config) : ManuvrOIC() {
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
ManuvrOIC::~ManuvrOIC() {
  platform.kernel()->removeSchedule(&_discovery_ping);
  platform.kernel()->removeSchedule(&_discovery_timeout);
  #if defined(__BUILD_HAS_THREADS)
    #if defined(__MACH__) && defined(__APPLE__)
      if (_thread_id > 0) {
        _thread_id = 0;
        pthread_cancel(_thread_id);
      }
    #else
    if (_thread_id > 0) {
      _thread_id = 0;
      pthread_cancel(_thread_id);
    }
    #endif
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
    if (en ^ isDiscoverable()) {
      #if defined(OC_SERVER)
        _discovery_timeout.enableSchedule(en);
        isDiscoverable(en);
        return 0;
      #endif
    }
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
    if (en ^ isDiscovering()) {
      _discovery_ping.enableSchedule(en);
      isDiscovering(en);
      if (en) {
        _discovery_ping.fireNow();
      }
      return 0;
    }
  }
  return -1;
}


/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* The Kernel is booting, or has-booted prior to our being instanced.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrOIC::attached() {
  if (EventReceiver::attached()) {
    // Discovery runs for 120 seconds, and repeats forever by default.
    _discovery_ping.repurpose(MANUVR_MSG_OIC_DISCOVER_PING);
    _discovery_ping.incRefs();
    _discovery_ping.alterSchedule(120000, -1, false, issue_requests_hook);
    _discovery_ping.enableSchedule(false);
    platform.kernel()->addSchedule(&_discovery_ping);

    _discovery_timeout.repurpose(MANUVR_MSG_OIC_DISCOVER_OFF, (EventReceiver*) this);
    _discovery_timeout.incRefs();
    _discovery_timeout.specific_target = (EventReceiver*) this;
    _discovery_timeout.priority(1);

    // Discovery runs for 30 seconds by default.
    _discovery_timeout.alterScheduleRecurrence(0);
    _discovery_timeout.alterSchedulePeriod(30000);
    _discovery_timeout.autoClear(false);
    _discovery_timeout.enableSchedule(false);
    platform.kernel()->addSchedule(&_discovery_timeout);


    handler.init = app_init_hook;
    handler.signal_event_loop = signal_event_loop;

    #if defined(OC_SERVER)
      handler.register_resources = register_resources;
    #endif

    #if defined(OC_CLIENT)
      handler.requests_entry = issue_requests_hook;
    #endif

    int init = oc_main_init(&handler);
    printf("OIC: attached()\n");
    if (init <= 0) {
      #if defined (__BUILD_HAS_THREADS)
        if (0 == _thread_id) {
          // If we are in a threaded environment, we will want a thread if there isn't one already.
          if (createThread(&_thread_id, nullptr, main_OIC_loop, (void*) this), nullptr) {
            Kernel::log("Failed to create iotivity-constrained thread.\n");
          }
        }
      #endif
    }
    return 1;
  }
  return 0;
}


/**
* We are being configured.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrOIC::erConfigure(Argument* opts) {
  int8_t return_value = 0;
  Argument* temp = opts->retrieveArgByKey("discovery_period");
  if (temp) {
    /* By specifying a discovery period, we are also asking for discovery. */
    // TODO: Need a better way to do this.
    uint32_t val = 0;
    if ((0 == temp->getValueAs(&val)) && (0 != val)) {
      _discovery_ping.alterSchedulePeriod(val);
      _discovery_ping.enableSchedule(true);
      return_value++;
    }
  }
  temp = opts->retrieveArgByKey("discovery_timeout");
  if (temp) {
    /* How long should we ourselves be discoverable? */
    // TODO: Need a better way to do this.
    uint32_t val = 0;
    if ((0 == temp->getValueAs(&val)) && (0 != val)) {
      return_value++;
    }
  }
  return return_value;
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
int8_t ManuvrOIC::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


int8_t ManuvrOIC::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_OIC_DISCOVER_OFF:
      isDiscoverable(false);
      if (getVerbosity() > 3) local_log.concat("Discoverable window exired.\n");
      return_value++;
      break;
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
  output->concatf("-- Discoverable:        %s  (%ums window)\n",
    (isDiscoverable() ? "yes" : "no"),
    _discovery_timeout.schedulePeriod()
  );
  output->concatf("-- Discovering:         %s  (%ums period)\n",
    (isDiscovering() ? "yes" : "no"),
    _discovery_ping.schedulePeriod()
  );

  #if defined(OC_CLIENT)
    output->concatf("-- Client support:\n");
  #endif

  #if defined(OC_SERVER)
    output->concatf("-- Server support:\n");
  #endif
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void ManuvrOIC::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);
  char c    = *(str);

  switch (c) {
    case 'D':   // Turn discovery on and off.
    case 'd':
      #if defined(OC_SERVER)
        local_log.concatf("makeDiscoverable(%d) returns %d\n", ('D' == c ? 1:0), makeDiscoverable('D' == c));
      #elif defined(OC_CLIENT)
        local_log.concatf("discoverOthers(%d) returns %d\n", ('D' == c ? 1:0), discoverOthers('D' == c));
      #endif
      break;
    default:
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT
#endif  // __MANUVR_LINUX
#endif  // MANUVR_OPENINTERCONNECT
