/*
File:   ManuvrTLSClient.cpp
Author: J. Ian Lindsay
Date:   2016.07.15

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


This class is meant to provide a transport translation layer for TLS.
Initial implentation is via mbedTLS.
*/

#include "ManuvrTLS.h"
#include <Kernel.h>

#if defined(__MANUVR_MBEDTLS)

/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

// TODO: This needs to be full-featured, and then culled based
//         on circumstance of connection.
int ManuvrTLS::allowed_ciphersuites[] = {
  MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,
  0
};


/**
* Passed-by-reference into the mbedtls library to facilitate logging.
* Must therefore be reachable by C-linkage.
*/
extern "C" {
  void ManuvrTLS::tls_log_shunt(void* ctx, int level, const char *file,
                            int line, const char *str) {
    StringBuilder _tls_log;
    _tls_log.concatf("%s:%04d: %s", file, line, str);
    Kernel::log(&_tls_log);
  }
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Constructor.
*/
ManuvrTLS::ManuvrTLS(BufferPipe* _n, int debug_lvl) : BufferPipe() {
  _bp_set_flag(BPIPE_FLAG_IS_BUFFERED, true);
  // mbedTLS will expect this array to be null-terminated. Zero it all...
  for (int x = 0; x < MAX_CIPHERSUITE_COUNT; x++) {
    allowed_ciphersuites[x] = 0;
  }
  mbedtls_ssl_config_init(&_conf);
  mbedtls_x509_crt_init(&_our_cert);
  mbedtls_pk_init(&_pkey);
  mbedtls_entropy_init(&_entropy);
  mbedtls_ctr_drbg_init(&_ctr_drbg);
  #if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(debug_lvl);
  #endif

  setNear(_n);
}


/**
* Destructor.
*/
ManuvrTLS::~ManuvrTLS() {
  mbedtls_ssl_config_free(&_conf);
  mbedtls_x509_crt_free(&_our_cert);
  mbedtls_pk_free(&_pkey);
  mbedtls_entropy_free(&_entropy);
  mbedtls_ctr_drbg_free(&_ctr_drbg);
}


void ManuvrTLS::throwError(int ret) {
  _log.concatf("%s::throwError(%d). Disconnecting...\n", pipeName(), ret);
  Kernel::log(&_log);
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
const char* ManuvrTLS::pipeName() { return _tls_pipe_name; }


#endif
