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

#ifndef pce_data_path
#define pce_data_path "/glftpd/ftp-data/glutil/precheck-data"
#endif

#ifndef gconf_log
#define gconf_log "/glftpd/ftp-data/glutil/precheck-data/gconf"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_WARNING|F_MSG_TYPE_ERROR|F_MSG_TYPE_NOTICE
#endif

#endif /* PCE_CONFIG_H_ */
