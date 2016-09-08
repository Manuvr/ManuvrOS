/*
File:   ManuvrOIC.cpp
Author: J. Ian Lindsay
Date:   2016.08.04

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


This is the root of Manuvr's OIC implementation. If this feature is desired,
  it will be compiled as any other EventReceiver. Resources (other EventReceivers)
  can then be fed to it to expose their capabilities to any connected counterparties.

Target is OIC v1.1.

*/

#ifndef __MANUVR_OIC_FRAMEWORK_H__
#define __MANUVR_OIC_FRAMEWORK_H__

#include <EventReceiver.h>
#include <map>


/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define OIC_FLAG_SUPPORT_SERVER   0x01    // Compiled with this support.
#define OIC_FLAG_SUPPORT_CLIENT   0x02    // Compiled with this support.
#define OIC_FLAG_IOTIVITY_READY   0x04    // IoTivity-constrained reports ready.
#define OIC_FLAG_DISCOVERABLE     0x08    // We will respond to discovery requests.
#define OIC_FLAG_DISCOVERING      0x10    // Discovery in-progress.

#define OIC_MAX_URI_LENGTH  128


class ManuvrOIC : public EventReceiver {
  public:
    ManuvrOIC();
    virtual ~ManuvrOIC();

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);

    int8_t makeDiscoverable(bool);
    int8_t discoverOthers(bool);

    inline bool supportServer() {   return _er_flag(OIC_FLAG_SUPPORT_SERVER); };
    inline bool supportClient() {   return _er_flag(OIC_FLAG_SUPPORT_CLIENT); };
    inline bool isDiscoverable() {  return _er_flag(OIC_FLAG_DISCOVERABLE);   };
    inline bool isDiscovering() {   return _er_flag(OIC_FLAG_DISCOVERING);    };
    inline bool frameworkReady() {  return _er_flag(OIC_FLAG_IOTIVITY_READY); };


    static ManuvrOIC* INSTANCE;

    static void app_init_hook();
    static void issue_requests_hook();


  protected:
    int8_t bootComplete();

    inline void frameworkReady(bool nu) {  _er_set_flag(OIC_FLAG_IOTIVITY_READY, nu);  };
    inline void isDiscoverable(bool nu) {  _er_set_flag(OIC_FLAG_DISCOVERABLE, nu);  };
    inline void isDiscovering(bool nu) {   _er_set_flag(OIC_FLAG_DISCOVERING, nu);   };


  private:
    #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
      // If we have a concept of threads...
      unsigned long _thread_id = 0;
    #endif
    /* This maps URIs to resources. */
    std::map<const char*, listenerFxnPtr> _uri_map;
};

#endif //__MANUVR_OIC_FRAMEWORK_H__
