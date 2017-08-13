#ifndef __MANUVR_ARGUMENT_H__
#define __MANUVR_ARGUMENT_H__

#include <string.h>

#include <CommonConstants.h>
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
class ManuvrMsg;

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
    Argument(uint8_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::UINT8)  {};
    Argument(uint16_t val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::UINT16) {};
    Argument(uint32_t val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::UINT32) {};
    Argument(int8_t   val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::INT8)   {};
    Argument(int16_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::INT16)  {};
    Argument(int32_t  val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::INT32)  {};
    Argument(float    val) : Argument((void*)(uintptr_t) val, sizeof(val), TCode::FLOAT)  {};

    Argument(uint8_t*  val) : Argument((void*) val, sizeof(val), TCode::UINT8_PTR)  {};
    Argument(uint16_t* val) : Argument((void*) val, sizeof(val), TCode::UINT16_PTR) {};
    Argument(uint32_t* val) : Argument((void*) val, sizeof(val), TCode::UINT32_PTR) {};
    Argument(int8_t*   val) : Argument((void*) val, sizeof(val), TCode::INT8_PTR)   {};
    Argument(int16_t*  val) : Argument((void*) val, sizeof(val), TCode::INT16_PTR)  {};
    Argument(int32_t*  val) : Argument((void*) val, sizeof(val), TCode::INT32_PTR)  {};
    Argument(float*    val) : Argument((void*) val, sizeof(val), TCode::FLOAT_PTR)  {};

    Argument(Vector3ui16* val) : Argument((void*) val, 6,  TCode::VECT_3_UINT16) {};
    Argument(Vector3i16*  val) : Argument((void*) val, 6,  TCode::VECT_3_INT16)  {};
    Argument(Vector3f*    val) : Argument((void*) val, 12, TCode::VECT_3_FLOAT)  {};
    Argument(Vector4f*    val) : Argument((void*) val, 16, TCode::VECT_4_FLOAT)  {};

    /* Character pointers. */
    Argument(const char* val) : Argument((void*) val, (strlen(val)+1), TCode::STR) {};
    Argument(char* val)       : Argument((void*) val, (strlen(val)+1), TCode::STR) {};

    /*
    * We typically want references to typeless swaths of memory be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(void* val, size_t len) : Argument(val, len, TCode::BINARY) {};

    /* These are system service pointers. Do not reap. */
    Argument(EventReceiver* val) : Argument((void*) val, sizeof(val), TCode::SYS_EVENTRECEIVER) {};
    Argument(ManuvrXport* val)   : Argument((void*) val, sizeof(val), TCode::SYS_MANUVR_XPORT)  {};
    Argument(BufferPipe* val)    : Argument((void*) val, sizeof(val), TCode::BUFFERPIPE)    {};

    Argument(FxnPointer* val)     : Argument((void*) val, sizeof(val), TCode::SYS_FXN_PTR)        {};
    Argument(ThreadFxnPtr* val)   : Argument((void*) val, sizeof(val), TCode::SYS_THREAD_FXN_PTR) {};
    Argument(ArgumentFxnPtr* val) : Argument((void*) val, sizeof(val), TCode::SYS_ARG_FXN_PTR)    {};
    Argument(PipeIOCallback* val) : Argument((void*) val, sizeof(val), TCode::SYS_PIPE_FXN_PTR)   {};

    /*
    * We typically want ManuvrMsg references to be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(ManuvrMsg* val) : Argument((void*) val, sizeof(val), TCode::SYS_MANUVRMSG) {};

    // TODO: This default behavior changed. Audit usage by commenting addArg(StringBuilder)
    Argument(StringBuilder* val)  : Argument(val, sizeof(val), TCode::STR_BUILDER)      {};
    Argument(Argument* val)       : Argument((void*) val, sizeof(val), TCode::ARGUMENT) {};
    Argument(Identity* val)       : Argument((void*) val, sizeof(val), TCode::IDENTITY) {};


    ~Argument();


    int8_t dropArg(Argument**, Argument*);

    inline void reapKey(bool en) {    _alter_flags(en, MANUVR_ARG_FLAG_REAP_KEY);      };
    inline bool reapKey() {           return _check_flags(MANUVR_ARG_FLAG_REAP_KEY);   };
    inline void reapValue(bool en) {  _alter_flags(en, MANUVR_ARG_FLAG_REAP_VALUE);    };
    inline bool reapValue() {         return _check_flags(MANUVR_ARG_FLAG_REAP_VALUE); };

    inline void*    pointer() {       return target_mem; };
    inline uint16_t length() {        return len;        };
    inline TCode    typeCode() {      return _t_code;    };
    inline uint8_t  typeCodeByte() {  return (uint8_t) _t_code;    };

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

    Argument* link(Argument* arg);
    inline Argument* append(uint8_t val) {          return link(new Argument(val));   }
    inline Argument* append(uint16_t val) {         return link(new Argument(val));   }
    inline Argument* append(uint32_t val) {         return link(new Argument(val));   }
    inline Argument* append(int8_t val) {           return link(new Argument(val));   }
    inline Argument* append(int16_t val) {          return link(new Argument(val));   }
    inline Argument* append(int32_t val) {          return link(new Argument(val));   }
    inline Argument* append(float val) {            return link(new Argument(val));   }

    inline Argument* append(uint8_t *val) {         return link(new Argument(val));   }
    inline Argument* append(uint16_t *val) {        return link(new Argument(val));   }
    inline Argument* append(uint32_t *val) {        return link(new Argument(val));   }
    inline Argument* append(int8_t *val) {          return link(new Argument(val));   }
    inline Argument* append(int16_t *val) {         return link(new Argument(val));   }
    inline Argument* append(int32_t *val) {         return link(new Argument(val));   }
    inline Argument* append(float *val) {           return link(new Argument(val));   }

    inline Argument* append(Vector3ui16 *val) {     return link(new Argument(val));   }
    inline Argument* append(Vector3i16 *val) {      return link(new Argument(val));   }
    inline Argument* append(Vector3f *val) {        return link(new Argument(val));   }
    inline Argument* append(Vector4f *val) {        return link(new Argument(val));   }

    inline Argument* append(void *val, int len) {   return link(new Argument(val, len));   }
    inline Argument* append(const char *val) {      return link(new Argument(val));   }
    inline Argument* append(StringBuilder *val) {   return link(new Argument(val));   }
    inline Argument* append(Argument *val) {        return link(new Argument(val));   }
    inline Argument* append(Identity *val) {        return link(new Argument(val));   }
    inline Argument* append(BufferPipe *val) {      return link(new Argument(val));   }
    inline Argument* append(EventReceiver *val) {   return link(new Argument(val));   }
    inline Argument* append(ManuvrXport *val) {     return link(new Argument(val));   }
    inline Argument* append(ManuvrMsg* val) {       return link(new Argument(val));   }

    inline Argument* append(FxnPointer *val) {      return link(new Argument(val));   }
    inline Argument* append(ThreadFxnPtr *val) {    return link(new Argument(val));   }
    inline Argument* append(ArgumentFxnPtr *val) {  return link(new Argument(val));   }
    inline Argument* append(PipeIOCallback *val) {  return link(new Argument(val));   }


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


  protected:
    Argument*   _next      = nullptr;
    const char* _key       = nullptr;
    uint16_t    len        = 0;
    uint8_t     _flags     = 0;
    TCode       _t_code    = TCode::NONE;

    Argument(TCode code);    // Protected constructor to which we delegate.
    Argument(void* ptr, int len, TCode code);   // Protected constructor to which we delegate.
    Argument(void* ptr, int len, uint8_t code) : Argument(ptr, len, (TCode) code){};   // TODO: Shim

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
