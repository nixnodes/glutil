/*
 * l_sb.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef L_SB_H_
#define L_SB_H_

#include <t_glob.h>


#define sconf_log "SCONF"


int updmode;
char *argv_off;
char GLROOT[PATH_MAX];
char SITEROOT_N[PATH_MAX];
char SITEROOT[PATH_MAX];
char DIRLOG[PATH_MAX];
char NUKELOG[PATH_MAX];
char DU_FLD[PATH_MAX];
char DUPEFILE[PATH_MAX];
char LASTONLOG[PATH_MAX];
char ONELINERS[PATH_MAX];
char FTPDATA[PATH_MAX];
char IMDBLOG[PATH_MAX];
char GAMELOG[PATH_MAX];
char TVLOG[PATH_MAX];
char GE1LOG[PATH_MAX];
char GE2LOG[PATH_MAX];
char GE3LOG[PATH_MAX];
char GE4LOG[PATH_MAX];
char ALTLOG[PATH_MAX];
char XLOG[PATH_MAX];
char SCONFLOG[PATH_MAX];
char GCONFLOG[PATH_MAX];

char *LOOPEXEC;

key_t SHM_IPC;

long long int db_max_size;

#ifdef GLCONF
char GLCONF_I[PATH_MAX];
#else
char GLCONF_I[PATH_MAX];
#endif

char b_spec1[PATH_MAX];


FILE *fd_log;
char LOGFILE[PATH_MAX];
char *NUKESTR;
char *NUKESTR_d[255];

#define MAX_REGSUB_OUT_LEN              32768

char rs_o[MAX_REGSUB_OUT_LEN];

#endif /* L_SB_H_ */
