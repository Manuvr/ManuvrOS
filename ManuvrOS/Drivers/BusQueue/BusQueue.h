
// TODO: This should become a template that bus queues should inherrit from.

enum class XferState {
  UNDEF,         // Freshly instanced.
  IDLE,          // Bus op is waiting somewhere outside of the queue.
  INITIATE,      // Waiting for initiation
  ADDR,          // Addressing phase. Sending the address.
  IO_WAIT,       // I/O operation in-progress.
  STOP,          // I/O operation in cleanup phase.
  COMPLETE,      // I/O op complete.
  FAULT          // Fault condition.
};
