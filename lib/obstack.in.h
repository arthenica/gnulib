/* obstack.h - object stack macros
   Copyright (C) 1988-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* Summary:

   All the apparent functions defined here are macros. The idea
   is that you would use these pre-tested macros to solve a
   very specific set of problems, and they would run fast.
   Caution: no side-effects in arguments please!! They may be
   evaluated MANY times!!

   These macros operate a stack of objects.  Each object starts life
   small, and may grow to maturity.  (Consider building a word syllable
   by syllable.)  An object can move while it is growing.  Once it has
   been "finished" it never changes address again.  So the "top of the
   stack" is typically an immature growing object, while the rest of the
   stack is of mature, fixed size and fixed address objects.

   These routines grab large chunks of memory, using a function you
   supply, called 'obstack_chunk_alloc'.  On occasion, they free chunks,
   by calling 'obstack_chunk_free'.  You must define them and declare
   them before using any obstack macros.

   Each independent stack is represented by a 'struct obstack'.
   Each of the obstack macros expects a pointer to such a structure
   as the first argument.

   One motivation for this package is the problem of growing char strings
   in symbol tables.  Unless you are "fascist pig with a read-only mind"
   --Gosper's immortal quote from HAKMEM item 154, out of context--you
   would not like to put any arbitrary upper limit on the length of your
   symbols.

   In practice this often means you will build many short symbols and a
   few long symbols.  At the time you are reading a symbol you don't know
   how long it is.  One traditional method is to read a symbol into a
   buffer, realloc()ating the buffer every time you try to read a symbol
   that is longer than the buffer.  This is beaut, but you still will
   want to copy the symbol from the buffer to a more permanent
   symbol-table entry say about half the time.

   With obstacks, you can work differently.  Use one obstack for all symbol
   names.  As you read a symbol, grow the name in the obstack gradually.
   When the name is complete, finalize it.  Then, if the symbol exists already,
   free the newly read name.

   The way we do this is to take a large chunk, allocating memory from
   low addresses.  When you want to build a symbol in the chunk you just
   add chars above the current "high water mark" in the chunk.  When you
   have finished adding chars, because you got to the end of the symbol,
   you know how long the chars are, and you can create a new object.
   Mostly the chars will not burst over the highest address of the chunk,
   because you would typically expect a chunk to be (say) 100 times as
   long as an average object.

   In case that isn't clear, when we have enough chars to make up
   the object, THEY ARE ALREADY CONTIGUOUS IN THE CHUNK (guaranteed)
   so we just point to it where it lies.  No moving of chars is
   needed and this is the second win: potentially long strings need
   never be explicitly shuffled. Once an object is formed, it does not
   change its address during its lifetime.

   When the chars burst over a chunk boundary, we allocate a larger
   chunk, and then copy the partly formed object from the end of the old
   chunk to the beginning of the new larger chunk.  We then carry on
   accreting characters to the end of the object as we normally would.

   A special macro is provided to add a single char at a time to a
   growing object.  This allows the use of register variables, which
   break the ordinary 'growth' macro.

   Summary:
        We allocate large chunks.
        We carve out one object at a time from the current chunk.
        Once carved, an object never moves.
        We are free to append data of any size to the currently
          growing object.
        Exactly one object is growing in an obstack at any one time.
        You can run one obstack per control block.
        You may have as many control blocks as you dare.
        Because of the way we do it, you can "unwind" an obstack
          back to a previous state. (You may remove objects much
          as you would with a stack.)
 */

/* Documentation (part of the GNU libc manual):
   <https://www.gnu.org/software/libc/manual/html_node/Obstacks.html>  */


/* Don't do the contents of this file more than once.  */
#ifndef _OBSTACK_H
#define _OBSTACK_H 1

#if defined __GL_GNULIB_HEADER
/* This file uses _GL_ATTRIBUTE_PURE, FLEXIBLE_ARRAY_MEMBER.  */
# if !_GL_CONFIG_H_INCLUDED
  #error "Please include config.h first."
# endif
#endif

/* For backward compatibility with older glibc,
   define the otherwise-unused macro PTR_INT_TYPE.  If __PTRDIFF_TYPE__ is
   defined, as with GNU C, use that; that way we don't pollute the
   namespace with <stddef.h>'s symbols.  Otherwise, include <stddef.h>
   and use ptrdiff_t.  */
#ifdef __PTRDIFF_TYPE__
# define PTR_INT_TYPE __PTRDIFF_TYPE__
#else
# include <stddef.h>
# define PTR_INT_TYPE ptrdiff_t
#endif

/* An integer type wide enough to hold a pointer value,
   if such a type is available.  */
#ifdef __UINTPTR_TYPE__
# define _OBSTACK_UINTPTR_TYPE __UINTPTR_TYPE__
#elif defined __STDC_VERSION__ && 199901L <= __STDC_VERSION__
# include <stdint.h>
# ifdef UINTPTR_MAX
#  define _OBSTACK_UINTPTR_TYPE uintptr_t
# endif
#endif

/* A type suitable for comparing pointers regardless of whether they
   point into the same object, e.g., (_OBSTACK_CPTR) p < (_OBSTACK_CPTR) q.
   If possible, use uintptr_t to avoid undefined behavior and pacify
   sanitizers.  Otherwise, use a pointer directly and hope for the best.  */
#ifdef _OBSTACK_UINTPTR_TYPE
typedef _OBSTACK_UINTPTR_TYPE _OBSTACK_CPTR;
#else
typedef char *_OBSTACK_CPTR;
#endif

/* These macros highlight the places where this implementation
   is different from the one in GNU libc.  */
#if defined __GL_GNULIB_HEADER
/* In Gnulib, we use sane types, especially for 64-bit hosts.  */
# define _OBSTACK_INDEX_T size_t
# define _OBSTACK_SIZE_T size_t
# define _OBSTACK_CHUNK_SIZE_T size_t
# define _OBSTACK_CAST(type, expr) (expr)
# define _OBSTACK_CHUNK_CONTENTS_SIZE FLEXIBLE_ARRAY_MEMBER
#else
/* For backward compatibility, glibc limits object sizes to int range.  */
# define _OBSTACK_INDEX_T int
# define _OBSTACK_SIZE_T unsigned int
# define _OBSTACK_CHUNK_SIZE_T long int
# define _OBSTACK_CAST(type, expr) ((type) (expr))
# define _OBSTACK_CHUNK_CONTENTS_SIZE 4
#endif

/* If B is the base of an object addressed by P, return the result of
   aligning P to the next multiple of A + 1.  B and P must be of type
   char *.  A + 1 must be a power of 2.  With no uintptr_t, play it
   safe and compute the alignment relative to B.  Otherwise, use the
   faster strategy of computing the alignment through uintptr_t.  */
#ifndef _OBSTACK_UINTPTR_TYPE
# define __PTR_ALIGN(B, P, A) \
   ((B) + (((P) - (B) + (A)) & ~(A)))
#else
# define __PTR_ALIGN(B, P, A) \
   ((P) + ((- (_OBSTACK_UINTPTR_TYPE) (P)) & (A)))
#endif

/* For memcpy, size_t.  */
#include <string.h>

#ifndef __attribute_pure__
# define __attribute_pure__ _GL_ATTRIBUTE_PURE
#endif

/* Not the same as _Noreturn, since it also works with function pointers.  */
#ifndef __attribute_noreturn__
# if ((defined __GNUC__ && 2 < __GNUC__ + (8 <= __GNUC_MINOR__)) \
      || defined __clang__ \
      || (defined __SUNPRO_C && 0x5110 <= __SUNPRO_C))
#  define __attribute_noreturn__ __attribute__ ((__noreturn__))
# else
#  define __attribute_noreturn__
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _obstack_chunk           /* Lives at front of each chunk. */
{
  char *limit;                  /* 1 past end of this chunk */
  struct _obstack_chunk *prev;  /* address of prior chunk or NULL */
  char contents[_OBSTACK_CHUNK_CONTENTS_SIZE]; /* objects begin here */
};

struct obstack          /* control current object in current chunk */
{
  _OBSTACK_CHUNK_SIZE_T chunk_size;  /* preferred size to allocate chunks in */
  struct _obstack_chunk *chunk; /* address of current struct obstack_chunk */
  char *object_base;            /* address of object we are building */
  char *next_free;              /* where to add next char to current object */
  char *chunk_limit;            /* address of char after current chunk */
  union
  {
    _OBSTACK_SIZE_T tempint;
    void *tempptr;
  } temp;                       /* Temporary for some macros.  */
  _OBSTACK_INDEX_T alignment_mask;  /* Mask of alignment for each object. */

  /* These prototypes vary based on 'use_extra_arg'.  */
  union
  {
    void *(*plain) (_OBSTACK_CHUNK_SIZE_T);
    void *(*extra) (void *, _OBSTACK_CHUNK_SIZE_T);
  } chunkfun;
  union
  {
    void (*plain) (void *);
    void (*extra) (void *, void *);
  } freefun;

  void *extra_arg;              /* first arg for chunk alloc/dealloc funcs */
  unsigned use_extra_arg : 1;     /* chunk alloc/dealloc funcs take extra arg */
  unsigned maybe_empty_object : 1; /* There is a possibility that the current
                                      chunk contains a zero-length object.  This
                                      prevents freeing the chunk if we allocate
                                      a bigger chunk to replace it. */
  unsigned alloc_failed : 1;      /* No longer used, as we now call the failed
                                     handler on error, but retained for binary
                                     compatibility.  */
};

/* Declare the external functions we use; they are in obstack.c.  */

/* Symbol mapping for gnulib.  */
#if defined __GL_GNULIB_HEADER && defined __GL_REPLACE_OBSTACK__
# define _obstack_newchunk rpl_obstack_newchunk
# define __obstack_free rpl_obstack_free
# define _obstack_begin rpl_obstack_begin
# define _obstack_begin_1 rpl_obstack_begin_1
# define _obstack_memory_used rpl_obstack_memory_used
# define _obstack_allocated_p rpl_obstack_allocated_p
#endif

/* The default name of the function for freeing a chunk is '_obstack_free',
   but gnulib overrides this by defining '__obstack_free' above.  */
#ifndef __obstack_free
# define __obstack_free _obstack_free
#endif

extern void _obstack_newchunk (struct obstack *, _OBSTACK_INDEX_T);
extern int _obstack_begin (struct obstack *,
                           _OBSTACK_INDEX_T, _OBSTACK_INDEX_T,
                           void *(*) (_OBSTACK_CHUNK_SIZE_T),
                           void (*) (void *));
extern int _obstack_begin_1 (struct obstack *,
                             _OBSTACK_INDEX_T, _OBSTACK_INDEX_T,
                             void *(*) (void *, _OBSTACK_CHUNK_SIZE_T),
                             void (*) (void *, void *), void *);
extern _OBSTACK_INDEX_T _obstack_memory_used (struct obstack *)
  __attribute_pure__;
extern void __obstack_free (struct obstack *, void *);


/* Error handler called when 'obstack_chunk_alloc' failed to allocate
   more memory.  This can be set to a user defined function which
   should either abort gracefully or use longjump - but shouldn't
   return.  The default action is to print a message and abort.  */
extern __attribute_noreturn__ void (*obstack_alloc_failed_handler) (void);

/* Exit value used when 'print_and_abort' is used.  */
extern int obstack_exit_failure;

/* Pointer to beginning of object being allocated or to be allocated next.
   Note that this might not be the final address of the object
   because a new chunk might be needed to hold the final size.  */

#define obstack_base(h) ((void *) (h)->object_base)

/* Size for allocating ordinary chunks.  */

#define obstack_chunk_size(h) ((h)->chunk_size)

/* Pointer to next byte not yet allocated in current chunk.  */

#define obstack_next_free(h)    ((h)->next_free)

/* Mask specifying low bits that should be clear in address of an object.  */

#define obstack_alignment_mask(h) ((h)->alignment_mask)

/* To prevent prototype warnings provide complete argument list.  */
#define obstack_init(h)							      \
  _obstack_begin ((h), 0, 0,						      \
                  _OBSTACK_CAST (void *(*) (_OBSTACK_CHUNK_SIZE_T),	      \
                                 obstack_chunk_alloc),			      \
                  _OBSTACK_CAST (void (*) (void *), obstack_chunk_free))

#define obstack_begin(h, size)						      \
  _obstack_begin ((h), (size), 0,					      \
                  _OBSTACK_CAST (void *(*) (_OBSTACK_CHUNK_SIZE_T),	      \
                                 obstack_chunk_alloc),			      \
                  _OBSTACK_CAST (void (*) (void *), obstack_chunk_free))

#define obstack_specify_allocation(h, size, alignment, chunkfun, freefun)     \
  _obstack_begin ((h), (size), (alignment),				      \
                  _OBSTACK_CAST (void *(*) (_OBSTACK_CHUNK_SIZE_T),	      \
                                 chunkfun),				      \
                  _OBSTACK_CAST (void (*) (void *), freefun))

#define obstack_specify_allocation_with_arg(h, size, alignment, chunkfun, freefun, arg) \
  _obstack_begin_1 ((h), (size), (alignment),				      \
                    _OBSTACK_CAST (void *(*) (void *, _OBSTACK_CHUNK_SIZE_T), \
                                   chunkfun),				      \
                    _OBSTACK_CAST (void (*) (void *, void *), freefun), arg)

#define obstack_chunkfun(h, newchunkfun)				      \
  ((h)->chunkfun.extra = _OBSTACK_CAST (void *(*) (void *,		      \
                                                   _OBSTACK_CHUNK_SIZE_T),    \
                                        newchunkfun))

#define obstack_freefun(h, newfreefun)					      \
  ((h)->freefun.extra = (void *(*) (void *, void *)) (newfreefun))

#define obstack_1grow_fast(h, achar) (*((h)->next_free)++ = (achar))

#define obstack_blank_fast(h, n) ((h)->next_free += (n))

#define obstack_memory_used(h) _obstack_memory_used (h)

#if (defined __GNUC__ || defined __clang__ \
     || (defined __SUNPRO_C && 0x5150 <= __SUNPRO_C))
# if defined __GNUC__ && ! (2 < __GNUC__ + (8 <= __GNUC_MINOR__))
#  define __extension__
# endif

/* For GNU C, if not -traditional,
   we can define these macros to compute all args only once
   without using a global variable.
   Also, we can avoid using the 'temp' slot, to make faster code.  */

# define obstack_object_size(OBSTACK)					      \
  __extension__								      \
    ({ struct obstack const *__o = (OBSTACK);				      \
       (_OBSTACK_SIZE_T) (__o->next_free - __o->object_base); })

/* The local variable is named __o1 to avoid a shadowed variable
   warning when invoked from other obstack macros.  */
# define obstack_room(OBSTACK)						      \
  __extension__								      \
    ({ struct obstack const *__o1 = (OBSTACK);				      \
       (_OBSTACK_SIZE_T) (__o1->chunk_limit - __o1->next_free); })

# define obstack_make_room(OBSTACK, length)				      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       _OBSTACK_INDEX_T __len = length;					      \
       if (obstack_room (__o) < __len)					      \
         _obstack_newchunk (__o, __len);				      \
       (void) 0; })

# define obstack_empty_p(OBSTACK)					      \
  __extension__								      \
    ({ struct obstack const *__o = (OBSTACK);				      \
       (__o->chunk->prev == 0						      \
        && __o->next_free == __PTR_ALIGN ((char *) __o->chunk,		      \
                                          __o->chunk->contents,		      \
                                          __o->alignment_mask)); })

# define obstack_grow(OBSTACK, where, length)				      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       _OBSTACK_INDEX_T __len = length;					      \
       if (obstack_room (__o) < __len)					      \
         _obstack_newchunk (__o, __len);				      \
       memcpy (__o->next_free, where, __len);				      \
       __o->next_free += __len;						      \
       (void) 0; })

# define obstack_grow0(OBSTACK, where, length)				      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       _OBSTACK_INDEX_T __len = length;					      \
       if (obstack_room (__o) <= __len)					      \
         _obstack_newchunk (__o, __len + 1);				      \
       memcpy (__o->next_free, where, __len);				      \
       __o->next_free += __len;						      \
       *(__o->next_free)++ = 0;						      \
       (void) 0; })

# define obstack_1grow(OBSTACK, datum)					      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       if (obstack_room (__o) < 1)					      \
         _obstack_newchunk (__o, 1);					      \
       obstack_1grow_fast (__o, datum);					      \
       (void) 0; })

/* These assume that the obstack alignment is good enough for pointers
   or ints, and that the data added so far to the current object
   shares that much alignment.  */

# define obstack_ptr_grow(OBSTACK, datum)				      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       if (obstack_room (__o) < sizeof (void *))			      \
         _obstack_newchunk (__o, sizeof (void *));			      \
       obstack_ptr_grow_fast (__o, datum); })

# define obstack_int_grow(OBSTACK, datum)				      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       if (obstack_room (__o) < sizeof (int))				      \
         _obstack_newchunk (__o, sizeof (int));				      \
       obstack_int_grow_fast (__o, datum); })

# define obstack_ptr_grow_fast(OBSTACK, aptr)				      \
  __extension__								      \
    ({ struct obstack *__o1 = (OBSTACK);				      \
       void *__p1 = __o1->next_free;					      \
       *(const void **) __p1 = (aptr);					      \
       __o1->next_free += sizeof (const void *);			      \
       (void) 0; })

# define obstack_int_grow_fast(OBSTACK, aint)				      \
  __extension__								      \
    ({ struct obstack *__o1 = (OBSTACK);				      \
       void *__p1 = __o1->next_free;					      \
       *(int *) __p1 = (aint);						      \
       __o1->next_free += sizeof (int);					      \
       (void) 0; })

# define obstack_blank(OBSTACK, length)					      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       _OBSTACK_INDEX_T __len = length;					      \
       if (obstack_room (__o) < __len)					      \
         _obstack_newchunk (__o, __len);				      \
       obstack_blank_fast (__o, __len);					      \
       (void) 0; })

# define obstack_alloc(OBSTACK, length)					      \
  __extension__								      \
    ({ struct obstack *__h = (OBSTACK);					      \
       obstack_blank (__h, (length));					      \
       obstack_finish (__h); })

# define obstack_copy(OBSTACK, where, length)				      \
  __extension__								      \
    ({ struct obstack *__h = (OBSTACK);					      \
       obstack_grow (__h, (where), (length));				      \
       obstack_finish (__h); })

# define obstack_copy0(OBSTACK, where, length)				      \
  __extension__								      \
    ({ struct obstack *__h = (OBSTACK);					      \
       obstack_grow0 (__h, (where), (length));				      \
       obstack_finish (__h); })

/* The local variable is named __o1 to avoid a shadowed variable
   warning when invoked from other obstack macros, typically obstack_free.  */
# define obstack_finish(OBSTACK)					      \
  __extension__								      \
    ({ struct obstack *__o1 = (OBSTACK);				      \
       void *__value = (void *) __o1->object_base;			      \
       if (__o1->next_free == __value)					      \
         __o1->maybe_empty_object = 1;					      \
       __o1->next_free							      \
         = __PTR_ALIGN (__o1->object_base, __o1->next_free,		      \
                        __o1->alignment_mask);				      \
       __o1->object_base = __o1->next_free;				      \
       __value; })

# define obstack_free(OBSTACK, OBJ)					      \
  __extension__								      \
    ({ struct obstack *__o = (OBSTACK);					      \
       void *__obj = (void *) (OBJ);					      \
       if ((_OBSTACK_CPTR) __o->chunk < (_OBSTACK_CPTR) __obj		      \
           && (_OBSTACK_CPTR) __obj < (_OBSTACK_CPTR) __o->chunk_limit)	      \
         __o->next_free = __o->object_base = (char *) __obj;		      \
       else								      \
         __obstack_free (__o, __obj); })

#else /* not __GNUC__-like */

# define obstack_object_size(h)						      \
  ((_OBSTACK_SIZE_T) ((h)->next_free - (h)->object_base))

# define obstack_room(h)						      \
  ((_OBSTACK_SIZE_T) ((h)->chunk_limit - (h)->next_free))

# define obstack_empty_p(h)						      \
  ((h)->chunk->prev == 0						      \
   && (h)->next_free == __PTR_ALIGN ((char *) (h)->chunk,		      \
                                     (h)->chunk->contents,		      \
                                     (h)->alignment_mask))

/* Note that the call to _obstack_newchunk is enclosed in (..., 0)
   so that we can avoid having void expressions
   in the arms of the conditional expression.
   Casting the third operand to void was tried before,
   but some compilers won't accept it.  */

# define obstack_make_room(h, length)					      \
  ((h)->temp.tempint = (length),					      \
   ((obstack_room (h) < (h)->temp.tempint)				      \
    ? (_obstack_newchunk (h, (h)->temp.tempint), 0) : 0),		      \
   (void) 0)

# define obstack_grow(h, where, length)					      \
  ((h)->temp.tempint = (length),					      \
   ((obstack_room (h) < (h)->temp.tempint)				      \
   ? (_obstack_newchunk ((h), (h)->temp.tempint), 0) : 0),		      \
   memcpy ((h)->next_free, where, (h)->temp.tempint),			      \
   (h)->next_free += (h)->temp.tempint,					      \
   (void) 0)

# define obstack_grow0(h, where, length)				      \
  ((h)->temp.tempint = (length),					      \
   ((obstack_room (h) <= (h)->temp.tempint)				      \
   ? (_obstack_newchunk ((h), (h)->temp.tempint + 1), 0) : 0),		      \
   memcpy ((h)->next_free, where, (h)->temp.tempint),			      \
   (h)->next_free += (h)->temp.tempint,					      \
   *((h)->next_free)++ = 0,						      \
   (void) 0)

# define obstack_1grow(h, datum)					      \
  (((obstack_room (h) < 1)						      \
    ? (_obstack_newchunk ((h), 1), 0) : 0),				      \
   obstack_1grow_fast (h, datum),					      \
   (void) 0)

# define obstack_ptr_grow(h, datum)					      \
  (((obstack_room (h) < sizeof (char *))				      \
    ? (_obstack_newchunk ((h), sizeof (char *)), 0) : 0),		      \
   obstack_ptr_grow_fast (h, datum))

# define obstack_int_grow(h, datum)					      \
  (((obstack_room (h) < sizeof (int))					      \
    ? (_obstack_newchunk ((h), sizeof (int)), 0) : 0),			      \
   obstack_int_grow_fast (h, datum))

# define obstack_ptr_grow_fast(h, aptr)					      \
  (((const void **) ((h)->next_free += sizeof (void *)))[-1] = (aptr),	      \
   (void) 0)

# define obstack_int_grow_fast(h, aint)					      \
  (((int *) ((h)->next_free += sizeof (int)))[-1] = (aint),		      \
   (void) 0)

# define obstack_blank(h, length)					      \
  ((h)->temp.tempint = (length),					      \
   ((obstack_room (h) < (h)->temp.tempint)				      \
   ? (_obstack_newchunk ((h), (h)->temp.tempint), 0) : 0),		      \
   obstack_blank_fast (h, (h)->temp.tempint),				      \
   (void) 0)

# define obstack_alloc(h, length)					      \
  (obstack_blank ((h), (length)), obstack_finish ((h)))

# define obstack_copy(h, where, length)					      \
  (obstack_grow ((h), (where), (length)), obstack_finish ((h)))

# define obstack_copy0(h, where, length)				      \
  (obstack_grow0 ((h), (where), (length)), obstack_finish ((h)))

# define obstack_finish(h)						      \
  (((h)->next_free == (h)->object_base					      \
    ? (((h)->maybe_empty_object = 1), 0)				      \
    : 0),								      \
   (h)->temp.tempptr = (h)->object_base,				      \
   (h)->next_free							      \
     = __PTR_ALIGN ((h)->object_base, (h)->next_free,			      \
                    (h)->alignment_mask),				      \
   (h)->object_base = (h)->next_free,					      \
   (h)->temp.tempptr)

# define obstack_free(h, obj)						      \
  ((h)->temp.tempptr = (void *) (obj),					      \
   (((_OBSTACK_CPTR) (h)->chunk < (_OBSTACK_CPTR) (h)->temp.tempptr	      \
     && (_OBSTACK_CPTR) (h)->temp.tempptr < (_OBSTACK_CPTR) (h)->chunk_limit) \
    ? (void) ((h)->next_free = (h)->object_base = (char *) (h)->temp.tempptr) \
    : __obstack_free (h, (h)->temp.tempptr)))

#endif /* not __GNUC__-like */

#ifdef __cplusplus
}       /* C++ */
#endif

#endif /* _OBSTACK_H */
