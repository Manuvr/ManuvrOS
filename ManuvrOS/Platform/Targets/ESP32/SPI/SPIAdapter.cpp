/*
File:   SPIAdapter.cpp
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
#include <Platform/Peripherals/SPI/SPIAdapter.h>

#if defined(CONFIG_MANUVR_SUPPORT_SPI)

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/spi_master.h>
#include "rom/ets_sys.h"
#include "esp_types.h"
#include "esp_attr.h"
#include "esp_intr.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/xtensa_api.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "rom/lldesc.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "esp_heap_caps.h"

static const char* LOG_TAG = "SPIAdapter";


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/

static lldesc_t _ll_tx;
static SPIBusOp* _threaded_op = nullptr;
static spi_device_handle_t spi2_handle;

#ifdef __cplusplus
}
#endif



/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

int8_t SPIAdapter::bus_init() {
	spi_bus_config_t bus_config;
  uint32_t b_flags = 0;
  if (255 != _opts.spi_clk) {
    b_flags |= SPICOMMON_BUSFLAG_SCLK;
    bus_config.sclk_io_num = (gpio_num_t) _opts.spi_clk;
  }
  else {
    bus_config.sclk_io_num = (gpio_num_t) -1;
  }
  if (255 != _opts.spi_mosi) {
    b_flags |= SPICOMMON_BUSFLAG_MOSI;
    bus_config.mosi_io_num = (gpio_num_t) _opts.spi_mosi;
  }
  else {
    bus_config.mosi_io_num = (gpio_num_t) -1;
  }
  if (255 != _opts.spi_miso) {
    b_flags |= SPICOMMON_BUSFLAG_MISO;
    bus_config.miso_io_num = (gpio_num_t) _opts.spi_miso;
  }
  else {
    bus_config.miso_io_num = (gpio_num_t) -1;
  }
	bus_config.quadwp_io_num   = -1;      // Not used
	bus_config.quadhd_io_num   = -1;      // Not used
	bus_config.max_transfer_sz = 0;       // 0 means use default.
  bus_config.flags           = b_flags;

	esp_err_t errRc = spi_bus_initialize(
		(spi_host_device_t) _opts.idx,
		&bus_config,
		1 // DMA Channel
	);

  if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "spi_bus_initialize(): rc=%d", errRc);
    return -1;
	}

	// spi_device_interface_config_t dev_config;
	// dev_config.address_bits     = 0;
	// dev_config.command_bits     = 0;
	// dev_config.dummy_bits       = 0;
	// dev_config.mode             = 0;
	// dev_config.duty_cycle_pos   = 0;
	// dev_config.cs_ena_posttrans = 0;
	// dev_config.cs_ena_pretrans  = 0;
	// dev_config.clock_speed_hz   = 10000000;
	// dev_config.spics_io_num     = (gpio_num_t) _opts.spi_cs;
	// dev_config.flags            = SPI_DEVICE_NO_DUMMY;
	// dev_config.queue_size       = 1;
	// dev_config.pre_cb           = NULL;
	// dev_config.post_cb          = NULL;
	// ESP_LOGI(LOG_TAG, "... Adding device bus.");
	// errRc = spi_bus_add_device((spi_host_device_t) 1, &dev_config, &spi2_handle);
	// if (errRc != ESP_OK) {
	// 	ESP_LOGE(LOG_TAG, "spi_bus_add_device(): rc=%d", errRc);
  //   return -2;
	// }
  return 0;
}

int8_t SPIAdapter::bus_deinit() {
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
int8_t SPIAdapter::attached() {
  if (EventReceiver::attached()) {
    // We should init the SPI library...
    bus_init();
    return 1;
  }
  return 0;
}




/*******************************************************************************
* BusOp functions below...
*******************************************************************************/

/**
* Calling this member will cause the bus operation to be started.
*
* @return 0 on success, or non-zero on failure.
*/
XferFault SPIBusOp::begin() {
  //time_began    = micros();
  //if (0 == _param_len) {
  //  // Obvious invalidity. We must have at least one transfer parameter.
  //  abort(XferFault::BAD_PARAM);
  //  return XferFault::BAD_PARAM;
  //}

  //if (SPI1->SR & SPI_FLAG_BSY) {
  //  Kernel::log("SPI op aborted before taking bus control.\n");
  //  abort(XferFault::BUS_BUSY);
  //  return XferFault::BUS_BUSY;
  //}

  set_state(XferState::INITIATE);  // Indicate that we now have bus control.

  _assert_cs(true);

  if (_param_len) {
    set_state(XferState::ADDR);
    for (int i = 0; i < _param_len; i++) {
      //SPI.transfer(xfer_params[i]);
    }
  }

  if (buf_len) {
    set_state((opcode == BusOpcode::TX) ? XferState::TX_WAIT : XferState::RX_WAIT);
    for (int i = 0; i < buf_len; i++) {
      //SPI.transfer(*(buf + i));
    }
  }

  markComplete();
  return XferFault::NONE;
}


/**
* Called from the ISR to advance this operation on the bus.
* Stay brief. We are in an ISR.
*
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::advance_operation(uint32_t status_reg, uint8_t data_reg) {
  //debug_log.concatf("advance_op(0x%08x, 0x%02x)\n\t %s\n\t status: 0x%08x\n", status_reg, data_reg, getStateString(), (unsigned long) hspi1.State);
  //Kernel::log(&debug_log);

  /* These are our transfer-size-invariant cases. */
  switch (xfer_state) {
    case XferState::COMPLETE:
      abort(XferFault::HUNG_IRQ);
      return 0;

    case XferState::TX_WAIT:
    case XferState::RX_WAIT:
      markComplete();
      return 0;

    case XferState::FAULT:
      return 0;

    case XferState::QUEUED:
    case XferState::ADDR:
    case XferState::STOP:
    case XferState::UNDEF:

    /* Below are the states that we shouldn't be in at this point... */
    case XferState::INITIATE:
    case XferState::IDLE:
      abort(XferFault::ILLEGAL_STATE);
      return 0;
  }

  return -1;
}


#endif   // CONFIG_MANUVR_SUPPORT_SPI
