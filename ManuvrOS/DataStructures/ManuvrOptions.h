#ifndef __MANUVR_OPTIONS_H__
#define __MANUVR_OPTIONS_H__
#include <stdarg.h>

#define MANUVR_CONF_FLAG_REAP_KEY       0x1000  //
#define MANUVR_CONF_FLAG_CLOBBERABLE    0x2000  //
#define MANUVR_CONF_FLAG_CONST_REDUCED  0x4000  // Key reduced to const.
#define MANUVR_CONF_FLAG_REQUIRED       0x8000


typedef struct config_item_t {
  const char*           key;
  void*                 value;
  struct config_item_t* next;
  uint8_t               type_code;
  uint16_t              _flags;
} ConfigItem;


class ManuvrOpt : public Argument {
  public:
    char*       key;

    void*       target_mem;
    uint16_t    len;         // This is sometimes not used. It is for data types that are not fixed-length.
    uint8_t     type_code;
    bool        reap;


  private:
    ConfigItem* _next;
    uint16_t    _flags;
};


#endif  // __MANUVR_OPTIONS_H__
