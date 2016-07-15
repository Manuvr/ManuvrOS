/*
File:   ManuvrTLS.h
Author: J. Ian Lindsay
Date:   2016.07.11

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


#ifndef __MANUVR_TLS_XFORMER_H__
#define __MANUVR_TLS_XFORMER_H__

#include "../CommTransformer.h"

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

class ManuvrTLSServer : public XportXformer {
  public:
    ManuvrTLS();
    ~ManuvrTLS();


  protected:


  private:
    void throwError(int ret);
    ~DtlsServer();

    mbedtls_ssl_cookie_ctx cookie_ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    #if defined(MBEDTLS_SSL_CACHE_C)
      mbedtls_ssl_cache_context cache;
    #endif
};



class ManuvrTLSClient : public XportXformer {
  public:
    ManuvrTLSClient(
             const unsigned char *priv_key,     size_t priv_key_len,
             const unsigned char *peer_pub_key, size_t peer_pub_key_len,
             const unsigned char *ca_pem,       size_t ca_pem_len,
             const unsigned char *psk,          size_t psk_len,
             const unsigned char *ident,        size_t ident_len,
             Nan::Callback* send_callback,
             Nan::Callback* hs_callback,
             Nan::Callback* error_callback,
             int debug_level);
    ~ManuvrTLSClient();

    int recv(unsigned char *buf, size_t len);
    int receive_data(unsigned char *buf, int len);
    int send_encrypted(const unsigned char *buf, size_t len);
    int send(const unsigned char *buf, size_t len);
    int step();
    int close();
    void store_data(const unsigned char *buf, size_t len);
    void error(int ret);

  private:
    void throwError(int ret);

    int allowed_ciphersuites[MAX_CIPHERSUITE_COUNT];
    mbedtls_ssl_context ssl_context;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt clicert;
    mbedtls_x509_crt cacert;
    mbedtls_pk_context pkey;
    mbedtls_timing_delay_context timer;
    const unsigned char *recv_buf;
    size_t recv_len;
};
#endif   // __MANUVR_TLS_XFORMER_H__
