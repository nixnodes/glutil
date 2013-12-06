/*
 * gv_off.c
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#include <t_glob.h>
#include <fp_types.h>

#include <gv_off.h>


uint64_t
g_t8_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint8_t*) (base + offset));
}

int64_t
g_ts8_ptr(void *base, size_t offset)
{
  return (int64_t) *((int8_t*) (base + offset));
}

uint64_t
g_t16_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint16_t*) (base + offset));
}

int64_t
g_ts16_ptr(void *base, size_t offset)
{
  return (int64_t) *((int16_t*) (base + offset));
}

uint64_t
g_t32_ptr(void *base, size_t offset)
{
  return (uint64_t) *((uint32_t*) (base + offset));
}

int64_t
g_ts32_ptr(void *base, size_t offset)
{
  return (int64_t) *((int32_t*) (base + offset));
}

uint64_t
g_t64_ptr(void *base, size_t offset)
{
  return *((uint64_t*) (base + offset));
}

int64_t
g_ts64_ptr(void *base, size_t offset)
{
  return *((int64_t*) (base + offset));
}

float
g_tf_ptr(void *base, size_t offset)
{
  return *((float*) (base + offset));
}

float
g_td_ptr(void *base, size_t offset)
{
  return *((double*) (base + offset));
}

