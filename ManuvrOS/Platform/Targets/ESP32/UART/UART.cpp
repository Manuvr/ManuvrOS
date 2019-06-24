#include <Platform/Platform.h>
#include <Platform/Peripherals/UART/UART.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/queue.h"

static const char* TAG = "uart_drvr";
QueueHandle_t uart_queues[]  = {nullptr, nullptr, nullptr};
ManuvrUART* uart_instances[] = {nullptr, nullptr, nullptr};



static void uart_event_task(void *pvParameters) {
    uart_event_t event;
    for (;;) {
      for (uint8_t port_idx = 0; port_idx < 3; port_idx++) {
        if (nullptr != uart_queues[port_idx]) {
          //Waiting for UART event.
          if(xQueueReceive(uart_queues[port_idx], (void * )&event, (portTickType)portMAX_DELAY)) {
            switch (event.type) {
              //Event of UART receving data
              case UART_DATA:
                {
                  size_t dlen = 0;
                  if (ESP_OK == uart_get_buffered_data_len((uart_port_t) port_idx, &dlen)) {
                    if (dlen > 0) {
                      uint8_t dtmp[dlen + 4];
                      bzero(dtmp, dlen + 4);
                      int rlen = uart_read_bytes((uart_port_t) port_idx, dtmp, dlen, portMAX_DELAY);
                      if (rlen > 0) {
                        uart_instances[port_idx]->fromCounterparty(dtmp, dlen, MEM_MGMT_RESPONSIBLE_BEARER);
                      }
                    }
                  }
                }
                break;

              //Event of HW FIFO overflow detected
              case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input((uart_port_t) port_idx);
                xQueueReset(uart_queues[port_idx]);
                break;
              //Event of UART ring buffer full
              case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input((uart_port_t) port_idx);
                xQueueReset(uart_queues[port_idx]);
                break;
              //Event of UART RX break detected
              case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
              //Event of UART parity check error
              case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
              //Event of UART frame error
              case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
              case UART_DATA_BREAK:
                  ESP_LOGI(TAG, "uart data break");
                  break;
              case UART_EVENT_MAX:
                  ESP_LOGI(TAG, "uart event max");
                  break;
              case UART_PATTERN_DET:
                //{
                //  uart_get_buffered_data_len((uart_port_t) port_idx, &buffered_size);
                //  int pos = uart_pattern_pop_pos((uart_port_t) port_idx);
                //  ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                //  if (pos == -1) {
                //      // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                //      // record the position. We should set a larger queue size.
                //      // As an example, we directly flush the rx buffer here.
                //      uart_flush_input((uart_port_t) port_idx);
                //  } else {
                //      uart_read_bytes((uart_port_t) port_idx, dtmp, pos, 100 / portTICK_PERIOD_MS);
                //      uint8_t pat[PATTERN_CHR_NUM + 1];
                //      memset(pat, 0, sizeof(pat));
                //      uart_read_bytes((uart_port_t) port_idx, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                //      ESP_LOGI(TAG, "read data: %s", dtmp);
                //      ESP_LOGI(TAG, "read pat : %s", pat);
                //  }
                //}
                break;
              //Others
              default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
          }
        }
      }
    }
    vTaskDelete(NULL);
}

#ifdef __cplusplus
}
#endif



int8_t ManuvrUART::_pf_conf() {
  if (_opts.port < 3) {
    if (nullptr == uart_queues[_opts.port]) {
      uart_word_length_t     databits;
      uart_parity_t          par_bits;
      uart_stop_bits_t       stp_bits;
      uart_hw_flowcontrol_t  flowctrl;
      bool spawn_thread = !((nullptr != uart_queues[0]) || (nullptr != uart_queues[1]) || (nullptr != uart_queues[2]));

      switch (_opts.stop_bits) {
        default:
        case UARTStopBit::STOP_1:    stp_bits = UART_STOP_BITS_1;     break;
        case UARTStopBit::STOP_1_5:  stp_bits = UART_STOP_BITS_1_5;   break;
        case UARTStopBit::STOP_2:    stp_bits = UART_STOP_BITS_2;     break;
      }
      switch (_opts.parity) {
        default:
        case UARTParityBit::NONE:    par_bits = UART_PARITY_DISABLE;  break;
        case UARTParityBit::EVEN:    par_bits = UART_PARITY_EVEN;     break;
        case UARTParityBit::ODD:     par_bits = UART_PARITY_ODD;      break;
      }
      switch (_opts.flow_control) {
        default:                           // These are software ideas. Hardware sees no flowcontrol.
        case UARTFlowControl::XON_XOFF_R:  // These are software ideas. Hardware sees no flowcontrol.
        case UARTFlowControl::XON_XOFF_T:  // These are software ideas. Hardware sees no flowcontrol.
        case UARTFlowControl::XON_XOFF_RT: // These are software ideas. Hardware sees no flowcontrol.
        case UARTFlowControl::NONE:         flowctrl = UART_HW_FLOWCTRL_DISABLE;  break;
        case UARTFlowControl::RTS:          flowctrl = UART_HW_FLOWCTRL_RTS;      break;
        case UARTFlowControl::CTS:          flowctrl = UART_HW_FLOWCTRL_CTS;      break;
        case UARTFlowControl::RTS_CTS:      flowctrl = UART_HW_FLOWCTRL_CTS_RTS;  break;
      }
      switch (_opts.bit_per_word) {
        default:
        case 8:  databits = UART_DATA_8_BITS;  break;
        case 7:  databits = UART_DATA_7_BITS;  break;
        case 6:  databits = UART_DATA_6_BITS;  break;
        case 5:  databits = UART_DATA_5_BITS;  break;
      }
      uart_config_t uart_config = {
        .baud_rate = (int) _opts.bitrate,
        .data_bits = databits,
        .parity = par_bits,
        .stop_bits = stp_bits,
        .flow_ctrl = flowctrl,
        .rx_flow_ctrl_thresh = 122,  // The FIFO is 128 bytes (I think?)
        .use_ref_tick = true
      };
      if (ESP_OK == uart_param_config((uart_port_t) _opts.port, &uart_config)) {
        unsigned int rxpin  = (255 == _opts.rx_pin)  ? UART_PIN_NO_CHANGE : _opts.rx_pin;
        unsigned int txpin  = (255 == _opts.tx_pin)  ? UART_PIN_NO_CHANGE : _opts.tx_pin;
        unsigned int rtspin = (255 == _opts.rts_pin) ? UART_PIN_NO_CHANGE : _opts.rts_pin;
        unsigned int ctspin = (255 == _opts.cts_pin) ? UART_PIN_NO_CHANGE : _opts.cts_pin;
        if (ESP_OK == uart_set_pin((uart_port_t) _opts.port, txpin, rxpin, rtspin, ctspin)) {
          uart_queues[_opts.port] = (QueueHandle_t) malloc(sizeof(QueueHandle_t));
          if (nullptr != uart_queues[_opts.port]) {
            uart_instances[_opts.port] = this;
            if (ESP_OK == uart_driver_install((uart_port_t) _opts.port, 256, 256, 10, &uart_queues[_opts.port], 0)) {
              if (spawn_thread) {
                xTaskCreate(uart_event_task, "uart_task", 3000, NULL, 12, NULL);
              }
              return 0;
            }
          }
        }
      }
    }
  }
  return -1;
}



int8_t ManuvrUART::_pf_write() {
  return 0;
}


int8_t ManuvrUART::_pf_read() {
  return 0;
}
