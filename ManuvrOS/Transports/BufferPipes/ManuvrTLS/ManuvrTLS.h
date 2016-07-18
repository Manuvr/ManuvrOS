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


These classes are meant to provide a transport translation layer for TLS.
Initial implentation is via mbedTLS.

One class is for the server (listener), and the other for client (initiator),
  since TLS distinguishes between those roles. I might consolidate this into
  a larger, single class that figures matters out for itself.

*/


#ifndef __MANUVR_TLS_XFORMER_H__
#define __MANUVR_TLS_XFORMER_H__

#include <DataStructures/BufferPipe.h>

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


#define MAX_CIPHERSUITE_COUNT   10


/*
* Clients and servers have these things in common...
*/
class ManuvrTLS : protected BufferPipe {
  public:
    ManuvrTLS();
    ~ManuvrTLS();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);


  protected:
    mbedtls_pk_context       _pkey;
    mbedtls_ssl_config       _conf;
    mbedtls_x509_crt         _our_cert;
    mbedtls_entropy_context  _entropy;
    mbedtls_ctr_drbg_context _ctr_drbg;

    virtual void throwError(int ret) =0;


  private:
};


class ManuvrTLSServer : public ManuvrTLS {
  public:
    ManuvrTLSServer();
    ~ManuvrTLSServer();

    void printDebug(StringBuilder*);


  protected:
    void throwError(int ret);


  private:
    mbedtls_ssl_cookie_ctx cookie_ctx;

    #if defined(MBEDTLS_SSL_CACHE_C)
      mbedtls_ssl_cache_context cache;
    #endif
};



class ManuvrTLSClient : protected ManuvrTLS {
  public:
    ManuvrTLSClient(
             const unsigned char *priv_key,     size_t priv_key_len,
             const unsigned char *peer_pub_key, size_t peer_pub_key_len,
             const unsigned char *ca_pem,       size_t ca_pem_len,
             const unsigned char *psk,          size_t psk_len,
             const unsigned char *ident,        size_t ident_len,
             int debug_level);
    ~ManuvrTLSClient();

    void printDebug(StringBuilder*);


    int recv(unsigned char *buf, size_t len);
    int receive_data(unsigned char *buf, int len);
    int send_encrypted(const unsigned char *buf, size_t len);
    int send(const unsigned char *buf, size_t len);
    int step();
    int close();
    void store_data(const unsigned char *buf, size_t len);
    void error(int ret);


  protected:
    void throwError(int ret);


  private:
    int allowed_ciphersuites[MAX_CIPHERSUITE_COUNT];
    mbedtls_x509_crt cacert;

    //mbedtls_timing_delay_context timer;
    const unsigned char *recv_buf;
    size_t recv_len;
};
#endif   // __MANUVR_TLS_XFORMER_H__
