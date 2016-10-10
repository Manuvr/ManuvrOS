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

#if defined(WITH_MBEDTLS) & defined(MBEDTLS_SSL_CLI_C)

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
ManuvrTLSClient::ManuvrTLSClient(BufferPipe* _n) : ManuvrTLS(_n, MBEDTLS_DEBUG_LEVEL) {
  _tls_pipe_name = "TLSClient";
  mbedtls_ssl_init(&_ssl);

  if (nullptr != _n) {
    // This is the point at which we detect if our underlying transport
    //   is a stream or datagram. This will impact our choices later on.
    int ret = mbedtls_ssl_config_defaults(&_conf,
                MBEDTLS_SSL_IS_CLIENT,
                _n->_bp_flag(BPIPE_FLAG_PIPE_PACKETIZED) ?
                  MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT
              );
    if (0 == ret) {
      mbedtls_ssl_conf_ciphersuites(&_conf, ManuvrTLS::allowed_ciphersuites);

      // TODO: Sec-fail. YOU FAIL! Remove once inter-op is demonstrated.
      mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

      // TODO: Need to be able to load a CA cert. If we also have
      //       runtime-writable storage, we could do cert-pinning at this point.
      //mbedtls_ssl_conf_ca_chain( &_conf, &cacert, nullptr);

      mbedtls_ssl_conf_rng(&_conf, mbedtls_ctr_drbg_random, &_ctr_drbg);
      mbedtls_ssl_conf_dbg(&_conf, tls_log_shunt, stdout);  // TODO: Suspect...

      // TODO: If appropriate, we load a certificate.
      //ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *)
      if (0 == ret) {
        // TODO: This should be loaded from secure storage.
        // TODO: This is a choice independent from the cert.
        // TODO: We hardcode the same IoTivity default creds used elsewhere.
        ret = mbedtls_ssl_conf_psk(&_conf, (const unsigned char*)"AAAAAAAAAAAAAAAA", 16, (const unsigned char*)"32323232-3232-3232-3232-323232323232", 36);
        if (0 == ret) {
          ret = mbedtls_ssl_setup(&_ssl, &_conf);
          if (0 == ret) {
            // TODO: This might be the lookup name if we had DNS.
            //ret = mbedtls_ssl_set_hostname(&_ssl, SERVER_NAME);
            if (0 == ret) {
              _log.concatf("ManuvrTLSClient(): Construction completed.\n");
            }
            else {
              _log.concatf("ManuvrTLSClient() failed: mbedtls_ssl_set_hostname returned 0x%04x\n", ret);
            }
          }
          else {
            _log.concatf("ManuvrTLSClient() failed: mbedtls_ssl_setup returned 0x%04x\n", ret);
          }
        }
        else {
          _log.concatf("ManuvrTLSClient() failed: mbedtls_ssl_conf_psk returned 0x%04x\n", ret);
        }
      }
      else {
        _log.concatf("ManuvrTLSClient() failed: mbedtls_x509_crt_parse returned 0x%04x\n", ret);
      }
    }
    else {
      _log.concatf("ManuvrTLSClient() failed: mbedtls_ssl_config_defaults returned 0x%04x\n", ret);
    }
  }
  Kernel::log(&_log);
}


/**
* Destructor.
*/
ManuvrTLSClient::~ManuvrTLSClient() {
  mbedtls_ssl_free(&_ssl);
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
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSClient::toCounterparty(StringBuilder* buf, int8_t mm) {
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
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrTLSClient::fromCounterparty(StringBuilder* buf, int8_t mm) {
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
void ManuvrTLSClient::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);
}

#endif
