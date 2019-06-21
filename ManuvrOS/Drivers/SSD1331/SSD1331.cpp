/***************************************************
  This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/684

  These displays use SPI to communicate, 4 or 5 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution


Software License Agreement (BSD License)

Copyright (c) 2012, Adafruit Industries
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holders nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************/

#include "SSD1331.h"


#if defined(CONFIG_MANUVR_SSD1331)

#ifdef __cplusplus
extern "C" {
#endif

#include "soc/gpio_sig_map.h"
#include "soc/spi_reg.h"
#include "soc/dport_reg.h"
#include "soc/dport_access.h"
#include "soc/spi_struct.h"

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
static uint16_t spi2_op_counter_0 = 0;
static uint16_t spi2_op_counter_1 = 0;



void IRAM_ATTR reset_spi2_dma() {
  periph_module_reset(PERIPH_SPI_DMA_MODULE);
  // Blindly clear all interrupts.
  SPI2.dma_int_clr.val = (
    SPI_OUT_TOTAL_EOF_INT_CLR |
    SPI_OUT_EOF_INT_CLR |
    SPI_OUT_DONE_INT_CLR |
    SPI_IN_SUC_EOF_INT_CLR |
    SPI_IN_ERR_EOF_INT_CLR |
    SPI_IN_DONE_INT_CLR |
    SPI_INLINK_DSCR_ERROR_INT_CLR |
    SPI_OUTLINK_DSCR_ERROR_INT_CLR |
    SPI_INLINK_DSCR_EMPTY_INT_CLR);
  SPI2.dma_int_clr.val = 0;
  SPI2.dma_conf.val |= (SPI_OUT_RST | SPI_IN_RST | SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
  SPI2.dma_out_link.val  = 0x00000000;
  SPI2.dma_in_link.val   = 0x00000000;
  SPI2.dma_conf.val &= ~(SPI_OUT_RST | SPI_IN_RST | SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
}



static void IRAM_ATTR dma_isr(void *arg) {
  uint32_t isr_val = SPI2.dma_int_st.val;  // Current interrupt bits.
  spi2_op_counter_1++;
  if (SPI2.dma_int_st.inlink_dscr_empty) {   /* lack of enough inlink descriptors.*/
    spi2_op_counter_1+=0x00000010;
    SPI2.dma_in_link.val  = 0x10000000;  // Clear everything but the stop bit.
  }
  if (SPI2.dma_int_st.outlink_dscr_error) {  /* outlink descriptor error.*/
    spi2_op_counter_1+=0x00000040;
  }
  if (SPI2.dma_int_st.inlink_dscr_error) {   /* inlink descriptor error.*/
    spi2_op_counter_1+=0x00000100;
  }
  if (SPI2.dma_int_st.in_done) {             /* completing usage of a inlink descriptor.*/
    spi2_op_counter_1+=0x00000400;
  }
  if (SPI2.dma_int_st.in_err_eof) {          /* receiving error.*/
    spi2_op_counter_1+=0x00001000;
  }
  if (SPI2.dma_int_st.in_suc_eof) {          /* completing receiving all the packets from host.*/
    spi2_op_counter_1+=0x01000000;
    SPI2.dma_in_link.val   = 0x10000000;  // Clear everything but the stop bit.
  }
  if (SPI2.dma_int_st.out_done) {            /* completing usage of a outlink descriptor .*/
    spi2_op_counter_1+=0x00010000;
  }
  if (SPI2.dma_int_st.out_eof) {             /* sending a packet to host done.*/
    spi2_op_counter_1+=0x00040000;
  }
  if (SPI2.dma_int_st.out_total_eof) {       /* sending all the packets to host done.*/
    spi2_op_counter_1+=0x10000000;
    SPI2.dma_out_link.val  = 0x10000000;  // Clear everything but the stop bit.
  }
  SPI2.dma_int_clr.val = isr_val;  // Blindly clear all interrupts.
  SPI2.dma_int_clr.val = 0x00000000;  // Blindly clear all interrupts.
}


static void IRAM_ATTR spi2_isr(void *arg) {
  if (SPI2.slave.trans_done) {
    SPIBusOp* tmp = _threaded_op;  // Concurrency "safety".
    if (tmp) {
      spi2_op_counter_0++;

      if (2 == tmp->transferParamLength()) {
        if (SPI2.slv_rd_bit.slv_rdata_bit >= SPI2.slv_rdbuf_dlen.bit_len) {
          // Internal CPLD register access doesn't use DMA, and expects us to
          // shuffle bytes around to avoid heaped buffers and DMA overhead.
          // buf+0/1 should be xfer_param[2/3].
          uint32_t word = SPI2.data_buf[0];
          *(tmp->buf+0) = (uint8_t) word & 0xFF;
          *(tmp->buf+1) = (uint8_t) (word >> 8) & 0xFF;
          tmp->markComplete();
        }
        else {
          // If the CPLD terminates the transaction before it finishes...
          tmp->abort(XferFault::DEV_FAULT);
        }
      }
      else {
        // This was a DMA transaction.
        bool enough_bits = false;
        if (BusOpcode::RX == tmp->get_opcode()) {
          enough_bits = (0x01000000 == (spi2_op_counter_1 & 0x01000000));
        }
        else {
          enough_bits = (0x10000000 == (spi2_op_counter_1 & 0x10000000));
        }

        if (enough_bits) {
          tmp->markComplete();
        }
        else {
          tmp->abort(XferFault::DEV_FAULT);
        }
      }
      _threaded_op = nullptr;
    }
    SPI2.slave.trans_done  = 0;  // Clear interrupt.
  }

  if (SPI2.slave.rd_sta_inten) {
    SPI2.slave.rd_sta_inten  = 0;  // Clear interrupt.
  }
  if (SPI2.slave.wr_sta_inten) {
    SPI2.slave.wr_sta_inten  = 0;  // Clear interrupt.
  }
  if (SPI2.slave.rd_buf_inten) {
    SPI2.slave.rd_buf_inten  = 0;  // Clear interrupt.
  }
  if (SPI2.slave.wr_buf_inten) {
    SPI2.slave.wr_buf_inten  = 0;  // Clear interrupt.
  }
}

#ifdef __cplusplus
}
#endif




/**
* Init of SPI2.
*/
void SSD1331::initSPI(uint8_t cpol, uint8_t cpha) {
  gpio_num_t p_cs    = (gpio_num_t) _opts.spi_cs;
  gpio_num_t p_clk   = (gpio_num_t) _opts.spi_clk;
  gpio_num_t p_mosi  = (gpio_num_t) _opts.spi_mosi;

  if (GPIO_IS_VALID_GPIO(p_cs) && GPIO_IS_VALID_GPIO(p_clk) && GPIO_IS_VALID_OUTPUT_GPIO(p_mosi)) {
    periph_module_enable(PERIPH_HSPI_MODULE);
    periph_module_enable(PERIPH_SPI_DMA_MODULE);

    SPI2.slave.slave_mode   = 1;  // CPLD drives the clock.
    SPI2.slave.wr_rd_buf_en = 1;  // We don't want any but buffer commands.
    SPI2.slave.cs_i_mode    = 2;  // Double-buffered CS signal.
    SPI2.slave.sync_reset   = 1;  // Reset the pins.
    //SPI2.slave.cmd_define   = 1;  // Use the custom slave command mode.

    SPI2.user.doutdin       = 1;  // Full-duplex. Needed to avoid command/status interpretation.
    SPI2.user.usr_mosi_highpart = 0;  // The RX buffer should use W0-7.
    SPI2.user.usr_miso_highpart = 1;  // The TX buffer should use W8-15.
    SPI2.user.usr_mosi      = 1;  // Enable RX shift register.
    SPI2.user.usr_miso      = 1;  // Enable TX shift register.
    // TODO: For some reason, these settings are required, or no clocks are recognized.
    SPI2.user.usr_command   = 1;
    SPI2.user1.val          = 0;  // No address phase.
    SPI2.cmd.usr = 1;
    SPI2.user2.usr_command_bitlen = 0;  // By doing this, we skip write to writing the buffer.
    SPI2.clock.clkcnt_l           = 0;  // Must be 0 in slave mode.
    SPI2.clock.clkcnt_h           = 0;  // Must be 0 in slave mode.
    SPI2.ctrl.fastrd_mode         = 0;  // No need of multi-lane SPI.
    SPI2.pin.ck_dis               = 1;  // We have no need of a clock output.
    SPI2.pin.ck_idle_edge         = cpol ? 1 : 0;  // CPOL

    /* NOTE: Apparently, there is a hardware bug that causes mode2 DMA
         transactions to fail.
    SPI2.user.ck_i_edge           = (cpol ^ cpha) ? 1 : 0;
    SPI2.ctrl2.mosi_delay_mode    = (cpol ^ cpha) ? 1 : 2;
    SPI2.ctrl2.miso_delay_mode    = 0;
    SPI2.ctrl2.miso_delay_num     = 0;
    SPI2.ctrl2.mosi_delay_num     = 0;
    */

    SPI2.user.ck_i_edge           = 0;
    SPI2.ctrl2.mosi_delay_mode    = 0;
    SPI2.ctrl2.miso_delay_mode    = 0;
    SPI2.ctrl2.miso_delay_num     = 2;
    SPI2.ctrl2.mosi_delay_num     = 3;


    SPI2.slave.trans_done         = 0;  // Clear txfr-done bit.
    SPI2.slave.trans_inten        = 1;  // Enable txfr-done interrupt.

    SPI2.dma_conf.out_eof_mode     = 1;  //
    SPI2.dma_conf.out_auto_wrback  = 1;  //
    //SPI2.dma_conf.outdscr_burst_en = 1;  // TX operations are bursted out of memory.
    SPI2.dma_conf.out_data_burst_en = 1;
    SPI2.dma_conf.indscr_burst_en   = 1;
    SPI2.dma_conf.outdscr_burst_en  = 1;

    reset_spi2_dma();
    for (int i = 0; i < 16; i++) SPI2.data_buf[i] = 0;

    SPI2.dma_int_ena.val  = (
      SPI_OUT_TOTAL_EOF_INT_ENA |
      //SPI_OUT_EOF_INT_ENA |
      //SPI_OUT_DONE_INT_ENA |
      //SPI_IN_DONE_INT_ENA |
      SPI_IN_SUC_EOF_INT_ENA |
      SPI_IN_ERR_EOF_INT_ENA |
      SPI_INLINK_DSCR_ERROR_INT_ENA |
      SPI_OUTLINK_DSCR_ERROR_INT_ENA |
      SPI_INLINK_DSCR_EMPTY_INT_ENA);

    DPORT_SET_PERI_REG_BITS(DPORT_SPI_DMA_CHAN_SEL_REG, 3, 1, 2);   // Point DMA channel to HSPI.

    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[p_cs],   PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[p_clk],  PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[p_mosi], PIN_FUNC_GPIO);

    gpio_set_direction(p_cs,   GPIO_MODE_OUTPUT);
    gpio_set_direction(p_clk,  GPIO_MODE_OUTPUT);
    gpio_set_direction(p_mosi, GPIO_MODE_OUTPUT);

    gpio_matrix_out( p_cs,   HSPIQ_OUT_IDX,  false, false);
    gpio_matrix_out( p_clk,  HSPIQ_OUT_IDX,  false, false);
    gpio_matrix_out( p_mosi, HSPIQ_OUT_IDX,  false, false);

    esp_intr_alloc(ETS_SPI2_INTR_SOURCE, ESP_INTR_FLAG_IRAM, spi2_isr, nullptr, nullptr);
    esp_intr_alloc(ETS_SPI2_DMA_INTR_SOURCE, ESP_INTR_FLAG_IRAM, dma_isr, nullptr, nullptr);
  }
}






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
SSD1331::SSD1331(const SSD1331Opts* o) : Image(96, 64, ImgBufferFormat::R5_G6_B5), BusAdapter(SSD1331_MAX_Q_DEPTH), _opts(o) {
}


/**
* Destructor. Should never be called.
*/
SSD1331::~SSD1331() {
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t SSD1331::io_op_callahead(BusOp* _op) {
  // Bus adapters don't typically do anything here, other
  //   than permit the transfer.
  return 0;
}

/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return SPI_CALLBACK_NOMINAL on success, or appropriate error code.
*/
int8_t SSD1331::io_op_callback(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;

  // There is zero chance this object will be a null pointer unless it was done on purpose.
  if (op->hasFault()) {
    Kernel::log("SSD1331::io_op_callback() rejected a callback because the bus op failed.\n");
    return SPI_CALLBACK_ERROR;
  }

  return SPI_CALLBACK_NOMINAL;
}


/**
* This is what is called when the class wants to conduct a transaction on the bus.
* Note that this is different from other class implementations, in that it checks for
*   callback population before clobbering it. This is because this class is also the
*   SPI driver. This might end up being reworked later.
*
* @param  _op  The bus operation to execute.
* @return Zero on success, or appropriate error code.
*/
int8_t SSD1331::queue_io_job(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;

  if (op) {
    if (nullptr == op->callback) {
      op->callback = (BusOpCallback*) this;
    }

    if (op->callback == (BusOpCallback*) this) {
      op->profile(true);
    }

    if (op->get_state() != XferState::IDLE) {
      Kernel::log("Tried to fire a bus op that is not in IDLE state.\n");
      return -4;
    }
    op->csActiveHigh(true);   // We enfoce this here to prevent having to
    op->setCSPin(_opts.spi_cs);  //   enforce it in many places.

    if ((nullptr == current_job) && (work_queue.size() == 0)){
      // If the queue is empty, fire the operation now.
      current_job = op;
      advance_work_queue();
    }
    else {    // If there is something already in progress, queue up.
      if (SSD1331_MAX_Q_DEPTH <= work_queue.size()) {
        Kernel::log("SSD1331::queue_io_job(): \t Bus queue at max size. Dropping transaction.\n");
        op->abort(XferFault::QUEUE_FLUSH);
        reclaim_queue_item(op);
        return -1;
      }

      if (0 > work_queue.insertIfAbsent(op)) {
        StringBuilder log("SSD1331::queue_io_job(): \t Double-insertion. Dropping transaction with no status change.\n");
        op->printDebug(&log);
        Kernel::log(&log);
        return -3;
      }
    }
    return 0;
  }
  return -5;
}


/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

int8_t SSD1331::bus_init() {
  initSPI(0, 0);
  return 0;
}

int8_t SSD1331::bus_deinit() {
  return 0;
}


/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t SSD1331::advance_work_queue() {
  int8_t return_value = 0;

  if (current_job) {
    switch (current_job->get_state()) {
      case XferState::TX_WAIT:
      case XferState::RX_WAIT:
        if (current_job->hasFault()) {
          _failed_xfers++;
          Kernel::log("SSD1331::advance_work_queue():\t Failed at IO_WAIT.\n");
        }
        else {
          current_job->markComplete();
        }
        // No break on purpose.
      case XferState::COMPLETE:
        _total_xfers++;
        reclaim_queue_item(current_job);
        current_job = nullptr;
        break;

      case XferState::IDLE:
      case XferState::INITIATE:
        switch (current_job->begin()) {
          case XferFault::NONE:     // Nominal outcome. Transfer started with no problens...
            break;
          case XferFault::BUS_BUSY:    // Bus appears to be in-use. State did not change.
            // Re-throw queue_ready event and try again later.
            Kernel::log("SPI advance_work_queue() tried to clobber an existing transfer on chain.\n");
            //Kernel::staticRaiseEvent(&event_spi_queue_ready);  // Bypass our method. Jump right to the target.
            break;
          default:    // Began the transfer, and it barffed... was aborted.
            Kernel::log("SSD1331::advance_work_queue():\t Failed to begin transfer after starting.\n");
            reclaim_queue_item(current_job);
            current_job = nullptr;
            break;
        }
        break;

      /* Cases below ought to be handled by ISR flow... */
      case XferState::ADDR:
        //Kernel::log("####################### advance_operation entry\n");
        //current_job->advance_operation(0, 0);
        //local_log.concat("####################### advance_operation exit\n");
        //current_job->printDebug(&local_log);
        //flushLocalLog();
        break;
      case XferState::STOP:
        Kernel::log("SPI State might be corrupted if we tried to advance_queue(). \n");
        break;
      default:
        Kernel::log("SPI advance_work_queue() default state \n");
        break;
    }
  }

  if (current_job == nullptr) {
    current_job = work_queue.dequeue();
    // Begin the bus operation.
    if (current_job) {
      if (XferFault::NONE != current_job->begin()) {
        Kernel::log("SPI advance_work_queue() tried to clobber an existing transfer on the pick-up.\n");
        Kernel::staticRaiseEvent(&SPIBusOp::event_spi_queue_ready);  // Bypass our method. Jump right to the target.
      }
      return_value++;
    }
    else {
      // No Queue! Relax...
    }
  }

  return return_value;
}


/**
* Purges only the work_queue. Leaves the currently-executing job.
*/
void SSD1331::purge_queued_work() {
  SPIBusOp* current = nullptr;
  while (work_queue.hasNext()) {
    current = work_queue.dequeue();
    current->abort(XferFault::QUEUE_FLUSH);
    reclaim_queue_item(current);
  }

  // Check this last to head off any silliness with bus operations colliding with us.
  purge_stalled_job();
}


/**
* Purges a stalled job from the active slot.
*/
void SSD1331::purge_stalled_job() {
  if (current_job) {
    current_job->abort(XferFault::QUEUE_FLUSH);
    reclaim_queue_item(current_job);
    current_job = nullptr;
  }
}


/**
* Return a vacant SPIBusOp to the caller, allocating if necessary.
*
* @return an SPIBusOp to be used. Only NULL if out-of-mem.
*/
SPIBusOp* SSD1331::new_op() {
  return (SPIBusOp*) BusAdapter::new_op();
}


/**
* Return a vacant SPIBusOp to the caller, allocating if necessary.
*
* @param  _op   The device pointer that owns jobs we wish purged.
* @param  _req  The device pointer that owns jobs we wish purged.
* @return an SPIBusOp to be used. Only NULL if out-of-mem.
*/
SPIBusOp* SSD1331::new_op(BusOpcode _op, BusOpCallback* _req) {
  SPIBusOp* return_value = new_op();
  return_value->set_opcode(_op);
  return_value->callback = _req;
  return return_value;
}


/**
* This fxn will either free() the memory associated with the SPIBusOp object, or it
*   will return it to the preallocation queue.
*
* @param item The SPIBusOp to be reclaimed.
*/
void SSD1331::reclaim_queue_item(SPIBusOp* op) {
  if (op->hasFault()) {    // Print failures.
    StringBuilder log;
    op->printDebug(&log);
    Kernel::log(&log);
  }

  if (op->returnToPrealloc()) {
    //if (getVerbosity() > 6) local_log.concatf("SSD1331::reclaim_queue_item(): \t About to wipe.\n");
    BusAdapter::return_op_to_pool(op);
  }
  else if (op->shouldReap()) {
    //if (getVerbosity() > 6) local_log.concatf("SSD1331::reclaim_queue_item(): \t About to reap.\n");
    delete op;
    _heap_frees++;
  }
  else {
    /* If we are here, it must mean that some other class fed us a const SPIBusOp,
       and wants us to ignore the memory cleanup. But we should at least set it
       back to IDLE.*/
    //if (getVerbosity() > 6) local_log.concatf("SSD1331::reclaim_queue_item(): \t Dropping....\n");
    op->set_state(XferState::IDLE);
  }
}











/*!
  @brief   SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
void SSD1331::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint8_t x1 = x;
  uint8_t y1 = y;
  if (x1 > 95) x1 = 95;
  if (y1 > 63) y1 = 63;

  uint8_t x2 = (x+w-1);
  uint8_t y2 = (y+h-1);
  if (x2 > 95) x2 = 95;
  if (y2 > 63) y2 = 63;

  if (x1 > x2) {
    uint8_t t = x2;
    x2 = x1;
    x1 = t;
  }
  if (y1 > y2) {
    uint8_t t = y2;
    y2 = y1;
    y1 = t;
  }

  _send_command(0x15); // Column addr set
  _send_command(x1);
  _send_command(x2);

  _send_command(0x75); // Column addr set
  _send_command(y1);
  _send_command(y2);

  //startWrite();
}


/**************************************************************************/
/*!
    @brief   Initialize SSD1331 chip
    Connects to the SSD1331 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/
void SSD1331::begin(uint32_t freq) {
    initSPI(0, 0);

    // Initialization Sequence
    _send_command(SSD1331_CMD_DISPLAYOFF);    // 0xAE
    _send_command(SSD1331_CMD_SETREMAP);   // 0xA0
    _send_command(0x72);        // RGB Color
    _send_command(SSD1331_CMD_STARTLINE);   // 0xA1
    _send_command(0x0);
    _send_command(SSD1331_CMD_DISPLAYOFFSET);   // 0xA2
    _send_command(0x0);
    _send_command(SSD1331_CMD_NORMALDISPLAY);    // 0xA4
    _send_command(SSD1331_CMD_SETMULTIPLEX);   // 0xA8
    _send_command(0x3F);        // 0x3F 1/64 duty
    _send_command(SSD1331_CMD_SETMASTER);    // 0xAD
    _send_command(0x8E);
    _send_command(SSD1331_CMD_POWERMODE);    // 0xB0
    _send_command(0x0B);
    _send_command(SSD1331_CMD_PRECHARGE);    // 0xB1
    _send_command(0x31);
    _send_command(SSD1331_CMD_CLOCKDIV);    // 0xB3
    _send_command(0xF0);  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    _send_command(SSD1331_CMD_PRECHARGEA);    // 0x8A
    _send_command(0x64);
    _send_command(SSD1331_CMD_PRECHARGEB);    // 0x8B
    _send_command(0x78);
    _send_command(SSD1331_CMD_PRECHARGEC);    // 0x8C
    _send_command(0x64);
    _send_command(SSD1331_CMD_PRECHARGELEVEL);    // 0xBB
    _send_command(0x3A);
    _send_command(SSD1331_CMD_VCOMH);      // 0xBE
    _send_command(0x3E);
    _send_command(SSD1331_CMD_MASTERCURRENT);    // 0x87
    _send_command(0x06);
    _send_command(SSD1331_CMD_CONTRASTA);    // 0x81
    _send_command(0x91);
    _send_command(SSD1331_CMD_CONTRASTB);    // 0x82
    _send_command(0x50);
    _send_command(SSD1331_CMD_CONTRASTC);    // 0x83
    _send_command(0x7D);
    _send_command(SSD1331_CMD_DISPLAYON);  //--turn on oled panel
}



/*!
  @brief  Change whether display is on or off
  @param   enable True if you want the display ON, false OFF
*/
void SSD1331::enableDisplay(bool enable) {
  _send_command(enable ? SSD1331_CMD_DISPLAYON : SSD1331_CMD_DISPLAYOFF);
}


/*!
 @brief   Adafruit_SPITFT Send Command handles complete sending of commands and data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */
int8_t SSD1331::_send_command(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes) {
  //SPI_BEGIN_TRANSACTION();
  setPin(_opts.spi_cs, false);

  setPin(_opts.dc, false);
  //spiWrite(commandByte); // Send the command byte
  setPin(_opts.dc, true);

  for (int i=0; i<numDataBytes; i++) {
    //spiWrite(*dataBytes); // Send the data bytes
    dataBytes++;
  }

  setPin(_opts.spi_cs, true);
  //SPI_END_TRANSACTION();
  return 0;
}





/*******************************************************************************
* ___     _                              These members are mandatory overrides
*  |   / / \ o     |  _  |_              from the BusOp class.
* _|_ /  \_/ o   \_| (_) |_)
*******************************************************************************/
/**
* Calling this member will cause the bus operation to be started.
*
* @return 0 on success, or non-zero on failure.
*/
XferFault SPIBusOp::begin() {
  //time_began    = micros();

  // TODO: This should be safe to cut, but need to run more abuse tests under timing
  //   permutations. Would really prefer to avoid explicit thread-safety overhead,
  //   since this might be called at many hundreds of hz.
  if (_threaded_op) {
    abort(XferFault::BUS_BUSY);
    return XferFault::BUS_BUSY;
  }

  switch (_param_len) {  // Length dictates our transfer setup.
    case 4:
    case 2:
      if (csAsserted()) {
        abort(XferFault::BUS_BUSY);
        return XferFault::BUS_BUSY;
      }
      break;

    default:
      abort(XferFault::BAD_PARAM);
      return XferFault::BAD_PARAM;
  }

  set_state(XferState::INITIATE);  // Indicate that we now have bus control.
  _threaded_op = this;
  reset_spi2_dma();
  spi2_op_counter_1 = 0;
  SPI2.data_buf[0] = 0;
  SPI2.data_buf[8] = 0;
  SPI2.slv_rd_bit.slv_rdata_bit = 0;


  SPI2.user.usr_mosi = 1;  // We turn on the RX/TX shift-registers.
  SPI2.user.usr_miso = 1;
  if (2 == _param_len) {
    // Two parameters means an internal CPLD access. We always want the
    //   Rx SR enabled to get the version and status that comes back in
    //   the first two bytes.
    SPI2.data_buf[8] = xfer_params[0] + (xfer_params[1] << 8);
    if (0 == buf_len) {
      // We can afford to read two bytes into the same space as our xfer_params,
      // We don't need DMA. Load the transfer parameters into the FIFO.
      // LSB will be first over the bus.
      buf = &xfer_params[2];  // Careful....
      buf_len = 2;
      set_state(opcode == BusOpcode::TX ? XferState::TX_WAIT : XferState::RX_WAIT);
    }
    else {
      set_state(XferState::ADDR);
    }
    SPI2.slave.sync_reset = 1;
    SPI2.slave.sync_reset = 0;
    SPI2.slv_rdbuf_dlen.bit_len = 15;
    SPI2.slv_wrbuf_dlen.bit_len = 15;
  }
  else {
    set_state(XferState::ADDR);
    // This will be a DMA transfer. Code it as two linked lists with the initial
    //   4 bytes from the read-side of the transfer consigned to a bit-bucket.
    //   The first two bytes will contain the version and conf as always, but
    //   IMU traffic should not be bothered with validating this.

    // NOTE: Over-running a buffer on purpose is very dangerous. We rely on
    //   input buffers being both 4-byte aligned, and padded to 4-bytes.
    unsigned int bits_to_xfer = (32 + (((3 + buf_len) & (~3)) << 3))-1;
    SPI2.dma_conf.out_eof_mode  = 0;

    // For a transmission, the buffer will contain outbound data.
    // There is no RX component here, so we can simply load in the list.
    _ll_tx.length = (3 + buf_len) & (~3);
    _ll_tx.size   = (3 + buf_len) & (~3);
    _ll_tx.owner  = LLDESC_HW_OWNED;
    _ll_tx.sosf   = 0;
    _ll_tx.offset = 0;
    _ll_tx.empty  = 0;
    _ll_tx.eof    = 1;
    _ll_tx.buf    = buf;

    switch (opcode) {
      case BusOpcode::TX:
        break;

      case BusOpcode::TX_CMD:
        break;

      case BusOpcode::RX:
         break;

      default:
        break;
    }
    SPI2.dma_out_link.addr  = ((uint32_t) &_ll_tx) & LLDESC_ADDR_MASK;
    SPI2.dma_out_link.start = 1;  // Signify DMA readiness.

    SPI2.slave.sync_reset = 1;
    SPI2.slave.sync_reset = 0;
    SPI2.slv_wrbuf_dlen.bit_len = bits_to_xfer;
    SPI2.slv_rdbuf_dlen.bit_len = bits_to_xfer;
  }
  SPI2.cmd.usr = 1;  // Start the transfer.
  // NOTE: REQ is a clock signal. We can dis-assert immediately, and the transfer
  //   will still proceed. If that should ever be convenient.
  _assert_cs(true);
  return XferFault::NONE;
}


/**
* Called from the ISR to advance this operation on the bus.
* Stay brief. We are in an ISR.
*
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::advance_operation(uint32_t status_reg, uint8_t data_reg) {
  /* These are our transfer-size-invariant cases. */
  switch (xfer_state) {
    case XferState::COMPLETE:
      abort(XferFault::HUNG_IRQ);
      return 0;

    case XferState::TX_WAIT:
    case XferState::RX_WAIT:
      //markComplete();
      return 0;

    case XferState::FAULT:
      return -1;

    case XferState::ADDR:
      if (buf_len > 0) {
        if (opcode == BusOpcode::TX) {
          set_state(XferState::TX_WAIT);
        }
        else {
          set_state(XferState::RX_WAIT);
        }
      }
      return 0;

    case XferState::QUEUED:
    case XferState::STOP:
    case XferState::UNDEF:

    /* Below are the states that we shouldn't be in at this point... */
    case XferState::INITIATE:
    case XferState::IDLE:
      abort(XferFault::ILLEGAL_STATE);
      break;
  }

  return -1;
}

#endif   // CONFIG_MANUVR_SSD1331
