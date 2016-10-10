/*
File:   ManuvrTLSServer.cpp
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

#if defined(WITH_MBEDTLS) & defined(MBEDTLS_SSL_SRV_C)


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

/**
* Callback to get PSK given identity. Prevents us from having to hard-code
*   authorized keys.
*/
int fetchPSKGivenID(void *parameter, mbedtls_ssl_context *ssl, const unsigned char *psk_identity, size_t identity_len) {
  //printf("PSK handler entry.\n");
  // Normally, the code below would be a shunt into an HSM, local key store,
  //   or some other means of storing keys securely.
  // We hard-code for the moment to test.
  if (0 == memcmp((const uint8_t*)psk_identity, (const uint8_t*)"32323232-3232-3232-3232-323232323232", identity_len)) {
    mbedtls_ssl_set_hs_psk(ssl, (const unsigned char*)"AAAAAAAAAAAAAAAA", 16);
    //printf("PSK handler departure with success.\n");
    return 0;
  }
  //printf("PSK handler departure with failure.\n");
  return 1;
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
ManuvrTLSServer::ManuvrTLSServer(BufferPipe* _n) : ManuvrTLS(_n, MBEDTLS_DEBUG_LEVEL) {
  _tls_pipe_name = "TLSServer";
  mbedtls_ssl_cookie_init(&_cookie_ctx);
  #if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init(&_cache);
  #endif

  mbedtls_ssl_conf_psk_cb(&_conf, fetchPSKGivenID, this);

  int ret = mbedtls_ssl_config_defaults(&_conf,
    MBEDTLS_SSL_IS_SERVER,
    _n->_bp_flag(BPIPE_FLAG_PIPE_PACKETIZED) ?
      MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM,
    MBEDTLS_SSL_PRESET_DEFAULT
  );

  if (0 == ret) {
    mbedtls_ssl_conf_min_version(&_conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    mbedtls_ssl_conf_rng(&_conf, mbedtls_ctr_drbg_random, &_ctr_drbg);
    mbedtls_ssl_conf_dbg(&_conf, tls_log_shunt, stdout);  // TODO: Suspect...

    ret = mbedtls_ssl_conf_own_cert(&_conf, &_our_cert, &_pkey);

    if (0 == ret) {
      ret = mbedtls_ssl_cookie_setup(
        &_cookie_ctx,
        mbedtls_ctr_drbg_random,
        &_ctr_drbg
      );

      if (0 == ret) {
        // TODO: Sec-fail. YOU FAIL! Remove once inter-op is demonstrated.
        mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_dtls_cookies(
          &_conf,
          mbedtls_ssl_cookie_write,
          mbedtls_ssl_cookie_check,
          &_cookie_ctx
        );
      }
      else {
        _log.concatf("TLSServer failed to mbedtls_ssl_cookie_setup() 0x%04x\n", ret);
      }
    }
    else {
      _log.concatf("TLSServer failed to mbedtls_ssl_conf_own_cert() 0x%04x\n", ret);
    }
  }
  else {
    _log.concatf("TLSServer failed to mbedtls_ssl_config_defaults() 0x%04x\n", ret);
  }
  Kernel::log(&_log);
}

/**
* Destructor.
*/
ManuvrTLSServer::~ManuvrTLSServer() {
  mbedtls_ssl_cookie_free(&_cookie_ctx);
  #if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free(&_cache);
  #endif
}



/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Inward toward the transport.
* This member receives plaintext from the application, and propagates a
*   ciphertext into the transport.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSServer::toCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveNear()) {
        /* We are not the transport driver, and we do no transformation. */
        return _near->toCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/**
* Outward toward the application (or into the accumulator).
* This member receives ciphertext from the transport, and propagates a
*   plaintext into the application.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSServer::fromCounterparty(StringBuilder* buf, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void ManuvrTLSServer::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);
}


#endif
