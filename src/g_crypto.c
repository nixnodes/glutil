/*
 * g_crypto.c
 *
 *  Created on: Nov 27, 2014
 *      Author: reboot
 */

#include "g_crypto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <openssl/sha.h>

__pid_sha1
crypto_calc_sha1(unsigned char*input, size_t size, __pid_sha1 psha1_out)
{
  memset(psha1_out, 0x0, sizeof(_pid_sha1));
  return (__pid_sha1) SHA1((unsigned char*)input, size,(unsigned char*) psha1_out );
}

char *
crypto_sha1_to_ascii(__pid_sha1 psha1, char *out)
{

  int i;
  char *o_out = out;
  for (i = 0; i < sizeof(_pid_sha1); i++)
    {
      snprintf(out, 3, "%.2hhx", psha1->data[i]);
      out += 2;
    }

  return o_out;
}
