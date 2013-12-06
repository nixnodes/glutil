/*
 * str.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <t_glob.h>
#include <l_error.h>
#include <str.h>

#include <regex.h>

char *
g_basename(char *input)
{
  char *b_ptr = strrchr(input, 0x2F);

  if (!b_ptr)
    {
      return input;
    }
  return b_ptr + 1;
}

char *
g_dirname(char *input)
{
  char *b_ptr = strrchr(input, 0x2F);
  if (!b_ptr)
    {
      input[0] = 0x0;
    }
  else
    {
      b_ptr[0] = 0x0;
    }
  return input;
}

int
remove_repeating_chars(char *string, char c)
{
  g_setjmp(0, "remove_repeating_chars", NULL, NULL);
  size_t s_len = strlen(string);
  int i, i_c = -1;

  for (i = 0; i <= s_len; i++, i_c = -1)
    {
      while (string[i + i_c] == c)
        {
          i_c++;
        }
      if (i_c > 0)
        {
          int ct_l = (s_len - i) - i_c;
          if (!memmove(&string[i], &string[i + i_c], ct_l))
            {
              return -1;
            }
          string[i + ct_l] = 0;
          i += i_c;
        }
      else
        {
          i += i_c + 1;
        }
    }

  return 0;
}

char *
strcp_s(char *dest, size_t max_size, char *source)
{
  size_t s_l = strlen(source);
  s_l >= max_size ? s_l = max_size - 1 : s_l;
  dest[s_l] = 0x0;
  return strncpy(dest, source, s_l);
}



int
is_ascii_text(uint8_t in_c)
{
  if ((in_c >= 0x0 && in_c <= 0x7F))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_lowercase_text(uint8_t in_c)
{
  if ((in_c >= 0x61 && in_c <= 0x7A))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_alphanumeric(uint8_t in_c)
{
  if ((in_c >= 0x61 && in_c <= 0x7A) || (in_c >= 0x41 && in_c <= 0x5A)
      || (in_c >= 0x30 && in_c <= 0x39))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_numeric(uint8_t in_c)
{
  if ((in_c >= 0x30 && in_c <= 0x39))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_numeric_float(uint8_t in_c)
{
  if ((in_c >= 0x30 && in_c <= 0x39) || in_c == 0x2E)
    {
      return 0;
    }

  return 1;
}

int
is_ascii_hexadecimal(uint8_t in_c)
{
  if ((in_c >= 0x41 && in_c <= 0x46) || (in_c >= 0x61 && in_c <= 0x66))
    {
      return 0;
    }
  return 1;
}

int
is_ascii_numhex(uint8_t in_c)
{
  if ((in_c >= 0x30 && in_c <= 0x39) || (in_c >= 0x41 && in_c <= 0x46)
      || (in_c >= 0x61 && in_c <= 0x66))
    {
      return 0;
    }
  return 1;
}

int
is_ascii_uppercase_text(uint8_t in_c)
{
  if ((in_c >= 0x41 && in_c <= 0x5A))
    {
      return 0;
    }

  return 1;
}

int
is_ascii_arith_bin_oper(char c)
{
  if (c == 0x2B || c == 0x2D || c == 0x2A || c == 0x2F || c == 0x25 || c == 0x26
      || c == 0x7C || c == 0x5E || c == 0x3C || c == 0x3E)
    {
      return 0;
    }
  return 1;
}


char *
s_char(char *input, char c, size_t max)
{
  size_t i = 0;
  while (input[0] && i < max)
    {
      if (input[0] == c)
        {
          return input;
        }
      input++;
      i++;
    }
  return NULL;
}

off_t
s_string(char *input, char *m, off_t offset, size_t i_l)
{
  off_t i, m_l = strlen(m);
  if (!i_l)
    {
      return offset;
    }
  for (i = offset; i <= i_l - m_l; i++)
    {
      if (!strncmp(&input[i], m, m_l))
        {
          return i;
        }
    }
  return offset;
}

off_t
s_string_r(char *input, char *m)
{
  off_t i, m_l = strlen(m), i_l = strlen(input);

  for (i = i_l - 1 - m_l; i >= 0; i--)
    {
      if (!strncmp(&input[i], m, m_l))
        {
          return i;
        }
    }
  return (off_t) -1;
}

size_t
g_floatstrlen(char *in)
{
  size_t c = 0;
  while (in[0] && !is_ascii_numeric_float((uint8_t) in[0]))
    {
      in++;
      c++;
    }
  return c;
}

int
split_string(char *line, char dl, pmda output_t)
{
  int i, p, c, llen = strlen(line);

  for (i = 0, p = 0, c = 0; i <= llen; i++)
    {

      while (line[i] == dl && line[i])
        i++;
      p = i;

      while (line[i] != dl && line[i] != 0xA && line[i])
        i++;

      if (i > p)
        {
          char *buffer = md_alloc(output_t, (i - p) + 10);
          if (!buffer)
            return -1;
          memcpy(buffer, &line[p], i - p);
          c++;
        }
    }
  return c;
}

int
split_string_sp_tab(char *line, pmda output_t)
{
  int i, p, c, llen = strlen(line);

  for (i = 0, p = 0, c = 0; i <= llen; i++)
    {

      while ((line[i] == 0x20 && line[i] != 0x9) && line[i])
        i++;
      p = i;

      while ((line[i] != 0x20 && line[i] != 0x9) && line[i] != 0xA && line[i])
        i++;

      if (i > p)
        {
          char *buffer = md_alloc(output_t, (i - p) + 10);
          if (!buffer)
            return -1;
          memcpy(buffer, &line[p], i - p);
          c++;
        }
    }
  return c;
}

int
is_char_uppercase(char c)
{
  if (c >= 0x41 && c <= 0x5A)
    {
      return 0;
    }
  return 1;
}

int
is_comp(char in)
{
  if (in == 0x21 || in == 0x3D || in == 0x3C || in == 0x3E)
    {
      return 0;
    }
  return 1;
}

int
is_opr(char in)
{
  if (in == 0x21 || in == 0x26 || in == 0x3D || in == 0x3C || in == 0x3E
      || in == 0x7C)
    {
      return 0;
    }
  return 1;
}

int
get_opr(char *in)
{
  if (!strncmp(in, "&&", 2) || !strncmp(in, "||", 2))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

char *
generate_chars(size_t num, char chr, char*buffer)
{
  g_setjmp(0, "generate_chars", NULL, NULL);
  bzero(buffer, 255);
  if (num < 1 || num > 254)
    {
      return buffer;
    }
  memset(buffer, (int) chr, num);

  return buffer;
}

char *
replace_char(char w, char r, char *string)
{
  int s_len = strlen(string), i;
  for (i = 0; i < s_len + 100 && string[i] != 0; i++)
    {
      if (string[i] == w)
        {
          string[i] = r;
        }
    }

  return string;
}

size_t
str_match(char *input, char *match)
{
  size_t i_l = strlen(input), m_l = strlen(match);

  size_t i;

  for (i = 0; i < i_l - m_l + 1; i++)
    {
      if (!strncmp(&input[i], match, m_l))
        {
          return i;
        }
    }

  return -1;
}



int
reg_match(char *expression, char *match, int flags)
{
  regex_t preg;
  size_t r;
  regmatch_t pmatch[REG_MATCHES_MAX];

  if ((r = regcomp(&preg, expression, (flags | REG_EXTENDED))))
    return r;

  r = regexec(&preg, match, REG_MATCHES_MAX, pmatch, 0);

  regfree(&preg);

  return r;
}

char *
string_replace(char *input, char *match, char *with, char *output,
    size_t max_out)
{
  size_t i_l = strlen(input), w_l = strlen(with), m_l = strlen(match);

  size_t m_off = str_match(input, match);

  if ((int) m_off < 0)
    {
      return output;
    }

  bzero(output, max_out);

  strncpy(output, input, m_off);
  strncpy(&output[m_off], with, w_l);
  strncpy(&output[m_off + w_l], &input[m_off + m_l], i_l - m_off - m_l);

  return output;
}
