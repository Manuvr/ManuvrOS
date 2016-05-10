/*
File:   ManuvrUDP.cpp
Author: J. Ian Lindsay
Date:   2016.03.31

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


This class implements a crude UDP connector.

*/

#if defined(MANUVR_SUPPORT_UDP)

#include "ManuvrSocket.h"
#include <DataStructures/StringBuilder.h>

const MessageTypeDef udp_message_defs[] = {
  #if defined (__ENABLE_MSG_SEMANTICS)
  {  MANUVR_MSG_UDP_RX    , 0,  "UDP_RX",         ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_UDP_TX    , 0,  "UDP_TX",         ManuvrMsg::MSG_ARGS_NONE }, //
  #else
  {  MANUVR_MSG_UDP_RX    , 0,  "UDP_RX",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  {  MANUVR_MSG_UDP_TX    , 0,  "UDP_TX",         ManuvrMsg::MSG_ARGS_NONE, NULL }, //
  #endif
};



volatile ManuvrUDP* ManuvrUDP::INSTANCE = NULL;

#if defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)

  /*
  * Since listening for connections on this transport involves blocking, we have a
  *   thread dedicated to the task...
  */
  void* _udp_socket_listener_loop(void* active_xport) {
    if (NULL != active_xport) {
      sigset_t set;
      sigemptyset(&set);
      //sigaddset(&set, SIGIO);
      sigaddset(&set, SIGQUIT);
      sigaddset(&set, SIGHUP);
      sigaddset(&set, SIGTERM);
      sigaddset(&set, SIGVTALRM);
      sigaddset(&set, SIGINT);
      int s = pthread_sigmask(SIG_BLOCK, &set, NULL);

      ManuvrUDP* listening_inst = (ManuvrUDP*) active_xport;
      ManuvrRunnable* event = NULL;
      StringBuilder output;
      int      cli_sock;
      struct sockaddr_in cli_addr;
      while (listening_inst->listening()) {
        unsigned int clientlen = sizeof(cli_addr);

        unsigned char buf[1024];

        // Read data from UDP port. Blocks...
        int n = recvfrom(listening_inst->getSockID(), buf, 1024, 0, (struct sockaddr *) &cli_addr, &clientlen);
        if (-1 == n) {
          output.concat("Failed to read UDP packet.\n");
        }
        else if (n > 0) {
          //bytes_received += n;
          event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
          StringBuilder* nu_data = new StringBuilder(buf, n);
          event->markArgForReap(event->addArg(nu_data), true);
          listening_inst->count_rx_bytes(n);
          output.concatf("UDP read %d bytes from client ", n);
          output.concat((char*) inet_ntoa(cli_addr.sin_addr));
          output.concat("\n");

          nu_data->printDebug(&output);
          Kernel::log(&output);

          Kernel::staticRaiseEvent(event);
        }
        else {
          // Don't thrash the CPU for no reason...
          sleep_millis(20);
        }

        for (uint16_t i = 0; i < sizeof(cli_addr);  i++) {
          // Zero the sockaddr structure for next use. The new transport
          //   instance should have copied it by now.
          *((uint8_t *) &cli_addr  + i) = 0;
        }
      }
      // Close the listener...
      // TODO: Is this all we need to do?   ---J. Ian Lindsay   Fri Jan 15 11:32:12 PST 2016
      close(listening_inst->getSockID());
    }
    else {
      Kernel::log("Tried to listen with a NULL transport.");
    }

    return NULL;
  }

#else
  // Threads are unsupported here.
#endif




/**
* Constructor.
*/
ManuvrUDP::ManuvrUDP(const char* addr, int port) : ManuvrSocket(addr, port, 0) {
  set_xport_state(MANUVR_XPORT_FLAG_HAS_MULTICAST | MANUVR_XPORT_FLAG_CONNECTIONLESS);

  // TODO: Singleton due-to-feature thrust. Need to ditch the singleton...
  if (NULL == INSTANCE) {
    INSTANCE = this;
    // TODO: ...as well as decide on a strategy for THIS:
    // Inform the Kernel of the codes we will be using...
    ManuvrMsg::registerMessages(udp_message_defs, sizeof(udp_message_defs) / sizeof(MessageTypeDef));
  }
}


ManuvrUDP::ManuvrUDP(const char* addr, int port, uint32_t opts) : ManuvrSocket(addr, port, opts) {
  set_xport_state(MANUVR_XPORT_FLAG_HAS_MULTICAST | MANUVR_XPORT_FLAG_CONNECTIONLESS);

  // TODO: Singleton due-to-feature thrust. Need to ditch the singleton...
  if (NULL == INSTANCE) {
    INSTANCE = this;
    // TODO: ...as well as decide on a strategy for THIS:
    // Inform the Kernel of the codes we will be using...
    ManuvrMsg::registerMessages(udp_message_defs, sizeof(udp_message_defs) / sizeof(MessageTypeDef));
  }
}



ManuvrUDP::~ManuvrUDP() {
  __kernel->unsubscribe(this);
}



/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/
int8_t ManuvrUDP::listen() {
  // We're being told to start listening on whatever address was provided to the constructor.
  // That means we are a server.
  if (0 < _sock) {
    Kernel::log("A UDP socket was told to listen when it already was. Doing nothing.");
    return -1;
  }

  _sockaddr.sin_family      = AF_INET;
  //_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  _sockaddr.sin_addr.s_addr = inet_addr(_addr);
  _sockaddr.sin_port        = htons(_port_number);

  _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // Open the socket...
  if (-1 == _sock) {
    Kernel::log("Failed to bind the server socket.\n");
    return -1;
  }

  /* Bind the server socket */
  if (bind(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    Kernel::log("Failed to bind the server socket.\n");
    return -1;
  }

  //initialized(true);
  createThread(&_thread_id, NULL, _udp_socket_listener_loop, (void*) this);

  listening(true);
  local_log.concatf("UDP Now listening at %s:%d.\n", _addr, _port_number);

  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}



// TODO: Nonsense in UDP land. generalize? Special-case? So many (?)...
int8_t ManuvrUDP::read_port() {
  return 0;
}


int8_t ManuvrUDP::reset() {
  initialized(true);
  return 0;
}

int8_t ManuvrUDP::connect() {
  return 0;
}

bool ManuvrUDP::write_port(unsigned char* out, int out_len) {
  return false;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrUDP::write_datagram(unsigned char* out, int out_len, const char* _addr, int port, uint32_t opts) {
  struct sockaddr_in _tmp_sockaddr;
  bool return_value = false;

  _tmp_sockaddr.sin_family      = AF_INET;
  _tmp_sockaddr.sin_port        = htons(port);
  _tmp_sockaddr.sin_addr.s_addr = inet_addr(_addr);
  memset(_tmp_sockaddr.sin_zero, '\0', sizeof(_tmp_sockaddr.sin_zero));

  if (-1 != _client_sock) {
    int result = sendto(_client_sock, out, out_len, 0, (const sockaddr*) &_tmp_sockaddr, sizeof(_tmp_sockaddr));
    if (-1 < result) {
      bytes_sent += result;

      // TODO: We must receive from the socket before we can send again...
      char buffer[1024];
      buffer[0] = '\0';
      int _rec_bytes = recvfrom(_client_sock, buffer, 1024, 0, NULL, NULL);
      Kernel::log(buffer);
      return_value = true;
    }
    else {
      Kernel::log("Failed to write a UDP datagram because of sentto().\n");
    }
  }
  else {
    Kernel::log("Failed to write a UDP datagram. No client socket.\n");
  }

  return return_value;
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
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrUDP::getReceiverName() {  return "ManuvrUDP";  }


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ManuvrUDP::printDebug(StringBuilder* output) {
  ManuvrXport::printDebug(output);

  output->concatf("-- _addr           %s:%d\n",  _addr, _port_number);
  output->concatf("-- _options        0x%08x\n", _options);
  output->concatf("-- _sock           0x%08x\n", _sock);
}



/**
* There is a NULL-check performed upstream for the scheduler member. So no need
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrUDP::bootComplete() {
  EventReceiver::bootComplete();   // Call up to get scheduler ref and class init.
  // We will suffer a 300ms latency if the platform's networking stack doesn't flush
  //   its buffer in time.
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(300);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);
  read_abort_event.enableSchedule(false);
  //__kernel->addSchedule(&read_abort_event);

  _client_sock = socket(PF_INET, SOCK_DGRAM, 0);

  listen();
  return 1;
}



/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ManuvrUDP::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }

  return return_value;
}



int8_t ManuvrUDP::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    case MANUVR_MSG_XPORT_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}



void ManuvrUDP::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    case 'w':
      write_datagram((unsigned char*) "Horrid hack", strlen("Horrid hack"), "127.0.0.1", 6001);
      break;
    case 'W':
      write_datagram((unsigned char*) str, strlen((const char*)str), "127.0.0.1", 6001);
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

#endif
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}


#endif  // UDP support
