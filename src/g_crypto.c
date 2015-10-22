/*
 * g_crypto.c
 *
 *  Created on: Nov 27, 2014
 *      Author: reboot
 */

#include "g_crypto.h"

#include <stdio.h>

_pid_sha1
c_get_file_sha1 (char *path)
{
  _pid_sha1 digest =
    { 0 };

  FILE *fh = fopen (path, "rb");

  if (NULL == fh)
    {
      return digest;
    }

  char buffer[(1024^2)];

  SHA_CTX context;

  SHA1_Init (&context);

  size_t r;

  while ((r = fread (buffer, sizeof(buffer), 1, fh)))
    {
      SHA1_Update (&context, buffer, r);
    }

  SHA1_Final (digest.data, &context);

  fclose(fh);

  return digest;
}
