/*
 * config.h
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifndef CONF_PREFIX
#define CONF_PREFIX "/glftpd"
#endif

/* gl root  */
#ifndef glroot
#define glroot CONF_PREFIX
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
#define IPC_KEY_SCONFLOG        0xDEAD2200
#define IPC_KEY_GCONFLOG        0xDEAD2300
#define IPC_KEY_ALTLOG          0xDEAD2400
#define IPC_KEY_X               0xDEAD2500

/*
 * log file path
 * setting this variable enables logging (default is off)
 */
#ifndef log_file
#define log_file ""
#endif

/* dirlog file path */
#ifndef dir_log
#define dir_log CONF_PREFIX "/ftp-data/logs/dirlog"
#endif

/* nukelog file path */
#ifndef nuke_log
#define nuke_log CONF_PREFIX "/ftp-data/logs/nukelog"
#endif

/* dupe file path */
#ifndef dupe_file
#define dupe_file CONF_PREFIX "/ftp-data/logs/dupefile"
#endif

/* last-on log file path */
#ifndef last_on_log
#define last_on_log CONF_PREFIX "/ftp-data/logs/laston.log"
#endif

/* oneliner file path */
#ifndef oneliner_file
#define oneliner_file CONF_PREFIX "/ftp-data/logs/oneliners.log"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file CONF_PREFIX "/ftp-data/glutil/db/imdb"
#endif

/* game log file path */
#ifndef game_log
#define game_log CONF_PREFIX "/ftp-data/glutil/db/game"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log CONF_PREFIX "/ftp-data/glutil/db/tvrage"
#endif

/* generic 1 log file path */
#ifndef ge1_log
#define ge1_log CONF_PREFIX "/ftp-data/logs/gen1.log"
#endif

/* generic 2 log file path */
#ifndef ge2_log
#define ge2_log CONF_PREFIX "/ftp-data/logs/gen2.log"
#endif

/* generic 3 log file path */
#ifndef ge3_log
#define ge3_log CONF_PREFIX "/ftp-data/logs/gen3.log"
#endif

/* generic 4 log file path */
#ifndef ge4_log
#define ge4_log CONF_PREFIX "/ftp-data/logs/gen4.log"
#endif

/* precheck global config */

#ifndef gconf_log
#define gconf_log CONF_PREFIX "/ftp-data/glutil/precheck-data/gconf"
#endif

/* alt log */

#ifndef alt_log
#define alt_log CONF_PREFIX "/ftp-data/logs/altlog"
#endif

/* filesystem shadow log */

#ifndef x_log
#define x_log "-"
#endif

/* see MANUAL */
#ifndef du_fld
#define du_fld CONF_PREFIX "/bin/glutil.folders"
#endif

/* file extensions to skip generating crc32 (SFV mode)*/
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv(\\.tmp|))$"
#endif

#endif /* CONFIG_H_ */
