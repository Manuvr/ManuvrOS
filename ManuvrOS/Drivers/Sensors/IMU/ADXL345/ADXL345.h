#include <Drivers/Sensors/SensorWrapper.h>
#include <Platform/Peripherals/I2C/I2CDeviceWithRegisters.h>

#ifndef __ADXL345_H__
#define __ADXL345_H__

#define ADXL345_REGISTER_DEVID           0x00
#define ADXL345_REGISTER_THRESH_TAP      0x1D
#define ADXL345_REGISTER_OFSX            0x1E
#define ADXL345_REGISTER_OFSY            0x1F
#define ADXL345_REGISTER_OFSZ            0x20
#define ADXL345_REGISTER_DUR             0x21
#define ADXL345_REGISTER_LATENT          0x22
#define ADXL345_REGISTER_WINDOW          0x23
#define ADXL345_REGISTER_THRESH_ACT      0x24
#define ADXL345_REGISTER_THRESH_INACT    0x25
#define ADXL345_REGISTER_TIME_INACT      0x26
#define ADXL345_REGISTER_ACT_INACT_CTL   0x27
#define ADXL345_REGISTER_THRESH_FF       0x28
#define ADXL345_REGISTER_TIME_FF         0x29
#define ADXL345_REGISTER_TAP_AXES        0x2A
#define ADXL345_REGISTER_ACT_TAP_STATUS  0x2B
#define ADXL345_REGISTER_BW_RATE         0x2C
#define ADXL345_REGISTER_POWER_CTL       0x2D
#define ADXL345_REGISTER_INT_ENABLE      0x2E
#define ADXL345_REGISTER_INT_MAP         0x2F
#define ADXL345_REGISTER_INT_SOURCE      0x30
#define ADXL345_REGISTER_DATA_FORMAT     0x31
#define ADXL345_REGISTER_DATAX           0x32  // 16-bit
#define ADXL345_REGISTER_DATAY           0x34  // 16-bit
#define ADXL345_REGISTER_DATAZ           0x36  // 16-bit
#define ADXL345_REGISTER_FIFO_CTRL       0x38
#define ADXL345_REGISTER_FIFO_STATUS     0x39

#define ADXL345_ADDRESS  0x53


/* Driver option flags */
#define ADXL345_OPT_IRQ1_PU         0x01
#define ADXL345_OPT_IRQ2_PU         0x02



/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class ADXL345Opts {
  public:
    const uint8_t addr;
    const uint8_t irq1;
    const uint8_t irq2;
    const uint8_t flags;

    ADXL345Opts(const ADXL345Opts* p) :
      addr(p->addr), irq1(p->irq1), irq2(p->irq2), flags(p->flags) {};

    ADXL345Opts(uint8_t _a, uint8_t _i1, uint8_t _i2, uint8_t _f) :
      addr(_a), irq1(_i1), irq2(_i2), flags(_f) {};


    inline bool usePullup1() const {
      return (ADXL345_OPT_IRQ1_PU == (ADXL345_OPT_IRQ1_PU & flags));
    };

    inline bool usePullup2() const {
      return (ADXL345_OPT_IRQ2_PU == (ADXL345_OPT_IRQ2_PU & flags));
    };


  private:
};




class ADXL345 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    ADXL345(uint8_t, uint8_t, uint8_t);
    ADXL345(const ADXL345Opts*);
    ADXL345();

    ~ADXL345();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDeviceWithRegisters... */
    int8_t register_write_cb(DeviceRegister*);
    int8_t register_read_cb(DeviceRegister*);
    void printDebug(StringBuilder*);


  private:
    const ADXL345Opts  _opts;

    inline uint8_t _get_scale_bits() {    return 0;    };    // TODO
};

#endif  // __ADXL345_H__
