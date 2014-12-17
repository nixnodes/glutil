/*
 * errno_int.h
 *
 *  Created on: Jul 21, 2014
 *      Author: reboot
 */

#ifndef ERRNO_INT_H_
#define ERRNO_INT_H_

typedef struct errno_msg_ref
{
  int code;
  char *err_msg;
  int has_errno;
} _emr, *__emr;

#define _E_MAX_MMB                      8192

#define _E_MSG_DEFAULT                  "an unknown error has occured"

#define _E_MSG_DEF_BNW                  "could not open file for writing"
#define _E_MSG_DEF_WF                   "write failed"
#define _E_MSG_DEF_CC                   " (compressed)"

#define _E_MSG_ED_DEF                   "directory enumeration failed"
#define _E_MSG_ED_ODFAIL                "could not open a directory stream"
#define _E_MSG_ED_ESD_LSTAT             "could not get directory status (fstat)"
#define _E_MSG_ED_ESD_DIRFD             "could not get directory stream file descriptor"

char erm_buf[2048];

char *
ie_tl(int code, __emr pemr);


_emr EMR_enum_dir[4];

char *
g_strerr_r(int errnum, char *buf, size_t buflen);

#endif /* ERRNO_INT_H_ */
