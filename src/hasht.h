/*
 * hasht.h
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#ifndef SRC_HASHT_H_
#define SRC_HASHT_H_

#include <stdlib.h>

struct entry_s
{
  unsigned char *key;
  unsigned short k_size;
  void *value;
  struct entry_s *next;
};

typedef struct entry_s entry_t;

#define F_HT_FREEVAL_ONCE	(uint8_t) 1

typedef void (*_cb_destroy) (entry_t * ptr);

#include <stdint.h>

struct hashtable_s
{
  size_t size;
  uint8_t flags;
  struct entry_s **table;
};

typedef struct hashtable_s hashtable_t;

hashtable_t *
ht_create (size_t size);
int
ht_destroy (hashtable_t * hashtable, _cb_destroy call);
int
ht_hash (hashtable_t *hashtable, unsigned char *key, size_t keyLength);
entry_t *
ht_newpair (unsigned char *key, size_t k_size, void *value, size_t size);
int
ht_remove (hashtable_t *hashtable, unsigned char *key, size_t k_size);
void
ht_set (hashtable_t *hashtable, unsigned char *key, size_t k_size, void *value,
	size_t size);
void *
ht_get (hashtable_t *hashtable, unsigned char *key, size_t k_size);

#endif /* SRC_HASHT_H_ */
