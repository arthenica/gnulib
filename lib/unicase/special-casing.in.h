/* Special casing table.
   Copyright (C) 2009-2025 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This file is free software.
   It is dual-licensed under "the GNU LGPLv3+ or the GNU GPLv2+".
   You can redistribute it and/or modify it under either
     - the terms of the GNU Lesser General Public License as published
       by the Free Software Foundation, either version 3, or (at your
       option) any later version, or
     - the terms of the GNU General Public License as published by the
       Free Software Foundation; either version 2, or (at your option)
       any later version, or
     - the same dual license "the GNU LGPLv3+ or the GNU GPLv2+".

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License and the GNU General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public
   License and of the GNU General Public License along with this
   program.  If not, see <https://www.gnu.org/licenses/>.  */

/* This file uses _GL_ATTRIBUTE_NONSTRING.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* A special casing context.
   A context is negated through x -> -x.  */
enum
{
  SCC_ALWAYS             = 0,
  SCC_FINAL_SIGMA        = 1,
  SCC_AFTER_SOFT_DOTTED  = 2,
  SCC_MORE_ABOVE         = 3,
  SCC_BEFORE_DOT         = 4,
  SCC_AFTER_I            = 5
};

struct special_casing_rule
{
  /* The first two bytes are the code, in big-endian order.  The third byte
     only distinguishes different rules pertaining to the same code.  */
  /*unsigned*/ char code[3] _GL_ATTRIBUTE_NONSTRING;

  /* True when this rule is not the last one for the given code.  */
  /*bool*/ unsigned int has_next : 1;

  /* Context.  */
  signed int context : 7;

  /* Language, or an empty string.  */
  char language[2];

  /* Mapping to upper case.  Between 0 and 3 characters.  Filled with 0s.  */
  unsigned short upper[3];

  /* Mapping to lower case.  Between 0 and 3 characters.  Filled with 0s.  */
  unsigned short lower[3];

  /* Mapping to title case.  Between 0 and 3 characters.  Filled with 0s.  */
  unsigned short title[3];

  /* Casefolding mapping.  Between 0 and 3 characters.  Filled with 0s.  */
  unsigned short casefold[3];
};

extern const struct special_casing_rule *
       gl_unicase_special_lookup (const char *str, size_t len);


#ifdef __cplusplus
}
#endif
