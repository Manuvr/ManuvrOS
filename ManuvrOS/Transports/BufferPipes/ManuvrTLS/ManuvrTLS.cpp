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

#if defined(WITH_MBED_TLS)

static void tls_log_shunt(void *ctx, int level, const char *file, int line, const char *str) {
  StringBuilder _tls_log;
  _tls_log.concatf("%s:%04d: %s", file, line, str);
  Kernel::log(&_tls_log);
}



ManuvrTLS::ManuvrTLS(int debug_lvl) : BufferPipe() {
  // mbedTLS will expect this array to be null-terminated. Zero it all...
  for (int x = 0; x < MAX_CIPHERSUITE_COUNT; x++) allowed_ciphersuites[x] = 0;

  mbedtls_ssl_config_init(&_conf);
  mbedtls_x509_crt_init(&_ourcert);
  mbedtls_pk_init(&_pkey);
  mbedtls_entropy_init(&_entropy);
  mbedtls_ctr_drbg_init(&_ctr_drbg);
  mbedtls_debug_set_threshold(debug_lvl);
}

ManuvrTLS::~ManuvrTLS() {
  // mbedTLS will expect this array to be null-terminated. Zero it all...
  for (int x = 0; x < MAX_CIPHERSUITE_COUNT; x++) allowed_ciphersuites[x] = 0;

  mbedtls_ssl_config_init(&_conf);
  mbedtls_x509_crt_init(&_ourcert);
  mbedtls_pk_init(&_pkey);
  mbedtls_entropy_init(&_entropy);
  mbedtls_ctr_drbg_init(&_ctr_drbg);
  mbedtls_debug_set_threshold(debug_lvl);
}

#endif
