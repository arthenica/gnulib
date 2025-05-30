/* Set data type implemented by a hash table with a linked list.
   Copyright (C) 2006, 2009-2025 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2018.

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

#ifndef _GL_LINKEDHASH_SET_H
#define _GL_LINKEDHASH_SET_H

#include "gl_set.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct gl_set_implementation gl_linkedhash_set_implementation;
#define GL_LINKEDHASH_SET &gl_linkedhash_set_implementation

#ifdef __cplusplus
}
#endif

#endif /* _GL_LINKEDHASH_SET_H */
