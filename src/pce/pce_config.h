/*
 * pce_config.h
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#ifndef PCE_CONFIG_H_
#define PCE_CONFIG_H_

#ifndef pce_logfile
#define pce_logfile "/ftp-data/logs/glutil-precheck.log"
#endif

/* game log file path */
#ifndef game_log
#define game_log "/ftp-data/logs/game.log"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log "/ftp-data/logs/tv.log"
#endif

#ifndef pce_data_path
#define pce_data_path "/ftp-data/glutil/precheck-data"
#endif

#ifndef gconf_log
#define gconf_log "/ftp-data/glutil/precheck-data/gconf"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_WARNING|F_MSG_TYPE_ERROR|F_MSG_TYPE_NOTICE
#endif

/* gl root  */
#ifndef glroot
#define glroot "/"
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* ftp-data, relative to gl root */
#ifndef ftp_data
#define ftp_data "/ftp-data"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file "/ftp-data/logs/imdb.log"
#endif

/* game log file path */
#ifndef game_log
#define game_log "/ftp-data/logs/game.log"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log "/ftp-data/logs/tv.log"
#endif


/* precheck global config */
#ifndef gconf_log
#define gconf_log "/ftp-data/glutil/precheck-data/gconf"
#endif


#define G_HFLAGS (a64 << 44)

#endif /* PCE_CONFIG_H_ */
