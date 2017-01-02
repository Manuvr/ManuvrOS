/*
File:   SPIBusOp.h
Author: J. Ian Lindsay
Date:   2014.07.01

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


This is the class that is used to keep bus operations on the SPI atomic.
*/


#ifndef __SPI_BUS_OP_H__
#define __SPI_BUS_OP_H__

  #include <Drivers/BusQueue/BusQueue.h>
  #include <DataStructures/StringBuilder.h>
  #include <Drivers/DeviceWithRegisters/DeviceRegister.h>
  #include <Kernel.h>

  /*
  * These flags are hosted by the member in the BusOp class.
  * Be careful when scrubing the field between re-use.
  */
  #define SPI_XFER_FLAG_NO_FLAGS        0x00   // By default, there are no flags set.
  #define SPI_XFER_FLAG_NO_FREE         0x01   // If set in a transaction's flags field, it will not be free()'d.
  #define SPI_XFER_FLAG_PREALLOCATE_Q   0x02   // If set, indicates this object should be returned to the prealloc queue.
  #define SPI_XFER_FLAG_DEVICE_CS_ASSRT 0x10   // CS pin is presently asserted.
  #define SPI_XFER_FLAG_DEVICE_CS_AH    0x20   // CS pin for device is active-high.
  #define SPI_XFER_FLAG_DEVICE_REG_INC  0x40   // If set, indicates this operation advances addresses in the target device.
  #define SPI_XFER_FLAG_PROFILE         0x80   // If set, this bus operation shall be profiled.


  #define SPI_CALLBACK_ERROR    -1
  #define SPI_CALLBACK_NOMINAL   0
  #define SPI_CALLBACK_RECYCLE   1


/*
* This class represents a single transaction on the SPI bus.
*/
class SPIBusOp : public BusOp {
  public:
    SPIBusOp();
    SPIBusOp(BusOpcode nu_op, BusOpCallback* requester);
    SPIBusOp(BusOpcode nu_op, BusOpCallback* requester, uint8_t cs, bool ah = false);
    virtual ~SPIBusOp();

    /* Mandatory overrides from the BusOp interface... */
    //XferFault advance();
    XferFault begin();
    void wipe();
    void printDebug(StringBuilder*);

    int8_t advance_operation(uint32_t status_reg, uint8_t data_reg);
    int8_t markComplete();

    /**
    * This will mark the bus operation complete with a given error code.
    * Overriden for simplicity. Marks the operation with failure code NO_REASON.
    *
    * @return 0 on success. Non-zero on failure.
    */
    inline int8_t abort() {    return abort(XferFault::NO_REASON); }
    int8_t abort(XferFault);


    void setBuffer(uint8_t *buf, unsigned int len);

    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2);
    void setParams(uint8_t p0, uint8_t p1);
    void setParams(uint8_t p0);
    inline uint8_t getTransferParam(int x) {  return xfer_params[x]; }

    inline void setCSPin(uint8_t pin) {   _cs_pin = pin;  };

    /* Flag management fxns... */
    bool shouldReap(bool);    // Override to set the reap behavior.
    bool returnToPrealloc(bool);
    bool devRegisterAdvance(bool);

    /**
    * The bus manager calls this fxn to decide if it ought to return this object to the preallocation
    *   queue following completion.
    *
    * @return true if the bus manager class should return this object to its preallocation queue.
    */
    inline bool returnToPrealloc() {  return (_flags & SPI_XFER_FLAG_PREALLOCATE_Q);  }

    /**
    * The bus manager calls this fxn to decide if it ought to return this object to the preallocation
    *   queue following completion.
    *
    * @return true if this bus operation is being profiled.
    */
    inline bool profile() {         return (_flags & SPI_XFER_FLAG_PROFILE);  }
    inline void profile(bool en) {
      _flags = (en) ? (_flags | SPI_XFER_FLAG_PROFILE) : (_flags & ~(SPI_XFER_FLAG_PROFILE));
    };

    /**
    * Is the chip select pin presently asserted?
    *
    * @return true if the CS pin is active.
    */
    inline bool csAsserted() {         return (_flags & SPI_XFER_FLAG_DEVICE_CS_ASSRT);  }
    inline void csAsserted(bool en) {
      _flags = (en) ? (_flags | SPI_XFER_FLAG_DEVICE_CS_ASSRT) : (_flags & ~(SPI_XFER_FLAG_DEVICE_CS_ASSRT));
    };

    /**
    * Is the chip select pin supposed to be active high?
    *
    * @return true if the CS pin is active.
    */
    inline bool csActiveHigh() {         return (_flags & SPI_XFER_FLAG_DEVICE_CS_AH);  }
    inline void csActiveHigh(bool en) {
      _flags = (en) ? (_flags | SPI_XFER_FLAG_DEVICE_CS_AH) : (_flags & ~(SPI_XFER_FLAG_DEVICE_CS_AH));
    };

    /**
    * The bus manager calls this fxn to decide if it ought to return this object to the preallocation
    *   queue following completion.
    *
    * @return true if the bus manager class should return this object to its preallocation queue.
    */
    inline bool devRegisterAdvance() {  return (_flags & SPI_XFER_FLAG_DEVICE_REG_INC);  }

    /**
    * The bus manager calls this fxn to decide if it ought to free this object after completion.
    *
    * @return true if the bus manager class should free() this object. False otherwise.
    */
    inline bool shouldReap() {        return ((_flags & SPI_XFER_FLAG_NO_FREE) == 0);   }


    static uint16_t  spi_wait_timeout;   // In microseconds. Per-byte.
    static ManuvrMsg event_spi_queue_ready;


  private:
    uint8_t xfer_params[4] = {0, 0, 0, 0};
    uint8_t  _param_len    = 0;
    uint8_t  _cs_pin       = 255;  // Chip-select pin.

    int8_t _assert_cs(bool);

    /* Members related to the work queue... */
    inline void step_queues(){  Kernel::isrRaiseEvent(&event_spi_queue_ready); }
};

#endif  // __SPI_BUS_OP_H__
