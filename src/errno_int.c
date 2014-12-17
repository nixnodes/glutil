/*
 * errno_int.c
 *
 *  Created on: Jul 21, 2014
 *      Author: reboot
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "errno_int.h"

char erm_buf[2048] =
  { 0 };

_emr EMR_enum_dir[] =
  {
    { .code = -2, .err_msg = _E_MSG_ED_ODFAIL, .has_errno = 1 },
    { .code = -3, .err_msg = _E_MSG_ED_ESD_LSTAT, .has_errno = 1 },
    { .code = -4, .err_msg = _E_MSG_ED_ESD_DIRFD, .has_errno = 1 },
    { .code = 0, .err_msg = _E_MSG_ED_DEF, .has_errno = 0 } };

char *
ie_tl(int code, __emr pemr)
{
  int i;

  for (i = 0; i < _E_MAX_MMB; i++)
    {
      if (pemr[i].code == code || 0 == pemr[i].code)
        {
          if (0 != pemr[i].has_errno)
            {
              snprintf(erm_buf, sizeof(erm_buf), "%s [%s]", pemr[i].err_msg,
                  strerror(errno));
              return erm_buf;
            }
          return pemr[i].err_msg;
        }
    }

  return _E_MSG_DEFAULT;
}

char *
g_strerr_r(int errnum, char *buf, size_t buflen)
{
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
  int ret = strerror_r(errnum, buf, buflen);
  if (0 != ret)
    {
      snprintf(buf, buflen, "int: strerror_r failure");
    }
  return buf;
#else
  return strerror_r(errnum, buf, buflen);
#endif
}
