/*
 * hasht.c
 *
 *  Created on: Oct 23, 2015
 *      Author: reboot
 */

#include <stdlib.h>
#include <string.h>

#include "hasht.h"

/* Create a new hashtable. */
hashtable_t *
ht_create (size_t size)
{

  hashtable_t *hashtable = NULL;

  if (size < 1)
    return NULL;

  /* Allocate the table itself. */
  if ((hashtable = (hashtable_t *) malloc (sizeof(hashtable_t))) == NULL)
    {
      return NULL;
    }

  /* Allocate pointers to the head nodes. */
  if ((hashtable->table = (entry_t**) calloc (sizeof(entry_t *), size)) == NULL)
    {
      return NULL;
    }

  hashtable->size = size;

  return hashtable;
}

/* Destroy a  hashtable. */
int
ht_destroy (hashtable_t * hashtable, _cb_destroy call)
{

  entry_t *ptr = NULL;

  size_t i;
  for (i = 0; i < hashtable->size; i++)
    {
      ptr = hashtable->table[i];

      if (!ptr)
	{
	  continue;
	}

      while (ptr)
	{
	  if (call)
	    {
	      call (ptr);
	    }
	  if (ptr->key)
	    {
	      free (ptr->key);
	    }
	  entry_t *t_ptr = ptr;
	  ptr = ptr->next;
	  free (t_ptr);
	}

    }

  free (hashtable->table);
  free (hashtable);

  return 0;
}

/* Hash a string for a particular hash table. */
int
ht_hash (hashtable_t *hashtable, unsigned char *key, size_t keyLength)
{

  size_t hash;
  int i = 0;

  /* jenkins algo  */
  for (hash = i = 0; i < keyLength; ++i)
    {
      hash += key[i], hash += (hash << 10), hash ^= (hash >> 6);
    }
  hash += (hash << 3), hash ^= (hash >> 11), hash += (hash << 15);

  return hash % hashtable->size;
}

/* Create a key-value pair. */
entry_t *
ht_newpair (unsigned char *key, size_t k_size, void *value, size_t size)
{
  entry_t *newpair;

  if ((newpair = (entry_t*) malloc (sizeof(entry_t))) == NULL)
    {
      abort ();
    }

  if ((newpair->key = (unsigned char*) malloc (k_size)) == NULL)
    {
      abort ();
    }

  memcpy (newpair->key, (void*) key, k_size);

  newpair->value = value;
  newpair->k_size = (unsigned short) k_size;
  newpair->next = NULL;

  return newpair;
}

/* Remove a key-value pair from a hash table. */
int
ht_remove (hashtable_t *hashtable, unsigned char *key, size_t k_size)
{
  int bin = 0;

  entry_t *next = NULL;
  entry_t *last = NULL;

  bin = ht_hash (hashtable, key, k_size);

  next = hashtable->table[bin];

  while (next != NULL && next->key != NULL
      && (k_size != next->k_size || memcmp (key, next->key, k_size)))
    {
      last = next;
      next = next->next;
    }

  if (next != NULL && next->key != NULL && k_size == next->k_size
      && memcmp (key, next->key, k_size) == 0)
    {

      free (next->key);

      if (hashtable->flags & F_HT_FREEVAL_ONCE)
	{
	  free (next->value);
	}

      /* We're at the start of the linked list in this bin. */
      if (next == hashtable->table[bin])
	{
	  hashtable->table[bin] = next->next;
	}
      else if (NULL == next->next)
	{
	  /* We're at the end of the linked list in this bin. */
	  last->next = NULL;

	}
      else /* We're in the middle of the list. */
	{
	  last->next = next->next;
	}

      free (next);

      return 0;
    }

  if (hashtable->flags & F_HT_FREEVAL_ONCE)
    {
      hashtable->flags ^= F_HT_FREEVAL_ONCE;
    }

  return 1;

}

/* Insert a key-value pair into a hash table. */
void
ht_set (hashtable_t *hashtable, unsigned char *key, size_t k_size, void *value,
	size_t size)
{
  int bin = 0;
  entry_t *newpair = NULL;
  entry_t *next = NULL;
  entry_t *last = NULL;

  bin = ht_hash (hashtable, key, k_size);

  next = hashtable->table[bin];

  while (next != NULL && next->key != NULL
      && (k_size != next->k_size || memcmp (key, next->key, k_size)))
    {
      last = next;
      next = next->next;
    }

  /* There's already a pair.  Let's replace that string. */
  if (next != NULL && next->key != NULL && k_size == next->k_size
      && memcmp (key, next->key, k_size) == 0)
    {
      next->value = value;

    }
  else
  /* Nope, could't find it.  Time to grow a pair. */
    {
      newpair = ht_newpair (key, k_size, value, size);

      /* We're at the start of the linked list in this bin. */
      if (next == hashtable->table[bin])
	{
	  newpair->next = next;
	  hashtable->table[bin] = newpair;

	}
      else if (next == NULL)
	{
	  /* We're at the end of the linked list in this bin. */
	  last->next = newpair;

	}
      else /* We're in the middle of the list. */
	{
	  newpair->next = next;
	  last->next = newpair;
	}
    }
}

/* Retrieve a key-value pair from a hash table. */
void *
ht_get (hashtable_t *hashtable, unsigned char *key, size_t k_size)
{
  int bin = 0;
  entry_t *pair;

  bin = ht_hash (hashtable, key, k_size);

  pair = hashtable->table[bin];
  while (pair != NULL && pair->key != NULL
      && (k_size != pair->k_size || memcmp (key, pair->key, k_size)))
    {
      pair = pair->next;
    }

  /* Did we actually find anything? */
  if (pair == NULL || pair->key == NULL || k_size != pair->k_size
      || memcmp (key, pair->key, k_size) != 0)
    {
      return NULL;
    }
  else
    {
      return pair->value;
    }

}
