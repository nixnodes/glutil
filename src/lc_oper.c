/*
 * lc_oper.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <lc_oper.h>

int
g_oper_and(int s, int d)
{
  return (s && d);
}

int
g_oper_or(int s, int d)
{
  return (s || d);
}

int
g_is_higher_f(float s, float d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower_f(float s, float d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher(uint64_t s, uint64_t d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_s(int64_t s, int64_t d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower(uint64_t s, uint64_t d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_lower_s(int64_t s, int64_t d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}



int
g_is_higher_2(uint64_t s, uint64_t d)
{
  return (s > d);
}

int
g_is_higher_2_s(int64_t s, int64_t d)
{
  return (s > d);
}

int
g_is_lower_2(uint64_t s, uint64_t d)
{
  return (s < d);
}

int
g_is_lower_2_s(int64_t s, int64_t d)
{
  return (s < d);
}

int
g_is_equal(uint64_t s, uint64_t d)
{
  return (s == d);
}

int
g_is_equal_s(int64_t s, int64_t d)
{
  return (s == d);
}

int
g_is_not_equal(uint64_t s, uint64_t d)
{
  return (s != d);
}

int
g_is_not_equal_s(int64_t s, int64_t d)
{
  return (s != d);
}

int
g_is_higherorequal(uint64_t s, uint64_t d)
{
  return (s >= d);
}

int
g_is_higherorequal_s(int64_t s, int64_t d)
{
  return (s >= d);
}

int
g_is_lowerorequal(uint64_t s, uint64_t d)
{
  return (s <= d);
}

int
g_is_lowerorequal_s(int64_t s, int64_t d)
{
  return (s <= d);
}

int
g_is(uint64_t s, uint64_t d)
{
  return s != 0;
}

int
g_is_s(int64_t s, int64_t d)
{
  return s != 0;
}

int
g_is_not(uint64_t s, uint64_t d)
{
  return s == 0;
}

int
g_is_not_s(int64_t s, int64_t d)
{
  return s == 0;
}

int
g_is_higher_d(double s, double d)
{
  if (s > d)
    {
      return 0;
    }
  return 1;
}


int
g_is_lower_d(double s, double d)
{
  if (s < d)
    {
      return 0;
    }
  return 1;
}

int
g_is_higher_f_2(float s, float d)
{
  return (s > d);
}

int
g_is_lower_f_2(float s, float d)
{
  return (s < d);
}

int
g_is_equal_f(float s, float d)
{
  return (s == d);
}

int
g_is_equal_d(double s, double d)
{
  return (s == d);
}

int
g_is_not_equal_f(float s, float d)
{
  return (s != d);
}

int
g_is_higherorequal_f(float s, float d)
{
  return (s >= d);
}

int
g_is_notequal_f(float s, float d)
{
  return (s >= d);
}


int
g_is_lowerorequal_f(float s, float d)
{
  return (s <= d);
}

int
g_is_f(float s, float d)
{
  return s != 0;
}

int
g_is_not_f(float s, float d)
{
  return s == 0;
}

