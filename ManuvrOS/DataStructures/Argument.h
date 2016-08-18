
#include <EnumeratedTypeCodes.h>

#include <DataStructures/StringBuilder.h>
#include <DataStructures/BufferPipe.h>
#include <DataStructures/PriorityQueue.h>
#include <DataStructures/LightLinkedList.h>
#include <DataStructures/Vector3.h>
#include <DataStructures/Quaternion.h>

/*
* These are flag definitions for Argument.
*/
#define ARG_FLAG_REAP_VALUE     0x01      // Should the pointer be freed?


class ManuvrXport;
class EventReceiver;
class ManuvrRunnable;

/* This is how we define arguments to messages. */
class Argument {
  public:
    /*
    * Hackery surrounding this member:
    * There is no point to storing a pointer to a heap ref to hold data that is not
    *   bigger than the pointer itself. So rather than malloc()/free() and populate
    *   this slot with things like int32, we will instead cast the value itself to a
    *   void* and store it in the pointer slot. When we do this, we need to be sure
    *   not to mark the pointer for reap.
    *
    * Glorious, glorious hackery. Keeping it. But do need to account for (and extend to)
    *   64-bit pointers.
    *        ---J. Ian Lindsay   Mon Oct 05 22:55:41 MST 2015
    */
    void*   target_mem;

    Argument();
    Argument(uint8_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), UINT8_FM)  {};
    Argument(uint16_t val) : Argument((void*)(uintptr_t) val, sizeof(val), UINT16_FM) {};
    Argument(uint32_t val) : Argument((void*)(uintptr_t) val, sizeof(val), UINT32_FM) {};
    Argument(int8_t   val) : Argument((void*)(uintptr_t) val, sizeof(val), INT8_FM)   {};
    Argument(int16_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), INT16_FM)  {};
    Argument(int32_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), INT32_FM)  {};
    Argument(float    val) : Argument((void*)(uintptr_t) val, sizeof(val), FLOAT_FM)  {};

    Argument(uint8_t*  val) : Argument((void*) val, sizeof(val), UINT8_PTR_FM)  {};
    Argument(uint16_t* val) : Argument((void*) val, sizeof(val), UINT16_PTR_FM) {};
    Argument(uint32_t* val) : Argument((void*) val, sizeof(val), UINT32_PTR_FM) {};
    Argument(int8_t*   val) : Argument((void*) val, sizeof(val), INT8_PTR_FM)   {};
    Argument(int16_t*  val) : Argument((void*) val, sizeof(val), INT16_PTR_FM)  {};
    Argument(int32_t*  val) : Argument((void*) val, sizeof(val), INT32_PTR_FM)  {};
    Argument(float*    val) : Argument((void*) val, sizeof(val), FLOAT_PTR_FM)  {};

    Argument(Vector3ui16* val) : Argument((void*) val, 6,  VECT_3_UINT16) {};
    Argument(Vector3i16*  val) : Argument((void*) val, 6,  VECT_3_INT16)  {};
    Argument(Vector3f*    val) : Argument((void*) val, 12, VECT_3_FLOAT)  {};
    Argument(Vector4f*    val) : Argument((void*) val, 16, VECT_4_FLOAT)  {};

    /*
    * Character pointers.
    */
    Argument(const char* val) : Argument((void*) val, (strlen(val)+1), STR_FM) {};
    Argument(char* val)       : Argument((void*) val, (strlen(val)+1), STR_FM) {};

    /*
    * We typically want references to typeless swaths of memory be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(void* val, uint16_t len) : Argument(val, len, BINARY_FM) {};

    /*
    * These are system service pointers. Do not reap.
    */
    Argument(EventReceiver* val) : Argument((void*) val, sizeof(val), SYS_EVENTRECEIVER_FM) {};
    Argument(ManuvrXport* val)   : Argument((void*) val, sizeof(val), SYS_MANUVR_XPORT_FM)  {};
    Argument(BufferPipe* val)    : Argument((void*) val, sizeof(val), BUFFERPIPE_PTR_FM)    {};

    /*
    * We typically want ManuvrRunnable references to be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(ManuvrRunnable* val) : Argument((void*) val, sizeof(val), SYS_RUNNABLE_PTR_FM) {};

    // TODO: This default behavior changed. Audit usage by commenting addArg(StringBuilder)
    Argument(StringBuilder* val)  : Argument(val, sizeof(val), STR_BUILDER_FM) {};


    virtual ~Argument();


    inline void reapValue(bool en) {  _alter_flags(en, ARG_FLAG_REAP_VALUE);    };
    inline bool reapValue() {         return _check_flags(ARG_FLAG_REAP_VALUE); };

    inline void*    pointer() {           return target_mem; };
    inline uint8_t  typeCode() {          return type_code;  };
    inline uint16_t length() {            return len;        };

    int8_t getValueAs(void *trg_buf);
    int8_t getValueAs(uint8_t idx, void *trg_buf);
    int8_t append(Argument* arg);

    int    argCount();
    int8_t sumAllLengths();
    Argument* retrieveArgByIdx(int idx);

    // TODO: These will be re-worked to support alternate type-systems.
    int8_t serialize(StringBuilder*);
    int8_t serialize_raw(StringBuilder*);

    virtual void printDebug(StringBuilder*);

    static char*    printBinStringToBuffer(unsigned char *str, int len, char *buffer);


  protected:
    Argument*   _next;
    uint16_t len;         // This is sometimes not used. It is for data types that are not fixed-length.
    uint8_t  _flags;
    uint8_t  type_code;

    Argument(void* ptr, int len, uint8_t code);  // Protected constructor to which we delegate.



  private:
    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint8_t mask) {
      _flags = (en) ? (_flags | mask) : (_flags & ~mask);
    };
    inline bool _check_flags(uint8_t mask) {
      return (_flags == (_flags & mask));
    };

    void wipe();
};
