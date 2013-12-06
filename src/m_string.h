/*
 * m_string.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#ifndef M_STRING_H_
#define M_STRING_H_

#define MAX_CPRG_STRING         4096

int
g_load_strm(__g_handle hdl);
int
g_cprg(void *arg, int m, int match_i_m, int reg_i_m, int regex_flags,
    uint32_t flags);

#endif /* M_STRING_H_ */
