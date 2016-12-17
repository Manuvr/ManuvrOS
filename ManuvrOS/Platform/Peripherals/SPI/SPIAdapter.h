#ifndef __TEENSY_SPI_DRIVER_H__
#define __TEENSY_SPI_DRIVER_H__

#include "SPIBusOp.h"
#include <Platform/Platform.h>


#define SPI_MAX_QUEUE_PRINT 3          // How many SPI queue items should we print for debug?
#define PREALLOCATED_SPI_JOBS    10    // How many SPI queue items should we have on-tap?

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define CPLD_FLAG_INT_OSC      0x01    // Running on internal oscillator.
#define CPLD_FLAG_EXT_OSC      0x02    // Running on external oscillator.
#define CPLD_FLAG_SVC_IRQS     0x04    // Should the CPLD respond to IRQ signals?
#define CPLD_FLAG_QUEUE_IDLE   0x08    // Is the SPI queue idle?
#define CPLD_FLAG_QUEUE_GUARD  0x10    // Prevent bus queue floods?
#define CPLD_FLAG_SPI_READY    0x20    // Is SPI1 initialized?


/*
* The SPI driver class.
*/
class SPIAdapter : public EventReceiver, public BusAdapter<SPIBusOp> {
  public:
    SPIAdapter();
    ~SPIAdapter();

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);
    int8_t advance_work_queue();
    SPIBusOp* new_op();
    SPIBusOp* new_op(BusOpcode, BusOpCallback*);

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    int8_t attached();      // This is called from the base notify().
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
      void printHardwareState(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT

    static SPIBusOp* current_queue_item;


  private:
    ManuvrMsg event_spi_callback_ready;
    ManuvrMsg event_spi_timeout;

    /* List of pending callbacks for bus transactions. */
    PriorityQueue<SPIBusOp*> callback_queue;
    uint32_t  bus_timeout_millis = 5;  // How long to spend in IO_WAIT?
    uint32_t  specificity_burden = 0;  // How many queue items have been deleted?
    uint8_t   spi_cb_per_event   = 3;  // Limit the number of callbacks processed per event.

    void purge_queued_work();     // Flush the work queue.
    void purge_queued_work_by_dev(BusOpCallback *dev);   // Flush the work queue by callback match
    void purge_stalled_job();     // TODO: Misnomer. Really purges the active job.
    int8_t service_callback_queue();
    void reclaim_queue_item(SPIBusOp*);

    /* Setup and init fxns. */
    void gpioSetup();
    bool _set_timer_base(uint16_t);
    void init_ext_clk();
    void init_spi(uint8_t cpol, uint8_t cpha);  // Pass 0 for CPHA 0.


    static SPIBusOp preallocated_bus_jobs[PREALLOCATED_SPI_JOBS];// __attribute__ ((section(".ccm")));
};

#endif
