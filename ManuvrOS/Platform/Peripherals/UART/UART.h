#include <Platform/Platform.h>

/* Flow-control strategies. */
enum class UARTFlowControl : uint8_t {
  NONE        = 0,
  XON_XOFF_R  = 1,
  XON_XOFF_T  = 2,
  XON_XOFF_RT = 3,
  RTS         = 4,
  CTS         = 5,
  RTS_CTS     = 6
};

/* Parity bit strategies. */
enum class UARTParityBit : uint8_t {
  NONE  = 0,
  EVEN  = 1,
  ODD   = 2
};

/* Stop bit definitions. */
enum class UARTStopBit : uint8_t {
  STOP_1    = 0,
  STOP_1_5  = 1,
  STOP_2    = 2
};

/* Conf representation for a port. */
typedef struct {
  uint16_t        rx_buf_len;
  uint16_t        tx_buf_len;
  uint32_t        bitrate;
  uint8_t         start_bits;
  uint8_t         bit_per_word;
  UARTStopBit     stop_bits;
  UARTParityBit   parity;
  UARTFlowControl flow_control;
  uint8_t         xoff_char;
  uint8_t         xon_char;
  uint8_t         rx_pin;
  uint8_t         tx_pin;
  uint8_t         rts_pin;
  uint8_t         cts_pin;
  uint8_t         port;
} ManuvrUARTOpts;


/*
* Abstraction layer for the individual comm ports.
*/
class ManuvrUART : public BufferPipe {
  public:
    ManuvrUART(const ManuvrUARTOpts*);
    ~ManuvrUART();

    /* Override from BufferPipe. */
    const char* pipeName();
    virtual int8_t toCounterparty(ManuvrPipeSignal, void*);
    virtual int8_t toCounterparty(StringBuilder* buf, int8_t mm);


    // Basic management and init,
    int8_t setParams(const ManuvrUARTOpts* o);
    void reset();
    void printDebug(StringBuilder*);

    // On-the-fly conf accessors...
    int8_t bitrate(uint32_t);
    inline uint32_t bitrate() {   return _opts.bitrate;  };

    inline bool ready() {    return _port_ready;    };
    inline bool flushed() {  return _port_flushed;  };



  protected:
    ManuvrUARTOpts _opts;

    // These values manipulated by driver.
    bool     _port_ready          = false;
    bool     _pending_conf_change = false;  // ManuvrUART sets true. Driver sets false.
    bool     _pending_reset       = false;  // ManuvrUART sets true. Driver sets false.
    bool     _port_flushed        = true;

    int8_t _pf_conf();
    int8_t _pf_write();
    int8_t _pf_read();
};
