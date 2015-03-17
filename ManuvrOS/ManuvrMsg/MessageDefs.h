/****************************************************************************************************
* These are message codes that might be raised from (potentially) anywhere in the program.          *
****************************************************************************************************/
/* 
* Reserved codes. These must be fully-supported, and never changed.
* We reserve the first 32 integers for protocol-level functions.
*/
  #define MANUVR_MSG_UNDEFINED            0x0000 // This is the invalid-in-use default code. 
  
/*
* Protocol-support codes. In order to have a device that can negotiate with other devices,
*   these codes must be fully-implemented.  
*/
  #define MANUVR_MSG_REPLY                0x0001 // This reply is for success-case.
  #define MANUVR_MSG_REPLY_RETRY          0x0002 // This reply asks for a reply of the given Unique ID.
  #define MANUVR_MSG_REPLY_PARSE_FAIL     0x0003 // This reply denotes that the packet failed to parse (despite passing checksum).
  
  #define MANUVR_MSG_SESS_ESTABLISHED     0x0004 // Session established.
  #define MANUVR_MSG_SESS_HANGUP          0x0005 // Session hangup.
  #define MANUVR_MSG_SESS_AUTH_CHALLENGE  0x0006 // A code for challenge-response authentication.

  #define MANUVR_MSG_SELF_DESCRIBE        0x0007 // No args? Asking for this data. One arg: Providing it.
  // Field order: 4 required null-terminated strings, two optional.
  // uint32:     MTU                (in terms of bytes)
  // String:     Protocol version   (IE: "0.0.1")
  // String:     Firmware version   (IE: "1.5.4")
  // String:     Hardware version   (IE: "4")
  // String:     Device class       (User-defined)
  // String:     Extended detail    (User-defined)
  
  #define MANUVR_MSG_LEGEND_TYPES         0x000A // No args? Asking for this legend. One arg: Legend provided. 
  #define MANUVR_MSG_LEGEND_MESSAGES      0x000B // No args? Asking for this legend. One arg: Legend provided.
  


/*
* System codes. These deal with messages within and around the core ManuvrOS functionality.
* We reserve all codes up to 0x1000 for ManuvrOS itself.
*
* This might seem like a large range, (and it is), but the idea is to contain all notions of
*   type, units, and basic functionality common with all devices running ManuvrOS (or some derivative).
*
* Anything after (and including) 0x1000 is a user-defined code that a given project or company can stake-out
*   for it's own use. Should conflicts arise, it means we are having adoption, and the protocol can be
*   incremented in version and a better strategy devised.
*/
  #define MANUVR_MSG_SYS_BOOT_COMPLETED   0x0020 // Raised when bootstrap is finished. This ought to be the first event to proc.
  #define MANUVR_MSG_SYS_BOOTLOADER       0x0021 // Reboots into the bootloader.
  #define MANUVR_MSG_SYS_REBOOT           0x0022 // Reboots into THIS program. IE, software reset.
  #define MANUVR_MSG_SYS_SHUTDOWN         0x0023 // Raised when the system is pending complete shutdown.
  #define MANUVR_MSG_PROGRAM_START        0x0024 // Starting an application on the receiver. Needs a string.
  #define MANUVR_MSG_SYS_PREALLOCATION    0x0025 // Any classes that do preallocation should listen for this.
  #define MANUVR_MSG_SYS_ISSUE_LOG_ITEM   0x0026 // Classes emit this to get their log data saved/sent.
  #define MANUVR_MSG_SYS_LOG_VERBOSITY    0x0027 // This tells client classes to adjust their log verbosity.
  #define MANUVR_MSG_SYS_ADVERTISE_SRVC   0x0028 // A system service might feel the need to advertise it's arrival.
  #define MANUVR_MSG_SYS_RETRACT_SRVC     0x0029 // A system service sends this to tell others to stop using it.

  // StaticHub and small scattered functionality
  #define MANUVR_MSG_RNG_BUFFER_EMPTY     0x0030 // The RNG couldn't keep up with our entropy demands.
  #define MANUVR_MSG_USER_DEBUG_INPUT     0x0031 // The user is issuing a direct debug command.
  #define MANUVR_MSG_INTERRUPTS_MASKED    0x0032 // Anything that depends on interrupts is now broken.
  
  // Time and Date                                                                                   
  #define MANUVR_MSG_SYS_DATETIME_CHANGED 0x0040 // Raised when the system time changes.
  #define MANUVR_MSG_SYS_SET_DATETIME     0x0041 //
  #define MANUVR_MSG_SYS_REPORT_DATETIME  0x0042 //

  
  // Power consumption models
  #define MANUVR_MSG_SYS_POWER_MODE       0x0050 // Read or set the power profile.

  // Codes specific to session-management and control  
  #define MANUVR_MSG_SESS_SUBCRIBE        0x0060 // Used to subscribe this session to other events.
  #define MANUVR_MSG_SESS_UNSUBCRIBE      0x0061 // Used to unsubscribe this session from other events.

  #define MANUVR_MSG_SESS_DUMP_DEBUG      0x0062 // Cause the XenoSession to dump its debug data.
  #define MANUVR_MSG_SESS_ORIGINATE_MSG   0x0063 // The session has something to say, and the transport ought to service it.
  
  
  // Scheduler
  #define MANUVR_MSG_SCHED_ENABLE_BY_PID  0x0100 // The given PID is being enabled.
  #define MANUVR_MSG_SCHED_DISABLE_BY_PID 0x0101 // The given PID is being disabled.
  #define MANUVR_MSG_SCHED_PROFILER_START 0x0102 // We want to profile the given PID.
  #define MANUVR_MSG_SCHED_PROFILER_STOP  0x0103 // We want to stop profiling the given PID.
  #define MANUVR_MSG_SCHED_PROFILER_DUMP  0x0104 // Dump the profiler data for all PIDs (no args) or given PIDs.
  #define MANUVR_MSG_SCHED_DUMP_META      0x0105 // Tell the Scheduler to dump its meta metrics.
  #define MANUVR_MSG_SCHED_DUMP_SCHEDULES 0x0106 // Tell the Scheduler to dump schedules.
  #define MANUVR_MSG_SCHED_WIPE_PROFILER  0x0107 // Tell the Scheduler to wipe its profiler data. Pass PIDs to be selective.
  #define MANUVR_MSG_SCHED_DEFERRED_EVENT 0x0108 // Tell the Scheduler to broadcast the attached Event so many ms into the future.
  #define MANUVR_MSG_SCHED_DEFINE         0x0109 // Define a new Schedule. Implies a return code. Careful...
  #define MANUVR_MSG_SCHED_DELETE         0x010A // Deletes an existing.
  
  // SD Card and file i/o
  #define MANUVR_MSG_SD_EJECTED           0x0120 // The SD card was just ejected.
  #define MANUVR_MSG_SD_INSERTED          0x0121 // An SD card was inserted.

  
  // Codes for various styles of serial bus
  #define MANUVR_MSG_I2C_QUEUE_READY      0x0200 // The i2c queue is ready for attention.
  #define MANUVR_MSG_I2C_DUMP_DEBUG       0x020F // Debug dump for i2c.
  // IRDa
  #define MANUVR_MSG_IRDA_ENABLE          0x0210 // Should IRDa be enabled?
  #define MANUVR_MSG_IRDA_CONFIG          0x0211 // IRDa configuration parameters are being passed.
  // SPI
  #define MANUVR_MSG_SPI_QUEUE_READY      0x0230 // There is a new job in the SPI bus queue.
  #define MANUVR_MSG_SPI_CB_QUEUE_READY   0x0231 // There is something ready in the callback queue. 
  // USB VCP stuff...
  #define MANUVR_MSG_SYS_USB_CONNECT      0x0250 // Something was plugged into our USB port.
  #define MANUVR_MSG_SYS_USB_DISCONNECT   0x0251 // Something was unplugged from our USB port.
  #define MANUVR_MSG_SYS_USB_CONFIGURED   0x0252 // USB device configured and ready for use.
  #define MANUVR_MSG_SYS_USB_SUSPEND      0x0253 // USB device suspended.
  #define MANUVR_MSG_SYS_USB_RESUME       0x0254 // Resumed....
  #define MANUVR_MSG_SYS_USB_RESET        0x0255 // USB system reset.
  


  // Codes related to the task of abstracting real-world units.
  #define MANUVR_MSG_LEGEND_UNITS         0x0300 // No args? Asking for this legend. One arg: Legend provided.

  // Event codes that pertain to frame buffers 
  #define MANUVR_MSG_LEGEND_FRAME_BUF     0x0310 // No args? Asking for this legend. One arg: Legend provided.
  #define MANUVR_MSG_DIRTY_FRAME_BUF      0x0311 // Something changed the framebuffer and we need to redraw.
  #define MANUVR_MSG_FRAME_BUF_CONTENT    0x0312 // Request (no args), or set (>0 args) the contents of a framebuffer.
  
  /* GPIO codes. This is an example of a hardware driver that is platform-dependent. Each supported platform needs
       its own driver that supports this event if this event is to be honored on that platform. */
  #define MANUVR_MSG_DIGITAL_READ         0x0400 // Read the given GPIO pin, however that is referenced on a given platformn.
  #define MANUVR_MSG_DIGITAL_WRITE        0x0401 // Write the given GPIO pin, however that is referenced on a given platformn.
  #define MANUVR_MSG_ANALOG_READ          0x0402 // Read the given GPIO pin, however that is referenced on a given platformn.
  #define MANUVR_MSG_ANALOG_WRITE         0x0403 // Write the given GPIO pin, however that is referenced on a given platformn.
  
  #define MANUVR_MSG_EVENT_ON_INTERRUPT   0x0410 // Fire the given event on the given interrupt condition.

  
  /*
  * The block comprised of [0x0500 - 0x1000) is being tentetively used for low-level hardware support drivers that
  *   are invarient across platforms. For example: Drivers for a certain kind of sensor. 
  * These will be deprecated soon... Do not code against them.
  */

  #define MANUVR_MSG_USER_BUTTON_PRESS    0x0500 // The user pushed a button with the given integer code.
  #define MANUVR_MSG_USER_BUTTON_RELEASE  0x0501 // The user released a button with the given integer code.
  #define MANUVR_MSG_SENSOR_ISL29033      0x0510 // The light sensor has something to say.
  #define MANUVR_MSG_SENSOR_ISL29033_IRQ  0x0511 // The light sensor IRQ_PIN.
  #define MANUVR_MSG_SENSOR_LPS331        0x0520 // The baro sensor has something to say.
  #define MANUVR_MSG_SENSOR_LPS331_IRQ_0  0x0521 // The baro sensor IRQ_0 pin.
  #define MANUVR_MSG_SENSOR_LPS331_IRQ_1  0x0522 // The baro sensor IRQ_1 pin.
  #define MANUVR_MSG_SENSOR_SI7021        0x0530 // The humidity sensor has something to say.
  #define MANUVR_MSG_SENSOR_TMP006        0x0540 // The thermopile has something to say.
  #define MANUVR_MSG_SENSOR_TMP006_IRQ    0x0541 // The thermopile IRQ pin changed state.
  #define MANUVR_MSG_SENSOR_INA219        0x0550 // The current sensor has something to say.

  
  
  /* Roving Networks Bluetooth radio codes */
  #define MANUVR_MSG_BT_CONNECTION_LOST   0x1000 // 
  #define MANUVR_MSG_BT_CONNECTION_GAINED 0x1001 // 

  #define MANUVR_MSG_BT_QUEUE_READY       0x1003 // There is action possible in the bluetooth queue.
  #define MANUVR_MSG_BT_RX_BUF_NOT_EMPTY  0x1004 // The host sent us data without indication of an end.
  #define MANUVR_MSG_BT_ENTERED_CMD_MODE  0x1005 // The module entered command mode.
  #define MANUVR_MSG_BT_EXITED_CMD_MODE   0x1006 // The module exited command mode.


