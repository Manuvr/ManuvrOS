/*
File:   TestDriver.cpp
Author: J. Ian Lindsay
Date:   2016.09.19

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

*/


#include "TestDriver.h"
#include <DataStructures/StringBuilder.h>

#if defined(MANUVR_TEST_DRIVER)

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

const MessageTypeDef message_defs[] = {
  #if defined (__HAS_CRYPT_WRAPPER)
    {  MANUVR_MSG_BNCHMRK_CRYPT,  0x0000,  "BNCHMRK_CRYPT",    ManuvrMsg::MSG_ARGS_NONE },
  #endif
  {  MANUVR_MSG_BNCHMRK_RNG,      0x0000,  "BNCHMRK_RNG",      ManuvrMsg::MSG_ARGS_NONE },
  {  MANUVR_MSG_BNCHMRK_MSG_LOAD, 0x0000,  "BNCHMRK_MSG_LOAD", ManuvrMsg::MSG_ARGS_NONE },
  {  MANUVR_MSG_BNCHMRK_FLOAT,    0x0000,  "BNCHMRK_FLOAT",    ManuvrMsg::MSG_ARGS_NONE }
};


static char* err_buf[128] = {0};



#if defined(__BUILD_HAS_DIGEST)
/*
* Digest algortithm tests.
* Because these are not reversible, this will take the form of
*   a known-answer test.
*/
int TestDriver::CRYPTO_TEST_HASHES() {
  local_log.concat("===< CRYPTO_TEST_HASHES >========================================\n");

  const char* hash_in0  = "";
  int i_len = 1524;
  uint8_t* hash_in1  = (uint8_t*) alloca(i_len);
  random_fill(hash_in1, i_len);

  Hashes* algs_to_test = list_supported_digests();

  int idx = 0;
  while (Hashes::NONE != algs_to_test[idx]) {
    int o_len = get_digest_output_length(algs_to_test[idx]);
    local_log.concatf("Testing %-14s  %3d-byte block\n--------------------------------------\n",
        get_digest_label(algs_to_test[idx]),
        o_len);

    uint8_t* hash_out0 = (uint8_t*) alloca(o_len);
    uint8_t* hash_out1 = (uint8_t*) alloca(o_len);

    if (wrapped_hash((uint8_t*) hash_in0, strlen(hash_in0), hash_out0, algs_to_test[idx])) {
      local_log.concat("Failed to hash.\n");
      return -1;
    }
    if (wrapped_hash((uint8_t*) hash_in1, i_len, hash_out1, algs_to_test[idx])) {
      local_log.concat("Failed to hash.\n");
      return -1;
    }
    local_log.concat("0-length: ");
    for (int i = 0; i < o_len; i++) local_log.concatf("%02x", *(hash_out0 + i));
    local_log.concat("\nhash_out: ");
    for (int i = 0; i < o_len; i++) local_log.concatf("%02x", *(hash_out1 + i));
    local_log.concat("\n\n");
    Kernel::log(&local_log);
    idx++;
  }
  return 0;
}
#else
int TestDriver::CRYPTO_TEST_HASHES() {
  // Build doesn't have asymmetric support.
  local_log.concat("Build doesn't have digest support. Skipping tests...\n");
  return 0;
}
#endif // __BUILD_HAS_DIGEST


#if defined(__BUILD_HAS_SYMMETRIC)
/*
* Symmetric algorthms.
*/
int TestDriver::CRYPTO_TEST_SYMMETRIC() {
  local_log.concat("===< CRYPTO_TEST_SYMMETRIC >=====================================\n");
  int i_len = 30;

  Cipher algs_to_test[] = {
    Cipher::SYM_AES_128_CBC,
    Cipher::SYM_AES_192_CBC,
    Cipher::SYM_AES_256_CBC,
    Cipher::SYM_BLOWFISH_CBC,
    //Cipher::SYM_NULL,
    Cipher::NONE
  };

  int ret = 0;
  int idx = 0;
  while (Cipher::NONE != algs_to_test[idx]) {
    const int block_size = get_cipher_block_size(algs_to_test[idx]);
    const int key_size   = get_cipher_key_length(algs_to_test[idx]);
    int o_len = get_cipher_aligned_size(algs_to_test[idx], i_len);
    uint8_t* plaintext_in  = (uint8_t*) alloca(o_len);
    random_fill(plaintext_in, o_len);

    local_log.concatf("Testing %-14s  %3d-byte block, %4d-bit key\n----------------------------------------------------\n",
        get_cipher_label(algs_to_test[idx]),
        block_size,
        key_size);

    if (0 != o_len % block_size) {
      o_len += (block_size - (o_len % block_size));
    }

    uint8_t* ciphertext    = (uint8_t*) alloca(o_len);
    uint8_t* plaintext_out = (uint8_t*) alloca(o_len);
    uint8_t iv[16];
    uint8_t key[(key_size>>3)];
    bzero(iv, 16);
    bzero(ciphertext, o_len);
    bzero(plaintext_out, o_len);
    random_fill(key, (key_size>>3));

    local_log.concat("Key:           ");
    for (int i = 0; i < (key_size>>3); i++) local_log.concatf("%02x", *(key + i));

    local_log.concat("\nPlaintext in:  ");
    for (int i = 0; i < i_len; i++) local_log.concatf("%02x", *(plaintext_in + i));
    local_log.concatf("\t(%d bytes)\n", i_len);

    ret = wrapped_sym_cipher((uint8_t*) plaintext_in, o_len, ciphertext, o_len, key, key_size, iv, algs_to_test[idx], OP_ENCRYPT);
    if (ret) {
      local_log.concatf("Failed to encrypt. Error %d\n", ret);
      return -1;
    }

    local_log.concat("Ciphertext:    ");
    for (int i = 0; i < o_len; i++) local_log.concatf("%02x", *(ciphertext + i));
    local_log.concatf("\t(%d bytes)\n", o_len);

    bzero(iv, 16);
    ret = wrapped_sym_cipher(ciphertext, o_len, plaintext_out, o_len, key, key_size, iv, algs_to_test[idx], OP_DECRYPT);
    if (ret) {
      local_log.concatf("Failed to decrypt. Error %d\n", ret);
      return -1;
    }

    local_log.concat("Plaintext out: ");
    for (int i = 0; i < o_len; i++) local_log.concatf("%02x", *(plaintext_out + i));
    local_log.concatf("\t(%d bytes)\n", o_len);

    // Now check that the plaintext versions match...
    for (int i = 0; i < i_len; i++) {
      if (*(plaintext_in + i) != *(plaintext_out + i)) {
        local_log.concat("Plaintext mismatch. Test fails.\n");
        return -1;
      }
    }
    local_log.concat("\n");
    Kernel::log(&local_log);
    idx++;
  }

  return 0;
}
#else
int TestDriver::CRYPTO_TEST_SYMMETRIC() {
  // Build doesn't have asymmetric support.
  Kernel::log("Build doesn't have symmetric support. Skipping tests...\n");
  return 0;
}
#endif // __BUILD_HAS_SYMMETRIC



#if defined(__BUILD_HAS_ASYMMETRIC)

struct Trips {  // <---- Check 'em
  size_t deltas[3];
};

static std::map<CryptoKey, Trips*>  asym_estimate_deltas;

/*
* Asymmetric algorthms.
*/
int TestDriver::CRYPTO_TEST_ASYMMETRIC_SET(Cipher c, CryptoKey* pks) {
  const int MSG_BUFFER_LEN  = 700;
  int ret = -1;
  uint8_t* test_message     = (uint8_t*) alloca(MSG_BUFFER_LEN);
  random_fill(test_message, MSG_BUFFER_LEN);

  while (CryptoKey::NONE != *pks) {
    size_t   public_estimate = 0;
    size_t   privat_estimate = 0;
    uint16_t sigbuf_estimate = 0;
    size_t   public_len      = 0;
    size_t   privat_len      = 0;
    size_t   sigbuf_len      = 0;
    if (!estimate_pk_size_requirements(*pks, &public_estimate, &privat_estimate, &sigbuf_estimate)) {
      local_log.concatf("\t Failed to estimate buffer requirements for %s.\n", get_pk_label(*pks));
      Kernel::log(&local_log);
      return -1;
    }
    uint8_t* public_buf  = (uint8_t*) alloca(public_estimate);
    uint8_t* privat_buf  = (uint8_t*) alloca(privat_estimate);
    uint8_t* sig_buffer  = (uint8_t*) alloca(sigbuf_estimate);
    public_len = public_estimate;
    privat_len = privat_estimate;
    sigbuf_len = sigbuf_estimate;

    int ret = wrapped_asym_keygen(c, *pks, public_buf, &public_len, privat_buf, &privat_len);
    if (0 == ret) {
      public_buf += (public_estimate - public_len);
      privat_buf += (privat_estimate - privat_len);

      local_log.concatf("\t Keygen for %s succeeds. Sizes:  %d / %d (pub/priv)\n", get_pk_label(*pks), public_len, privat_len);
      local_log.concatf("\t Public:  (%d bytes)\n", public_len);
      local_log.concatf("\t Private: (%d bytes)\n", privat_len);

      ret = wrapped_sign_verify(
        c, *pks, Hashes::SHA256,
        test_message, MSG_BUFFER_LEN,
        sig_buffer, &sigbuf_len,
        privat_buf, privat_len,
        OP_SIGN
      );

      if (0 == ret) {
        local_log.concatf("\t Signing operation succeeded. Sig is %d bytes.\n", sigbuf_len);

        ret = wrapped_sign_verify(
          c, *pks, Hashes::SHA256,
          test_message, MSG_BUFFER_LEN,
          sig_buffer, &sigbuf_len,
          public_buf, public_len,
          OP_VERIFY
        );
        if (0 == ret) {
          local_log.concatf("\t Verify operation succeeded.\n");
          local_log.concatf(
            "\t Size estimate deltas:\t%d / %d / %d  (pub/priv/sig)\n\n",
            (public_estimate - public_len),
            (privat_estimate - privat_len),
            (sigbuf_estimate - sigbuf_len)
          );
          if (nullptr != asym_estimate_deltas[*pks]) {
            asym_estimate_deltas[*pks]->deltas[0] = (strict_min(asym_estimate_deltas[*pks]->deltas[0], (public_estimate - public_len)));
            asym_estimate_deltas[*pks]->deltas[1] = (strict_min(asym_estimate_deltas[*pks]->deltas[1], (privat_estimate - privat_len)));
            asym_estimate_deltas[*pks]->deltas[2] = (strict_min(asym_estimate_deltas[*pks]->deltas[2], (sigbuf_estimate - sigbuf_len)));
          }
          else {
            asym_estimate_deltas[*pks] = new Trips;
            asym_estimate_deltas[*pks]->deltas[0] = (public_estimate - public_len);
            asym_estimate_deltas[*pks]->deltas[1] = (privat_estimate - privat_len);
            asym_estimate_deltas[*pks]->deltas[2] = (sigbuf_estimate - sigbuf_len);
          }
        }
        else {
          crypt_error_string(ret, (char*)&err_buf, 128);
          local_log.concatf("\t Verify for %s failed with err %s (code 0x%04x).\n", get_pk_label(*pks), (char*)&err_buf, ret);
        }
      }
      else {
        crypt_error_string(ret, (char*)&err_buf, 128);
        local_log.concatf("\t Signing for %s failed with err %s (code 0x%04x).\n", get_pk_label(*pks), (char*)&err_buf, ret);
      }
    }
    else {
      crypt_error_string(ret, (char*)&err_buf, 128);
      local_log.concatf("\t Keygen for %s failed with err %s (code 0x%04x).\n", get_pk_label(*pks), (char*)&err_buf, ret);
    }
    Kernel::log(&local_log);
    pks++;
  }
  Kernel::log(&local_log);
  return ret;
}


/*
* Asymmetric algorthms.
*/
int TestDriver::CRYPTO_TEST_ASYMMETRIC() {
  CryptoKey _pks[] = {CryptoKey::RSA_2048, CryptoKey::RSA_4096, CryptoKey::NONE};

  for (int i = 0; i < 1; i++) {
    Kernel::log("===< CRYPTO_TEST_ASYMMETRIC_ECDSA >==============================\n");
    if (CRYPTO_TEST_ASYMMETRIC_SET(Cipher::ASYM_ECDSA, list_supported_curves())) return -1;

    Kernel::log("===< CRYPTO_TEST_ASYMMETRIC_ECP >================================\n");
    if (CRYPTO_TEST_ASYMMETRIC_SET(Cipher::ASYM_ECKEY, list_supported_curves())) return -1;

    Kernel::log("===< CRYPTO_TEST_ASYMMETRIC_RSA >================================\n");
    if (CRYPTO_TEST_ASYMMETRIC_SET(Cipher::ASYM_RSA, &_pks[0]))  return -1;
  }

  // Print the estimate differentials.
	std::map<CryptoKey, Trips*>::iterator it;
  StringBuilder local_log;
  for (it = asym_estimate_deltas.begin(); it != asym_estimate_deltas.end(); it++) {
    local_log.concatf("\t %s\n", get_pk_label(it->first));
    local_log.concatf("\t Public:  %d\n", it->second->deltas[0]);
    local_log.concatf("\t Private: %d\n", it->second->deltas[1]);
    local_log.concatf("\t Sig:     %d\n\n", it->second->deltas[2]);
  }
  Kernel::log(&local_log);
  return 0;
}
#else
int TestDriver::CRYPTO_TEST_ASYMMETRIC() {
  // Build doesn't have asymmetric support.
  Kernel::log("Build doesn't have asymmetric support. Skipping tests...\n");
  return 0;
}
#endif  // __BUILD_HAS_ASYMMETRIC




/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/


TestDriver::TestDriver() : EventReceiver("TestDriver") {
  // Inform the Kernel of the codes we will be using...
  ManuvrMsg::registerMessages(message_defs, sizeof(message_defs) / sizeof(MessageTypeDef));
}


TestDriver::~TestDriver() {
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t TestDriver::attached() {
  if (EventReceiver::attached()) {
    return 1;
  }
  return 0;
}


/**
* Debug support function.
*
* @param A pointer to a StringBuffer object to receive the output.
*/
void TestDriver::printDebug(StringBuilder* output) {
  if (nullptr == output) return;
  EventReceiver::printDebug(output);
}



/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t TestDriver::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}



int8_t TestDriver::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_BNCHMRK_RNG:
      {
        // Empty the entropy pool.
        for (int i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) {
          randomInt();
        }
        unsigned long t0 = millis();
        for (int i = 0;i < 1000000; i++) {
          // Threaded or not, this should test the maximum randomness rate
          // available to the program.
          randomInt();   // This ought to block when the pool is drained.
        }
        unsigned long t1 = millis();
        local_log.concatf("RNG test:   %ul ms (%.2f bits/sec)\n", t1 - t0, (24/(double)(t1 - t0)));
        for (int i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) {
          local_log.concatf("Random number: 0x%08x\n", randomInt());
        }
      }
      return_value++;
      break;
    case MANUVR_MSG_BNCHMRK_FLOAT:
      {
        float a = 1.001;
        unsigned long t0 = millis();
        for (int i = 0;i < 1000000;i++) {
          a += 0.01 * sqrtf(a);
        }
        unsigned long t1 = millis();
        local_log.concatf("Floating-point test: %ul ms\n \t Value: %.5f\n \t%ul Mflops\n", t1 - t0, (double) a, (3000000000/(t1 - t0)));
      }
      return_value++;
      break;
    case MANUVR_MSG_BNCHMRK_MSG_LOAD:
      return_value++;
      break;
    #if defined (__HAS_CRYPT_WRAPPER)
      case MANUVR_MSG_BNCHMRK_CRYPT:
        CRYPTO_TEST_HASHES();
        //CRYPTO_TEST_SYMMETRIC();
        CRYPTO_TEST_ASYMMETRIC();
        return_value++;
        break;
    #endif  // __HAS_CRYPT_WRAPPER
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}



#if defined(MANUVR_CONSOLE_SUPPORT)
void TestDriver::procDirectDebugInstruction(StringBuilder *input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int temp_int = 0;
  if (strlen(str) > 1) {
    // We allow a short-hand for the sake of short commands that involve a single int.
    temp_int = atoi(str + 1);
  }

  switch (c) {
    case '?':
      local_log.concat("\tr <x>)  Dump <x> random 32-bit ints.\n");
      local_log.concat("\tb)      Show benchmark results.\n\n");
      local_log.concat("\t---< Available benchmarks >--------.\n");
      local_log.concat("\tb1)     RNG\n");
      #if defined (__HAS_CRYPT_WRAPPER)
        local_log.concat("\tb2)     Cryptographic\n");
      #endif
      local_log.concat("\tb3)     Floating point\n");
      local_log.concat("\tb4)     Message load\n");
      break;

    case 'b':
      switch (temp_int) {
        case 0:
          //printBenchmarkResults(&local_log);
          break;
        case 1:   Kernel::raiseEvent(MANUVR_MSG_BNCHMRK_RNG, nullptr);
          break;
        #if defined (__HAS_CRYPT_WRAPPER)
          case 2:   Kernel::raiseEvent(MANUVR_MSG_BNCHMRK_CRYPT, nullptr);
            break;
        #endif  // __HAS_CRYPT_WRAPPER
        case 3:   Kernel::raiseEvent(MANUVR_MSG_BNCHMRK_FLOAT, nullptr);
          break;
        case 4:   Kernel::raiseEvent(MANUVR_MSG_BNCHMRK_MSG_LOAD, nullptr);
          break;
        default:
          local_log.concat("Unsupported test.\n");
          break;
      }
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif // MANUVR_CONSOLE_SUPPORT


#endif // MANUVR_TEST_DRIVER
