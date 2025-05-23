/* Functions to support link list bitsets.

   Copyright (C) 2002-2004, 2006, 2009-2015, 2018-2025 Free Software
   Foundation, Inc.

   Contributed by Michael Hayes (m.hayes@elec.canterbury.ac.nz).

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include "bitset/list.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obstack.h"

/* This file implements linked-list bitsets.  These bitsets can be of
   arbitrary length and are more efficient than arrays of bits for
   large sparse sets.

   Usually if all the bits in an element are zero we remove the element
   from the list.  However, a side effect of the bit caching is that we
   do not always notice when an element becomes zero.  Hence the
   lbitset_weed function which removes zero elements.  */


/* Number of words to use for each element.  The larger the value the
   greater the size of the cache and the shorter the time to find a given bit
   but the more memory wasted for sparse bitsets and the longer the time
   to search for set bits.

   The routines that dominate timing profiles are lbitset_elt_find
   and lbitset_elt_link, especially when accessing the bits randomly.  */

#define LBITSET_ELT_WORDS 2

typedef bitset_word lbitset_word;

#define LBITSET_WORD_BITS BITSET_WORD_BITS

/* Number of bits stored in each element.  */
#define LBITSET_ELT_BITS \
  ((unsigned) (LBITSET_ELT_WORDS * LBITSET_WORD_BITS))

/* Lbitset element.   We use an array of bits for each element.
   These are linked together in a doubly-linked list.  */
typedef struct lbitset_elt_struct
{
  struct lbitset_elt_struct *next;      /* Next element.  */
  struct lbitset_elt_struct *prev;      /* Previous element.  */
  bitset_windex index;  /* bitno / BITSET_WORD_BITS.  */
  bitset_word words[LBITSET_ELT_WORDS]; /* Bits that are set.  */
}
lbitset_elt;


enum lbitset_find_mode
  { LBITSET_FIND, LBITSET_CREATE, LBITSET_SUBST };

static lbitset_elt lbitset_zero_elts[3]; /* Elements of all zero bits.  */

/* Obstack to allocate bitset elements from.  */
static struct obstack lbitset_obstack;
static bool lbitset_obstack_init = false;
static lbitset_elt *lbitset_free_list;  /* Free list of bitset elements.  */

extern void debug_lbitset (bitset);

#define LBITSET_CURRENT1(X) \
  ((lbitset_elt *) (void *) ((char *) (X) - offsetof (lbitset_elt, words)))

#define LBITSET_CURRENT(X) LBITSET_CURRENT1((X)->b.cdata)

#define LBITSET_HEAD(X) ((X)->l.head)
#define LBITSET_TAIL(X) ((X)->l.tail)

/* Allocate a lbitset element.  The bits are not cleared.  */
static inline lbitset_elt *
lbitset_elt_alloc (void)
{
  lbitset_elt *elt;

  if (lbitset_free_list != NULL)
    {
      elt = lbitset_free_list;
      lbitset_free_list = elt->next;
    }
  else
    {
      if (!lbitset_obstack_init)
        {
          lbitset_obstack_init = true;

          /* Let particular systems override the size of a chunk.  */

#ifndef OBSTACK_CHUNK_SIZE
# define OBSTACK_CHUNK_SIZE 0
#endif

          /* Let them override the alloc and free routines too.  */

#ifndef OBSTACK_CHUNK_ALLOC
# define OBSTACK_CHUNK_ALLOC xmalloc
#endif

#ifndef OBSTACK_CHUNK_FREE
# define OBSTACK_CHUNK_FREE free
#endif

#if !(defined __GNUC__ || defined __clang__)
# define __alignof__(type) 0
#endif

          obstack_specify_allocation (&lbitset_obstack, OBSTACK_CHUNK_SIZE,
                                      __alignof__ (lbitset_elt),
                                      OBSTACK_CHUNK_ALLOC,
                                      OBSTACK_CHUNK_FREE);
        }

      /* Perhaps we should add a number of new elements to the free
         list.  */
      elt = (lbitset_elt *) obstack_alloc (&lbitset_obstack,
                                           sizeof (lbitset_elt));
    }

  return elt;
}


/* Allocate a lbitset element.  The bits are cleared.  */
static inline lbitset_elt *
lbitset_elt_calloc (void)
{
  lbitset_elt *elt = lbitset_elt_alloc ();
  memset (elt->words, 0, sizeof (elt->words));
  return elt;
}


static inline void
lbitset_elt_free (lbitset_elt *elt)
{
  elt->next = lbitset_free_list;
  lbitset_free_list = elt;
}


/* Unlink element ELT from bitset BSET.  */
static inline void
lbitset_elt_unlink (bitset bset, lbitset_elt *elt)
{
  lbitset_elt *next = elt->next;
  lbitset_elt *prev = elt->prev;

  if (prev)
    prev->next = next;

  if (next)
    next->prev = prev;

  if (LBITSET_HEAD (bset) == elt)
    LBITSET_HEAD (bset) = next;
  if (LBITSET_TAIL (bset) == elt)
    LBITSET_TAIL (bset) = prev;

  /* Update cache pointer.  Since the first thing we try is to insert
     before current, make current the next entry in preference to the
     previous.  */
  if (LBITSET_CURRENT (bset) == elt)
    {
      if (next)
        {
          bset->b.cdata = next->words;
          bset->b.cindex = next->index;
        }
      else if (prev)
        {
          bset->b.cdata = prev->words;
          bset->b.cindex = prev->index;
        }
      else
        {
          bset->b.csize = 0;
          bset->b.cdata = NULL;
        }
    }

  lbitset_elt_free (elt);
}


/* Cut the chain of bitset BSET before element ELT and free the
   elements.  */
static inline void
lbitset_prune (bitset bset, lbitset_elt *elt)
{
  if (!elt)
    return;

  if (elt->prev)
    {
      LBITSET_TAIL (bset) = elt->prev;
      bset->b.cdata = elt->prev->words;
      bset->b.cindex = elt->prev->index;
      elt->prev->next = NULL;
    }
  else
    {
      LBITSET_HEAD (bset) = NULL;
      LBITSET_TAIL (bset) = NULL;
      bset->b.cdata = NULL;
      bset->b.csize = 0;
    }

  lbitset_elt *next;
  for (; elt; elt = next)
    {
      next = elt->next;
      lbitset_elt_free (elt);
    }
}


/* Are all bits in an element zero?  */
static inline bool
lbitset_elt_zero_p (lbitset_elt *elt)
{
  for (int i = 0; i < LBITSET_ELT_WORDS; i++)
    if (elt->words[i])
      return false;
  return true;
}


/* Link the bitset element into the current bitset linked list.  */
static inline void
lbitset_elt_link (bitset bset, lbitset_elt *elt)
{
  bitset_windex windex = elt->index;

  lbitset_elt *current = bset->b.csize ? LBITSET_CURRENT (bset) : LBITSET_HEAD (bset);

  /* If this is the first and only element, add it in.  */
  if (LBITSET_HEAD (bset) == NULL)
    {
      elt->next = elt->prev = NULL;
      LBITSET_HEAD (bset) = elt;
      LBITSET_TAIL (bset) = elt;
    }

  /* If this index is less than that of the current element, it goes
     somewhere before the current element.  */
  else if (windex < bset->b.cindex)
    {
      lbitset_elt *ptr;
      for (ptr = current;
           ptr->prev && ptr->prev->index > windex; ptr = ptr->prev)
        continue;

      if (ptr->prev)
        ptr->prev->next = elt;
      else
        LBITSET_HEAD (bset) = elt;

      elt->prev = ptr->prev;
      elt->next = ptr;
      ptr->prev = elt;
    }

  /* Otherwise, it must go somewhere after the current element.  */
  else
    {
      lbitset_elt *ptr;
      for (ptr = current;
           ptr->next && ptr->next->index < windex; ptr = ptr->next)
        continue;

      if (ptr->next)
        ptr->next->prev = elt;
      else
        LBITSET_TAIL (bset) = elt;

      elt->next = ptr->next;
      elt->prev = ptr;
      ptr->next = elt;
    }

  /* Set up so this is the first element searched.  */
  bset->b.cindex = windex;
  bset->b.csize = LBITSET_ELT_WORDS;
  bset->b.cdata = elt->words;
}


static lbitset_elt *
lbitset_elt_find (bitset bset, bitset_windex windex,
                  enum lbitset_find_mode mode)
{
  lbitset_elt *current;

  if (bset->b.csize)
    {
      current = LBITSET_CURRENT (bset);
      /* Check if element is the cached element.  */
      if ((windex - bset->b.cindex) < bset->b.csize)
        return current;
    }
  else
    {
      current = LBITSET_HEAD (bset);
    }

  if (current)
    {
      lbitset_elt *elt;
      if (windex < bset->b.cindex)
        {
          for (elt = current;
               elt->prev && elt->index > windex; elt = elt->prev)
            continue;
        }
      else
        {
          for (elt = current;
               elt->next && (elt->index + LBITSET_ELT_WORDS - 1) < windex;
               elt = elt->next)
            continue;
        }

      /* ELT is the nearest to the one we want.  If it's not the one
         we want, the one we want does not exist.  */
      if (windex - elt->index < LBITSET_ELT_WORDS)
        {
          bset->b.cindex = elt->index;
          bset->b.csize = LBITSET_ELT_WORDS;
          bset->b.cdata = elt->words;
          return elt;
        }
    }

  switch (mode)
    {
    default:
      abort ();

    case LBITSET_FIND:
      return NULL;

    case LBITSET_CREATE:
      windex -= windex % LBITSET_ELT_WORDS;
      lbitset_elt *elt = lbitset_elt_calloc ();
      elt->index = windex;
      lbitset_elt_link (bset, elt);
      return elt;

    case LBITSET_SUBST:
      return &lbitset_zero_elts[0];
    }
}


/* Weed out the zero elements from the list.  */
static inline void
lbitset_weed (bitset bset)
{
  lbitset_elt *next;
  for (lbitset_elt *elt = LBITSET_HEAD (bset); elt; elt = next)
    {
      next = elt->next;
      if (lbitset_elt_zero_p (elt))
        lbitset_elt_unlink (bset, elt);
    }
}


/* Set all bits in the bitset to zero.  */
static void
lbitset_zero (bitset bset)
{
  lbitset_elt *head = LBITSET_HEAD (bset);
  if (!head)
    return;

  /* Clear a bitset by freeing the linked list at the head element.  */
  lbitset_prune (bset, head);
}


/* Is DST == SRC?  */
static inline bool
lbitset_equal_p (bitset dst, bitset src)
{
  if (src == dst)
    return true;

  lbitset_weed (src);
  lbitset_weed (dst);
  lbitset_elt *selt;
  lbitset_elt *delt;
  for (selt = LBITSET_HEAD (src), delt = LBITSET_HEAD (dst);
       selt && delt; selt = selt->next, delt = delt->next)
    {
      if (selt->index != delt->index)
        return false;

      for (int j = 0; j < LBITSET_ELT_WORDS; j++)
        if (delt->words[j] != selt->words[j])
          return false;
    }
  return !selt && !delt;
}


/* Copy bits from bitset SRC to bitset DST.  */
static inline void
lbitset_copy_ (bitset dst, bitset src)
{
  if (src == dst)
    return;

  lbitset_zero (dst);

  lbitset_elt *head = LBITSET_HEAD (src);
  if (!head)
    return;

  lbitset_elt *prev = NULL;
  lbitset_elt *tmp;
  lbitset_elt *elt = head;
  do
    {
      tmp = lbitset_elt_alloc ();
      tmp->index = elt->index;
      tmp->prev = prev;
      tmp->next = NULL;
      if (prev)
        prev->next = tmp;
      else
        LBITSET_HEAD (dst) = tmp;
      prev = tmp;

      memcpy (tmp->words, elt->words, sizeof (elt->words));

      elt = elt->next;
    }
  while (elt);
  LBITSET_TAIL (dst) = tmp;

  dst->b.csize = LBITSET_ELT_WORDS;
  dst->b.cdata = LBITSET_HEAD (dst)->words;
  dst->b.cindex = LBITSET_HEAD (dst)->index;
}


static void
lbitset_copy (bitset dst, bitset src)
{
  if (BITSET_COMPATIBLE_ (dst, src))
    lbitset_copy_ (dst, src);
  else
    bitset_copy_ (dst, src);
}

/* Copy bits from bitset SRC to bitset DST.  Return true if
   bitsets different.  */
static inline bool
lbitset_copy_cmp (bitset dst, bitset src)
{
  if (src == dst)
    return false;

  if (!LBITSET_HEAD (dst))
    {
      lbitset_copy (dst, src);
      return LBITSET_HEAD (src) != NULL;
    }

  if (lbitset_equal_p (dst, src))
    return false;

  lbitset_copy (dst, src);
  return true;
}


static bitset_bindex
lbitset_resize (bitset src, bitset_bindex size)
{
  BITSET_NBITS_ (src) = size;

  /* Need to prune any excess bits.  FIXME.  */
  return size;
}

/* Set bit BITNO in bitset DST.  */
static void
lbitset_set (bitset dst, bitset_bindex bitno)
{
  bitset_windex windex = bitno / BITSET_WORD_BITS;

  lbitset_elt_find (dst, windex, LBITSET_CREATE);

  dst->b.cdata[windex - dst->b.cindex] |=
    (bitset_word) 1 << (bitno % BITSET_WORD_BITS);
}


/* Reset bit BITNO in bitset DST.  */
static void
lbitset_reset (bitset dst, bitset_bindex bitno)
{
  bitset_windex windex = bitno / BITSET_WORD_BITS;

  if (!lbitset_elt_find (dst, windex, LBITSET_FIND))
    return;

  dst->b.cdata[windex - dst->b.cindex] &=
    ~((bitset_word) 1 << (bitno % BITSET_WORD_BITS));

  /* If all the data is zero, perhaps we should unlink it now...  */
}


/* Test bit BITNO in bitset SRC.  */
static bool
lbitset_test (bitset src, bitset_bindex bitno)
{
  bitset_windex windex = bitno / BITSET_WORD_BITS;

  return (lbitset_elt_find (src, windex, LBITSET_FIND)
          && ((src->b.cdata[windex - src->b.cindex]
               >> (bitno % BITSET_WORD_BITS))
              & 1));
}


static void
lbitset_free (bitset bset)
{
  lbitset_zero (bset);
}


/* Find list of up to NUM bits set in BSET starting from and including
 *NEXT and store in array LIST.  Return with actual number of bits
 found and with *NEXT indicating where search stopped.  */
static bitset_bindex
lbitset_list_reverse (bitset bset, bitset_bindex *list,
                      bitset_bindex num, bitset_bindex *next)
{
  lbitset_elt *elt = LBITSET_TAIL (bset);
  if (!elt)
    return 0;

  bitset_bindex n_bits = (elt->index + LBITSET_ELT_WORDS) * BITSET_WORD_BITS;
  bitset_bindex rbitno = *next;

  if (rbitno >= n_bits)
    return 0;

  bitset_bindex bitno = n_bits - (rbitno + 1);

  bitset_windex windex = bitno / BITSET_WORD_BITS;

  /* Skip back to starting element.  */
  for (; elt && elt->index > windex; elt = elt->prev)
    continue;

  if (!elt)
    return 0;

  unsigned bitcnt;
  if (windex >= elt->index + LBITSET_ELT_WORDS)
    {
      /* We are trying to start in no-mans land so start
         at end of current elt.  */
      bitcnt = BITSET_WORD_BITS - 1;
      windex = elt->index + LBITSET_ELT_WORDS - 1;
    }
  else
    {
      bitcnt = bitno % BITSET_WORD_BITS;
    }

  bitset_bindex count = 0;
  bitset_bindex bitoff = windex * BITSET_WORD_BITS;

  /* If num is 1, we could speed things up with a binary search
     of the word of interest.  */

  while (elt)
    {
      bitset_word *srcp = elt->words;

      for (; (windex - elt->index) < LBITSET_ELT_WORDS;
           windex--)
        {
          bitset_word word = srcp[windex - elt->index];
          if (bitcnt + 1 < BITSET_WORD_BITS)
            /* We're starting in the middle of a word: smash bits to ignore.  */
            word &= ((bitset_word) 1 << (bitcnt + 1)) - 1;
          BITSET_FOR_EACH_BIT_REVERSE(pos, word)
            {
              list[count++] = bitoff + pos;
              if (count >= num)
                {
                  *next = n_bits - (bitoff + pos);
                  return count;
                }
            }
          bitoff -= BITSET_WORD_BITS;
          bitcnt = BITSET_WORD_BITS - 1;
        }

      elt = elt->prev;
      if (elt)
        {
          windex = elt->index + LBITSET_ELT_WORDS - 1;
          bitoff = windex * BITSET_WORD_BITS;
        }
    }

  *next = n_bits - (bitoff + 1);
  return count;
}


/* Find list of up to NUM bits set in BSET starting from and including
   *NEXT and store in array LIST.  Return with actual number of bits
   found and with *NEXT indicating where search stopped.  */
static bitset_bindex
lbitset_list (bitset bset, bitset_bindex *list,
              bitset_bindex num, bitset_bindex *next)
{
  lbitset_elt *head = LBITSET_HEAD (bset);
  if (!head)
    return 0;

  bitset_windex windex;
  lbitset_elt *elt;

  bitset_bindex bitno = *next;
  bitset_bindex count = 0;

  if (!bitno)
    {
      /* This is the most common case.  */

      /* Start with the first element.  */
      elt = head;
      windex = elt->index;
      bitno = windex * BITSET_WORD_BITS;
    }
  else
    {
      windex = bitno / BITSET_WORD_BITS;

      /* Skip to starting element.  */
      for (elt = head;
           elt && (elt->index + LBITSET_ELT_WORDS - 1) < windex;
           elt = elt->next)
        continue;

      if (!elt)
        return 0;

      if (windex < elt->index)
        {
          windex = elt->index;
          bitno = windex * BITSET_WORD_BITS;
        }
      else
        {
          bitset_word *srcp = elt->words;

          /* We are starting within an element.  */

          for (; (windex - elt->index) < LBITSET_ELT_WORDS; windex++)
            {
              bitset_word word = srcp[windex - elt->index] >> (bitno % BITSET_WORD_BITS);
              BITSET_FOR_EACH_BIT (pos, word)
                {
                  list[count++] = bitno + pos;
                  if (count >= num)
                    {
                      *next = bitno + pos + 1;
                      return count;
                    }
                }
              bitno = (windex + 1) * BITSET_WORD_BITS;
            }

          elt = elt->next;
          if (elt)
            {
              windex = elt->index;
              bitno = windex * BITSET_WORD_BITS;
            }
        }
    }

  while (elt)
    {
      bitset_word *srcp = elt->words;

      /* Is there enough room to avoid checking in each iteration? */
      if ((count + LBITSET_ELT_BITS) < num)
        {
          /* The coast is clear, plant boot!  */

#if LBITSET_ELT_WORDS == 2
          bitset_word word = srcp[0];
          if (word)
            BITSET_FOR_EACH_BIT (pos, word)
              list[count++] = bitno + pos;
          windex++;
          bitno = windex * BITSET_WORD_BITS;

          word = srcp[1];
          if (word)
            BITSET_FOR_EACH_BIT (pos, word)
              list[count++] = bitno + pos;
          windex++;
          bitno = windex * BITSET_WORD_BITS;
#else
          for (int i = 0; i < LBITSET_ELT_WORDS; i++)
            {
              bitset_word word = srcp[i];
              if (word)
                BITSET_FOR_EACH_BIT (pos, word)
                  list[count++] = bitno + pos;
              windex++;
              bitno = windex * BITSET_WORD_BITS;
            }
#endif
        }
      else
        {
          /* Tread more carefully since we need to check
             if array overflows.  */
          for (int i = 0; i < LBITSET_ELT_WORDS; i++)
            {
              bitset_word word = srcp[i];
              if (word)
                BITSET_FOR_EACH_BIT (pos, word)
                  {
                    list[count++] = bitno + pos;
                    if (count >= num)
                      {
                        *next = bitno + pos + 1;
                        return count;
                      }
                  }
              windex++;
              bitno = windex * BITSET_WORD_BITS;
            }
        }

      elt = elt->next;
      if (elt)
        {
          windex = elt->index;
          bitno = windex * BITSET_WORD_BITS;
        }
    }

  *next = bitno;
  return count;
}


static bool
lbitset_empty_p (bitset dst)
{
  lbitset_elt *elt;
  lbitset_elt *next;

  for (elt = LBITSET_HEAD (dst); elt; elt = next)
    {
      next = elt->next;
      if (!lbitset_elt_zero_p (elt))
        return false;
      /* Weed as we go.  */
      lbitset_elt_unlink (dst, elt);
    }

  return true;
}


/* Ensure that any unused bits within the last element are clear.  */
static inline void
lbitset_unused_clear (bitset dst)
{
  bitset_bindex n_bits = BITSET_SIZE_ (dst);
  unsigned last_bit = n_bits % LBITSET_ELT_BITS;

  if (last_bit)
    {
      lbitset_elt *elt = LBITSET_TAIL (dst);
      bitset_word *srcp = elt->words;
      bitset_windex windex = n_bits / BITSET_WORD_BITS;

      srcp[windex - elt->index]
        &= ((bitset_word) 1 << (last_bit % BITSET_WORD_BITS)) - 1;
      windex++;

      for (; (windex - elt->index) < LBITSET_ELT_WORDS; windex++)
        srcp[windex - elt->index] = 0;
    }
}


static void
lbitset_ones (bitset dst)
{
  /* This is a decidedly unfriendly operation for a linked list
     bitset!  It makes a sparse bitset become dense.  An alternative
     is to have a flag that indicates that the bitset stores the
     complement of what it indicates.  */

  bitset_windex windex
    = (BITSET_SIZE_ (dst) + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS;

  for (bitset_windex i = 0; i < windex; i += LBITSET_ELT_WORDS)
    {
      /* Create new elements if they cannot be found.  */
      lbitset_elt *elt = lbitset_elt_find (dst, i, LBITSET_CREATE);
      memset (elt->words, -1, sizeof (elt->words));
    }

  lbitset_unused_clear (dst);
}


static void
lbitset_not (bitset dst, bitset src)
{
  bitset_windex windex
    = (BITSET_SIZE_ (dst) + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS;

  for (bitset_windex i = 0; i < windex; i += LBITSET_ELT_WORDS)
    {
      /* Create new elements for dst if they cannot be found
         or substitute zero elements if src elements not found.  */
      lbitset_elt *selt = lbitset_elt_find (src, i, LBITSET_SUBST);
      lbitset_elt *delt = lbitset_elt_find (dst, i, LBITSET_CREATE);

      for (unsigned j = 0; j < LBITSET_ELT_WORDS; j++)
        delt->words[j] = ~selt->words[j];
    }
  lbitset_unused_clear (dst);
  lbitset_weed (dst);
}


/* Is DST == DST | SRC?  */
static bool
lbitset_subset_p (bitset dst, bitset src)
{
  for (lbitset_elt *selt = LBITSET_HEAD (src), *delt = LBITSET_HEAD (dst);
       selt || delt; selt = selt->next, delt = delt->next)
    {
      if (!selt)
        selt = &lbitset_zero_elts[0];
      else if (!delt)
        delt = &lbitset_zero_elts[0];
      else if (selt->index != delt->index)
        {
          if (selt->index < delt->index)
            {
              lbitset_zero_elts[2].next = delt;
              delt = &lbitset_zero_elts[2];
            }
          else
            {
              lbitset_zero_elts[1].next = selt;
              selt = &lbitset_zero_elts[1];
            }
        }

      for (unsigned j = 0; j < LBITSET_ELT_WORDS; j++)
        if (delt->words[j] != (selt->words[j] | delt->words[j]))
          return false;
    }
  return true;
}


/* Is DST & SRC == 0?  */
static bool
lbitset_disjoint_p (bitset dst, bitset src)
{
  for (lbitset_elt *selt = LBITSET_HEAD (src), *delt = LBITSET_HEAD (dst);
       selt && delt; selt = selt->next, delt = delt->next)
    {
      if (selt->index != delt->index)
        {
          if (selt->index < delt->index)
            {
              lbitset_zero_elts[2].next = delt;
              delt = &lbitset_zero_elts[2];
            }
          else
            {
              lbitset_zero_elts[1].next = selt;
              selt = &lbitset_zero_elts[1];
            }
          /* Since the elements are different, there is no
             intersection of these elements.  */
          continue;
        }

      for (unsigned j = 0; j < LBITSET_ELT_WORDS; j++)
        if (selt->words[j] & delt->words[j])
          return false;
    }
  return true;
}


static bool
lbitset_op3_cmp (bitset dst, bitset src1, bitset src2, enum bitset_ops op)
{
  lbitset_elt *selt1 = LBITSET_HEAD (src1);
  lbitset_elt *selt2 = LBITSET_HEAD (src2);
  lbitset_elt *delt = LBITSET_HEAD (dst);
  bool changed = false;

  LBITSET_HEAD (dst) = NULL;
  dst->b.csize = 0;

  bitset_windex windex1 = (selt1) ? selt1->index : BITSET_WINDEX_MAX;
  bitset_windex windex2 = (selt2) ? selt2->index : BITSET_WINDEX_MAX;

  while (selt1 || selt2)
    {
      bitset_windex windex;
      lbitset_elt *stmp1;
      lbitset_elt *stmp2;

      /* Figure out whether we need to substitute zero elements for
         missing links.  */
      if (windex1 == windex2)
        {
          windex = windex1;
          stmp1 = selt1;
          stmp2 = selt2;
          selt1 = selt1->next;
          windex1 = (selt1) ? selt1->index : BITSET_WINDEX_MAX;
          selt2 = selt2->next;
          windex2 = (selt2) ? selt2->index : BITSET_WINDEX_MAX;
        }
      else if (windex1 < windex2)
        {
          windex = windex1;
          stmp1 = selt1;
          stmp2 = &lbitset_zero_elts[0];
          selt1 = selt1->next;
          windex1 = (selt1) ? selt1->index : BITSET_WINDEX_MAX;
        }
      else
        {
          windex = windex2;
          stmp1 = &lbitset_zero_elts[0];
          stmp2 = selt2;
          selt2 = selt2->next;
          windex2 = (selt2) ? selt2->index : BITSET_WINDEX_MAX;
        }

      /* Find the appropriate element from DST.  Begin by discarding
         elements that we've skipped.  */
      lbitset_elt *dtmp;
      while (delt && delt->index < windex)
        {
          changed = true;
          dtmp = delt;
          delt = delt->next;
          lbitset_elt_free (dtmp);
        }
      if (delt && delt->index == windex)
        {
          dtmp = delt;
          delt = delt->next;
        }
      else
        dtmp = lbitset_elt_calloc ();

      /* Do the operation, and if any bits are set, link it into the
         linked list.  */
      bitset_word *srcp1 = stmp1->words;
      bitset_word *srcp2 = stmp2->words;
      bitset_word *dstp = dtmp->words;
      switch (op)
        {
        default:
          abort ();

        case BITSET_OP_OR:
          for (unsigned i = 0; i < LBITSET_ELT_WORDS; i++, dstp++)
            {
              bitset_word tmp = *srcp1++ | *srcp2++;

              if (*dstp != tmp)
                {
                  changed = true;
                  *dstp = tmp;
                }
            }
          break;

        case BITSET_OP_AND:
          for (unsigned i = 0; i < LBITSET_ELT_WORDS; i++, dstp++)
            {
              bitset_word tmp = *srcp1++ & *srcp2++;

              if (*dstp != tmp)
                {
                  changed = true;
                  *dstp = tmp;
                }
            }
          break;

        case BITSET_OP_XOR:
          for (unsigned i = 0; i < LBITSET_ELT_WORDS; i++, dstp++)
            {
              bitset_word tmp = *srcp1++ ^ *srcp2++;

              if (*dstp != tmp)
                {
                  changed = true;
                  *dstp = tmp;
                }
            }
          break;

        case BITSET_OP_ANDN:
          for (unsigned i = 0; i < LBITSET_ELT_WORDS; i++, dstp++)
            {
              bitset_word tmp = *srcp1++ & ~(*srcp2++);

              if (*dstp != tmp)
                {
                  changed = true;
                  *dstp = tmp;
                }
            }
          break;
        }

      if (!lbitset_elt_zero_p (dtmp))
        {
          dtmp->index = windex;
          /* Perhaps this could be optimised...  */
          lbitset_elt_link (dst, dtmp);
        }
      else
        {
          lbitset_elt_free (dtmp);
        }
    }

  /* If we have elements of DST left over, free them all.  */
  if (delt)
    {
      changed = true;
      lbitset_prune (dst, delt);
    }

  return changed;
}


static bool
lbitset_and_cmp (bitset dst, bitset src1, bitset src2)
{
  lbitset_elt *selt1 = LBITSET_HEAD (src1);
  lbitset_elt *selt2 = LBITSET_HEAD (src2);

  if (!selt2)
    {
      lbitset_weed (dst);
      bool changed = !LBITSET_HEAD (dst);
      lbitset_zero (dst);
      return changed;
    }
  else if (!selt1)
    {
      lbitset_weed (dst);
      bool changed = !LBITSET_HEAD (dst);
      lbitset_zero (dst);
      return changed;
    }
  else
    return lbitset_op3_cmp (dst, src1, src2, BITSET_OP_AND);
}


static void
lbitset_and (bitset dst, bitset src1, bitset src2)
{
  lbitset_and_cmp (dst, src1, src2);
}


static bool
lbitset_andn_cmp (bitset dst, bitset src1, bitset src2)
{
  lbitset_elt *selt1 = LBITSET_HEAD (src1);
  lbitset_elt *selt2 = LBITSET_HEAD (src2);

  if (!selt2)
    {
      return lbitset_copy_cmp (dst, src1);
    }
  else if (!selt1)
    {
      lbitset_weed (dst);
      bool changed = !LBITSET_HEAD (dst);
      lbitset_zero (dst);
      return changed;
    }
  else
    return lbitset_op3_cmp (dst, src1, src2, BITSET_OP_ANDN);
}


static void
lbitset_andn (bitset dst, bitset src1, bitset src2)
{
  lbitset_andn_cmp (dst, src1, src2);
}


static bool
lbitset_or_cmp (bitset dst, bitset src1, bitset src2)
{
  lbitset_elt *selt1 = LBITSET_HEAD (src1);
  lbitset_elt *selt2 = LBITSET_HEAD (src2);

  if (!selt2)
    return lbitset_copy_cmp (dst, src1);
  else if (!selt1)
    return lbitset_copy_cmp (dst, src2);
  else
    return lbitset_op3_cmp (dst, src1, src2, BITSET_OP_OR);
}


static void
lbitset_or (bitset dst, bitset src1, bitset src2)
{
  lbitset_or_cmp (dst, src1, src2);
}


static bool
lbitset_xor_cmp (bitset dst, bitset src1, bitset src2)
{
  lbitset_elt *selt1 = LBITSET_HEAD (src1);
  lbitset_elt *selt2 = LBITSET_HEAD (src2);

  if (!selt2)
    return lbitset_copy_cmp (dst, src1);
  else if (!selt1)
    return lbitset_copy_cmp (dst, src2);
  else
    return lbitset_op3_cmp (dst, src1, src2, BITSET_OP_XOR);
}


static void
lbitset_xor (bitset dst, bitset src1, bitset src2)
{
  lbitset_xor_cmp (dst, src1, src2);
}



/* Vector of operations for linked-list bitsets.  */
static struct bitset_vtable lbitset_vtable = {
  lbitset_set,
  lbitset_reset,
  bitset_toggle_,
  lbitset_test,
  lbitset_resize,
  bitset_size_,
  bitset_count_,
  lbitset_empty_p,
  lbitset_ones,
  lbitset_zero,
  lbitset_copy,
  lbitset_disjoint_p,
  lbitset_equal_p,
  lbitset_not,
  lbitset_subset_p,
  lbitset_and,
  lbitset_and_cmp,
  lbitset_andn,
  lbitset_andn_cmp,
  lbitset_or,
  lbitset_or_cmp,
  lbitset_xor,
  lbitset_xor_cmp,
  bitset_and_or_,
  bitset_and_or_cmp_,
  bitset_andn_or_,
  bitset_andn_or_cmp_,
  bitset_or_and_,
  bitset_or_and_cmp_,
  lbitset_list,
  lbitset_list_reverse,
  lbitset_free,
  BITSET_LIST
};


/* Return size of initial structure.  */
size_t
lbitset_bytes (MAYBE_UNUSED bitset_bindex n_bits)
{
  return sizeof (struct lbitset_struct);
}


/* Initialize a bitset.  */
bitset
lbitset_init (bitset bset, MAYBE_UNUSED bitset_bindex n_bits)
{
  BITSET_NBITS_ (bset) = n_bits;
  bset->b.vtable = &lbitset_vtable;
  return bset;
}


void
lbitset_release_memory (void)
{
  lbitset_free_list = NULL;
  if (lbitset_obstack_init)
    {
      lbitset_obstack_init = false;
      obstack_free (&lbitset_obstack, NULL);
    }
}


/* Function to be called from debugger to debug lbitset.  */
void
debug_lbitset (bitset bset)
{
  if (!bset)
    return;

  for (lbitset_elt *elt = LBITSET_HEAD (bset); elt; elt = elt->next)
    {
      fprintf (stderr, "Elt %lu\n", (unsigned long) elt->index);
      for (unsigned i = 0; i < LBITSET_ELT_WORDS; i++)
        {
          bitset_word word = elt->words[i];

          fprintf (stderr, "  Word %u:", i);
          for (unsigned j = 0; j < LBITSET_WORD_BITS; j++)
            if ((word & ((bitset_word) 1 << j)))
              fprintf (stderr, " %u", j);
          fprintf (stderr, "\n");
        }
    }
}
