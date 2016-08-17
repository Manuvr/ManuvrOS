#ifndef __MANUVR_OPTIONS_H__
#define __MANUVR_OPTIONS_H__

#include <ManuvrMsg/ManuvrMsg.h>

#define MANUVR_CONF_FLAG_REAP_KEY       0x1000  //
#define MANUVR_CONF_FLAG_CLOBBERABLE    0x2000  //
#define MANUVR_CONF_FLAG_CONST_REDUCED  0x4000  // Key reduced to const.
#define MANUVR_CONF_FLAG_REQUIRED       0x8000


/*
* TODO: This will be built on a linked-list until a non-std library for
*         map is chosen.
* TODO: We are facing the same problems here as we are with MsgDefs: How
*         to keep string bloat as small as we can.
*/
class ManuvrOpt : public Argument {
  public:
    ManuvrOpt();
    ~ManuvrOpt();
    const char*  _key;

    void printDebug(StringBuilder* out);


  private:
    ManuvrOpt * _next;
    uint16_t    _flags;

    // TODO: Might-should move this to someplace more accessable?
    static uintptr_t get_const_from_char_ptr(char*);
};

#endif  // __MANUVR_OPTIONS_H__
