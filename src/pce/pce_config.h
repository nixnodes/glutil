/*
 * pce_config.h
 *
 *  Created on: Dec 10, 2013
 *      Author: reboot
 */

#ifndef PCE_CONFIG_H_
#define PCE_CONFIG_H_

/* gl root  */
#ifndef glroot
#ifndef PATH_PREFIX
#define glroot "/"
#else
#define glroot PATH_PREFIX
#endif
#endif

#ifndef PATH_PREFIX
#define PATH_PREFIX ""
#endif

#define PCE_F_OPT_SHAREDMEM     (a64 << 44)

#ifndef pce_logfile
#define pce_logfile PATH_PREFIX "/ftp-data/logs/glutil-precheck.log"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file PATH_PREFIX "/ftp-data/glutil/db/imdb"
#endif

/* game log file path */
#ifndef game_log
#define game_log PATH_PREFIX "/ftp-data/glutil/db/game"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log PATH_PREFIX "/ftp-data/glutil/db/tvrage"
#endif

#ifndef pce_data_path
#define pce_data_path PATH_PREFIX "/ftp-data/glutil/precheck-data"
#endif

#ifndef gconf_log
#define gconf_log PATH_PREFIX "/ftp-data/glutil/precheck-data/gconf"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_WARNING|F_MSG_TYPE_ERROR
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* ftp-data, relative to gl root */
#ifndef ftp_data
#define ftp_data "/ftp-data"
#endif

/* precheck global config */
#ifndef gconf_log
#define gconf_log PATH_PREFIX  "/ftp-data/glutil/precheck-data/gconf"
#endif

#define G_HFLAGS PCE_F_OPT_SHAREDMEM

#endif /* PCE_CONFIG_H_ */
