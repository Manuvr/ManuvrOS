/*
File:   SPIAdapter.h
Author: J. Ian Lindsay
Date:   2016.12.17

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


*/


#ifndef __SPI_DRIVER_TEMPLATE_H__
#define __SPI_DRIVER_TEMPLATE_H__

#include "SPIBusOp.h"
#include <Platform/Platform.h>


/* Compile-time bounds on memory usage. */
#ifndef CONFIG_SPIADAPTER_MAX_QUEUE_PRINT
  // How many queue items should we print for debug?
  #define CONFIG_SPIADAPTER_MAX_QUEUE_PRINT 3
#endif
#ifndef CONFIG_SPIADAPTER_MAX_QUEUE_DEPTH
  // How deep should the queue be allowed to become before rejecting work?
  #define CONFIG_SPIADAPTER_MAX_QUEUE_DEPTH 12
#endif
#ifndef CONFIG_SPIADAPTER_PREALLOC_COUNT
  // How many queue items should we have on-tap?
  #define CONFIG_SPIADAPTER_PREALLOC_COUNT  4
#endif


#define MANUVR_MSG_SPI_QUEUE_READY      0x0230 // There is a new job in the SPI bus queue.
#define MANUVR_MSG_SPI_CB_QUEUE_READY   0x0231 // There is something ready in the callback queue.

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define SPI_FLAG_SPI_READY    0x01    // Is SPI1 initialized?
#define SPI_FLAG_QUEUE_IDLE   0x02    // Is the SPI queue idle?
#define SPI_FLAG_QUEUE_GUARD  0x04    // Prevent bus queue floods?
#define SPI_FLAG_RESERVED_0   0x08    // Reserved
#define SPI_FLAG_RESERVED_1   0x10    // Reserved
#define SPI_FLAG_CPOL         0x20    // Bus configuration details.
#define SPI_FLAG_CPHA         0x40    // Bus configuration details.
#define SPI_FLAG_MASTER       0x80    // Bus configuration details.


/*
* Set pin def to 255 to mark it as unused.
*/
class SPIAdapterOpts {
  public:
    const uint8_t idx;
    const uint8_t spi_clk;
    const uint8_t spi_mosi;
    const uint8_t spi_miso;

    SPIAdapterOpts(const SPIAdapterOpts* p) :
      idx(p->idx),
      spi_clk(p->spi_clk),
      spi_mosi(p->spi_mosi),
      spi_miso(p->spi_miso),
      _flags(p->_flags)
    {};

    SPIAdapterOpts(
      uint8_t _i,
      uint8_t _spi_clk,
      uint8_t _spi_mosi,
      uint8_t _spi_miso
    ) :
      idx(_i),
      spi_clk(_spi_clk),
      spi_mosi(_spi_mosi),
      spi_miso(_spi_miso),
      _flags(SPI_FLAG_MASTER | SPI_FLAG_CPOL | SPI_FLAG_CPHA)
    {};

    SPIAdapterOpts(
      uint8_t _i,
      uint8_t _spi_clk,
      uint8_t _spi_mosi,
      uint8_t _spi_miso,
      uint8_t _f
    ) :
      idx(_i),
      spi_clk(_spi_clk),
      spi_mosi(_spi_mosi),
      spi_miso(_spi_miso),
      _flags(_f)
    {};


  private:
    const uint8_t _flags;
};


/*
* The SPI driver class.
*/
class SPIAdapter : public EventReceiver, public BusAdapter<SPIBusOp> {
  public:
    SPIAdapter(const SPIAdapterOpts*);
    ~SPIAdapter();

    int8_t init();

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);
    int8_t advance_work_queue();

    void purge_queued_work_by_dev(BusOpCallback *dev);   // Flush the work queue by callback match

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    void printHardwareState(StringBuilder*);

    void reclaim_queue_item(SPIBusOp*);


  protected:
    int8_t attached();      // This is called from the base notify().

  private:
    const SPIAdapterOpts _opts;
    ManuvrMsg event_spi_callback_ready;
    ManuvrMsg event_spi_timeout;

    /* List of pending callbacks for bus transactions. */
    PriorityQueue<SPIBusOp*> callback_queue;
    uint32_t  bus_timeout_millis = 5;  // How long to spend in IO_WAIT?
    uint8_t   spi_cb_per_event   = 3;  // Limit the number of callbacks processed per event.

    int8_t service_callback_queue();

    /* Overrides from the BusAdapter interface */
    int8_t bus_init();
    int8_t bus_deinit();

    static SPIBusOp preallocated_bus_jobs[CONFIG_SPIADAPTER_PREALLOC_COUNT];// __attribute__ ((section(".ccm")));
};

#endif  // __SPI_DRIVER_TEMPLATE_H__
