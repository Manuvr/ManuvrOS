/*
File:   i2c-adapter.h
Author: J. Ian Lindsay
Date:   2014.03.10


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


This class is supposed to be an i2c abstraction layer. The goal is to have
an object of this class that can be instantiated and used to communicate 
with i2c devices (as a bus master) regardless of the platform.

Prime targets for this layer are the linux operating system and the Teensy3,
but adding support for other platforms ought to be easy.
*/




#ifndef I2C_ABSTRACTION_LAYER_ADAPTER         // This is meant to prevent double-inclusion.
  #define I2C_ABSTRACTION_LAYER_ADAPTER 1

  using namespace std;
  
  #include <inttypes.h>
  #include <stdint.h>
  #include <stdarg.h>
  #include "DataStructures/LightLinkedList.h"
  #include "StringBuilder/StringBuilder.h"
  #include "ManuvrOS/EventManager.h"
  #include "ManuvrOS/Drivers/DeviceWithRegisters/DeviceRegister.h"

  #define I2CADAPTER_MAX_QUEUE_PRINT 3


  #define I2C_OPERATION_READ  I2C_Direction_Receiver
  #define I2C_OPERATION_WRITE I2C_Direction_Transmitter
  #define I2C_OPERATION_PING  0x04
  
  
  #define I2C_XFER_STATE_INITIATE  0x00
  #define I2C_XFER_STATE_START     0x01
  #define I2C_XFER_STATE_ADDR      0x02
  #define I2C_XFER_STATE_SUBADDR   0x03
  #define I2C_XFER_STATE_BODY      0x04
  #define I2C_XFER_STATE_DMA_WAIT  0x05
  #define I2C_XFER_STATE_STOP      0x06
  #define I2C_XFER_STATE_COMPLETE  0x07   // The stop signal was sent.
  
  
  #define I2C_ERR_CODE_NO_ERROR    0
  #define I2C_ERR_CODE_NO_CASE     -1
  #define I2C_ERR_CODE_NO_REASON   -2
  #define I2C_ERR_CODE_BUS_FAULT   -3
  #define I2C_ERR_CODE_DEF_CASE    -4
  #define I2C_ERR_CODE_ADDR_2TX    -5   // We were told to send an i2c address twice in a row.
  #define I2C_ERR_CODE_BAD_OP      -6
  #define I2C_ERR_CODE_NO_DEVICE   -7   // A transfer was aborted because there was no pointer to the device.
  #define I2C_ERR_CODE_TIMEOUT     -8
  #define I2C_ERR_CODE_CLASS_ABORT -9
  #define I2C_ERR_CODE_BUS_BUSY    -10

  #define I2C_ERR_SLAVE_BUS_FAULT    -3   // The bus failed us.
  #define I2C_ERR_SLAVE_NOT_FOUND   -11   // When a slave device we expected to find is not found.
  #define I2C_ERR_SLAVE_EXISTS      -12   // When we try to add a slave device that has already been added.
  #define I2C_ERR_SLAVE_COLLISION   -13   // When we try to add a slave device that has an address already used by another slave.
  #define I2C_ERR_SLAVE_INSERTION   -14   // Generic error for slave insertion. Usually implies out of memory.
  #define I2C_ERR_SLAVE_ASSIGN_CLOB -15   // Assigning a bus adapter to this slave would clobber an existing assignment.
  #define I2C_ERR_SLAVE_INVALID     -16   // Something makes this slave not appropriate for the requested operation.
  #define I2C_ERR_SLAVE_UNDEFD_REG  -17   // The requested register was not defined.
  #define I2C_ERR_SLAVE_REG_IS_RO   -18   // We tried to write to a register defined as read-only.
  
  
  
  #define I2C_BUS_STATE_NO_INIT  0x00
  #define I2C_BUS_STATE_ERROR    0x01
  #define I2C_BUS_STATE_READY    0x02


  // Forward declaration. Definition order in this file is very important.
  class I2CDevice;
  class I2CAdapter;
  
  
  /*
  * This class represents an atomic operation on the i2c bus.
  */
  class I2CQueuedOperation {
    public:
      static int next_txn_id;

      bool      initiated;     // Is this item fresh or is it waiting on a reply?
      bool      reap_buffer;   // If true, we will reap the buffer.

      uint8_t   opcode;        // What is the nature of this work-queue item?
      int       txn_id;        // How are we going to keep track of this item?
      int8_t    err_code;      // If we had an error, the code will be here.

      uint8_t dev_addr;
      int16_t sub_addr;

      uint8_t xfer_state;
      uint8_t remaining_bytes;
      uint8_t len;
      uint8_t *buf;

      I2CDevice  *requester;
      I2CAdapter *device;
      
      int8_t verbosity;


      I2CQueuedOperation(uint8_t nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len);
      ~I2CQueuedOperation(void);

      
      /*
      * This queue item can begin executing. This is where any bus access should be initiated.
      */
      int8_t begin(void);


      /**
      * Inlined. If this job is done processing. Errors or not.
      *
      * @return true if so. False otherwise.
      */
      inline bool completed(void) {  return (xfer_state == I2C_XFER_STATE_COMPLETE);  }

      /**
      * Decide if we need to send a subaddress.
      *
      * @return true if we do. False otherwise.
      */
      inline bool need_to_send_subaddr(void) {	return ((sub_addr != -1) && !subaddr_sent);  }

      /*
      * Called from the ISR to advance this operation on the bus.
      */
      int8_t advance_operation(uint32_t status_reg);

      /* Call to mark something completed that may not be. */
      void markComplete(void);
      int8_t abort(void);
      int8_t abort(int8_t);
      
      /* Debug aides */
      void printDebug(void);
      void printDebug(StringBuilder*);
      

    private:
      bool      subaddr_sent;    // Have the subaddress been sent yet?
      static ManuvrEvent event_queue_ready;
      
      int8_t init_dma();
      static const char* getErrorString(int8_t code);
      static const char* getOpcodeString(uint8_t code);
      static const char* getStateString(uint8_t code);

  };
  
  
  /*
  * This is the class that represents the actual i2c peripheral (master).
  */
  class I2CAdapter : public EventReceiver {
    public:
      bool bus_error;
      bool bus_online;
      int dev;
      
      I2CQueuedOperation* current_queue_item;
        
      I2CAdapter(uint8_t dev_id = 1);         // Constructor takes a bus ID as an argument.
      ~I2CAdapter(void);           // Destructor


      // Builds a special bus transaction that does nothing but test for the presence or absence of a slave device.
      void ping_slave_addr(uint8_t);

      int8_t addSlaveDevice(I2CDevice*);     // Adds a new device to the bus.
      int8_t removeSlaveDevice(I2CDevice*);  // Removes a device from the bus.

      // This is the fxn that is called to do I/O on the bus.
      bool insert_work_item(I2CQueuedOperation *);

      /* Debug aides */
      const char* getReceiverName();
      void printPingMap(StringBuilder *);
      void printDebug(StringBuilder *);
      void printDevs(StringBuilder *);
      void printDevs(StringBuilder *, uint8_t dev_num);
      
      /* Overrides from EventReceiver */
      void procDirectDebugInstruction(StringBuilder *);
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);

      
      // These are meant to be called from the bus jobs. They deal with specific bus functions
      //   that may or may not be present on a given platform.
      int8_t generateStart();    // Generate a start condition on the bus.
      int8_t generateStop();     // Generate a stahp condition on the bus.
      int8_t dispatchOperation(I2CQueuedOperation*);   // Start the given operation on the bus.
      bool switch_device(uint8_t nu_addr);


    protected:
      void __class_initializer();
      int8_t bootComplete();      // This is called from the base notify().


    private:
      bool ping_run;
      bool full_ping_running;
      int8_t ping_map[128];
      int8_t last_used_bus_addr;

      LinkedList<I2CQueuedOperation*> work_queue;     // A work queue to keep transactions in order.
      LinkedList<I2CDevice*> dev_list;                // A list of active slaves on this bus.

      void gpioSetup();
      
      void advance_work_queue(void);                  // Called from the ISR via Event. Advances the bus.
      
      int get_slave_dev_by_addr(uint8_t search_addr);
      void purge_queued_work_by_dev(I2CDevice *dev);
      void purge_queued_work();
      void purge_stalled_job();
  };

  
  
  
  /*
  * This class represents a slave device on the bus. It should be extended by any class
  *   representing an i2c device, and should implement the given virtuals.
  *
  * Since the platform we are coding for uses an interrupt-driven i2c implementation,
  *   we will need to have callbacks. 
  */
  class I2CDevice {
    public:
      uint8_t _dev_addr;

      /*
      * Constructor
      */
      I2CDevice(void);
      ~I2CDevice(void);
      
      // Callback for requested operation completion.
      virtual void operationCompleteCallback(I2CQueuedOperation*);
      
      /* If your device needs something to happen immediately prior to bus I/O... */
      virtual bool operationCallahead(I2CQueuedOperation*);
      
      bool assignBusInstance(I2CAdapter *);              // Needs to be called by the i2c class during insertion.
      bool assignBusInstance(volatile I2CAdapter *bus);  // Trivial override.

      bool disassignBusInstance(void);                   // This is to be called from the adapter's unassignment function.
      
      /* Debug aides */
      virtual void printDebug(StringBuilder*);

      
    protected:
      // Writes <byte_count> bytes from <buf> to the sub-address <sub_addr> of i2c device <dev_addr>.
      // Returns true if the job was accepted. False on error.
      bool writeX(int sub_addr, uint16_t byte_count, uint8_t *buf);
      bool readX(int sub_addr, uint8_t len, uint8_t *buf);

      bool write8(uint8_t dat);
      bool write8(int sub_addr, uint8_t dat);
      bool write16(int sub_addr, uint16_t dat);

      // Convenience functions for reading bytes from a given i2c address/sub-address...
      bool read8(void);
      bool read8(int sub_addr);
      bool read16(int sub_addr);
      bool read16(void);


    private:
      I2CAdapter* _bus;
  };
  
  
  /*
  * This class is an extension of I2CDevice for the special (most common) case where the
  *   device being represented has an internal register set. This extended class just takes
  *   some of the burden of routine register definition and manipulation off the extending class.
  */
  class I2CDeviceWithRegisters : public I2CDevice {
    public:
      I2CDeviceWithRegisters(void);
      ~I2CDeviceWithRegisters(void);

      bool sync(void);
      

    protected:
      LinkedList<DeviceRegister*> reg_defs;     // Here is where registers will be enumerated.
      //uint8_t*  pooled_registers     = NULL;    // TODO: Make this happen!
      bool      multi_access_support;     // TODO: Make this happen! Depends on pooled_registers.


      // Callback for requested operation completion.
      virtual void operationCompleteCallback(I2CQueuedOperation*);
      /* If your device needs something to happen immediately prior to bus I/O... */
      virtual bool operationCallahead(I2CQueuedOperation*);

      bool defineRegister(uint16_t _addr, uint8_t  val, bool dirty, bool unread, bool writable);
      bool defineRegister(uint16_t _addr, uint16_t val, bool dirty, bool unread, bool writable);
      bool defineRegister(uint16_t _addr, uint32_t val, bool dirty, bool unread, bool writable);
      
      DeviceRegister* getRegisterByBaseAddress(int b_addr);
      
      unsigned int regValue(uint8_t base_addr);
      
      bool regUpdated(uint8_t base_addr);
      void markRegRead(uint8_t base_addr);
      
      int8_t writeIndirect(uint8_t base_addr, uint8_t val);
      int8_t writeIndirect(uint8_t base_addr, uint8_t val, bool defer);

      int8_t writeDirtyRegisters(void);   // Automatically writes all registers marked as dirty.
  
      // Sync the given list of registers. And an override that syncs all registers.
      int8_t syncRegisters(void); 
  
      /* Low-level stuff. These are the ONLY fxns in this ENTIRE class that should care */
      /*   about ACTUAL register addresses. Everything else ought to be using the index */
      /*   for the desired register in reg_defs[].                                      */
      int8_t writeRegister(uint8_t base_addr);
      int8_t readRegister(uint8_t base_addr);
      int8_t readRegisters(uint8_t count, ...);
      /* This is the end of the low-level functions.                                    */
      
      /* Debug stuff... */
      virtual void printDebug(StringBuilder* temp);
      
      
    private:
      uint8_t reg_count;

      int8_t writeRegister(DeviceRegister *reg);
      int8_t readRegister(DeviceRegister *reg);
  };  
  

  
#ifndef STM32F4XX
  // To maintain uniformity with ST's code in an non-ST env...
  #define  I2C_Direction_Transmitter      ((uint8_t)0x00)
  #define  I2C_Direction_Receiver         ((uint8_t)0x01)
  #define IS_I2C_DIRECTION(DIRECTION) (((DIRECTION) == I2C_Direction_Transmitter) || \
                                       ((DIRECTION) == I2C_Direction_Receiver))
#endif

#if defined(RASPI)
  #include <stdlib.h>
  #include <unistd.h>
  #include <linux/i2c-dev.h>
  #include <sys/types.h>
  #include <sys/ioctl.h>
  #include <sys/stat.h>
  #include <fstream>
  #include <iostream>
  #include <fcntl.h>
  #include <inttypes.h>
  #include <ctype.h>
#endif

#include "StaticHub/StaticHub.h"

#endif  //I2C_ABSTRACTION_LAYER_ADAPTER

