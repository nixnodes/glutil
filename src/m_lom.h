/*
 * m_lom.h
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <stdint.h>

#include <m_general.h>

#ifndef M_LOM_H_
#define M_LOM_H_

#define F_LM_CPRG               (a32 << 1)
#define F_LM_LOM                (a32 << 2)

#define F_LM_TYPES              (F_LM_CPRG|F_LM_LOM)


#define F_LOM_LVAR_KNOWN                (a32 << 1)
#define F_LOM_RVAR_KNOWN                (a32 << 2)
#define F_LOM_FLOAT                     (a32 << 3)
#define F_LOM_INT                       (a32 << 4)
#define F_LOM_HASOPER                   (a32 << 5)
#define F_LOM_FLOAT_DBL                 (a32 << 6)
#define F_LOM_INT_S                     (a32 << 7)

#define F_LOM_TYPES                     (F_LOM_FLOAT|F_LOM_INT|F_LOM_INT_S)
#define F_LOM_VAR_KNOWN                 (F_LOM_LVAR_KNOWN|F_LOM_RVAR_KNOWN)

#define F_GLT_LEFT              (a32 << 1)
#define F_GLT_RIGHT             (a32 << 2)

#define F_GLT_DIRECT    (F_GLT_LEFT|F_GLT_RIGHT)

#define MAX_LOM_STRING          4096

typedef struct ___lom_strings_header
{
  uint32_t flags;
  g_op g_oper_ptr;
  __g_match m_ref;
  char string[8192];
} _lom_s_h, *__lom_s_h;

typedef int
__d_lom_vp(void *d_ptr, void *_lom);

__d_lom_vp g_lom_var_int, g_lom_var_uint, g_lom_var_float;
int
g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm);
int
g_load_lom(__g_handle hdl);
int
g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
    uint32_t flags);
int
g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
    size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
    uint32_t flags);

int
g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags);

int
opt_g_lom(void *arg, int m, uint32_t flags);

#endif /* M_LOM_H_ */
