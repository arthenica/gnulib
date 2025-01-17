# exponentl.m4
# serial 7
dnl Copyright (C) 2007-2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.
AC_DEFUN_ONCE([gl_LONG_DOUBLE_EXPONENT_LOCATION],
[
  AC_REQUIRE([gl_BIGENDIAN])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([where to find the exponent in a 'long double'],
    [gl_cv_cc_long_double_expbit0],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define NWORDS \
  ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
typedef union { long double value; unsigned int word[NWORDS]; }
        memory_long_double;
static unsigned int ored_words[NWORDS];
static unsigned int anded_words[NWORDS];
static void add_to_ored_words (long double *x)
{
  memory_long_double m;
  size_t i;
  /* Clear it first, in case
     sizeof (long double) < sizeof (memory_long_double).  */
  memset (&m, 0, sizeof (memory_long_double));
  m.value = *x;
  for (i = 0; i < NWORDS; i++)
    {
      ored_words[i] |= m.word[i];
      anded_words[i] &= m.word[i];
    }
}
int main ()
{
  static long double samples[5] = { 0.25L, 0.5L, 1.0L, 2.0L, 4.0L };
  size_t j;
  FILE *fp = fopen ("conftest.out", "w");
  if (fp == NULL)
    return 1;
  for (j = 0; j < NWORDS; j++)
    anded_words[j] = ~ (unsigned int) 0;
  for (j = 0; j < 5; j++)
    add_to_ored_words (&samples[j]);
  /* Remove bits that are common (e.g. if representation of the first mantissa
     bit is explicit).  */
  for (j = 0; j < NWORDS; j++)
    ored_words[j] &= ~anded_words[j];
  /* Now find the nonzero word.  */
  for (j = 0; j < NWORDS; j++)
    if (ored_words[j] != 0)
      break;
  if (j < NWORDS)
    {
      size_t i;
      for (i = j + 1; i < NWORDS; i++)
        if (ored_words[i] != 0)
          {
            fprintf (fp, "unknown");
            return (fclose (fp) != 0);
          }
      for (i = 0; ; i++)
        if ((ored_words[j] >> i) & 1)
          {
            fprintf (fp, "word %d bit %d", (int) j, (int) i);
            return (fclose (fp) != 0);
          }
    }
  fprintf (fp, "unknown");
  return (fclose (fp) != 0);
}
        ]])],
        [gl_cv_cc_long_double_expbit0=`cat conftest.out`],
        [gl_cv_cc_long_double_expbit0="unknown"],
        [
          dnl When cross-compiling, in general we don't know. It depends on the
          dnl ABI and compiler version. There are too many cases.
          gl_cv_cc_long_double_expbit0="unknown"
          case "$host_os" in
            mingw* | windows*)
              # On native Windows (little-endian), we know the result
              # in two cases: mingw, MSVC.
              AC_EGREP_CPP([Known], [
#ifdef __MINGW32__
 Known
#endif
                ], [gl_cv_cc_long_double_expbit0="word 2 bit 0"])
              AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
                ], [gl_cv_cc_long_double_expbit0="word 1 bit 20"])
              ;;
          esac
        ])
      rm -f conftest.out
    ])
  case "$gl_cv_cc_long_double_expbit0" in
    word*bit*)
      word=`echo "$gl_cv_cc_long_double_expbit0" | sed -e 's/word //' -e 's/ bit.*//'`
      bit=`echo "$gl_cv_cc_long_double_expbit0" | sed -e 's/word.*bit //'`
      AC_DEFINE_UNQUOTED([LDBL_EXPBIT0_WORD], [$word],
        [Define as the word index where to find the exponent of 'long double'.])
      AC_DEFINE_UNQUOTED([LDBL_EXPBIT0_BIT], [$bit],
        [Define as the bit index in the word where to find bit 0 of the exponent of 'long double'.])
      ;;
  esac
])
