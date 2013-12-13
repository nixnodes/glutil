/*
 * exech.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <exech.h>
#include <glutil.h>

#include <t_glob.h>

int
g_compile_exech(pmda mech, __g_handle hdl, char *instr)
{
  size_t pl, p1, vl1;
  char *in_ptr = instr;

  if (!in_ptr[0])
    {
      return 2600;
    }

  md_init(mech, 16);
  __d_exec_ch ptr = md_alloc(mech, sizeof(_d_exec_ch));

  if (!ptr)
    {
      return 2601;
    }

  ptr->st_ptr = in_ptr;

  if (hdl->g_proc1_lookup)
    {
      for (p1 = 0, pl = 0; in_ptr[0]; p1++, in_ptr++, pl++)
        {
          if (pl > 0 && in_ptr[-1] == 0x5C)
            {
              continue;
            }
          if (in_ptr[0] == 0x7B)
            {
              ptr->len = pl;
              pl = 0;
              ptr = md_alloc(mech, sizeof(_d_exec_ch));
              if (!ptr)
                {
                  return 2602;
                }

              do_gcb: ;

              in_ptr++;
              ptr->st_ptr = in_ptr;
              ptr->dtr.hdl = hdl;
              vl1 = 0;

              ptr->callback = hdl->g_proc1_lookup(hdl->_x_ref, ptr->st_ptr,
              NULL, 0, &ptr->dtr);
              if (!ptr->callback)
                {
                  return 2603;
                }

              while (((in_ptr[0] != 0x7D) || in_ptr[-1] == 0x5C) && in_ptr[0])
                {

                  in_ptr++;
                  vl1++;
                }
              if (!in_ptr[0])
                {
                  return 2604;
                }

              if (in_ptr[0] != 0x7D)
                {
                  return 2605;
                }

              ptr->len = vl1;
              ptr = md_alloc(mech, sizeof(_d_exec_ch));
              if (!ptr)
                {
                  return 2606;
                }

              if (in_ptr[1] && ((in_ptr[1] == 0x7B) && in_ptr[0] != 0x5C))
                {
                  in_ptr++;
                  goto do_gcb;
                }
              else
                {
                  in_ptr++;
                  ptr->st_ptr = in_ptr;
                }

              if (!in_ptr[0])
                {
                  break;
                }
            }
        }

      ptr->len = pl;
    }
  else
    {
      ptr->len = strlen(ptr->st_ptr);
    }

  return 0;

}

char *
g_exech_build_string(void *d_ptr, pmda mech, __g_handle hdl, char *output,
    size_t maxlen)
{
  p_md_obj ptr = md_first(mech);
  __d_exec_ch ch_ptr;

  size_t cw = 0;

  while (ptr)
    {
      ch_ptr = (__d_exec_ch ) ptr->ptr;
      if (!ch_ptr->callback)
        {
          if (!ch_ptr->len)
            {
              goto end_l;
            }
          if (cw + ch_ptr->len >= maxlen)
            {
              print_str(M_EXECH_DCNBT, hdl->file);
              break;
            }
          strncpy(output, ch_ptr->st_ptr, ch_ptr->len);
          output += ch_ptr->len;
          cw += ch_ptr->len;
        }
      else
        {
          char *rs_ptr = ch_ptr->callback(d_ptr, ch_ptr->st_ptr, hdl->mv1_b,
          MAX_VAR_LEN, &ch_ptr->dtr);
          if (rs_ptr)
            {
              size_t rs_len = strlen(rs_ptr);
              if (cw + rs_len >= maxlen)
                {
                  print_str(M_EXECH_DCNBT, hdl->file);
                  break;
                }
              if (!rs_len)
                {
                  goto end_l;
                }
              strncpy(output, rs_ptr, rs_len);
              output += rs_len;
              cw += rs_len;
            }
        }
      end_l:

      ptr = ptr->next;
    }
  output[0] = 0x0;
  return output;
}

