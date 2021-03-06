/*
 * macros_t.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef MACROS_T_H_
#define MACROS_T_H_

#define F_MMODE_EXEC            (a32 << 1)
#define F_MMODE_LIST            (a32 << 2)

#define SSD_MAX_LINE_SIZE       32768
#define SSD_MAX_LINE_PROC       15000
//#define SSD_MAX_FILE_SIZE     (V_MB*32)

#include <glutil.h>

#include <stdio.h>
#include <xref.h>

int
ref_to_val_macro (void *arg, char *match, char *output, size_t max_size,
		  void *mppd);
char **
process_macro (void * arg, char **out);

int
ssd_4macro (char *name, unsigned char type, void *arg, __g_eds eds);

int
list_macros (void);

_d_rtv_lk ref_to_val_lk_macro;

#endif /* MACROS_T_H_ */
