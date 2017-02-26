/*
File:   I2CAdapter.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


This class is supposed to be an i2c abstraction layer. The goal is to have
an object of this class that can be instantiated and used to communicate
with i2c devices (as a bus master) regardless of the platform.

This file is the tortured result of growing pains since the beginning of
  ManuvrOS. It has been refactored fifty-eleven times, suffered the brunt
  of all porting efforts, and has reached the point where it must be split
  apart into a more-portable platform-abstraction strategy.
*/




#ifndef I2C_ABSTRACTION_LAYER_ADAPTER         // This is meant to prevent double-inclusion.
  #define I2C_ABSTRACTION_LAYER_ADAPTER 1

  #include <inttypes.h>
  #include <stdint.h>
  #include <stdarg.h>
  #include <DataStructures/LightLinkedList.h>
  //#include <DataStructures/StringBuilder.h>
  #include <Platform/Platform.h>
  //#include <Kernel.h>
  #include <Drivers/BusQueue/BusQueue.h>
  #include <Drivers/DeviceWithRegisters/DeviceRegister.h>

  /* Compile-time bounds on memory usage. */
  #ifndef I2CADAPTER_MAX_QUEUE_PRINT
    // How many queue items should we print for debug?
    #define I2CADAPTER_MAX_QUEUE_PRINT 3
  #endif
  #ifndef I2CADAPTER_MAX_QUEUE_DEPTH
    // How deep should the queue be allowed to become before rejecting work?
    #define I2CADAPTER_MAX_QUEUE_DEPTH 12
  #endif
  #ifndef I2CADAPTER_PREALLOC_COUNT
    // How many queue items should we have on-tap?
    #define I2CADAPTER_PREALLOC_COUNT 4
  #endif

  /*
  * These are used as function-return codes, and have nothing to do with bus
  *   operations.
  */
  #define I2C_ERR_SLAVE_NO_ERROR      0   // No error.
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

  /*
  * These state flags are hosted by the EventReceiver. This may change in the future.
  * Might be too much convention surrounding their assignment across inherritence.
  */
  #define I2C_BUS_FLAG_BUS_ERROR  0x01    // While this is true, don't interact with the RN.
  #define I2C_BUS_FLAG_BUS_ONLINE 0x02    // Set when the module is verified to be in command mode.
  #define I2C_BUS_FLAG_PING_RUN   0x04    // Have we run a full bus discovery?
  #define I2C_BUS_FLAG_PINGING    0x08    // Are we running a full ping?


  /* These are transfer flags specific to I2C. */
  #define I2C_BUSOP_FLAG_SUBADDR         0x80    // Send a sub-address?
  #define I2C_BUSOP_FLAG_VERBOSITY_MASK  0x07    // Low three bits store verbosity.

  // Forward declaration. Definition order in this file is very important.
  class I2CDevice;
  class I2CAdapter;


  enum class I2CPingState {
    NONE = 0,
    NEG  = 1,
    POS  = 2,
    RES  = 3
  };


  #define I2C_ADAPT_OPT_FLAG_HZ0      0x0100   //
  #define I2C_ADAPT_OPT_FLAG_HZ1      0x0200   //
  #define I2C_ADAPT_OPT_FLAG_SCL_PU   0x0400   // SCL pullup.
  #define I2C_ADAPT_OPT_FLAG_SDA_PU   0x0800   // SDA pullup.

  class I2CAdapterOptions {
    public:
      I2CAdapterOptions(const I2CAdapterOptions* obj) :
        adapter(obj->adapter),
        sda_pin(obj->sda_pin),
        scl_pin(obj->scl_pin),
        def_flags(obj->def_flags)
      {};

      I2CAdapterOptions(uint8_t a, uint8_t d, uint8_t c) :
        adapter(a),
        sda_pin(d),
        scl_pin(c),
        def_flags(I2C_ADAPT_OPT_FLAG_SDA_PU | I2C_ADAPT_OPT_FLAG_SCL_PU)
      {};

      I2CAdapterOptions(uint8_t a, uint8_t d, uint8_t c, uint16_t f) :
        adapter(a),
        sda_pin(d),
        scl_pin(c),
        def_flags(f)
      {};

      const uint8_t  adapter;
      const uint8_t  sda_pin;
      const uint8_t  scl_pin;
      const uint16_t def_flags;
  };


  /*
  * This class represents an atomic operation on the i2c bus.
  */
  class I2CBusOp : public BusOp {
    public:
      I2CAdapter* device  = nullptr;
      int16_t sub_addr = -1;
      uint8_t dev_addr =  0;

      I2CBusOp();
      I2CBusOp(BusOpcode nu_op, BusOpCallback* requester);
      I2CBusOp(BusOpcode nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len);
      virtual ~I2CBusOp();

      /* Mandatory overrides from the BusOp interface... */
      //XferFault advance();
      XferFault begin();
      void wipe();
      void printDebug(StringBuilder*);

      int8_t advance_operation(uint32_t status_reg);
      void markComplete();

      /**
      * This will mark the bus operation complete with a given error code.
      * Overriden for simplicity. Marks the operation with failure code NO_REASON.
      *
      * @return 0 on success. Non-zero on failure.
      */
      inline int8_t abort() {    return abort(XferFault::NO_REASON); }
      int8_t abort(XferFault);

      /**
      * Decide if we need to send a subaddress.
      *
      * @return true if we do. False otherwise.
      */
      inline bool need_to_send_subaddr() {	return ((sub_addr != -1) && !subaddr_sent());  }

      inline int8_t getVerbosity() {   return (_flags & I2C_BUSOP_FLAG_VERBOSITY_MASK);   };
      inline void setVerbosity(int8_t v) {
        _flags = (v & I2C_BUSOP_FLAG_VERBOSITY_MASK) | (_flags & ~I2C_BUSOP_FLAG_VERBOSITY_MASK);
      };


    private:
      inline bool subaddr_sent() {  return (_flags & I2C_BUSOP_FLAG_SUBADDR);  };
      inline void subaddr_sent(bool en) {
        _flags = (en) ? (_flags | I2C_BUSOP_FLAG_SUBADDR) : (_flags & ~(I2C_BUSOP_FLAG_SUBADDR));
      };


      static ManuvrMsg event_queue_ready;
  };


  /*
  * This is the class that represents the actual i2c peripheral (master).
  */
  class I2CAdapter : public EventReceiver, public BusAdapter<I2CBusOp> {
    public:
      I2CAdapter(const I2CAdapterOptions*);  // Constructor takes a bus ID and pins as arguments.
      ~I2CAdapter();           // Destructor

      /* Overrides from the BusAdapter interface */
      int8_t io_op_callahead(BusOp*);
      int8_t io_op_callback(BusOp*);
      int8_t queue_io_job(BusOp*);
      I2CBusOp* new_op(BusOpcode, BusOpCallback*);

      /* Overrides from EventReceiver */
      int8_t notify(ManuvrMsg*);
      int8_t callback_proc(ManuvrMsg*);
      #if defined(MANUVR_CONSOLE_SUPPORT)
        void procDirectDebugInstruction(StringBuilder*);
        void printDebug(StringBuilder*);

        /* Debug aides */
        void printHardwareState(StringBuilder*);
        void printPingMap(StringBuilder*);
        void printDevs(StringBuilder*);
        void printDevs(StringBuilder*, uint8_t dev_num);
      #endif  //MANUVR_CONSOLE_SUPPORT


      // Builds a special bus transaction that does nothing but test for the presence or absence of a slave device.
      void ping_slave_addr(uint8_t);

      int8_t addSlaveDevice(I2CDevice*);     // Adds a new device to the bus.
      int8_t removeSlaveDevice(I2CDevice*);  // Removes a device from the bus.


      // These are meant to be called from the bus jobs. They deal with specific bus functions
      //   that may or may not be present on a given platform.
      int8_t generateStart();    // Generate a start condition on the bus.
      int8_t generateStop();     // Generate a stahp condition on the bus.

      inline bool busError() {          return (_er_flag(I2C_BUS_FLAG_BUS_ERROR));  };
      inline bool busOnline() {         return (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)); };
      inline void busError(bool nu) {   _er_set_flag(I2C_BUS_FLAG_BUS_ERROR, nu);   };
      inline void busOnline(bool nu) {  _er_set_flag(I2C_BUS_FLAG_BUS_ONLINE, nu);  };

      inline int8_t getAdapterId() {  return(_bus_opts.adapter);  };

      inline void raiseQueueReady() {  Kernel::isrRaiseEvent(&_queue_ready);  };


    protected:
      int8_t attached();      // This is called from the base notify().

      /* Overrides from the BusAdapter interface */
      int8_t advance_work_queue();
      int8_t bus_init();      // This must be provided on a per-platform basis.
      int8_t bus_deinit();    // This must be provided on a per-platform basis.


    private:
      const I2CAdapterOptions _bus_opts;
      int8_t  ping_map[32];

      LinkedList<I2CDevice*> dev_list;    // A list of active slaves on this bus.
      ManuvrMsg _queue_ready;


      int get_slave_dev_by_addr(uint8_t search_addr);
      void reclaim_queue_item(I2CBusOp*);
      void purge_queued_work_by_dev(I2CDevice *dev);
      void purge_queued_work();
      void purge_stalled_job();

      I2CPingState get_ping_state_by_addr(uint8_t addr);
      void set_ping_state_by_addr(uint8_t addr, I2CPingState nu);


      static I2CBusOp __prealloc_pool[I2CADAPTER_PREALLOC_COUNT];
      static char _ping_state_chr[4];
  };


  /*
  * This class represents a slave device on the bus. It should be extended by any class
  *   representing an i2c device, and should implement the given virtuals.
  *
  * Since the platform we are coding for uses an interrupt-driven i2c implementation,
  *   we will need to have callbacks.
  */
  class I2CDevice : public BusOpCallback {
    public:
      const uint8_t _dev_addr;

      /*
      * Constructor
      */
      I2CDevice(uint8_t); // Takes device address.
      ~I2CDevice();

      // Callback for requested operation completion.
      virtual int8_t io_op_callahead(BusOp*);
      virtual int8_t io_op_callback(BusOp*);
      virtual int8_t queue_io_job(BusOp*);

      bool assignBusInstance(I2CAdapter*);   // Needs to be called by the i2c class during insertion.
      bool disassignBusInstance();           // This is to be called from the adapter's unassignment function.

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
      bool read8();
      bool read8(int sub_addr);
      bool read16(int sub_addr);
      bool read16();


    private:
      I2CAdapter* _bus = nullptr;
  };


  /*
  * This class is an extension of I2CDevice for the special (most common) case where the
  *   device being represented has an internal register set. This extended class just takes
  *   some of the burden of routine register definition and manipulation off the extending class.
  */
  class I2CDeviceWithRegisters : public I2CDevice {
    public:
      I2CDeviceWithRegisters(uint8_t); // Takes device address.
      ~I2CDeviceWithRegisters();

      bool sync();


    protected:
      LinkedList<DeviceRegister*> reg_defs;     // Here is where registers will be enumerated.
      //uint8_t*  pooled_registers     = NULL;    // TODO: Make this happen!
      bool      multi_access_support;     // TODO: Make this happen! Depends on pooled_registers.


      // Callback for requested operation completion.
      virtual int8_t io_op_callahead(BusOp*);
      virtual int8_t io_op_callback(BusOp*);

      bool defineRegister(uint16_t _addr, uint8_t  val, bool dirty, bool unread, bool writable);
      bool defineRegister(uint16_t _addr, uint16_t val, bool dirty, bool unread, bool writable);
      bool defineRegister(uint16_t _addr, uint32_t val, bool dirty, bool unread, bool writable);

      DeviceRegister* getRegisterByBaseAddress(int b_addr);

      unsigned int regValue(uint8_t base_addr);

      bool regUpdated(uint8_t base_addr);
      void markRegRead(uint8_t base_addr);

      int8_t writeIndirect(uint8_t base_addr, uint8_t val);
      int8_t writeIndirect(uint8_t base_addr, uint8_t val, bool defer);

      int8_t writeDirtyRegisters();   // Automatically writes all registers marked as dirty.

      // Sync the given list of registers. And an override that syncs all registers.
      int8_t syncRegisters();

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

      int8_t writeRegister(DeviceRegister* reg);
      int8_t readRegister(DeviceRegister* reg);
  };

#endif  //I2C_ABSTRACTION_LAYER_ADAPTER
