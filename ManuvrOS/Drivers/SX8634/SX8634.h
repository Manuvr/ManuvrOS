/* Test driver for MCP3918 */
#include <inttypes.h>
#include <stdint.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#ifndef __SX8634_DRIVER_H__
#define __SX8634_DRIVER_H__

// These are the register definitions.
#define SX8634_REG_RESERVED_00              0x00
#define SX8634_REG_RESERVED_01              0x01
#define SX8634_REG_RESERVED_02              0x02
#define SX8634_REG_RESERVED_03              0x03
#define SX8634_REG_I2C_ADDR                 0x04
#define SX8634_REG_ACTIVE_SCAN_PERIOD       0x05
#define SX8634_REG_DOZE_SCAN_PERIOD         0x06
#define SX8634_REG_PASSIVE_TIMER            0x07
#define SX8634_REG_RESERVED_04              0x08
#define SX8634_REG_CAP_MODE_MISC            0x09
#define SX8634_REG_CAP_MODE_11_8            0x0A
#define SX8634_REG_CAP_MODE_7_4             0x0B
#define SX8634_REG_CAP_MODE_3_0             0x0C
#define SX8634_REG_CAP_SNSITIVTY_0_1        0x0D
#define SX8634_REG_CAP_SNSITIVTY_2_3        0x0E
#define SX8634_REG_CAP_SNSITIVTY_4_5        0x0F
#define SX8634_REG_CAP_SNSITIVTY_6_7        0x10
#define SX8634_REG_CAP_SNSITIVTY_8_9        0x11
#define SX8634_REG_CAP_SNSITIVTY_10_11      0x12
#define SX8634_REG_CAP_THRESH_0             0x13
#define SX8634_REG_CAP_THRESH_1             0x14
#define SX8634_REG_CAP_THRESH_2             0x15
#define SX8634_REG_CAP_THRESH_3             0x16
#define SX8634_REG_CAP_THRESH_4             0x17
#define SX8634_REG_CAP_THRESH_5             0x18
#define SX8634_REG_CAP_THRESH_6             0x19
#define SX8634_REG_CAP_THRESH_7             0x1A
#define SX8634_REG_CAP_THRESH_8             0x1B
#define SX8634_REG_CAP_THRESH_9             0x1C
#define SX8634_REG_CAP_THRESH_10            0x1D
#define SX8634_REG_CAP_THRESH_11            0x1E
#define SX8634_REG_CAP_PER_COMP             0x1F
#define SX8634_REG_RESERVED_05              0x20
#define SX8634_REG_BTN_CONFIG               0x21
#define SX8634_REG_BTN_AVG_THRESH           0x22
#define SX8634_REG_BTN_COMP_NEG_THRESH      0x23
#define SX8634_REG_BTN_COMP_NEG_CNT_MAX     0x24
#define SX8634_REG_BTN_HYSTERESIS           0x25
#define SX8634_REG_BTN_STUCK_AT_TIMEOUT     0x26
#define SX8634_REG_SLD_SLD_CONFIG           0x27
#define SX8634_REG_SLD_STUCK_AT_TIMEOUT     0x28
#define SX8634_REG_SLD_HYSTERESIS           0x29
#define SX8634_REG_RESERVED_06              0x2A
#define SX8634_REG_SLD_NORM_MSB             0x2B
#define SX8634_REG_SLD_NORM_LSB             0x2C
#define SX8634_REG_SLD_AVG_THRESHOLD        0x2D
#define SX8634_REG_SLD_COMP_NEG_THRESH      0x2E
#define SX8634_REG_SLD_COMP_NEG_CNT_MAX     0x2F
#define SX8634_REG_SLD_MOVE_THRESHOLD       0x30
#define SX8634_REG_RESERVED_07              0x31
#define SX8634_REG_RESERVED_08              0x32
#define SX8634_REG_MAP_WAKEUP_SIZE          0x33
#define SX8634_REG_MAP_WAKEUP_VALUE_0       0x34
#define SX8634_REG_MAP_WAKEUP_VALUE_1       0x35
#define SX8634_REG_MAP_WAKEUP_VALUE_2       0x36
#define SX8634_REG_MAP_AUTOLIGHT_0          0x37
#define SX8634_REG_MAP_AUTOLIGHT_1          0x38
#define SX8634_REG_MAP_AUTOLIGHT_2          0x39
#define SX8634_REG_MAP_AUTOLIGHT_3          0x3A
#define SX8634_REG_MAP_AUTOLIGHT_GRP_0_MSB  0x3B
#define SX8634_REG_MAP_AUTOLIGHT_GRP_0_LSB  0x3C
#define SX8634_REG_MAP_AUTOLIGHT_GRP_1_MSB  0x3D
#define SX8634_REG_MAP_AUTOLIGHT_GRP_1_LSB  0x3E
#define SX8634_REG_MAP_SEGMENT_HYSTERESIS   0x3F
#define SX8634_REG_GPIO_7_4                 0x40
#define SX8634_REG_GPIO_3_0                 0x41
#define SX8634_REG_GPIO_OUT_PWR_UP          0x42
#define SX8634_REG_GPIO_AUTOLIGHT           0x43
#define SX8634_REG_GPIO_POLARITY            0x44
#define SX8634_REG_GPIO_INTENSITY_ON_0      0x45
#define SX8634_REG_GPIO_INTENSITY_ON_1      0x46
#define SX8634_REG_GPIO_INTENSITY_ON_2      0x47
#define SX8634_REG_GPIO_INTENSITY_ON_3      0x48
#define SX8634_REG_GPIO_INTENSITY_ON_4      0x49
#define SX8634_REG_GPIO_INTENSITY_ON_5      0x4A
#define SX8634_REG_GPIO_INTENSITY_ON_6      0x4B
#define SX8634_REG_GPIO_INTENSITY_ON_7      0x4C
#define SX8634_REG_GPIO_INTENSITY_OFF_0     0x4D
#define SX8634_REG_GPIO_INTENSITY_OFF_1     0x4E
#define SX8634_REG_GPIO_INTENSITY_OFF_2     0x4F
#define SX8634_REG_GPIO_INTENSITY_OFF_3     0x50
#define SX8634_REG_GPIO_INTENSITY_OFF_4     0x51
#define SX8634_REG_GPIO_INTENSITY_OFF_5     0x52
#define SX8634_REG_GPIO_INTENSITY_OFF_6     0x53
#define SX8634_REG_GPIO_INTENSITY_OFF_7     0x54
#define SX8634_REG_RESERVED_09              0x55
#define SX8634_REG_GPIO_FUNCTION            0x56
#define SX8634_REG_GPIO_INC_FACTOR          0x57
#define SX8634_REG_GPIO_DEC_FACTOR          0x58
#define SX8634_REG_GPIO_INC_TIME_7_6        0x59
#define SX8634_REG_GPIO_INC_TIME_5_4        0x5A
#define SX8634_REG_GPIO_INC_TIME_3_2        0x5B
#define SX8634_REG_GPIO_INC_TIME_1_0        0x5C
#define SX8634_REG_GPIO_DEC_TIME_7_6        0x5D
#define SX8634_REG_GPIO_DEC_TIME_5_4        0x5E
#define SX8634_REG_GPIO_DEC_TIME_3_2        0x5F
#define SX8634_REG_GPIO_DEC_TIME_1_0        0x60
#define SX8634_REG_GPIO_OFF_DELAY_7_6       0x61
#define SX8634_REG_GPIO_OFF_DELAY_5_4       0x62
#define SX8634_REG_GPIO_OFF_DELAY_3_2       0x63
#define SX8634_REG_GPIO_OFF_DELAY_1_0       0x64
#define SX8634_REG_GPIO_PULL_UP_DOWN_7_4    0x65
#define SX8634_REG_GPIO_PULL_UP_DOWN_3_0    0x66
#define SX8634_REG_GPIO_INTERRUPT_7_4       0x67
#define SX8634_REG_GPIO_INTERRUPT_3_0       0x68
#define SX8634_REG_GPIO_DEBOUNCE            0x69
#define SX8634_REG_RESERVED_0A              0x6A
#define SX8634_REG_RESERVED_0B              0x6B
#define SX8634_REG_RESERVED_0C              0x6C
#define SX8634_REG_RESERVED_0D              0x6D
#define SX8634_REG_RESERVED_0E              0x6E
#define SX8634_REG_RESERVED_0F              0x6F
#define SX8634_REG_CAP_PROX_ENABLE          0x70
#define SX8634_REG_RESERVED_10              0x71
#define SX8634_REG_RESERVED_11              0x72
#define SX8634_REG_RESERVED_12              0x73
#define SX8634_REG_RESERVED_13              0x74
#define SX8634_REG_RESERVED_14              0x75
#define SX8634_REG_RESERVED_15              0x76
#define SX8634_REG_RESERVED_16              0x77
#define SX8634_REG_RESERVED_17              0x78
#define SX8634_REG_RESERVED_18              0x79
#define SX8634_REG_RESERVED_19              0x7A
#define SX8634_REG_RESERVED_1A              0x7B
#define SX8634_REG_RESERVED_1B              0x7C
#define SX8634_REG_RESERVED_1C              0x7D
#define SX8634_REG_RESERVED_1D              0x7E
#define SX8634_REG_SPM_CRC                  0x7F


#define SX8634_FLAG_SM_MASK          0x07
#define SX8634_FLAG_SM_NO_INIT       0x01
#define SX8634_FLAG_SM_SPM_READ      0x02
#define SX8634_FLAG_SM_CONF_WRITE    0x03
#define SX8634_FLAG_SM_READY         0x04
#define SX8634_FLAG_SM_NVM_BURN0     0x05
#define SX8634_FLAG_SM_NVM_BURN1     0x06
#define SX8634_FLAG_SM_NVM_BURN2     0x07


#define SX8634_DEFAULT_I2C_ADDR      0x2B


enum class SX8634OpMode : uint8_t {
  ACTIVE      = 0,
  DOZE        = 1,
  SLEEP       = 2
};

enum class SX8634GPIOMode : uint8_t {
  OUTPUT      = 0,
  PWM         = 1,
  INPUT       = 2
};


class SX8634GPIOConf {
  public:

  private:
};


/*******************************************************************************
* Options object
*******************************************************************************/

/**
* Set pin def to 255 to mark it as unused.
*/
class SX8634Opts {
  public:
    const uint8_t i2c_addr;
    const uint8_t reset_pin;
    const uint8_t irq_pin;

    /** Copy constructor. */
    SX8634Opts(const SX8634Opts* o) :
      i2c_addr(o->i2c_addr),
      reset_pin(o->reset_pin),
      irq_pin(o->irq_pin),
      _flags(o->_flags) {};

    /**
    * Constructor.
    *
    * @param reset pin
    * @param irq pin
    * @param Initial flags
    */
    SX8634Opts(
      uint8_t addr,
      uint8_t reset,
      uint8_t irq
    ) :
      i2c_addr(addr),
      reset_pin(reset),
      irq_pin(irq),
      _flags(0) {};


  private:
    const uint8_t _flags;
};



/*******************************************************************************
*
*******************************************************************************/


class SX8634 : public I2CDevice {
  public:
    SX8634(const SX8634Opts*);
    ~SX8634();

    int8_t  reset();
    int8_t  init();

    bool buttonPressed(uint8_t);
    bool buttonReleased(uint8_t);

    //
    int8_t  setGPIOState(uint8_t pin, uint8_t value);
    uint8_t getGPIOState(uint8_t pin);



  private:
    const SX8634Opts _opts;

    uint8_t  _registers[128];    // Register shadows
    uint8_t  _io_buffer[128];    // I/O operation buffer

    int8_t  _clear_registers();
    int8_t  _read_full_spm();    // Mirror the SPM into our shadow.
    int8_t  _ll_pin_init();
};

#endif  // __SX8634_DRIVER_H__
