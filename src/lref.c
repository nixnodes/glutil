/*
 * lref.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include <l_sb.h>
#include <i_math.h>
#include <log_op.h>
#include <x_f.h>
#include <gv_off.h>
#include <str.h>

#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <lref.h>
#include <xref.h>

char *
g_get_stf(char *match)
{
  while (match[0] != 0x7D && match[0] != 0x23 && match[0])
    {
      match++;
    }
  if (match[0] == 0x23)
    {
      match++;
      return match;
    }
  return NULL;
}

char *
dt_rval_spec_slen(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  char *p_o = ((__d_drt_h ) mppd)->fp_rval1(arg, match, output, max_size, mppd);

  if (p_o)
    {
      snprintf(output, max_size, "%zu", strlen(p_o));
    }
  else
    {
      output[0] = 0x0;
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

char *
dt_rval_spec_tf_pl(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, localtime(&uts));
  return output;
}

char *
dt_rval_spec_tf_pgm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  time_t uts = (time_t) g_ts32_ptr(arg, ((__d_drt_h ) mppd)->vp_off1);
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc, gmtime(&uts));
  return output;
}

char *
dt_rval_spec_tf_l(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      localtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

char *
dt_rval_spec_tf_gm(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  strftime(output, max_size, ((__d_drt_h ) mppd)->direc,
      gmtime(&((__d_drt_h ) mppd)->ts_1));
  return output;
}

char *
dt_rval_spec_math_u64(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, &v_b);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(uint64_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_u32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, &v_b);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(uint32_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_u16(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, &v_b);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(uint16_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_u8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, &v_b);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(uint8_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_s64(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = (void*) &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_ptr);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(int64_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_s32(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = (void*) &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_ptr);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(int32_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_s16(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = (void*) &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_ptr);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(int16_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_s8(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = (void*) &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, v_ptr);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(int8_t*) v_ptr);
  return output;
}

char *
dt_rval_spec_math_f(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  unsigned char v_b[16] =
    { 0 };
  void *v_ptr = &v_b;
  g_math_res(arg, &((__d_drt_h ) mppd)->math, &v_b);
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc, *(float*) v_ptr);
  return output;
}

char *
dt_rval_spec_regsub_dg(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  __d_drt_h _mppd = (__d_drt_h) mppd;

  char *rs_p = _mppd->fp_rval2(arg, match, output, max_size, mppd);
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

  char *rs_p = _mppd->fp_rval2(arg, match, output, max_size, mppd);
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

  char *rs_p = _mppd->fp_rval2(arg, match, output, max_size, mppd);

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

  char *rs_p = _mppd->fp_rval2(arg, match, output, max_size, mppd);

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
  if (defdc)
    {
      match = g_get_stf(match);

      if (match)
        {
          if (strncmp("%s", defdc, 2))
            {
              size_t i = 0;
              while (match[0] != 0x7D && match[0] != 0x2C && match[0] != 0x29
                  && match[0] != 0x3A && match[0] && i < sizeof(mppd->direc) - 2)
                {
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
  if (!match[0])
    {
      return NULL;
    }

  if (i < 6)
    {
      switch (id[0])
        {
      case 0x6C:
        mppd->fp_rval1 = mppd->hdl->g_proc1_lookup(arg, match, output, max_size,
            mppd);
        if (!mppd->fp_rval1)
          {
            return NULL;
          }
        return as_ref_to_val_lk(match, dt_rval_spec_slen, mppd, NULL);
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
        if (errno == ERANGE)
          {
            return NULL;
          }

        return dt_rval_spec_gc;
      case 0x6D:
        ;
        int m_ret = 0, m_ret2;

        if ((m_ret2 = g_process_math_string(mppd->hdl, match, &mppd->math,
            &m_ret, 0)))
          {
            printf("ERROR: [%d] [%d]: could not process math string\n", m_ret2,
                m_ret);
            return NULL;
          }

        if (mppd->math.offset)
          {
            switch (((__g_math ) mppd->math.objects->ptr)->flags & F_MATH_TYPES)
              {
            case F_MATH_INT:
              switch (((__g_math ) mppd->math.objects->ptr)->vb)
                {
              case 1:
                return as_ref_to_val_lk(match, dt_rval_spec_math_u8, mppd,
                    "%hhu");
              case 2:
                return as_ref_to_val_lk(match, dt_rval_spec_math_u16, mppd,
                    "%hu");
              case 4:
                return as_ref_to_val_lk(match, dt_rval_spec_math_u32, mppd,
                    "%u");
              case 8:
                return as_ref_to_val_lk(match, dt_rval_spec_math_u64, mppd,
                    "%llu");
              default:
                return NULL;
                }
            case F_MATH_INT_S:
              switch (((__g_math ) mppd->math.objects->ptr)->vb)
                {
              case 1:
                return as_ref_to_val_lk(match, dt_rval_spec_math_s8, mppd,
                    "%hhd");
              case 2:
                return as_ref_to_val_lk(match, dt_rval_spec_math_s16, mppd,
                    "%hd");
              case 4:
                return as_ref_to_val_lk(match, dt_rval_spec_math_s32, mppd,
                    "%d");
              case 8:
                return as_ref_to_val_lk(match, dt_rval_spec_math_s64, mppd,
                    "%lld");
              default:
                return NULL;
                }
            case F_MATH_FLOAT:
              return as_ref_to_val_lk(match, dt_rval_spec_math_f, mppd, "%f");
            default:
              return NULL;
              }
          }
        else
          {
            return NULL;
          }
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

        mppd->fp_rval2 = mppd->hdl->g_proc1_lookup(arg, p_match, output,
            max_size, mppd);

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
  else
    {
      bzero(output, max_size);
    }

  end:

  md_g_free(&md_s);

  return r;
}
