/*
File:   ManuvrTCP.cpp
Author: J. Ian Lindsay
Date:   2015.09.17

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


This driver is designed to give Manuvr platform-abstracted TCP socket connection.
This is basically only for linux for now.

*/

#if defined(MANUVR_SUPPORT_TCPSOCKET)

#include "ManuvrTCP.h"
#include "FirmwareDefs.h"

#include <Kernel.h>

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

/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/

#if defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)
  #include <arpa/inet.h>

  // Threaded platforms will need this to compensate for a loss of ISR.
  extern void* xport_read_handler(void* active_xport);


  /*
  * Since listening for connections on this transport involves blocking, we have a
  *   thread dedicated to the task...
  */
  void* socket_listener_loop(void* active_xport) {
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

      ManuvrTCP* listening_inst = (ManuvrTCP*) active_xport;
      StringBuilder output;
      if (0 != s) {
        output.concatf("pthread_sigmask() returned an error: %d\n", s);
        Kernel::log(&output);
        return NULL;
      }
      int      cli_sock;
      struct sockaddr_in cli_addr;
      while (listening_inst->listening()) {
        unsigned int clientlen = sizeof(cli_addr);

        /* Wait for client connection */
        if ((cli_sock = accept(listening_inst->getSockID(), (struct sockaddr *) &cli_addr, &clientlen)) < 0) {
          output.concat("Failed to accept client connection.\n");
        }
        else {
          ManuvrTCP* nu_connection = new ManuvrTCP(listening_inst, cli_sock, &cli_addr);
          nu_connection->setPipeStrategy(listening_inst->getPipeStrategy());

          ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_ADVERTISE_SRVC);
          event->addArg((EventReceiver*) nu_connection);
          Kernel::staticRaiseEvent(event);

          output.concatf("TCP Client connected: %s\n", (char*) inet_ntoa(cli_addr.sin_addr));
        }
        Kernel::log(&output);

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
  // No special globals needed for this platform.
#endif


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Constructor.
*/
ManuvrTCP::ManuvrTCP(const char* addr, int port) : ManuvrSocket(addr, port, 0) {
  setReceiverName("ManuvrTCP");
  set_xport_state(MANUVR_XPORT_FLAG_STREAM_ORIENTED | MANUVR_XPORT_FLAG_HAS_MULTICAST);
}


ManuvrTCP::ManuvrTCP(const char* addr, int port, uint32_t opts) : ManuvrSocket(addr, port, opts) {
  setReceiverName("ManuvrTCP");
  set_xport_state(MANUVR_XPORT_FLAG_STREAM_ORIENTED | MANUVR_XPORT_FLAG_HAS_MULTICAST);
}


/**
* This constructor is called by a listening instance of ManuvrTCP.
*/
// TODO: This is very ugly.... Might need a better way of fractioning into new threads...
ManuvrTCP::ManuvrTCP(ManuvrTCP* listening_instance, int sock, struct sockaddr_in* nu_sockaddr) : ManuvrSocket(listening_instance->_addr, listening_instance->_port_number, 0) {
  setReceiverName("ManuvrTCP");
  set_xport_state(MANUVR_XPORT_FLAG_STREAM_ORIENTED | MANUVR_XPORT_FLAG_HAS_MULTICAST);
  _sock          = sock;
  _options       = listening_instance->_options;

  listening_instance->_connections.insert(this);  // TODO: This is starting to itch...

  for (uint16_t i = 0; i < sizeof(_sockaddr);  i++) {
    // Copy the sockaddr struct into this instance.
    *((uint8_t *) &_sockaddr + i) = *(((uint8_t*)nu_sockaddr) + i);
  }

  bootComplete();
  connected(true);  // TODO: Possibly not true....
}


/**
* Destructor
*/
ManuvrTCP::~ManuvrTCP() {
}



/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Inward toward the transport.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTCP::toCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      return (write_port(buf->string(), buf->length()) ? MEM_MGMT_RESPONSIBLE_CREATOR : MEM_MGMT_RESPONSIBLE_CALLER);

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      // TODO: Freeing the buffer?
      return (write_port(buf->string(), buf->length()) ? MEM_MGMT_RESPONSIBLE_BEARER : MEM_MGMT_RESPONSIBLE_CALLER);

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}



/*******************************************************************************
* ___________                                                  __
* \__    ___/___________    ____   ____________   ____________/  |_
*   |    |  \_  __ \__  \  /    \ /  ___/\____ \ /  _ \_  __ \   __\
*   |    |   |  | \// __ \|   |  \\___ \ |  |_> >  <_> )  | \/|  |
*   |____|   |__|  (____  /___|  /____  >|   __/ \____/|__|   |__|
*                       \/     \/     \/ |__|
* These members are particular to the transport driver and any implicit
*   protocol it might contain.
*******************************************************************************/

int8_t ManuvrTCP::connect() {
  // We're being told to act as a client.
  if (listening()) {
    Kernel::log("A TCP socket was told to connect while it is listening. Doing nothing.");
    return -1;
  }

  if (connected()) {
    Kernel::log("A TCP socket was told to connect while it already was. Doing nothing.");
    return -1;
  }

  //in_addr_t temp_addr = inet_network(_addr);

  _sockaddr.sin_family      = AF_INET;
  _sockaddr.sin_addr.s_addr = inet_addr(_addr);
  _sockaddr.sin_port        = htons(_port_number);

  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...

  /* Bind the server socket */
  if (::connect(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    StringBuilder log;
    log.concatf("Failed to connect to %s:%d.\n", _addr, _port_number);
    Kernel::log(&log);
    return -1;
  }

  initialized(true);
  connected(true);

  return 0;
}



int8_t ManuvrTCP::listen() {
  // We're being told to start listening on whatever address was provided to the constructor.
  // That means we are a server.
  if (_sock) {
    Kernel::log("A TCP socket was told to listen when it already was. Doing nothing.");
    return -1;
  }

  //in_addr_t temp_addr = inet_network(_addr);

  _sockaddr.sin_family      = AF_INET;
  _sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //_sockaddr.sin_addr.s_addr = temp_addr;
  _sockaddr.sin_port        = htons(_port_number);

  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...

  /* Bind the server socket */
  if (bind(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    Kernel::log("Failed to bind the server socket.\n");
    return -1;
  }
  /* Listen on the server socket */
  if (::listen(_sock, 4) < 0) {
    Kernel::log("Failed to listen on server socket\n");
    return -1;
  }

  initialized(true);
  createThread(&_thread_id, NULL, socket_listener_loop, (void*) this);

  listening(true);
  local_log.concatf("TCP Now listening at %s:%d.\n", _addr, _port_number);

  flushLocalLog();
  return 0;
}


int8_t ManuvrTCP::reset() {
  initialized(true);
  return 0;
}



int8_t ManuvrTCP::read_port() {
  if (connected()) {
    unsigned char *buf = (unsigned char *) alloca(256);
    int n;

    while (connected()) {
      n = read(_sock, buf, 255);
      if (n > 0) {
        bytes_received += n;
        BufferPipe::fromCounterparty(buf, n, MEM_MGMT_RESPONSIBLE_BEARER);
      }
      else {
        // Don't thrash the CPU for no reason...
        sleep_millis(20);
      }
    }
  }
  else if (getVerbosity() > 1) {
    local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
  }

  _thread_id = 0;

  flushLocalLog();
  return 0;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*
* @param  int              An identifier for a specific socket.
* @param  unsigned char*   The buffer containing the outbound data.
* @param  int              The length of data in the buffer.
* @return bool             false on error, true on success.
*/
bool ManuvrTCP::write_port(unsigned char* out, int out_len) {
  if (getSockID() == -1) {
    #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 2) {
        local_log.concatf("Unable to write to transport: %s\n", _addr);
        Kernel::log(&local_log);
      }
    #endif
    return false;
  }

  if (connected()) {
    int bytes_written = (int) write(getSockID(), out, out_len);
    bytes_sent += bytes_written;
    if (bytes_written == out_len) {
      return true;
    }
    Kernel::log("Failed to send bytes to client");
  }
  return false;
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
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrTCP::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;

  ManuvrXport::printDebug(temp);
  temp->concatf("-- _addr           %s:%d\n",  _addr, _port_number);
  temp->concatf("-- _options        0x%08x\n", _options);
  temp->concatf("-- _sock           0x%08x\n", _sock);
}


/**
* TODO: Until I do something smarter...
* We are obliged to call the ManuvrXport's version of bootComplete(), which in turn
*   will call the EventReceiver version of that fxn.
* ---J. Ian Lindsay   Thu Dec 03 03:25:48 MST 2015
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrTCP::bootComplete() {   // ?? TODO ??
  EventReceiver::bootComplete();

  // We will suffer a 300ms latency if the platform's networking stack doesn't flush
  //   its buffer in time.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_RECEIVE);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.originator      = (EventReceiver*) this;
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(300);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);
  __kernel->addSchedule(&read_abort_event);

  reset();
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
int8_t ManuvrTCP::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_XPORT_SEND:
      event->clearArgs();
      break;
    default:
      break;
  }

  return return_value;
}



int8_t ManuvrTCP::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


#endif  //MANUVR_SUPPORT_TCPSOCKET
