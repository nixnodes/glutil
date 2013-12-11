/*
 * pce_config.h
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#ifndef PCE_CONFIG_H_
#define PCE_CONFIG_H_

#include <misc.h>
#include <t_glob.h>

#define G_HFLAGS F_OPT_SHAREDMEM

#ifndef pce_logfile
#define pce_logfile "/glftpd/ftp-data/logs/glutil-precheck.log"
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

#ifndef LOG_LEVEL
#define LOG_LEVEL F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_WARNING|F_MSG_TYPE_ERROR|F_MSG_TYPE_NOTICE
#endif

#endif /* PCE_CONFIG_H_ */
