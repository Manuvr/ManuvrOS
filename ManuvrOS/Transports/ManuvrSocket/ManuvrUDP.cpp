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

#include <DataStructures/StringBuilder.h>
#include "ManuvrUDP.h"


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

      if (0 == s) {
        while (listening_inst->listening()) {
          // Read data from UDP port. Blocks...
          if (0 > listening_inst->read_port()) {
            // Don't thrash the CPU for no reason...
            sleep_millis(20);
          }
        }
      }
      else {
        Kernel::log("Failed to thread the UDP listener loop.\n");
      }

      // Close the listener...
      listening_inst->disconnect();
    }
    else {
      Kernel::log("Tried to listen with a NULL transport.");
    }

    return NULL;
  }
#else
  // Threads are unsupported here.
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
*
* @param  addr  An IPv4 address in dotted quad notation.
* @param  port  A 16-bit port number.
*/
ManuvrUDP::ManuvrUDP(const char* addr, int port) : ManuvrSocket(addr, port, 0) {
  setReceiverName("ManuvrUDP");
  set_xport_state(MANUVR_XPORT_FLAG_HAS_MULTICAST | MANUVR_XPORT_FLAG_CONNECTIONLESS);
  _bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED, true);

  // Per RFC1122: Minimum reassembly buffer is 576 bytes of effective MTU.
  _xport_mtu = 576;
}


/**
* Constructor.
*
* @param  addr  An IPv4 address in dotted quad notation.
* @param  port  A 16-bit port number.
* @param  opts  An options mask to pass to the underlying socket implentation.
*/
ManuvrUDP::ManuvrUDP(const char* addr, int port, uint32_t opts) : ManuvrSocket(addr, port, opts) {
  setReceiverName("ManuvrUDP");
  set_xport_state(MANUVR_XPORT_FLAG_HAS_MULTICAST | MANUVR_XPORT_FLAG_CONNECTIONLESS);
  _bp_set_flag(BPIPE_FLAG_PIPE_PACKETIZED, true);

  // Per RFC1122: Minimum reassembly buffer is 576 bytes of effective MTU.
  _xport_mtu = 576;
}


/**
* Destructor.
* Clean up any outstanding exchanges.
* The UDPPipe classes will be informed of our demise and take their own measures
*   as appropriate.
*/
ManuvrUDP::~ManuvrUDP() {
	std::map<uint16_t, UDPPipe*>::iterator it;
	for (it = _open_replies.begin(); it != _open_replies.end(); it++) {
    delete it->second;
	}
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
int8_t ManuvrUDP::toCounterparty(StringBuilder* buf, int8_t mm) {
//  switch (mm) {
//    case MEM_MGMT_RESPONSIBLE_CALLER:
//      // NOTE: No break. This might be construed as a way of saying CREATOR.
//    case MEM_MGMT_RESPONSIBLE_CREATOR:
//      /* The system that allocated this buffer either...
//          a) Did so with the intention that it never be free'd, or...
//          b) Has a means of discovering when it is safe to free.  */
//      return (write_datagram(buf, len, _ip, _port, 0) ? MEM_MGMT_RESPONSIBLE_CREATOR : MEM_MGMT_RESPONSIBLE_CALLER);
//
//    case MEM_MGMT_RESPONSIBLE_BEARER:
//      /* We are now the bearer. That means that by returning non-failure, the
//          caller will expect _us_ to manage this memory.  */
//      // TODO: Freeing the buffer? Let UDP do it?
//      return (write_datagram(buf, len, _ip, _port, 0) ? MEM_MGMT_RESPONSIBLE_BEARER : MEM_MGMT_RESPONSIBLE_CALLER);
//
//    default:
//      /* This is more ambiguity than we are willing to bear... */
//      return MEM_MGMT_RESPONSIBLE_ERROR;
//  }
  Kernel::log("ManuvrUDP has not yet implemented toCounterparty().\n");
  return MEM_MGMT_RESPONSIBLE_ERROR;
}

/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrUDP::fromCounterparty(StringBuilder* buf, int8_t mm) {
//  switch (mm) {
//    case MEM_MGMT_RESPONSIBLE_CALLER:
//      // NOTE: No break. This might be construed as a way of saying CREATOR.
//    case MEM_MGMT_RESPONSIBLE_CREATOR:
//      /* The system that allocated this buffer either...
//          a) Did so with the intention that it never be free'd, or...
//          b) Has a means of discovering when it is safe to free.  */
//      if (haveFar()) {
//        return _far->fromCounterparty(buf, len, mm);
//      }
//      else {
//        _accumulator.concat(buf, len);
//        return MEM_MGMT_RESPONSIBLE_BEARER;   // We take responsibility.
//      }
//
//    case MEM_MGMT_RESPONSIBLE_BEARER:
//      /* We are now the bearer. That means that by returning non-failure, the
//          caller will expect _us_ to manage this memory.  */
//      if (haveFar()) {
//        /* We are not the transport driver, and we do no transformation. */
//        return _far->fromCounterparty(buf, len, mm);
//      }
//      else {
//        _accumulator.concat(buf, len);
//        return MEM_MGMT_RESPONSIBLE_BEARER;   // We take responsibility.
//      }
//
//    default:
//      /* This is more ambiguity than we are willing to bear... */
//      return MEM_MGMT_RESPONSIBLE_ERROR;
//  }
  Kernel::log("ManuvrUDP has not yet implemented fromCounterparty().\n");
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
/**
* We're being told to start listening on whatever address was provided to the
*   constructor. That means we are a server.
*
* @return 0 on success. Negative value on failure.
*/
int8_t ManuvrUDP::listen() {
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
    Kernel::log("Failed to bind the UDP server socket.\n");
    return -1;
  }

  /* Bind the server socket */
  if (bind(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    Kernel::log("Failed to bind the UDP server socket.\n");
    return -1;
  }

  //initialized(true);
  createThread(&_thread_id, NULL, _udp_socket_listener_loop, (void*) this);
  listening(true);
  local_log.concatf("UDP Now listening at %s:%d.\n", _addr, _port_number);

  flushLocalLog();
  return 0;
}


/**
* Read data from UDP port.
*
* NOTE: Blocks. So only call from within a thread.
* TODO: ALL OF THIS RUNS IN A THREAD. IT IS NOT SAFE AS IT STANDS.
*
* @return 0 on success. Negative value on failure.
*/
int8_t ManuvrUDP::read_port() {
  int8_t return_value = -1;
  unsigned char buf[_xport_mtu];
  struct sockaddr_in cli_addr;
  for (uint16_t i = 0; i < sizeof(cli_addr); i++) {
    *((uint8_t *) &cli_addr  + i) = 0;   // Zero the sockaddr structure for use.
  }

  unsigned int clientlen = sizeof(cli_addr);
  int n = recvfrom(getSockID(), buf, _xport_mtu, 0, (struct sockaddr*) &cli_addr, &clientlen);
  if (-1 == n) {
    local_log.concat("Failed to read UDP packet.\n");
  }
  else if (n > 0) {
    // We received a packet. Send it further away.
    bytes_received += n;   // Log the bytes.
    if (getVerbosity() > 6) {
      local_log.concatf("UDP read %d bytes from counterparty (%s).\n", n, (const char*) inet_ntoa(cli_addr.sin_addr));
    }
    UDPPipe* related_pipe = _open_replies[cli_addr.sin_port];
    if (NULL == related_pipe) {
      // Non-existence. Create...
      related_pipe = new UDPPipe(this, cli_addr.sin_addr.s_addr, cli_addr.sin_port);
      if (_pipe_strategy) related_pipe->setPipeStrategy(_pipe_strategy);

      if (MEM_MGMT_RESPONSIBLE_BEARER == ((BufferPipe*)related_pipe)->fromCounterparty(buf, n, MEM_MGMT_RESPONSIBLE_BEARER)) {
        // The pipe copied the buffer. Success.
        // Since we don't have a pipe, we create one and realize that there will
        //   be nothing on the other side to take the buffer. So we only broadcast
        //   a system-wide message if mem-mgmt responsibility for the buffer was
        //   accepted by the bearer.
        _open_replies[cli_addr.sin_port] = related_pipe;
        ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
        // Because we allocated the pipe, we must clean it up if it is not taken.
        event->setOriginator((EventReceiver*) this);
        //event->addArg(related_pipe);   // Add the newly-minted pipe.
        // Convey the transport. This is optional, but helps downstream classes
        //   make choices about binding to the BufferPipe.
        //event->addArg((ManuvrXport*) this);
        Kernel::staticRaiseEvent(event);
      }
      else {
        if (getVerbosity() > 2) {
          local_log.concat("UDPPipe failed to take the buffer. Dropping this created pipe:\n");
          related_pipe->printDebug(&local_log);
        }
        delete related_pipe;
      }
    }
    else {
      // We have a related pipe.
      switch (((BufferPipe*)related_pipe)->fromCounterparty(buf, n, MEM_MGMT_RESPONSIBLE_BEARER)) {
        case MEM_MGMT_RESPONSIBLE_BEARER:
          // Success
          break;
        case MEM_MGMT_RESPONSIBLE_CREATOR:
          if (getVerbosity() > 3) local_log.concat("UDPPipe took the buffer, but will probably fail (RESPONSIBLE_CREATOR).\n");
        case MEM_MGMT_RESPONSIBLE_CALLER:
        default:
          break;
      }

      if (!related_pipe->persistAfterReply()) {
        if (getVerbosity() > 3) local_log.concat("Attempting orderly cleanup of UDPPipe.\n");
        delete related_pipe;
      }
    }
    return_value = 0;
  }
  flushLocalLog();
  return return_value;
}


/**
* Resets the transport driver into a state of being freshly-initialized,
*   to the extent that is possible.
*
* @return false on error and true on success.
*/
int8_t ManuvrUDP::reset() {
  initialized(false);
	std::map<uint16_t, UDPPipe*>::iterator it;
	for (it = _open_replies.begin(); it != _open_replies.end(); it++) {
    delete it->second;
	}

  disconnect();

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
*
* @param  out     The buffer to send.
* @param  out_len The size of the buffer.
* @param  addr    The target IP as a network-order 32-bit unsigned.
* @param  port    The target port as a native-order 16-bit unsigned.
* @param  opts    Any options to the network driver.
* @return false on error and true on success.
*/
bool ManuvrUDP::write_datagram(unsigned char* out, int out_len, uint32_t addr, int port, uint32_t opts) {
  struct sockaddr_in _tmp_sockaddr;
  bool return_value = false;

  int _client_sock = socket(PF_INET, SOCK_DGRAM, 0);

  _tmp_sockaddr.sin_family      = AF_INET;
  _tmp_sockaddr.sin_port        = htons(port);
  _tmp_sockaddr.sin_addr.s_addr = addr;
  memset(_tmp_sockaddr.sin_zero, '\0', sizeof(_tmp_sockaddr.sin_zero));

  if (-1 != _client_sock) {
    int result = sendto(_client_sock, out, out_len, 0, (const sockaddr*) &_tmp_sockaddr, sizeof(_tmp_sockaddr));
    if (-1 < result) {
      bytes_sent += result;

      UDPPipe* related_pipe = _open_replies[_tmp_sockaddr.sin_port];
      if (NULL == related_pipe) {
        // Non-existence. Create...
        related_pipe = new UDPPipe(this, _tmp_sockaddr.sin_addr.s_addr, _tmp_sockaddr.sin_port);
        _open_replies[_tmp_sockaddr.sin_port] = related_pipe;
      }

      // TODO: We must receive from the socket before we can send again...
      char buffer[_xport_mtu];
      buffer[0] = '\0';
      int _rec_bytes = recvfrom(_client_sock, buffer, _xport_mtu, 0, NULL, NULL);
      Kernel::log(buffer);
      return_value = true;
    }
    else {
      if (getVerbosity() > 3) Kernel::log("Failed to write a UDP datagram because of sentto().\n");
    }
  }
  else {
    if (getVerbosity() > 3) Kernel::log("Failed to write a UDP datagram. No client socket.\n");
  }

  return return_value;
}


/**
* UDP pipes call this during their destructors to cause the
*   issuing class to clean up references.
*
* @param  buf    A pointer to the buffer.
* @param  len    How long the buffer is.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrUDP::udpPipeDestroyCallback(UDPPipe* _dead_walking) {
  _open_replies.erase(_dead_walking->getPort());
  return 0;
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
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrUDP::attached() {
  EventReceiver::attached();
  // Because this is not a stream-oriented transport, the timeout value is used
  //   to periodically flush our connection cache.
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(300);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);
  read_abort_event.enableSchedule(false);
  //platform.kernel()->addSchedule(&read_abort_event);

  listen();
  return 1;
}


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

  output->concatf("--\n-- _open_replies \n");
  std::map<uint16_t, UDPPipe*>::iterator it;
	for (it = _open_replies.begin(); it != _open_replies.end(); it++) {
		it->second->printDebug(output);
	}
  output->concat("\n");
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
int8_t ManuvrUDP::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_XPORT_RECEIVE:
      if (event->isOriginator((EventReceiver*) this)) {
        // If we originated this message, it means we attached a BufferPipe that
        //   we allocated. If it is still attached to the event, it means it was
        //   not joined to any other pipe. Clean it up.
        BufferPipe* tmp_pipe = NULL;
        switch (event->getArgumentType(0)) {
          case BUFFERPIPE_PTR_FM:
            // The pipe was NOT taken. Clean it up.
            if (0 == event->getArgAs(&tmp_pipe)) {
              // We rely on the UDPPipe to call us back to trigger a cleanup of
              //   the entry in _open_replies.
              delete (UDPPipe*) tmp_pipe;  // TODO: Any safer way?
            }
            break;
          case SYS_MANUVR_XPORT_FM:
            // This is a good indication that the pipe was taken.
            break;
          default:
            break;
        }
      }
    default:
      break;
  }

  return return_value;
}



int8_t ManuvrUDP::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_XPORT_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void ManuvrUDP::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  // MANUVR_CONSOLE_SUPPORT

#endif  // UDP support
