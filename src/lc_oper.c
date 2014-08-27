/*
 * lc_oper.c
 *
 *  Created on: Dec 5, 2013
 *      Author: reboot
 */

#include <glutil.h>
#include "lc_oper.h"

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
g_is_higher_2(void *s, void * d)
{
  return *((uint64_t*) s) > *((uint64_t*) d);
}

int
g_is_higher_2_s(void *s, void * d)
{
  return *((int64_t*) s) > *((int64_t*) d);
}

int
g_is_lower_2(void *s, void * d)
{
  return *((uint64_t*) s) < *((uint64_t*) d);
}

int
g_is_lower_2_s(void *s, void * d)
{
  return *((int64_t*) s) < *((int64_t*) d);
}

int
g_is_equal(void *s, void * d)
{
  return (*((uint64_t*) s) == *((uint64_t*) d));
}

int
g_is_equal_s(void *s, void * d)
{
  return *((int64_t*) s) == *((int64_t*) d);
}

int
g_is_not_equal(void *s, void * d)
{
  return *((uint64_t*) s) != *((uint64_t*) d);
}

int
g_is_not_equal_s(void *s, void * d)
{
  return *((int64_t*) s) != *((int64_t*) d);
}

int
g_is_higherorequal(void *s, void * d)
{
  return *((uint64_t*) s) >= *((uint64_t*) d);
}

int
g_is_higherorequal_s(void *s, void * d)
{
  return *((int64_t*) s) >= *((int64_t*) d);
}

int
g_is_lowerorequal(void *s, void * d)
{
  return *((uint64_t*) s) <= *((uint64_t*) d);
}

int
g_is_lowerorequal_s(void *s, void * d)
{
  return *((int64_t*) s) <= *((int64_t*) d);
}

int
g_is(void *s, void * d)
{
  return *((uint64_t*) s) == *((uint64_t*) d);
}

int
g_is_s(void *s, void * d)
{
  return *((int64_t*) s) == *((int64_t*) d);
}

int
g_is_not(void *s, void * d)
{
  return *((uint64_t*) s) != *((uint64_t*) d);
}

int
g_is_not_s(void *s, void * d)
{
  return *((int64_t*) s) != *((int64_t*) d);
}

int
g_is_higher_f_2(void *s, void * d)
{
  return *((float*) s) > *((float*) d);
}

int
g_is_lower_f_2(void *s, void * d)
{
  return *((float*) s) < *((float*) d);
}

int
g_is_higherorequal_f(void *s, void * d)
{
  return *((float*) s) >= *((float*) d);
}

int
g_is_lowerorequal_f(void *s, void * d)
{
  return *((float*) s) <= *((float*) d);
}

int
g_is_f(void *s, void *d)
{
  return *((float*) s) == *((float*) d);
}

int
g_is_not_f(void *s, void * d)
{
  return *((float*) s) != *((float*) d);
}

