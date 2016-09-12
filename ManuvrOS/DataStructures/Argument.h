#ifndef __MANUVR_ARGUMENT_H__
#define __MANUVR_ARGUMENT_H__

#include <string.h>

#include <EnumeratedTypeCodes.h>
#include <Types/TypeTranscriber.h>

#include <DataStructures/Vector3.h>
#include <DataStructures/Quaternion.h>

#if defined(MANUVR_CBOR)
  #include <Types/cbor-cpp/cbor.h>
#endif

/*
* These are flag definitions for Argument.
*/
#define MANUVR_ARG_FLAG_REAP_VALUE     0x01  // Should the pointer be freed?
#define MANUVR_ARG_FLAG_DIRECT_VALUE   0x02  // The value is NOT a pointer.
#define MANUVR_ARG_FLAG_REAP_KEY       0x10  //
#define MANUVR_ARG_FLAG_CLOBBERABLE    0x20  //
#define MANUVR_ARG_FLAG_CONST_REDUCED  0x40  // Key reduced to const.
#define MANUVR_ARG_FLAG_REQUIRED       0x80

class StringBuilder;
class BufferPipe;
class ManuvrXport;
class EventReceiver;
class Identity;
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
    * Glorious, glorious hackery. Keeping it.
    *        ---J. Ian Lindsay   Mon Oct 05 22:55:41 MST 2015
    */
    void*   target_mem = nullptr;
//float f = 10.5f; void *vp = *(uint32_t*) &f;
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

    /* Character pointers. */
    Argument(const char* val) : Argument((void*) val, (strlen(val)+1), STR_FM) {};
    Argument(char* val)       : Argument((void*) val, (strlen(val)+1), STR_FM) {};

    /*
    * We typically want references to typeless swaths of memory be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(void* val, size_t len) : Argument(val, len, BINARY_FM) {};

    /* These are system service pointers. Do not reap. */
    Argument(EventReceiver* val) : Argument((void*) val, sizeof(val), SYS_EVENTRECEIVER_FM) {};
    Argument(ManuvrXport* val)   : Argument((void*) val, sizeof(val), SYS_MANUVR_XPORT_FM)  {};
    Argument(BufferPipe* val)    : Argument((void*) val, sizeof(val), BUFFERPIPE_PTR_FM)    {};

    Argument(FxnPointer* val)     : Argument((void*) val, sizeof(val), SYS_FXN_PTR_FM)        {};
    Argument(ThreadFxnPtr* val)   : Argument((void*) val, sizeof(val), SYS_THREAD_FXN_PTR_FM) {};
    Argument(ArgumentFxnPtr* val) : Argument((void*) val, sizeof(val), SYS_ARG_FXN_PTR_FM)    {};
    Argument(PipeIOCallback* val) : Argument((void*) val, sizeof(val), SYS_PIPE_FXN_PTR_FM)   {};

    /*
    * We typically want ManuvrRunnable references to be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(ManuvrRunnable* val) : Argument((void*) val, sizeof(val), SYS_RUNNABLE_PTR_FM) {};

    // TODO: This default behavior changed. Audit usage by commenting addArg(StringBuilder)
    Argument(StringBuilder* val)  : Argument(val, sizeof(val), STR_BUILDER_FM)          {};
    Argument(Argument* val)       : Argument((void*) val, sizeof(val), ARGUMENT_PTR_FM) {};
    Argument(Identity* val)       : Argument((void*) val, sizeof(val), IDENTITY_FM)     {};


    ~Argument();


    inline void reapKey(bool en) {    _alter_flags(en, MANUVR_ARG_FLAG_REAP_KEY);      };
    inline bool reapKey() {           return _check_flags(MANUVR_ARG_FLAG_REAP_KEY);   };
    inline void reapValue(bool en) {  _alter_flags(en, MANUVR_ARG_FLAG_REAP_VALUE);    };
    inline bool reapValue() {         return _check_flags(MANUVR_ARG_FLAG_REAP_VALUE); };

    inline void*    pointer() {       return target_mem; };
    inline uint8_t  typeCode() {      return type_code;  };
    inline uint16_t length() {        return len;        };

    inline const char* getKey() {         return _key;  };
    inline void setKey(const char* k) {      _key = k;  };

    int collectKeys(StringBuilder*);

    int8_t getValueAs(void *trg_buf);
    int8_t getValueAs(uint8_t idx, void *trg_buf);
    int8_t getValueAs(const char*, void *trg_buf);

    int    argCount();
    int    sumAllLengths();
    Argument* retrieveArgByIdx(int idx);
    Argument* retrieveArgByKey(const char*);

    inline Argument* append(uint8_t val) {          return _append(new Argument(val));   }
    inline Argument* append(uint16_t val) {         return _append(new Argument(val));   }
    inline Argument* append(uint32_t val) {         return _append(new Argument(val));   }
    inline Argument* append(int8_t val) {           return _append(new Argument(val));   }
    inline Argument* append(int16_t val) {          return _append(new Argument(val));   }
    inline Argument* append(int32_t val) {          return _append(new Argument(val));   }
    inline Argument* append(float val) {            return _append(new Argument(val));   }

    inline Argument* append(uint8_t *val) {         return _append(new Argument(val));   }
    inline Argument* append(uint16_t *val) {        return _append(new Argument(val));   }
    inline Argument* append(uint32_t *val) {        return _append(new Argument(val));   }
    inline Argument* append(int8_t *val) {          return _append(new Argument(val));   }
    inline Argument* append(int16_t *val) {         return _append(new Argument(val));   }
    inline Argument* append(int32_t *val) {         return _append(new Argument(val));   }
    inline Argument* append(float *val) {           return _append(new Argument(val));   }

    inline Argument* append(Vector3ui16 *val) {     return _append(new Argument(val));   }
    inline Argument* append(Vector3i16 *val) {      return _append(new Argument(val));   }
    inline Argument* append(Vector3f *val) {        return _append(new Argument(val));   }
    inline Argument* append(Vector4f *val) {        return _append(new Argument(val));   }

    inline Argument* append(void *val, int len) {   return _append(new Argument(val, len));   }
    inline Argument* append(const char *val) {      return _append(new Argument(val));   }
    inline Argument* append(StringBuilder *val) {   return _append(new Argument(val));   }
    inline Argument* append(Argument *val) {        return _append(new Argument(val));   }
    inline Argument* append(Identity *val) {        return _append(new Argument(val));   }
    inline Argument* append(BufferPipe *val) {      return _append(new Argument(val));   }
    inline Argument* append(EventReceiver *val) {   return _append(new Argument(val));   }
    inline Argument* append(ManuvrXport *val) {     return _append(new Argument(val));   }
    inline Argument* append(ManuvrRunnable *val) {  return _append(new Argument(val));   }

    inline Argument* append(FxnPointer *val) {      return _append(new Argument(val));   }
    inline Argument* append(ThreadFxnPtr *val) {    return _append(new Argument(val));   }
    inline Argument* append(ArgumentFxnPtr *val) {  return _append(new Argument(val));   }
    inline Argument* append(PipeIOCallback *val) {  return _append(new Argument(val));   }


    // TODO: These will be re-worked to support alternate type-systems.
    int8_t serialize(StringBuilder*);
    int8_t serialize_raw(StringBuilder*);

    void valToString(StringBuilder*);
    void printDebug(StringBuilder*);

    static char*  printBinStringToBuffer(unsigned char *str, int len, char *buffer);

    #if defined(MANUVR_CBOR)
    static int8_t encodeToCBOR(Argument*, StringBuilder*);
    static Argument* decodeFromCBOR(uint8_t*, unsigned int);
    static inline Argument* decodeFromCBOR(StringBuilder* buf) {
      return decodeFromCBOR(buf->string(), buf->length());
    };
    #endif

    #if defined(MANUVR_JSON)
    static int8_t encodeToJSON(Argument*, StringBuilder*);
    static Argument* decodeFromJSON(StringBuilder*);
    #endif


  private:
    Argument*   _next      = nullptr;
    const char* _key       = nullptr;
    uint16_t    len        = 0;
    uint8_t     _flags     = 0;
    uint8_t     type_code  = NOTYPE_FM;

    Argument(void* ptr, int len, uint8_t code);  // Protected constructor to which we delegate.

    Argument* _append(Argument* arg);

    void wipe();

    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint8_t mask) {
      _flags = (en) ? (_flags | mask) : (_flags & ~mask);
    };
    inline bool _check_flags(uint8_t mask) {
      return (mask == (_flags & mask));
    };

    // TODO: Might-should move this to someplace more accessable?
    static uintptr_t get_const_from_char_ptr(char*);
};

#endif  // __MANUVR_ARGUMENT_H__
