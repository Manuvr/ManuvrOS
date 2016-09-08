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

static pthread_cond_t cv;


int oc_storage_config(const char *store) {
  // We already handle storage in Manuvr.
  // Do nothing.
}

long oc_storage_read(const char *store, uint8_t *buf, size_t size) {
  Argument *res = platform.getConfKey(store);
  if (nullptr != res) {
    int temp_len = res->length();
    if (temp_len > 0) {
      uint8_t* temp = alloca(temp_len);
      if (0 == res->getValueAs(0, &temp)) {
        memcpy(buf, temp, temp_len);
        return 1;
      }
    }
  }
  return 0;
}


long oc_storage_write(const char *store, uint8_t *buf, size_t size) {
  Argument *res = new Argument((void*) buf, len);
  res->setKey(store);
  res->reapValue(false);  // TODO: Probably wrong.
  if (nullptr != res) {
    return platform.setConfKey(res);
  }
  return 0;
}


void oc_signal_main_loop(void) {  pthread_cond_signal(&cv); }

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
  return (uint16_t)randomInt();
}

void oc_random_destroy() {
  // Manuvr never tears down it's RNG... Why would you do this?
  // Do nothing.
}

void oc_network_event_handler_mutex_init(void);
void oc_network_event_handler_mutex_lock(void);
void oc_network_event_handler_mutex_unlock(void);

void oc_send_buffer(oc_message_t * message);

#ifdef OC_SECURITY
uint16_t oc_connectivity_get_dtls_port(void) {
  // TODO: Derive from preprocessor options.
  return 5684;
}
#endif /* OC_SECURITY */

int oc_connectivity_init() {
  memset(&mcast, 0, sizeof(struct sockaddr_storage));
  memset(&server, 0, sizeof(struct sockaddr_storage));

  struct sockaddr_in6 *m = (struct sockaddr_in6*)&mcast;
  m->sin6_family = AF_INET6;
  m->sin6_port = htons(COAP_PORT_UNSECURED);
  m->sin6_addr = in6addr_any;

  struct sockaddr_in6 *l = (struct sockaddr_in6*)&server;
  l->sin6_family = AF_INET6;
  l->sin6_addr = in6addr_any;
  l->sin6_port = 0;

#ifdef OC_SECURITY
  memset(&secure, 0, sizeof(struct sockaddr_storage));
  struct sockaddr_in6 *sm = (struct sockaddr_in6*)&secure;
  sm->sin6_family = AF_INET6;
  sm->sin6_port = 0;
  sm->sin6_addr = in6addr_any;
#endif /* OC_SECURITY */

  server_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  mcast_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  if(server_sock < 0 || mcast_sock < 0) {
    LOG("ERROR creating server sockets\n");
    return -1;
  }

#ifdef OC_SECURITY
  secure_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (secure_sock < 0) {
    LOG("ERROR creating secure socket\n");
    return -1;
  }
#endif /* OC_SECURITY */

  if(bind(server_sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
    LOG("ERROR binding server socket %d\n", errno);
    return -1;
  }

  struct ipv6_mreq mreq;
  memset(&mreq, 0, sizeof(mreq));
  if(inet_pton(AF_INET6, ALL_COAP_NODES_V6, (void*)&mreq.ipv6mr_multiaddr)
     != 1) {
    LOG("ERROR setting mcast addr\n");
    return -1;
  }
  mreq.ipv6mr_interface = 0;
  if(setsockopt(mcast_sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq,
    sizeof(mreq)) == -1) {
    LOG("ERROR setting mcast join option %d\n", errno);
    return -1;
  }
  int reuse = 1;
  if(setsockopt(mcast_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
    sizeof(reuse)) == -1) {
    LOG("ERROR setting reuseaddr option %d\n", errno);
    return -1;
  }
  if(bind(mcast_sock, (struct sockaddr*)&mcast, sizeof(mcast)) == -1) {
    LOG("ERROR binding mcast socket %d\n", errno);
    return -1;
  }

#ifdef OC_SECURITY
  if(setsockopt(secure_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
    sizeof(reuse)) == -1) {
    LOG("ERROR setting reuseaddr option %d\n", errno);
    return -1;
  }
  if(bind(secure_sock, (struct sockaddr*)&secure, sizeof(secure)) == -1) {
    LOG("ERROR binding smcast socket %d\n", errno);
    return -1;
  }

  socklen_t socklen = sizeof(secure);
  if (getsockname(secure_sock,
      (struct sockaddr*)&secure,
      &socklen) == -1) {
    LOG("ERROR obtaining secure socket information %d\n", errno);
    return -1;
  }

  dtls_port = ntohs(sm->sin6_port);
#endif /* OC_SECURITY */

  if (pthread_create(&event_thread, NULL, &network_event_thread, NULL)
      != 0) {
    LOG("ERROR creating network polling thread\n");
    return -1;
  }

  LOG("Successfully initialized connectivity\n");

  return 0;
}

  void oc_connectivity_shutdown() {
    close(server_sock);
    close(mcast_sock);
    #ifdef OC_SECURITY
      close(secure_sock);
    #endif /* OC_SECURITY */

    pthread_cancel(event_thread);
    Kernel::log("oc_connectivity_shutdown\n");
  }

  void oc_send_multicast_message(oc_message_t *message);

  void oc_clock_init() {
    // Manuvr deals with this.
  }

  oc_clock_time_t oc_clock_time(void) {
    return millis();
  }

  unsigned long oc_clock_seconds(void) {
    return (millis() / 1000);
  }

  void oc_clock_wait(oc_clock_time_t t) {
    sleep_millis(t);
  }
}


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


static void issue_requests() {
  printf("OIC: issue_requests()\n");
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
