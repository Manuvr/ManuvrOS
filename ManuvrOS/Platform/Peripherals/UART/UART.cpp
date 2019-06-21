#include <Platform/Platform.h>
#include "UART.h"



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
ManuvrUART::ManuvrUART(const ManuvrUARTOpts* opts) : BufferPipe() {
  memcpy(&_opts, opts, sizeof(ManuvrUARTOpts));
  _bp_set_flag(BPIPE_FLAG_IS_TERMINUS, true);
}


ManuvrUART::~ManuvrUART() {}



/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* These functions can stop at transport.
*******************************************************************************/
const char* ManuvrUART::pipeName() { return "ManuvrUART"; }

/**
* Pass a signal to the UART.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t ManuvrUART::toCounterparty(ManuvrPipeSignal _sig, void* _args) {
  switch (_sig) {
    case ManuvrPipeSignal::XPORT_CONNECT:
      return 0;
    case ManuvrPipeSignal::XPORT_DISCONNECT:
      return 0;

    case ManuvrPipeSignal::XPORT_RESET:
      reset();
      return 0;

    case ManuvrPipeSignal::XPORT_LISTEN:
      return 0;

    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }

  return -1;
}


/**
* Ends up calling TX fxns
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrUART::toCounterparty(StringBuilder* buf, int8_t mm) {
  { //if (connected()) {
    char* working_chunk = nullptr;
    while (buf->count()) {
      working_chunk = buf->position(0);
      printf("%s", (const char*) working_chunk);
      buf->drop_position(0);
    }
    return MEM_MGMT_RESPONSIBLE_BEARER;
  }
  return MEM_MGMT_RESPONSIBLE_ERROR;
}



/*******************************************************************************
* Ringbuffer fxns for client classes. These happen in the client's thread.
*******************************************************************************/


/*
* The client class can call this at any time to reconfigure the UART.
*/
int8_t ManuvrUART::setParams(const ManuvrUARTOpts* opts) {
  // First, we need to check to see if we need to reallocate the buffers...
  if (_opts.rx_buf_len != opts->rx_buf_len) {
    if (nullptr != _rx_buf) {
      free(_rx_buf);
      _rx_buf = nullptr;
    }
  }
  if (_opts.tx_buf_len != opts->tx_buf_len) {
    if (nullptr != _tx_buf) {
      free(_tx_buf);
      _tx_buf = nullptr;
    }
  }
  memcpy(&_opts, opts, sizeof(ManuvrUARTOpts));   // Clobber the old conf.

  printf("Setting params for UART");;

  if (0 < _opts.rx_buf_len) {
    // Enable the receiver.
    if (nullptr == _rx_buf) {
      _rx_buf = (uint8_t*) malloc(_opts.rx_buf_len);
    }
    if (_rx_buf) {
      memset(_rx_buf, 0, _opts.rx_buf_len);
    }
    else {
      printf("Failure to allocate Rx buffer of size %u for port %u.\n", _opts.rx_buf_len, _opts.port);
      return -1;
    }
  }

  if (0 < _opts.tx_buf_len) {
    // Enable the transmitter.
    if (nullptr == _tx_buf) {
      _tx_buf = (uint8_t*) malloc(_opts.tx_buf_len);
    }
    if (_tx_buf) {
      memset(_tx_buf, 0, _opts.tx_buf_len);
    }
    else {
      printf("Failure to allocate Tx buffer of size %u for port %u.\n", _opts.tx_buf_len, _opts.port);
      return -2;
    }
  }

  return _pf_conf();
}



/*
* Setting this bit will cause the driver to reset our port's registers.
* TODO: Maybe we ought to wipe the buffers here? Presently, the driver class
*   does it after reset.
* TODO: Might-should force synchronicity here by adding _wait_for_port_reset()?
*/
void ManuvrUART::reset() {
  _pending_reset = true;
}


void ManuvrUART::printDebug(StringBuilder* out) {
  out->concatf("ManuvrUART %u:\n", _opts.port);
  out->concatf("\t rx_buf_len    %u\n", _opts.rx_buf_len  );
  out->concatf("\t tx_buf_len    %u\n", _opts.tx_buf_len  );
  out->concatf("\t bitrate       %u\n", _opts.bitrate     );
  out->concatf("\t start_bits    %u\n", _opts.start_bits  );
  out->concatf("\t stop_bits     %u\n", _opts.stop_bits   );
  out->concatf("\t bit_per_word  %u\n", _opts.bit_per_word);
  out->concatf("\t parity        %u\n", (uint8_t) _opts.parity      );
  out->concatf("\t flow_control  %u\n", (uint8_t) _opts.flow_control);
  out->concatf("\t xoff_char     %u\n", _opts.xoff_char   );
  out->concatf("\t xon_char      %u\n", _opts.xon_char    );
  out->concat("\t ------------------\n");
  out->concatf("\t _rx_bytes     %u\n", _rx_bytes);
  out->concatf("\t _tx_bytes     %u\n", _tx_bytes);
  out->concatf("\t _rx_w_pos     %u\n", _rx_w_pos);
  out->concatf("\t _rx_r_pos     %u\n", _rx_r_pos);
  out->concatf("\t _tx_w_pos     %u\n", _tx_w_pos);
  out->concatf("\t _tx_r_pos     %u\n\n", _tx_r_pos);
}



/*******************************************************************************
* Ringbuffer fxns for client classes. These happen in the client's thread.
*******************************************************************************/

/**
* @return The number of bytes taken from the client class and queued for send.
*/
uint32_t ManuvrUART::write(uint8_t* buf, unsigned int len) {
  uint32_t ret = 0;
  if (_tx_enabled() && ready()) {
    // How much of the buffer can we accept?
    uint32_t tx_buf_free = txBufferAvailable();
    uint32_t accept_len  = strict_min(len, tx_buf_free);
    if (0 < accept_len) {
      // There is space for at least some of the data. Copy it.
      uint32_t len_until_wrap = _opts.tx_buf_len - _tx_w_pos;
      uint32_t copy_len = strict_min(accept_len, len_until_wrap);

      // We proceed as if we can do this in a single copy operation...
      memcpy((_tx_buf + _tx_w_pos), buf, copy_len);
      if (accept_len != copy_len) {
        // We've wrapped. Copy the remainder to the start of the buffer.
        memcpy(_tx_buf, (buf + copy_len), (accept_len - copy_len));
        _tx_w_pos = (accept_len - copy_len);
      }
      else {
        _tx_w_pos += accept_len;
      }
      _tx_bytes += accept_len;
    }
    ret = accept_len;
  }
  return ret;
}


/**
* @return The number of bytes relinquished to the client class as a read.
*/
uint32_t ManuvrUART::read(uint8_t* buf, unsigned int len) {
  uint32_t ret = 0;
  if (_rx_enabled() && ready()) {
    // How much buffer can we relinquish?
    uint32_t rx_buf_size = rxBytesWaiting();
    uint32_t relinq_len  = strict_min(len, rx_buf_size);
    if (0 < relinq_len) {
      // There is at least some data to be given up. Copy it.
      uint32_t len_until_wrap = _opts.rx_buf_len - _rx_r_pos;
      uint32_t copy_len = strict_min(relinq_len, len_until_wrap);

      // We proceed as if we can do this in a single copy operation...
      memcpy(buf, (_rx_buf + _rx_r_pos), copy_len);
      if (relinq_len != copy_len) {
        // We've wrapped. Copy the remainder to the correct offset.
        memcpy((buf + copy_len), _rx_buf, (relinq_len - copy_len));
        _rx_r_pos = (relinq_len - copy_len);
      }
      else {
        _rx_r_pos += relinq_len;
      }
      _rx_bytes -= relinq_len;
      ret = relinq_len;
    }
  }
  return ret;
}


/**
* Read until (and including) the occurance of a specified character. This is
*   useful as a simple tokenizing mechanism.
* If the stop character does not occur within the given length, the entire given
*   length will be relinquished.
*
* @return The number of bytes relinquished to the client class as a read.
*/
uint32_t ManuvrUART::readUntil(uint8_t* buf, unsigned int len, uint8_t stop) {
  uint32_t ret = 0;
  if (_rx_enabled() && ready()) {
    // How much buffer can we relinquish?
    uint32_t rx_buf_size = rxBytesWaiting();
    uint32_t relinq_len  = strict_min(len, rx_buf_size);
    if (0 < relinq_len) {
      // There is at least some data to be given up. Copy it.
      uint32_t len_until_wrap = _opts.rx_buf_len - _rx_r_pos;
      uint32_t copy_len = strict_min(relinq_len, len_until_wrap);
      bool stop_found = false;
      uint32_t buf_offset = 0;

      // We proceed as if we can do this in a single copy operation...
      while (!stop_found && (buf_offset < copy_len)) {
        stop_found = (stop == *(_rx_buf + _rx_r_pos + buf_offset));
        *(buf + buf_offset) = *(_rx_buf + _rx_r_pos + buf_offset);
        buf_offset++;
      }

      if (!stop_found) {    // If we haven't found the stop...
        if (relinq_len > copy_len) {
          // We've wrapped. Copy the remainder to the correct offset.
          uint32_t idx = 0;
          while (!stop_found && (buf_offset < relinq_len)) {
            stop_found = (stop == *(_rx_buf + idx + buf_offset));
            *(buf + buf_offset) = *(_rx_buf + idx + buf_offset);
            buf_offset++;
            idx++;
          }
          _rx_r_pos = idx;
        }
        else {
          _rx_r_pos += buf_offset;
        }
      }
      else {
        _rx_r_pos += buf_offset;
      }

      _rx_bytes -= buf_offset;
      ret = buf_offset;
    }
  }
  return ret;
}


/**
* @return True if the pending rx buffer contains a given character.
*/
bool ManuvrUART::rxContains(uint8_t stop) {
  if (_rx_enabled()) {
    uint32_t rx_buf_size = rxBytesWaiting();
    if (0 < rx_buf_size) {
      uint32_t len_until_wrap = _opts.rx_buf_len - _rx_r_pos;
      uint32_t search_len     = strict_min(rx_buf_size, len_until_wrap);
      uint32_t buf_offset = 0;
      while (buf_offset < search_len) {
        if (stop == *(_rx_buf + _rx_r_pos + buf_offset)) {
          return true;
        }
        buf_offset++;
      }
      if (rx_buf_size > search_len) {
        uint32_t idx = 0;
        while (buf_offset < rx_buf_size) {
          if (stop == *(_rx_buf + idx + buf_offset)) {
            return true;
          }
          buf_offset++;
          idx++;
        }
      }
    }
  }
  return false;
}



/*******************************************************************************
* Ringbuffer fxns for driver class. These happen in the driver's thread.
*******************************************************************************/

/**
* @return The number of bytes relinquished to the driver.
*/
uint32_t ManuvrUART::_take_tx_buffer(uint8_t* taken, unsigned int len) {
  uint32_t ret = 0;
  if (_tx_buf) {
    // How much buffer can we relinquish?
    uint32_t tx_buf_size = txBytesWaiting();
    uint32_t relinq_len  = strict_min(len, tx_buf_size);
    if (0 < relinq_len) {
      // There is at least some data to be given up. Copy it.
      uint32_t len_until_wrap = _opts.tx_buf_len - _tx_r_pos;
      uint32_t copy_len = strict_min(relinq_len, len_until_wrap);

      // We proceed as if we can do this in a single copy operation...
      memcpy(taken, (_tx_buf + _tx_r_pos), copy_len);
      if (relinq_len != copy_len) {
        // We've wrapped. Copy the remainder to the correct offset.
        memcpy((taken + copy_len), _tx_buf, (relinq_len - copy_len));
        _tx_r_pos = (relinq_len - copy_len);
      }
      else {
        _tx_r_pos += relinq_len;
      }
      _tx_bytes -= relinq_len;
      ret = relinq_len;
    }
  }
  return ret;
}


/**
* @return The number of bytes accepted from the driver.
*/
uint32_t ManuvrUART::_give_rx_buffer(uint8_t* given, unsigned int len) {
  uint32_t ret = 0;
  if (_rx_buf) {
    // How much of the buffer can we accept?
    uint32_t rx_buf_free = rxBufferAvailable();
    uint32_t accept_len  = strict_min(len, rx_buf_free);
    if (0 < accept_len) {
      // There is space for at least some of the data. Copy it.
      uint32_t len_until_wrap = _opts.rx_buf_len - _rx_w_pos;
      uint32_t copy_len = strict_min(accept_len, len_until_wrap);

      // We proceed as if we can do this in a single copy operation...
      memcpy((_rx_buf + _rx_w_pos), given, copy_len);
      if (accept_len != copy_len) {
        // We've wrapped. Copy the remainder to the start of the buffer.
        memcpy(_rx_buf, (given + copy_len), (accept_len - copy_len));
        _rx_w_pos = (accept_len - copy_len);
      }
      else {
        _rx_w_pos += accept_len;
      }
      _rx_bytes += accept_len;
    }
    ret = accept_len;
  }
  return ret;
}


/**
* Wipes the RX/TX buffers.
*/
void ManuvrUART::_wipe_buffers() {
  _rx_bytes = 0;
  _tx_bytes = 0;
  _rx_w_pos = 0;
  _rx_r_pos = 0;
  _tx_w_pos = 0;
  _tx_r_pos = 0;
  if (_rx_buf) {  memset(_rx_buf, 0, _opts.rx_buf_len);  }
  if (_tx_buf) {  memset(_tx_buf, 0, _opts.tx_buf_len);  }
}
