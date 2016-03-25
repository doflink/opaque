/**
*   Copyright(C) 2011-2015 Intel Corporation All Rights Reserved.
*
*   The source code, information  and  material ("Material") contained herein is
*   owned  by Intel Corporation or its suppliers or licensors, and title to such
*   Material remains  with Intel Corporation  or its suppliers or licensors. The
*   Material  contains proprietary information  of  Intel or  its  suppliers and
*   licensors. The  Material is protected by worldwide copyright laws and treaty
*   provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
*   modified, published, uploaded, posted, transmitted, distributed or disclosed
*   in any way  without Intel's  prior  express written  permission. No  license
*   under  any patent, copyright  or  other intellectual property rights  in the
*   Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
*   implication, inducement,  estoppel or  otherwise.  Any  license  under  such
*   intellectual  property  rights must  be express  and  approved  by  Intel in
*   writing.
*
*   *Third Party trademarks are the property of their respective owners.
*
*   Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
*   this  notice or  any other notice embedded  in Materials by Intel or Intel's
*   suppliers or licensors in any way.
*/

#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <stdint.h>

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */
#include "sgx_trts.h"
#include "sgx_tcrypto.h"

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */

const char *key_str = "helloworld123123";
const sgx_aes_gcm_128bit_key_t *key = (const sgx_aes_gcm_128bit_key_t *) key_str;

void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}


// encrypt() and decrypt() should be called from enclave code only
void encrypt(uint8_t *plaintext, uint32_t length, 
	     uint8_t *iv, uint8_t *ciphertext,
	     sgx_aes_gcm_128bit_tag_t *mac) {
  // encrypt using a global key
  // TODO: fix this; should use key obtained from client 
  
  // key size is 12 bytes/128 bits
  // IV size is 12 bytes/96 bits
  // MAC size is 16 bytes/128 bits

  sgx_status_t rand_status = sgx_read_rand(iv, SGX_AESGCM_IV_SIZE);
  sgx_status_t status = sgx_rijndael128GCM_encrypt(key,
						   plaintext, length,
						   ciphertext,
						   iv, SGX_AESGCM_IV_SIZE,
						   NULL, 0,
						   mac);
  switch(status) {
  case SGX_ERROR_INVALID_PARAMETER:
    break;
  case SGX_ERROR_OUT_OF_MEMORY:
    break;
  case SGX_ERROR_UNEXPECTED:
    break;
  }
  
}


void decrypt(const uint8_t *iv, const uint8_t *ciphertext, uint32_t length, 
	     const sgx_aes_gcm_128bit_tag_t *mac,
	     uint8_t *plaintext) {

  // encrypt using a global key
  // TODO: fix this; should use key obtained from client 
  
  // key size is 12 bytes/128 bits
  // IV size is 12 bytes/96 bits
  // MAC size is 16 bytes/128 bits

  sgx_status_t status = sgx_rijndael128GCM_decrypt(key,
						   ciphertext, length,
						   plaintext,
						   iv, SGX_AESGCM_IV_SIZE,
						   NULL, 0,
						   mac);
  switch(status) {
  case SGX_ERROR_INVALID_PARAMETER:
    break;
  case SGX_ERROR_OUT_OF_MEMORY:
    break;
  case SGX_ERROR_UNEXPECTED:
    break;
  }
  
}

void ecall_encrypt(uint8_t *plaintext, uint32_t length,
		   uint8_t *ciphertext, uint32_t cipher_length) {

  // one buffer to store IV (12 bytes) + ciphertext + mac (16 bytes)
  assert(cipher_length == length + SGX_AESGCM_IV_SIZE + SGX_AESGCM_MAC_SIZE);
  uint8_t *iv_ptr = ciphertext;
  sgx_aes_gcm_128bit_tag_t *mac_ptr = ciphertext + SGX_AESGCM_IV_SIZE + length;
  
  encrypt(plaintext, length, iv, &mac_ptr);
}


// returns the number of attributes for this row
uint32_t get_num_col(uint8_t *row) {
  uint32_t *num_col_ptr = (uint32_t *) row;
  return *num_col_ptr;
}

uint8_t *get_attr(uint8_t **attr_ptr, uint32_t *attr_len, 
		  uint8_t *row_ptr, uint8_t *row, uint32_t length) {
  if (row_ptr >= row + length) {
    return NULL;
  }

  uint8_t *ret_row_ptr = row_ptr;

  *attr_ptr = row_ptr + 4;
  *attr_len = * ((uint32_t *) row_ptr);

  ret_row_ptr += 4 + *attr_len;
  return ret_row_ptr;
}

int ecall_filter_single_row(int op_code, uint8_t *row, uint32_t length) {

  // row is in format of [num cols][attr1 size][attr1][attr2 size][attr2] ...
  // num cols is 4 bytes; attr size is also 4 bytes
  int ret = 1;

  uint32_t num_cols = get_num_col(row);
  if (num_cols == 0) {
    return 0;
  }

  uint8_t *row_ptr = row + 4;
  
  if (op_code == 0) {
    // find the second row
    uint8_t *attr_ptr = NULL;
    uint32_t attr_len = 0;

    row_ptr = get_attr(&attr_ptr, &attr_len, row_ptr, row, length);
    row_ptr = get_attr(&attr_ptr, &attr_len, row_ptr, row, length);

    // TODO: decrypt value here
    // value should be int

    int *value_ptr = (int *) row_ptr;

    if (*value_ptr >= 3) {
      ret = 0;
    }

  } else {
    ret = 0;
  }

  return ret;
}