/*
File:   INA219.h
Author: J. Ian Lindsay
Date:   2014.05.27

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

#ifndef __INA219_DRIVER_H__
#define __INA219_DRIVER_H__

#include "Platform/Peripherals/I2C/I2CAdapter.h"
#include "Drivers/Sensors/SensorWrapper.h"


/* Sensor registers that exist in hardware */
#define INA219_REG_CONFIGURATION    0x00
#define INA219_REG_SHUNT_VOLTAGE    0x01
#define INA219_REG_BUS_VOLTAGE      0x02
#define INA219_REG_POWER            0x03
#define INA219_REG_CURRENT          0x04
#define INA219_REG_CALIBRATION      0x05


#define INA219_CONFIG_RESET                    (0x8000)  // Reset Bit

#define INA219_CONFIG_BVOLTAGERANGE_MASK       (0x2000)  // Bus Voltage Range Mask
#define INA219_CONFIG_BVOLTAGERANGE_16V        (0x0000)  // 0-16V Range
#define INA219_CONFIG_BVOLTAGERANGE_32V        (0x2000)  // 0-32V Range

#define INA219_CONFIG_GAIN_MASK                (0x1800)  // Gain Mask
#define INA219_CONFIG_GAIN_1_40MV              (0x0000)  // Gain 1, 40mV Range
#define INA219_CONFIG_GAIN_2_80MV              (0x0800)  // Gain 2, 80mV Range
#define INA219_CONFIG_GAIN_4_160MV             (0x1000)  // Gain 4, 160mV Range
#define INA219_CONFIG_GAIN_8_320MV             (0x1800)  // Gain 8, 320mV Range

#define INA219_CONFIG_BADCRES_MASK             (0x0780)  // Bus ADC Resolution Mask
#define INA219_CONFIG_BADCRES_9BIT             (0x0080)  // 9-bit bus res = 0..511
#define INA219_CONFIG_BADCRES_10BIT            (0x0100)  // 10-bit bus res = 0..1023
#define INA219_CONFIG_BADCRES_11BIT            (0x0200)  // 11-bit bus res = 0..2047
#define INA219_CONFIG_BADCRES_12BIT            (0x0400)  // 12-bit bus res = 0..4097

#define INA219_CONFIG_SADCRES_MASK             (0x0078)  // Shunt ADC Resolution and Averaging Mask
#define INA219_CONFIG_SADCRES_9BIT_1S_84US     (0x0000)  // 1 x 9-bit shunt sample
#define INA219_CONFIG_SADCRES_10BIT_1S_148US   (0x0008)  // 1 x 10-bit shunt sample
#define INA219_CONFIG_SADCRES_11BIT_1S_276US   (0x0010)  // 1 x 11-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_1S_532US   (0x0018)  // 1 x 12-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_2S_1060US  (0x0048)  // 2 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_4S_2130US  (0x0050)  // 4 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US  (0x0058)  // 8 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_16S_8510US (0x0060)  // 16 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_32S_17MS   (0x0068)  // 32 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_64S_34MS   (0x0070)  // 64 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_128S_69MS  (0x0078)  // 128 x 12-bit shunt samples averaged together

#define INA219_CONFIG_MODE_MASK                 (0x0007)  // Operating Mode Mask
#define INA219_CONFIG_MODE_POWERDOWN            (0x0000)
#define INA219_CONFIG_MODE_SVOLT_TRIGGERED      (0x0001)
#define INA219_CONFIG_MODE_BVOLT_TRIGGERED      (0x0002)
#define INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED  (0x0003)
#define INA219_CONFIG_MODE_ADCOFF               (0x0004)
#define INA219_CONFIG_MODE_SVOLT_CONTINUOUS     (0x0005)
#define INA219_CONFIG_MODE_BVOLT_CONTINUOUS     (0x0006)
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS (0x0007)


/* Sensor registers that exist in software */
#define INA219_REG_CLASS_MODE       0xF000   // Used to set the class mode. See class mode defs...
#define INA219_REG_BATTERY_SIZE     0xF001   // Used to inform the class about battery size (mAH)
#define INA219_REG_BATTERY_MIN_V    0xF002   // Used to inform the class about the battery's min voltage (float).
#define INA219_REG_BATTERY_MAX_V    0xF003   // Used to inform the class about the battery's max voltage (float).
#define INA219_REG_BATTERY_CHEM     0xF004   // Used to inform the class about the battery's chemistry. This helps inform the voltage/capacity curve.


/*
* Sensor modes
* These are bit-masked fields.
*/
#define INA219_MODE_BATTERY         0x01     // If set, we have a battery on one side of us.
#define INA219_MODE_INTEGRATION     0x02     // If set, we will integrate our current usage and provide a datum for it.


/*
* Battery chemistries...
* Different battery chemistries have different discharge curves. This struct is a point on a discharge curve.
* We should build a series of constants of this type for each chemistry we want to support. Since these curves
*   amount to physical constants, they shouldn't ever consume prescious RAM. Keep them isolated to flash.
*/
typedef struct v_cap_point {
  float percent_of_max_voltage;     // The percentage of the battery's stated maximum voltage.
  float capacity_derate;            // The percentage by which to multiply the battery's stated capacity (if you want AH remaining).
} V_Cap_Point;


#define INA219_BATTERY_CHEM_UNDEF   0x00     // No chemistry information provided. We will use a nieve metric of 80% voltage is dead, and linear to that point.
#define INA219_BATTERY_CHEM_LIPOLY  0x01
#define INA219_BATTERY_CHEM_LIPO4   0x02
#define INA219_BATTERY_CHEM_NICAD   0x03
#define INA219_BATTERY_CHEM_NIMH    0x04
#define INA219_BATTERY_CHEM_ALKALI  0x05

// TODO: Fill these in with some empirical data...
const V_Cap_Point chem_index_0[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};
const V_Cap_Point chem_index_1[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};
const V_Cap_Point chem_index_2[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};
const V_Cap_Point chem_index_3[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};
const V_Cap_Point chem_index_4[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};
const V_Cap_Point chem_index_5[] = {{1.000, 1.00},
                                    {0.975, 0.90},
                                    {0.950, 0.80},
                                    {0.925, 0.70},
                                    {0.900, 0.50},
                                    {0.875, 0.35},
                                    {0.850, 0.20},
                                    {0.825, 0.10},
                                    {0.800, 0.05},
                                    {0.775, 0.00}};


#define INA219_I2CADDR        0x40


class INA219 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    INA219(uint8_t addr = INA219_I2CADDR);
    ~INA219();


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
    bool init_complete;

    /* Properties of a battery that have nothing to do with the INA219. */
    float    batt_min_v;        // At what voltage is the battery considered dead?
    float    batt_max_v;        // What is the battery voltage at full-charge?
    uint16_t batt_capacity;     // How many mAh can this battery hold?
    uint8_t  batt_chemistry;    // What sort of capacity/voltage curve should we have?
    bool     battery_monitor;   // Is battery monitoring enabled?

    float shunt_value;          // How big is the shunt resistor (in ohms)?
    uint8_t max_voltage_delta;  // How much voltage might drop across the shunt? Assume short-circuit is possible. Must be either 16 or 32.

    /* Contents of registers that really exist in the hardware. */
    uint16_t reg_configuration;
    uint16_t reg_calibration;

    float readBusVoltage();
    float readShuntVoltage();
    float readCurrent();
    SensorError setBatteryMode(bool nu_mode);   // Pass true to set the battery mode on. False to turn it off.

    bool process_read_data();   // Takes the read data, updates SensorWrapper class, zeros registers.
};


#endif
