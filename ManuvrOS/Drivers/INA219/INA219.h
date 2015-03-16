/*
File:   INA219.h
Author: J. Ian Lindsay
Date:   2014.05.27



This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef __INA219_DRIVER_H__
#define __INA219_DRIVER_H__

using namespace std;

#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"
#include "ManuvrOS/Drivers/SensorWrapper/SensorWrapper.h"


/* Sensor registers that exist in hardware */
#define INA219_REG_CONFIGURATION    0x00
#define INA219_REG_SHUNT_VOLTAGE    0x01
#define INA219_REG_BUS_VOLTAGE      0x02
#define INA219_REG_POWER            0x03
#define INA219_REG_CURRENT          0x04
#define INA219_REG_CALIBRATION      0x05

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


#define INA219_I2CADDR        0x4A


class INA219 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    INA219(uint8_t addr = INA219_I2CADDR);
    ~INA219(void);
    
    
    /* Overrides from SensorWrapper */
    int8_t init(void);
    int8_t readSensor(void);

    int8_t setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    int8_t getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.
    
    /* Overrides from I2CDevice... */
    void operationCompleteCallback(I2CQueuedOperation*);
    void printDebug(StringBuilder*);

    static constexpr const V_Cap_Point* batt_capacity_curves[6] = {chem_index_0, chem_index_1, chem_index_2, chem_index_3, chem_index_4, chem_index_5};  // Ten points on the curve ought to be enough for reliable interpolation.

    

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

    float readBusVoltage(void);
    float readShuntVoltage(void);
    float readCurrent(void);
    int8_t setBatteryMode(bool nu_mode);   // Pass true to set the battery mode on. False to turn it off.
    
    bool process_read_data(void);   // Takes the read data, updates SensorWrapper class, zeros registers.
};


#endif
