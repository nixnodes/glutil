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
#include <exech.h>

#include <errno.h>
#include <time.h>
#include <unistd.h>
#ifdef _G_SSYS_THREAD
#include <pthread.h>
#endif

#include <lref.h>
#include <xref.h>

char *
g_extract_vfield(char *input, char *output, size_t max_size, size_t offset)
{
  char *o_out = output;
  input += offset;
  size_t c = 0;

  max_size--;

  while ((input[0] != 0x7D) && 0 != input[0] && c < max_size)
    {

      while (input[0] == 0x5C)
        {
          output[0] = input[0];
          input++;
          output++;
          c++;
        }
      output[0] = input[0];
      input++;
      output++;
      c++;
    }

  if (c == max_size)
    {
      return NULL;
    }

  return o_out;
}

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

static char*
l_mppd_shex_resnp(char *input, char *output, size_t max_size, void** l_nr,
    uint32_t flags)
{
  size_t st_len = strlen(input);

  if (st_len >= max_size)
    {
      st_len = max_size - 1;
    }

  strncpy(output, input, st_len);
  output[st_len] = 0x0;

  size_t c = 0, w_c = 0;

  char *ptr = output;

  while (ptr[0] && ptr[0] != 0x3A && ptr[0] != 0x7D && ptr[0] != 0x23)
    {
      if (ptr[0] == 0x5C)
        {
          memmove(ptr, &ptr[1], strlen(&ptr[1]) + 1);
          c++;
        }
      c++;
      w_c++;
      ptr++;
    }

  if (flags & F_MPPD_SHX_TZERO)
    {
      output[w_c] = 0x0;
    }

  if ( NULL != l_nr)
    {
      ptr = &input[c];

      if (ptr[0] == 0x3A)
        {
          ptr++;
        }

      *l_nr = (void*) ptr;
    }

  return output;
}

char*
l_mppd_shell_ex(char *input, char *output, size_t max_size, void** l_nr, char l,
    char r, uint32_t flags)
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
      return l_mppd_shex_resnp(ptr, output, max_size, l_nr, flags);
    }

  size_t st_len = strlen(ptr);

  if (st_len >= max_size)
    {
      st_len = max_size - 1;
    }

  strncpy(output, ptr, st_len);
  output[st_len] = 0x0;

  uint32_t lvl = 1;
  size_t c = 0;

  ptr = output;

  while (0x0 != ptr[0])
    {
      if (ptr[0] == 0x5C)
        {
          memmove(ptr, &ptr[1], strlen(&ptr[1]) + 1);
          c++;
        }
      else if (ptr[0] == left)
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

  if ( NULL != l_nr)
    {
      ptr = &input[c + 2];

      if (ptr[0] == 0x3A)
        {
          ptr++;
        }

      *l_nr = (void*) ptr;
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

  if (NULL != p_o)
    {
      snprintf(output, max_size, "%zu", strlen(p_o));
    }
  else
    {
      return "0";
    }
  return output;
}

#ifdef _G_SSYS_CRYPTO

#include <g_crypto.h>

static char *
dt_rval_spec_sha1(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0),
      ((__d_drt_h ) mppd)->mppd_next);

  if (NULL != p_o)
    {
      _pid_sha1 out, *o_ptr = crypto_calc_sha1((unsigned char*) p_o,
          strlen(p_o), &out);
      if (NULL != o_ptr)
        {
          return crypto_sha1_to_ascii(o_ptr, output);
        }
      else
        {
          output[0] = 0x0;
          return output;
        }
    }
  else
    {
      output[0] = 0x0;
      return output;
    }
  return output;
}
#endif

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
      snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, p_o);
      g_dirname(output);
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
dt_rval_spec_print_format_int(void *arg, char *match, char *output,
    size_t max_size, void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h ) mppd;
  __d_drt_h _mppd_next = _mppd->mppd_next;

  char *s_ptr;
  if (NULL
      == (s_ptr = g_exech_build_string(arg, &_mppd->sub_mech, _mppd->hdl,
          _mppd_next->tp_b0, sizeof(_mppd_next->tp_b0))))
    {
      print_str("ERROR: g_omfp_eassemble: could not assemble print string\n");
#ifdef _G_SSYS_THREAD
      mutex_lock(&mutex_glob00);
#endif
      gfl |= F_OPT_KILL_GLOBAL;
#ifdef _G_SSYS_THREAD
              pthread_mutex_unlock(&mutex_glob00);
#endif
      output[0] = 0x0;
      return output;
    }

  snprintf(output, max_size, _mppd->direc, _mppd_next->tp_b0);

  return output;
}

typedef struct tm*
(*g_tp_p)(const time_t *__timer);

static char *
dt_rval_spec_tf_m(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_tp_p tproc_ptr = (g_tp_p) ((__d_drt_h ) mppd)->st_p1;

  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_b);

  time_t uts = *((time_t*) v_ptr);

  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, tproc_ptr(&uts));
  return output;
}

static char *
dt_rval_spec_tf_p(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_tp_p tproc_ptr = (g_tp_p) ((__d_drt_h ) mppd)->st_p1;
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, tproc_ptr(&uts));
  return output;
}

static char *
dt_rval_spec_tf_gm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_tp_p tproc_ptr = (g_tp_p) ((__d_drt_h ) mppd)->st_p1;
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      tproc_ptr(&((__d_drt_h ) mppd)->ts_1));
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

  char *rs_p = _mppd->fp_rval1(arg, match, ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);
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

  char *rs_p = _mppd->fp_rval1(arg, match, _mppd->tp_b0,
      sizeof(_mppd->tp_b0), _mppd->mppd_next);

  char *rep_p;
  if ( NULL != _mppd->fp_rval2)
    {
      rep_p = _mppd->fp_rval2(arg, (char*)_mppd->varg_l, _mppd->r_rep,
          sizeof(_mppd->r_rep), _mppd->mppd_aux_next);
      _mppd->r_rep_l = strlen(rep_p);
    }
  else
    {
      rep_p = (char*)_mppd->r_rep;
    }

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
          strncpy(&rs_o[rs_w], rep_p, _mppd->r_rep_l);
          rs_o[_mppd->r_rep_l+rs_w] = 0x0;
          return rs_o;
        }
      if (rm[0].rm_so == rm[0].rm_eo)
        {
          if (!rm[0].rm_so)
            {
              strncpy(rs_o, rep_p, _mppd->r_rep_l);
              strncpy(&rs_o[_mppd->r_rep_l], rs_p, o_l);
              rs_o[_mppd->r_rep_l+o_l] = 0x0;
            }
          else if (rm[0].rm_so == o_l)
            {
              strncpy(rs_o, rs_p, o_l);
              strncpy(&rs_o[o_l], rep_p, _mppd->r_rep_l);
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
      strncpy(&rs_o[rs_w], rep_p, _mppd->r_rep_l);
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

  char *rs_p = _mppd->fp_rval1(arg, match, ((__d_drt_h ) mppd)->tp_b0,
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

  char *rs_p = _mppd->fp_rval1(arg, match,
      ((__d_drt_h ) mppd)->tp_b0, sizeof(((__d_drt_h ) mppd)->tp_b0), _mppd->mppd_next);

  char *rep_p;
  if ( NULL != _mppd->fp_rval2)
    {
      rep_p = _mppd->fp_rval2(arg, (char*)_mppd->varg_l, _mppd->r_rep,
          sizeof(_mppd->r_rep), _mppd->mppd_aux_next);
      _mppd->r_rep_l = strlen(rep_p);
    }
  else
    {
      rep_p = (char*)_mppd->r_rep;
    }

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
  strncpy(&rs_o[rm[0].rm_so], rep_p, _mppd->r_rep_l);
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
      if (ptr[0] == 0x5C)
        {
          memmove(ptr, &ptr[1], strlen(&ptr[1]) + 1);
        }
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

static void*
rt_af_print_format_int(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
  int r;

  __d_drt_h _mppd = mppd;

  void *l_next_ref;

  char *s_ptr = l_mppd_shell_ex(match, _mppd->tp_b0, sizeof(_mppd->tp_b0),
      &l_next_ref,
      LMS_EX_L,
      LMS_EX_R, F_MPPD_SHX_TZERO);

  if (NULL == s_ptr)
    {
      print_str(
          "ERROR: rt_af_print_format_int: could not parse print string: '%s'\n",
          match);
    }

  mppd->mppd_next = l_mppd_create_copy(mppd);

  if ((r = g_compile_exech(&_mppd->sub_mech, _mppd->hdl, _mppd->tp_b0)))
    {
      print_str(
          "ERROR: rt_af_print_format_int: [%d]: could not compile print string: '%s'\n",
          r, s_ptr);
      return NULL;
    }

  return as_ref_to_val_lk(match, dt_rval_spec_print_format_int, mppd, "%s");
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

static void*
rt_af_regex(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd, char *id)
{
  char *r_match = match;

  mppd->mppd_next = l_mppd_create_copy(mppd);
  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
      mppd->mppd_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_regex: could not resolve directive: '%s'\n",
          match);
      return NULL;
    }

  if ( NULL == ((__d_drt_h ) mppd->mppd_next)->varg_l
      || ((__d_drt_h ) mppd->mppd_next)->varg_l[0] == 0x0)
    {
      print_str("ERROR: rt_af_regex: could not resolve pattern\n");
      return NULL;
    }

  char *r_b = malloc(REG_PATTERN_BUFFER_MAX + 1);
  void *l_next_ref = NULL;

  char *s_ptr = l_mppd_shell_ex(((__d_drt_h ) mppd->mppd_next)->varg_l, r_b,
  REG_PATTERN_BUFFER_MAX, &l_next_ref,
  LMS_EX_L,
  LMS_EX_R, F_MPPD_SHX_TZERO);

  if (NULL == s_ptr)
    {
      print_str("ERROR: rt_af_regex: could not parse pattern: '%s'\n",
          ((__d_drt_h ) mppd->mppd_next)->varg_l);
      return NULL;
    }

  match = (char*) l_next_ref;

  if (NULL == match)
    {
      print_str(
          "ERROR: rt_af_regex: could not extract pattern: l_mppd_shell_ex failed\n");
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
      if (match[0] == 0x0)
        {
          mppd->r_rep[0] = 0x0;
          mppd->r_rep_l = 0;

        }
      else
        {
          if (match[0] == 0x28)
            {

              mppd->mppd_aux_next = l_mppd_create_copy(mppd);
              mppd->fp_rval2 = mppd->hdl->g_proc1_lookup(arg, match, output,
                  max_size, mppd->mppd_aux_next);

              if (NULL == mppd->fp_rval1)
                {
                  print_str(
                      "ERROR: rt_af_regex: could not resolve directive: '%s'\n",
                      match);
                  return NULL;
                }
            }
          else
            {
              char *ptr = l_mppd_shell_ex(match, mppd->r_rep,
              REG_PATTERN_BUFFER_MAX, &l_next_ref,
              LMS_EX_L,
              LMS_EX_R, F_MPPD_SHX_TZERO);

              if (NULL == ptr)
                {
                  print_str(
                      "ERROR: rt_af_regex: could not process substitution string\n");
                  free(r_b);
                  return NULL;
                }

              mppd->r_rep_l = strlen(mppd->r_rep);
            }
        }
    }
  else
    {
      free(r_b);
      return NULL;
    }

  mppd->regex_flags |= REG_EXTENDED;

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
          case 0x6E:
            mppd->regex_flags |= REG_NEWLINE;
            break;
          case 0x62:
            if (mppd->regex_flags & REG_EXTENDED)
              {
                mppd->regex_flags ^= REG_EXTENDED;
              }
            break;
            }
          id_f++;
        }
    }

  int rr = regcomp(&mppd->preg, s_ptr, mppd->regex_flags);

  if (rr)
    {
      print_str("ERROR: rt_af_regex: [%d] could not compile pattern: '%s'\n",
          rr, s_ptr);
      free(r_b);
      return NULL;
    }

  free(r_b);
  mppd->flags |= _D_DRT_HASREGEX;

  return as_ref_to_val_lk(r_match, cb_p, mppd, "%s");
}

static void*
rt_af_time(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd, char *id)
{
  strncpy(mppd->direc, "%d/%h/%Y %H:%M:%S", 18);
  as_ref_to_val_lk(match, NULL, mppd, "%d/%h/%Y %H:%M:%S");

  switch (id[1])
    {
  case 0x6C:
    mppd->st_p1 = (void*) localtime;
    break;
  default:
    mppd->st_p1 = (void*) gmtime;
    break;
    }

  if (match[0] == 0x28)
    {
      md_init(&mppd->chains, 8);
      md_init(&mppd->math, 8);

      int ret, p_ret;

      if ((ret = g_process_math_string(mppd->hdl, match, &mppd->math,
          &mppd->chains, &p_ret, NULL, 0, 0)))
        {
          print_str(
              "ERROR: rt_af_time: [%d] [%d]: could not process math string\n",
              ret, p_ret);
          return NULL;
        }

      return dt_rval_spec_tf_m;
    }
  else
    {
      if (is_ascii_numeric((uint8_t) match[0]) && match[0] != 0x2B
          && match[0] != 0x2D)
        {
          int vb;
          mppd->vp_off1 = (size_t) mppd->hdl->g_proc2(mppd->hdl->_x_ref, match,
              &vb);

          return dt_rval_spec_tf_p;
        }
      else
        {
          errno = 0;
          mppd->ts_1 = (time_t) strtol(match, NULL, 10);
          if (errno == ERANGE || errno == EINVAL)
            {
              print_str("ERROR: rt_af_time: numeric conversion failed: '%s'",
                  match);
              return NULL;
            }
          return dt_rval_spec_tf_gm;
        }
    }
}

static void*
rt_af_spec_chr(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{
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

}

#include <cfgv.h>

static char *
dt_rval_spec_sf(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h ) mppd;
  char *p_b0 = _mppd->fp_rval1(arg, match, _mppd->tp_b0, sizeof(_mppd->tp_b0),
      _mppd->mppd_next);

  return ref_to_val_get_cfgval(p_b0, _mppd->r_rep,
  NULL,
  F_CFGV_BUILD_FULL_STRING, output, max_size);
}

static void*
rt_af_sf(void *arg, char *match, char *output, size_t max_size, __d_drt_h mppd)
{

  mppd->mppd_next = l_mppd_create_copy(mppd);

  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
      mppd->mppd_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_sf: could not resolve: '%s'\n", match);
      return NULL;
    }

  if (NULL == ((__d_drt_h ) mppd->mppd_next)->varg_l
      || ((__d_drt_h ) mppd->mppd_next)->varg_l[0] == 0x0)
    {
      print_str("ERROR: rt_af_sf: could not resolve field argument: '%s'\n",
          match);
      return NULL;
    }

  void *l_next_ref;

  char *ptr = l_mppd_shell_ex(((__d_drt_h ) mppd->mppd_next)->varg_l,
      mppd->r_rep, sizeof(mppd->r_rep), &l_next_ref,
      LMS_EX_L,
      LMS_EX_R, F_MPPD_SHX_TZERO);

  if (NULL == ptr || ptr[0] == 0x0)
    {
      print_str("ERROR: rt_af_sf: could not resolve key: %s\n",
          ((__d_drt_h ) mppd->mppd_next)->varg_l);
      return NULL;
    }

  return as_ref_to_val_lk(match, dt_rval_spec_sf, mppd, "%s");
}

static char *
dt_rval_so_isascii(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h ) mppd;
  char *p_b0 = _mppd->fp_rval1(arg, match, _mppd->tp_b0, sizeof(_mppd->tp_b0),
      _mppd->mppd_next);

  while (0 != p_b0[0])
    {
      if (!(p_b0[0] > 0 && p_b0[0] < 0x80))
        {
          output[0] = 0x30;
          output[1] = 0x0;
          return output;
        }
      p_b0++;
    }
  output[0] = 0x31;
  output[1] = 0x0;

  return output;
}

static void*
rt_af_strops_go(void *input, char *match, __d_drt_h mppd)
{
  if (!strncmp(input, "ascii", 5))
    {
      return as_ref_to_val_lk(match, dt_rval_so_isascii, mppd, "%s");
    }
  else
    {
      print_str("ERROR: rt_af_strops_go: invalid option: '%s'\n", input);
    }

  return NULL;
}

static void*
rt_af_strops(void *arg, char *match, char *output, size_t max_size,
    __d_drt_h mppd)
{

  mppd->mppd_next = l_mppd_create_copy(mppd);

  mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
      mppd->mppd_next);

  if (NULL == mppd->fp_rval1)
    {
      print_str("ERROR: rt_af_strops: could not resolve: '%s'\n", match);
      return NULL;
    }

  if (NULL == ((__d_drt_h ) mppd->mppd_next)->varg_l
      || ((__d_drt_h ) mppd->mppd_next)->varg_l[0] == 0x0)
    {
      print_str("ERROR: rt_af_strops: could not resolve field argument: '%s'\n",
          match);
      return NULL;
    }

  void *l_next_ref;

  char *ptr = l_mppd_shell_ex(((__d_drt_h ) mppd->mppd_next)->varg_l,
      mppd->r_rep, sizeof(mppd->r_rep), &l_next_ref,
      LMS_EX_L,
      LMS_EX_R, F_MPPD_SHX_TZERO);

  if (NULL == ptr || ptr[0] == 0x0)
    {
      print_str("ERROR: rt_af_strops: could not resolve option: %s\n",
          ((__d_drt_h ) mppd->mppd_next)->varg_l);
      return NULL;
    }

  return rt_af_strops_go(ptr, match, mppd);

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
      case 0x51:
        return rt_af_print_format_int(arg, match, output, max_size, mppd);
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
        return rt_af_time(arg, match, output, max_size, mppd, id);
      case 0x63:
        ;
        return rt_af_spec_chr(arg, match, output, max_size, mppd);
      case 0x6D:
        ;
        return ref_to_val_af_math(arg, match, output, max_size, mppd);
        break;
      case 0x72:
        ;
        return rt_af_regex(arg, match, output, max_size, mppd, id);
        break;
      case 0x78:
        ;
        return rt_af_sf(arg, match, output, max_size, mppd);
        break;
      case 0x73:
        ;
        return rt_af_strops(arg, match, output, max_size, mppd);
        break;
#ifdef _G_SSYS_CRYPTO
      case 0x53:
        mppd->mppd_next = l_mppd_create_copy(mppd);

        mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
            mppd->mppd_next);

        if (NULL == mppd->fp_rval1)
          {
            return NULL;
          }

        return as_ref_to_val_lk(match, dt_rval_spec_sha1, (__d_drt_h ) mppd,
        NULL);
        break;
#endif
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

  if (split_string_l(query, 0x40, &md_s, 2) != 2)
    {
      output[0] = 0x0;
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
