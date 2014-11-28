/*
 * g_crypto.h
 *
 *  Created on: Nov 27, 2014
 *      Author: reboot
 */

#ifndef SRC_G_CRYPTO_H_
#define SRC_G_CRYPTO_H_

#include <sys/types.h>

#include <openssl/sha.h>

typedef struct ___pid_sha1
{
  unsigned char data[SHA_DIGEST_LENGTH];
} _pid_sha1, *__pid_sha1;

__pid_sha1
crypto_calc_sha1(unsigned char*input, size_t size, __pid_sha1 psha1_out);
char *
crypto_sha1_to_ascii(__pid_sha1 psha1, char *out);

#endif /* SRC_G_CRYPTO_H_ */
