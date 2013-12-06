/*
 * l_sb.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef L_SB_H_
#define L_SB_H_

#include <glutil.h>
#include <t_glob.h>

/* gl root  */
#ifndef glroot
#define glroot "/glftpd"
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* ftp-data, relative to gl root */
#ifndef ftp_data
#define ftp_data "/ftp-data"
#endif

/* glftpd's user segment ipc key */
#ifndef shm_ipc
#define shm_ipc 0x0000DEAD
#endif

/* glutil data buffers ipc keys */
#define IPC_KEY_DIRLOG          0xDEAD1000
#define IPC_KEY_NUKELOG         0xDEAD1100
#define IPC_KEY_DUPEFILE        0xDEAD1200
#define IPC_KEY_LASTONLOG       0xDEAD1300
#define IPC_KEY_ONELINERS       0xDEAD1400
#define IPC_KEY_IMDBLOG         0xDEAD1500
#define IPC_KEY_GAMELOG         0xDEAD1600
#define IPC_KEY_TVRAGELOG       0xDEAD1700
#define IPC_KEY_GEN1LOG         0xDEAD1800
#define IPC_KEY_GEN2LOG         0xDEAD1900
#define IPC_KEY_GEN3LOG         0xDEAD2000
#define IPC_KEY_GEN4LOG         0xDEAD2100

/*
 * log file path
 * setting this variable enables logging (default is off)
 */
#ifndef log_file
#define log_file ""
#endif

/* dirlog file path */
#ifndef dir_log
#define dir_log "/glftpd/ftp-data/logs/dirlog"
#endif

/* nukelog file path */
#ifndef nuke_log
#define nuke_log "/glftpd/ftp-data/logs/nukelog"
#endif

/* dupe file path */
#ifndef dupe_file
#define dupe_file "/glftpd/ftp-data/logs/dupefile"
#endif

/* last-on log file path */
#ifndef last_on_log
#define last_on_log "/glftpd/ftp-data/logs/laston.log"
#endif

/* oneliner file path */
#ifndef oneliner_file
#define oneliner_file "/glftpd/ftp-data/logs/oneliners.log"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file "/glftpd/ftp-data/logs/imdb.log"
#endif

/* game log file path */
#ifndef game_log
#define game_log "/glftpd/ftp-data/logs/game.log"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log "/glftpd/ftp-data/logs/tv.log"
#endif

/* generic 1 log file path */
#ifndef ge1_log
#define ge1_log "/glftpd/ftp-data/logs/gen1.log"
#endif

/* generic 2 log file path */
#ifndef ge2_log
#define ge2_log "/glftpd/ftp-data/logs/gen2.log"
#endif

/* generic 3 log file path */
#ifndef ge3_log
#define ge3_log "/glftpd/ftp-data/logs/gen3.log"
#endif

/* generic 3 log file path */
#ifndef ge4_log
#define ge4_log "/glftpd/ftp-data/logs/gen4.log"
#endif

/* see README file about this */
#ifndef du_fld
#define du_fld "/glftpd/bin/glutil.folders"
#endif

/* file extensions to skip generating crc32 (SFV mode)*/
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv(\\.tmp|))$"
#endif



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

#endif /* L_SB_H_ */
