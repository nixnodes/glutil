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
#include <openssl/md5.h>

#pragma pack(push, 4)

typedef struct ___pid_sha1
{
  unsigned char data[SHA_DIGEST_LENGTH];
} _pid_sha1, *__pid_sha1;

typedef struct ___pid_sha224
{
  unsigned char data[SHA224_DIGEST_LENGTH];
} _pid_sha224, *__pid_sha224;

typedef struct ___pid_sha384
{
  unsigned char data[SHA384_DIGEST_LENGTH];
} _pid_sha384, *__pid_sha384;

typedef struct ___pid_sha512
{
  unsigned char data[SHA512_DIGEST_LENGTH];
} _pid_sha512, *__pid_sha512;

typedef struct ___pid_md5
{
  unsigned char data[MD5_DIGEST_LENGTH];
} _pid_md5, *__pid_md5;

#pragma pack(pop)

#endif /* SRC_G_CRYPTO_H_ */
