/*
 * lref.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <i_math.h>
#include <log_op.h>
#include <x_f.h>
#include <gv_off.h>
#include <str.h>
#include <m_lom.h>

#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <lref.h>
#include <xref.h>

static void *
l_mppd_create_copy(__d_drt_h mppd)
{
  __d_drt_h mppd_next = calloc(1, sizeof(_d_drt_h));

  if (NULL == mppd_next)
    {
      fprintf(stderr, "CRITICAL: unable to allocate memory\n");
      abort();
    }

  //memcpy(mppd_next, mppd, sizeof(_d_drt_h));
  mppd_next->hdl = mppd->hdl;
  mppd_next->flags = mppd->flags;
  //memcpy((char*) mppd_next->direc, (char*) mppd->direc, sizeof(mppd->direc));

  return mppd_next;
}

#define LMS_EX_L          0x28
#define LMS_EX_R          0x29

char*
l_mppd_shell_ex(char *input, char *output, size_t max_size, __d_drt_h mppd)
{
  char *ptr = input;
  char left, right;

  if (ptr[0] == LMS_EX_L)
    {
      left = LMS_EX_L;
      right = LMS_EX_R;
      ptr++;
    }
  else
    {
      while (ptr[0] && ptr[0] != 0x3A)
        {
          ptr++;
        }

      if (ptr[0] == 0x3A)
        {
          ptr++;
        }

      if ( NULL != mppd)
        {
          mppd->varg_l = ptr;
        }

      return input;
    }

  size_t st_len = strlen(ptr);

  if (st_len >= max_size)
    {
      st_len = max_size - 1;
    }

  strncpy(output, ptr, st_len);

  uint32_t lvl = 1;
  size_t c = 0;

  ptr = output;

  while (0x0 != ptr[0])
    {
      if (ptr[0] == left)
        {
          lvl++;
        }
      else if (ptr[0] == right)
        {
          lvl--;
        }

      if (lvl == 0)
        {
          break;
        }

      ptr++;
      c++;
    }

  if (ptr[0] == 0x0)
    {
      return NULL;
    }

  if (lvl == 0)
    {
      ptr[0] = 0x0;
    }

  if ( NULL != mppd)
    {
      ptr = &input[c + 2];

      if (ptr[0] == 0x3A)
        {
          ptr++;
        }

      mppd->varg_l = ptr;
    }

  return output;
}

char *
g_get_stf(char *match)
{
  char *ptr = strchr(match, 0x0);

  if (NULL == ptr || ptr == match)
    {
      return NULL;
    }

  ptr--;

  if (ptr[0] == 0x29)
    {
      return NULL;
    }

  while (ptr[0] != 0x7D && ptr[0] != 0x23 && ptr[0])
    {
      ptr--;
    }
  if (ptr[0] == 0x23)
    {
      ptr++;
      return ptr;
    }
  return NULL;
}

char *
dt_rval_spec_slen(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0),
      ((__d_drt_h ) mppd)->mppd_next);

  if (p_o)
    {
      snprintf(output, max_size, "%zu", strlen(p_o));
    }
  else
    {
      return "0";
    }
  return output;
}

char *
dt_rval_spec_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{

  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match, output, max_size,
      ((__d_drt_h ) mppd)->mppd_next);

  if (NULL != p_o)
    {
      snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, g_basename(p_o));
    }
  else
    {
      output[0] = 0x0;
    }

  return output;
}

char *
dt_rval_spec_dirname(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0),
      ((__d_drt_h ) mppd)->mppd_next);

  if (NULL != p_o)
    {
      snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, g_dirname(p_o));
    }
  else
    {
      output[0] = 0x0;
    }

  return output;
}

static char *
dt_rval_spec_conditional(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __rt_c cond = (__rt_c ) ((__d_drt_h ) mppd)->rt_cond;

  if (0 == g_lom_match(((__d_drt_h ) mppd)->hdl, arg, &cond->match))
    {
      __d_drt_h mppd_next = (__d_drt_h ) ((__d_drt_h ) mppd)->mppd_next;
      char *p_o = cond->p_exec(arg, match, ((__d_drt_h ) mppd)->tp_b0,
          sizeof(((__d_drt_h ) mppd)->tp_b0), mppd_next);

      if (NULL != p_o)
        {
          snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, p_o);
        }
      else
        {
          output[0] = 0x0;
        }
    }
  else
    {
      __d_drt_h mppd_aux_next = (__d_drt_h ) ((__d_drt_h ) mppd)->mppd_aux_next;
      char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match,
          ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0),
          mppd_aux_next);

      if (NULL != p_o)
        {
          snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, p_o);
        }
      else
        {
          output[0] = 0x0;
        }
    }

  return output;
}

char *
dt_rval_spec_gc(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h ) mppd;
  if (_mppd->t_1 > max_size)
    {
      _mppd->t_1 = max_size;
    }

  memset(output, _mppd->c_1, _mppd->t_1);
  output[_mppd->t_1] = 0x0;

  return output;
}

static char *
dt_rval_spec_print(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((__d_drt_h ) mppd)->st_p);
  return output;
}

static char *
dt_rval_spec_print_format(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h mppd_next = (__d_drt_h ) ((__d_drt_h ) mppd)->mppd_next;

  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, ((__d_drt_h ) mppd)->st_p0,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0),
      mppd_next);

  if (NULL != p_o)
    {
      snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, p_o);
    }
  else
    {
      output[0] = 0x0;
    }

  return output;
}

static char *
dt_rval_spec_tf_pl(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, localtime(&uts));
  return output;
}

static char *
dt_rval_spec_tf_pgm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, gmtime(&uts));
  return output;
}

static char *
dt_rval_spec_tf_l(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      localtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

static char *
dt_rval_spec_tf_gm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      gmtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

#define dt_rval_spec_math_pp(type) { \
  unsigned char v_b[16] = \
    { 0 }; \
  void *v_ptr = &v_b; \
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_b); \
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(type) v_ptr); \
}

char *
dt_rval_spec_math_u64(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(uint64_t*);
  return output;
}

char *
dt_rval_spec_math_u32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(uint32_t*);
  return output;
}

char *
dt_rval_spec_math_u16(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(uint16_t*);
  return output;
}

char *
dt_rval_spec_math_u8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(uint8_t*);
  return output;
}

char *
dt_rval_spec_math_s64(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(int64_t*);
  return output;
}

char *
dt_rval_spec_math_s32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(int32_t*);

  return output;
}

char *
dt_rval_spec_math_s16(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(int16_t*);
  return output;
}

char *
dt_rval_spec_math_s8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(int8_t*);
  return output;
}

char *
dt_rval_spec_math_f(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  dt_rval_spec_math_pp(float*);
  return output;
}

char *
dt_rval_spec_regsub_dg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h) mppd;

  char *rs_p = _mppd->fp_rval2(arg, match, ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);
  size_t o_l = strlen(rs_p) + 1;

  if (output != rs_p)
    {
      output = strncpy(rs_o, rs_p, o_l);
    }
  else
    {
      output = rs_p;
    }

  regmatch_t rm[4];
  char *m_p = output;

  while (!regexec(&_mppd->preg, m_p, 2, rm, 0))
    {
      if (rm[0].rm_so == rm[0].rm_eo)
        {
          //output[0] = 0x0;
          return output;
        }
      m_p = memmove(&m_p[rm[0].rm_so], &m_p[rm[0].rm_eo], o_l - rm[0].rm_eo);
    }

  return output;
}

char *
dt_rval_spec_regsub_g(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h) mppd;

  char *rs_p = _mppd->fp_rval2(arg, match, ((__d_drt_h ) mppd)->tp_b0,
      sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);

  size_t o_l = strlen(rs_p);

  regmatch_t rm[2];
  char *m_p = rs_p;
  size_t rs_w = 0;
  output = rs_o;
  regoff_t r_eo = -1;

  while (!regexec(&_mppd->preg, m_p, 1, rm, 0))
    {
      if (rm[0].rm_so == 0 && rm[0].rm_eo == o_l)
        {
          strncpy(&rs_o[rs_w], _mppd->r_rep, _mppd->r_rep_l);
          rs_o[_mppd->r_rep_l+rs_w] = 0x0;
          return rs_o;
        }
      if (rm[0].rm_so == rm[0].rm_eo)
        {
          if (!rm[0].rm_so)
            {
              strncpy(rs_o, _mppd->r_rep, _mppd->r_rep_l);
              strncpy(&rs_o[_mppd->r_rep_l], rs_p, o_l);
              rs_o[_mppd->r_rep_l+o_l] = 0x0;
            }
          else if (rm[0].rm_so == o_l)
            {
              strncpy(rs_o, rs_p, o_l);
              strncpy(&rs_o[o_l], _mppd->r_rep, _mppd->r_rep_l);
              rs_o[_mppd->r_rep_l+o_l] = 0x0;
            }
          else
            {
              return rs_p;
            }
          break;
        }

      if (rm[0].rm_so == (regoff_t)-1)
        {
          return rs_o;
        }

      strncpy(&rs_o[rs_w], m_p, rm[0].rm_so);
      rs_w += (size_t)rm[0].rm_so;
      strncpy(&rs_o[rs_w], _mppd->r_rep, _mppd->r_rep_l);
      rs_w += _mppd->r_rep_l;
      m_p = &m_p[rm[0].rm_eo];
      r_eo = rm[0].rm_eo;
    }

  if (m_p != rs_p)
    {
      size_t fw_l = strlen(rs_p) - r_eo;
      strncpy(&rs_o[rs_w], m_p, fw_l);
      rs_w += fw_l;
      rs_o[rs_w] = 0x0;
    }
  else
    {
      return rs_p;
    }

  return rs_o;
}

char *
dt_rval_spec_regsub_d(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h) mppd;

  char *rs_p = _mppd->fp_rval2(arg, match, ((__d_drt_h ) mppd)->tp_b0,
      sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);

  regmatch_t rm[2];

  if (regexec(&_mppd->preg, rs_p, 2, rm, 0) == REG_NOMATCH)
    {
      return rs_p;
    }

  size_t o_l = strlen(rs_p) + 1;

  if (output != rs_p)
    {
      output = strncpy(rs_o, rs_p, o_l);
    }
  else
    {
      output = rs_p;
    }

  if (rm[0].rm_so == rm[0].rm_eo)
    {
      //output[0] = 0x0;
      return output;
    }

  memmove(&output[rm[0].rm_so], &output[rm[0].rm_eo], o_l - rm[0].rm_eo);

  return output;
}

char *
dt_rval_spec_regsub(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h) mppd;

  char *rs_p = _mppd->fp_rval2(arg, match,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);

  regmatch_t rm[2];

  if (regexec(&_mppd->preg, rs_p, 2, rm, 0) == REG_NOMATCH)
    {
      return rs_p;
    }

  size_t o_l = strlen(rs_p) + 1;

  if (rm[0].rm_so == rm[0].rm_eo)
    {
      //output[0] = 0x0;
      return rs_p;
    }

  strncpy(rs_o, rs_p, rm[0].rm_so);
  strncpy(&rs_o[rm[0].rm_so], _mppd->r_rep, _mppd->r_rep_l);
  strncpy(&rs_o[rm[0].rm_so+_mppd->r_rep_l], &rs_p[rm[0].rm_eo], o_l - rm[0].rm_eo);

  rs_o[rm[0].rm_so+_mppd->r_rep_l + (o_l - rm[0].rm_eo)] = 0x0;

  return rs_o;
}

#define MAX_RR_IN       0x1000000

void *
as_ref_to_val_lk(char *match, void *c, __d_drt_h mppd, char *defdc)
{

  if (NULL != defdc)
    {
      match = g_get_stf(match);

      if (NULL != match)
        {
          size_t i = 0;
          while ((match[0] != 0x7D && match[0] != 0x2C && match[0] != 0x29
              && match[0] != 0x3A && match[0] && i < sizeof(mppd->direc) - 2))
            {
              if (match[0] == 0x5C)
                {
                  while (match[0] == 0x5C)
                    {
                      match++;
                    }
                }
              mppd->direc[i] = match[0];
              i++;
              match++;
            }

          if (1 < strlen(mppd->direc))
            {
              mppd->direc[i] = 0x0;
              goto ct;
            }

        }
      if (mppd->direc[0] == 0x0)
        {
          size_t defdc_l = strlen(defdc);
          strncpy(mppd->direc, defdc, defdc_l);
          mppd->direc[defdc_l] = 0x0;
        }

    }

  ct:

  return c;
}
/*
 int g_proc_nested (char *input, char d, void *arg) {

 }

 void *
 ref_to_val_af_math_bc(void *arg, char *match, char *output, size_t max_size,
 __d_drt_h mppd) {
 _g_math math_c = {0};


 }
 */
void *
ref_to_val_af_math(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  int m_ret = 0, m_ret2;
  md_init(&mppd->chains, 8);
  md_init(&mppd->math, 8);

  if ((m_ret2 = g_process_math_string(mppd->hdl, match, &mppd->math,
      &mppd->chains, &m_ret, NULL, 0, 0)))
    {
      print_str("ERROR: [%d] [%d]: could not process math string\n", m_ret2,
          m_ret);
      return NULL;
    }

  if (mppd->math.offset)
    {
      __g_math math_f = m_get_def_val(&mppd->math);

      switch (math_f->flags & F_MATH_TYPES)
        {
      case F_MATH_INT:
        switch (math_f->vb)
          {
        case 1:
          return as_ref_to_val_lk(match, dt_rval_spec_math_u8, mppd, "%hhu");
        case 2:
          return as_ref_to_val_lk(match, dt_rval_spec_math_u16, mppd, "%hu");
        case 4:
          return as_ref_to_val_lk(match, dt_rval_spec_math_u32, mppd, "%u");
        case 8:
          return as_ref_to_val_lk(match, dt_rval_spec_math_u64, mppd, "%llu");
        default:
          return NULL;
          }
        break;
      case F_MATH_INT_S:
        switch (math_f->vb)
          {
        case 1:
          return as_ref_to_val_lk(match, dt_rval_spec_math_s8, mppd, "%hhd");
        case 2:
          return as_ref_to_val_lk(match, dt_rval_spec_math_s16, mppd, "%hd");
        case 4:
          return as_ref_to_val_lk(match, dt_rval_spec_math_s32, mppd, "%d");
        case 8:
          return as_ref_to_val_lk(match, dt_rval_spec_math_s64, mppd, "%lld");
        default:
          return NULL;
          }
        break;
      case F_MATH_FLOAT:
        return as_ref_to_val_lk(match, dt_rval_spec_math_f, mppd, "%.2f");
      default:
        return NULL;
        }
    }
  else
    {
      return NULL;
    }
}

#define RT_AF_RTP(arg, match, output, max_size, mppd) { \
  mppd->mppd_next = l_mppd_create_copy(mppd); \
  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size, mppd->mppd_next); \
  if (!mppd->fp_rval1) \
    { \
      return NULL; \
    } \
}

static void*
rt_af_basedir(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  RT_AF_RTP(arg, match, output, max_size, mppd);
  return as_ref_to_val_lk(match, dt_rval_spec_basedir, mppd, "%s");
}

static void*
rt_af_dirname(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  RT_AF_RTP(arg, match, output, max_size, mppd);
  return as_ref_to_val_lk(match, dt_rval_spec_dirname, mppd, "%s");
}

static void*
rt_af_conditional(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  int ret;

  mppd->rt_cond = calloc(1, sizeof(_rt_c));
  __rt_c cond = (void*) mppd->rt_cond;

  char *ptr, *lom_st = ptr = strdup(match), *trigger;

  while (ptr[0] && ptr[0] != 0x3A)
    {
      ptr++;
      match++;
    }

  if (ptr[0] != 0x3A)
    {
      free(lom_st);
      free(mppd->rt_cond);
      mppd->rt_cond = NULL;
      return NULL;
    }

  ptr[0] = 0x0;
  ptr++;
  match++;

  trigger = match;

  int r;

  if ((r = g_process_lom_string(mppd->hdl, lom_st, &cond->match, &ret,
  F_GM_ISLOM | F_GM_NAND)))
    {
      print_str("ERROR: %s: [%d] [%d]: could not load LOM string\n",
          mppd->hdl->file, r, ret);
      free(lom_st);
      free(mppd->rt_cond);
      mppd->rt_cond = NULL;
      return NULL;
    }

  free(lom_st);

  mppd->mppd_next = l_mppd_create_copy(mppd);
  mppd->mppd_aux_next = l_mppd_create_copy(mppd);

  cond->p_exec = mppd->hdl->g_proc1_lookup(arg, trigger, output, max_size,
      mppd->mppd_next);

  if (NULL == cond->p_exec)
    {
      print_str("ERROR: rt_af_conditional->p_exec: could not resolve '%s'\n",
          trigger);
      free(mppd->rt_cond);
      mppd->rt_cond = NULL;
      return NULL;
    }

  if (NULL == ((__d_drt_h ) mppd->mppd_next)->varg_l
      || ((__d_drt_h ) mppd->mppd_next)->varg_l[0] == 0x0)
    {
      print_str("ERROR: rt_af_conditional: could not resolve 'else' proc\n");
      return NULL;
    }

  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg,
      ((__d_drt_h ) mppd->mppd_next)->varg_l, output, max_size,
      mppd->mppd_aux_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_conditional->fp_rval1: could not resolve '%s'\n",
          ((__d_drt_h ) mppd->mppd_next)->varg_l);
      return NULL;
    }

  return as_ref_to_val_lk(match, dt_rval_spec_conditional, mppd, "%s");
}

static void*
rt_af_print(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  mppd->st_p = strdup(match);
  char *ptr = mppd->st_p;

  while (ptr[0] && ptr[0] != 0x7D)
    {
      ptr++;
    }

  if (ptr[0] == 0x7D)
    {
      ptr[0] = 0x0;
    }

  return as_ref_to_val_lk(match, dt_rval_spec_print, mppd, "%s");
}

static void*
rt_af_print_format(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{

  mppd->mppd_next = l_mppd_create_copy(mppd);

  mppd->st_p0 = strdup(match);

  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
      mppd->mppd_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_print_format: could not resolve '%s'\n", match);

      return NULL;
    }

  return as_ref_to_val_lk(match, dt_rval_spec_print_format, mppd, "%s");
}

static char *
dt_rval_xstat(void *arg, char *match, char *output, size_t max_size, void *mppd)
{
  __d_drt_h mppd_x = (__d_drt_h ) ((__d_drt_h ) mppd)->mppd_next;
  __d_drt_h mppd_f = (__d_drt_h ) ((__d_drt_h ) mppd)->mppd_aux_next;

  char *p_o = ((__d_drt_h ) mppd)->fp_rval2(arg, ((__d_drt_h ) mppd)->varg_l,
      output, max_size, mppd_f);

  if (NULL != p_o)
    {
      __d_xref pxrf = ((__d_drt_h ) mppd)->v_p0;

      if (0 == g_preproc_dm(p_o, pxrf, 0x0, NULL))
        {
          char *x_p_o = ((__d_drt_h ) mppd)->fp_rval1((void*) pxrf,
              ((__d_drt_h ) mppd)->st_p0, ((__d_drt_h ) mppd)->tp_b0,
              sizeof(((__d_drt_h ) mppd)->tp_b0), mppd_x);

          snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, x_p_o);
        }
      else
        {
          output[0] = 0x0;
        }
    }
  else
    {
      output[0] = 0x0;
    }

  return output;
}

static void*
rt_af_xstat(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{

  mppd->mppd_next = l_mppd_create_copy(mppd);
  mppd->mppd_aux_next = l_mppd_create_copy(mppd);

  mppd->st_p0 = strdup(match);
  mppd->v_p0 = calloc(1, sizeof(_d_xref));

  mppd->fp_rval1 = ref_to_val_lk_x(mppd->v_p0, match, output, max_size,
      mppd->mppd_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_xstat: could not resolve '%s'\n", match);
      return NULL;
    }

  if (NULL == ((__d_drt_h ) mppd->mppd_next)->varg_l
      || ((__d_drt_h ) mppd->mppd_next)->varg_l[0] == 0x0)
    {
      print_str("ERROR: rt_af_xstat: could not resolve field argument\n");
      return NULL;
    }

  mppd->fp_rval2 = mppd->hdl->g_proc1_lookup(arg,
      ((__d_drt_h ) mppd->mppd_next)->varg_l, output, max_size,
      mppd->mppd_aux_next);

  if (NULL == mppd->fp_rval2)
    {
      print_str("ERROR: rt_af_xstat: could not resolve '%s'\n",
          ((__d_drt_h ) mppd->mppd_next)->varg_l);
      return NULL;
    }

  return as_ref_to_val_lk(match, dt_rval_xstat, mppd, "%s");
}

void *
ref_to_val_af(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  match++;
  char *id = match;
  size_t i = 0;
  while (match[0] != 0x3A && match[0])
    {
      i++;
      match++;
    }
  if (match[0] != 0x3A)
    {
      return NULL;
    }
  match++;
  if (0x0 == match[0])
    {
      return NULL;
    }

  if (i < 6)
    {
      switch (id[0])
        {
      case 0x58:
        return rt_af_xstat(arg, match, output, max_size, mppd);
      case 0x50:
        return rt_af_print_format(arg, match, output, max_size, mppd);
      case 0x70:
        return rt_af_print(arg, match, output, max_size, mppd);
      case 0x4C:
        return rt_af_conditional(arg, match, output, max_size, mppd);
      case 0x62:
        return rt_af_basedir(arg, match, output, max_size, mppd);
      case 0x64:
        return rt_af_dirname(arg, match, output, max_size, mppd);
      case 0x71:
        return as_ref_to_val_lk(match, ((__d_drt_h ) mppd)->st_ptr0,
            (__d_drt_h ) mppd, "%s");
        break;
      case 0x6C:
        ;
        mppd->mppd_next = l_mppd_create_copy(mppd);

        mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
            mppd->mppd_next);

        if (NULL == mppd->fp_rval1)
          {
            return NULL;
          }
        return as_ref_to_val_lk(match, dt_rval_spec_slen, (__d_drt_h ) mppd,
        NULL);
        break;
      case 0x74:
        ;
        strncpy(mppd->direc, "%d/%h/%Y %H:%M:%S", 18);
        as_ref_to_val_lk(match, NULL, mppd, "%d/%h/%Y %H:%M:%S");
        if (is_ascii_numeric((uint8_t) match[0]) && match[0] != 0x2B
            && match[0] != 0x2D)
          {
            int vb;
            mppd->vp_off1 = (size_t) mppd->hdl->g_proc2(mppd->hdl->_x_ref,
                match, &vb);
            switch (id[1])
              {
            case 0x6C:
              return dt_rval_spec_tf_pl;
              break;
            default:
              return dt_rval_spec_tf_pgm;
              break;
              }
          }
        else
          {
            errno = 0;
            mppd->ts_1 = (time_t) strtol(match, NULL, 10);
            if (errno == ERANGE)
              {
                return NULL;
              }

            switch (id[1])
              {
            case 0x6C:
              return dt_rval_spec_tf_l;
              break;
            default:
              return dt_rval_spec_tf_gm;
              break;
              }
          }

        break;
      case 0x63:
        ;
        char *c = match;
        while (match[0] != 0x3A && match[0] != 0x29 && match[0])
          {
            match++;
          }

        if (!match[0])
          {
            return NULL;
          }

        match++;

        if (match[0] == 0x5C)
          {
            match++;
            switch (match[0])
              {
            case 0x6E:
              mppd->c_1 = 0xA;
              break;
            case 0x72:
              mppd->c_1 = 0xD;
              break;
            case 0x5C:
              mppd->c_1 = 0x5C;
              break;
            case 0x74:
              mppd->c_1 = 0x9;
              break;
            case 0x73:
              mppd->c_1 = 0x20;
              break;
            default:
              mppd->c_1 = match[0];
              }
          }
        else
          {
            mppd->c_1 = match[0];
          }

        errno = 0;
        mppd->t_1 = strtoul(c, NULL, 10);
        if (mppd->t_1 == ULONG_MAX && errno == ERANGE)
          {
            return NULL;
          }

        return dt_rval_spec_gc;
      case 0x6D:
        ;
        return ref_to_val_af_math(arg, match, output, max_size, mppd);
        break;
      case 0x72:
        ;
        char *p_match;

        if (match[0] == 0x28)
          {
            p_match = &match[1];
            size_t m_dep = 0;

            while (match[0])
              {
                if (match[0] == 0x28)
                  {
                    m_dep++;
                  }
                else if (match[0] == 0x29)
                  {
                    m_dep--;
                    if (!m_dep)
                      {
                        break;
                      }
                  }
                match++;
              }

            if (!match[0])
              {
                return NULL;
              }

            match++;
          }
        else
          {
            p_match = match;
            while (match[0] != 0x3A && match[0])
              {
                match++;
              }
          }

        if (match[0] != 0x3A)
          {
            return NULL;
          }

        match++;

        char *r_match = match;
        size_t r_l = 0;

        while (((match[0] != 0x7D && match[0] != 0x3A && match[0] != 0x2C)
            || match[-1] == 0x5C) && match[0])
          {
            match++;
            r_l++;
          }

        if (!match[0])
          {
            return NULL;
          }

        if (r_l > MAX_RR_IN || !r_l)
          {
            return NULL;
          }

        char *r_b = malloc(r_l + 1);

        //strncpy(r_b, r_match, r_l);

        match = r_match;
        r_l = 0;
        while (((match[0] != 0x7D && match[0] != 0x3A && match[0] != 0x2C)
            || match[-1] == 0x5C) && match[0])
          {
            if (match[0] == 0x5C && match[1] != 0x5C)
              {
                match++;
                continue;
              }
            r_b[r_l] = match[0];
            match++;
            r_l++;
          }

        r_b[r_l] = 0x0;

        mppd->mppd_next = l_mppd_create_copy(mppd);
        mppd->fp_rval2 = mppd->hdl->g_proc1_lookup(arg, p_match, output,
            max_size, mppd->mppd_next);

        if (!mppd->fp_rval2)
          {
            free(r_b);
            return NULL;
          }

        void *cb_p;

        char *id_f = id;
        id_f++;
        if (id[1] == 0x64)
          {
            cb_p = dt_rval_spec_regsub_d;
          }
        else if (id[1] == 0x73)
          {
            cb_p = dt_rval_spec_regsub;
            if (match[0] == 0x3A)
              {
                match++;
                mppd->r_rep_l = 0;
                while (((match[0] != 0x7D && match[0] != 0x2C)
                    || match[-1] == 0x5C) && match[0] && mppd->r_rep_l < 2048)
                  {
                    if (match[0] == 0x5C && match[1] != 0x5C)
                      {
                        match++;
                        continue;
                      }
                    mppd->r_rep[mppd->r_rep_l] = match[0];
                    mppd->r_rep_l++;
                    match++;
                  }
                mppd->r_rep[mppd->r_rep_l] = 0x0;
              }
          }
        else
          {
            free(r_b);
            return NULL;
          }

        if (id_f[1] == 0x2F)
          {
            id_f = &id[2];
            while (id_f[0] != 0x3A && id_f[0])
              {
                switch (id_f[0])
                  {
                case 0x67:
                  if (cb_p == dt_rval_spec_regsub_d)
                    {
                      cb_p = dt_rval_spec_regsub_dg;
                    }
                  else
                    {
                      cb_p = dt_rval_spec_regsub_g;
                    }
                  break;
                case 0x69:
                  mppd->regex_flags |= REG_ICASE;
                  break;
                  }
                id_f++;
              }
          }

        int rr = regcomp(&mppd->preg, r_b, REG_EXTENDED | mppd->regex_flags);

        mppd->flags |= _D_DRT_HASREGEX;

        free(r_b);

        if (rr)
          {
            return NULL;
          }

        return as_ref_to_val_lk(r_match, cb_p, mppd, "%s");
        }
    }
  return NULL;
}

int
rtv_q(void *query, char *output, size_t max_size)
{
  mda md_s =
    { 0 };

  md_init(&md_s, 2);
  p_md_obj ptr;

  if (split_string(query, 0x40, &md_s) != 2)
    {
      bzero(output, max_size);
      return 0;
    }

  ptr = md_s.objects;

  char *rtv_l = g_dgetf((char*) ptr->ptr);

  if (!rtv_l)
    {
      bzero(output, max_size);
      return 0;
    }

  ptr = ptr->next;

  int r = 0;
  char *rtv_q = (char*) ptr->ptr;

  if (!strncmp(rtv_q, _MC_GLOB_SIZE, 4))
    {
      snprintf(output, max_size, "%llu", (ulint64_t) get_file_size(rtv_l));
    }
  else if (!strncmp(rtv_q, "count", 5))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl, hdl.file))
        {
          goto end;
        }

      snprintf(output, max_size, "%llu",
          (ulint64_t) get_file_size(rtv_l) / (ulint64_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, "corrupt", 7))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl, hdl.file))
        {
          goto end;
        }

      snprintf(output, max_size, "%llu",
          (ulint64_t) get_file_size(rtv_l) % (ulint64_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, "bsize", 5))
    {
      _g_handle hdl =
        { 0 };
      size_t rtv_ll = strlen(rtv_l);

      strncpy(hdl.file, rtv_l, rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
      if (determine_datatype(&hdl, hdl.file))
        {
          goto end;
        }

      snprintf(output, max_size, "%u", (uint32_t) hdl.block_sz);
    }
  else if (!strncmp(rtv_q, _MC_GLOB_MODE, 4))
    {
      return g_l_fmode_n(rtv_l, max_size, output);
    }
  else if (!strncmp(rtv_q, "file", 4))
    {
      snprintf(output, max_size, "%s", rtv_l);
    }
  else if (!strncmp(rtv_q, "read", 4))
    {
      snprintf(output, max_size, "%d",
          !access(rtv_l, R_OK) ? 1 : errno == EACCES ? 0 : -1);
    }
  else if (!strncmp(rtv_q, "write", 5))
    {
      snprintf(output, max_size, "%d",
          !access(rtv_l, W_OK) ? 1 : errno == EACCES ? 0 : -1);
    }
#ifdef HAVE_ZLIB_H
  else if (!strncmp(rtv_q, "comp", 4))
    {
      snprintf(output, max_size, "%d", !g_is_file_compressed(rtv_l) ? 1 : 0);
    }
#endif
  else
    {
      bzero(output, max_size);
    }

  end:

  md_g_free(&md_s);

  return r;
}
