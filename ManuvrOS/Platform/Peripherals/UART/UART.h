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

    // Data transaction fxns.
    uint32_t write(uint8_t* buf, unsigned int len);
    uint32_t read(uint8_t* buf, unsigned int len);
    uint32_t readUntil(uint8_t* buf, unsigned int len, uint8_t stop);
    bool     rxContains(uint8_t stop);
    inline uint32_t txBytesWaiting() {  return _tx_bytes;  };
    inline uint32_t rxBytesWaiting() {  return _rx_bytes;  };
    inline uint32_t txBufferAvailable() {  return (_opts.tx_buf_len - txBytesWaiting());  };
    inline uint32_t rxBufferAvailable() {  return (_opts.rx_buf_len - rxBytesWaiting());  };

    // On-the-fly conf accessors...
    int8_t bitrate(uint32_t);
    inline uint32_t bitrate() {   return _opts.bitrate;  };

    inline bool ready() {    return _port_ready;    };
    inline bool flushed() {  return _port_flushed && (txBytesWaiting() == 0); };



  private:
    ManuvrUARTOpts _opts;
    uint8_t* _rx_buf   = nullptr;  // Bytes from the hardware.
    uint8_t* _tx_buf   = nullptr;  // Bytes to the hardware.
    uint16_t _rx_bytes = 0;  // Number of bytes in rx buffer.
    uint16_t _rx_w_pos = 0;  // Advanced by _give_rx_buffer()
    uint16_t _rx_r_pos = 0;  // Advanced by read()
    uint16_t _tx_bytes = 0;  // Number of bytes in tx buffer.
    uint16_t _tx_w_pos = 0;  // Advanced by write()
    uint16_t _tx_r_pos = 0;  // Advanced by _take_tx_buffer()

    // These values manipulated by driver.
    bool     _port_ready          = false;
    bool     _pending_conf_change = false;  // ManuvrUART sets true. Driver sets false.
    bool     _pending_reset       = false;  // ManuvrUART sets true. Driver sets false.
    bool     _port_flushed        = true;

    // Called by the driver class to move data into and out of the client class.
    uint32_t _take_tx_buffer(uint8_t*, unsigned int);
    uint32_t _give_rx_buffer(uint8_t*, unsigned int);
    void _wipe_buffers();

    int8_t _pf_conf();
    int8_t _pf_write();
    int8_t _pf_read();

    bool _rx_enabled() {  return (nullptr != _rx_buf);  };
    bool _tx_enabled() {  return (nullptr != _tx_buf);  };
};
