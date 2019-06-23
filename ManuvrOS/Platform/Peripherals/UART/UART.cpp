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
  memcpy(&_opts, opts, sizeof(ManuvrUARTOpts));   // Clobber the old conf.
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
}
