/* Return a string describing the type of a file.

   Copyright (C) 1993-1994, 2001-2002, 2004-2005, 2009-2025 Free Software
   Foundation, Inc.

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

/* Written by Paul Eggert and Jim Meyering.  */

#ifndef FILE_TYPE_H
#define FILE_TYPE_H 1

/* This file uses _GL_ATTRIBUTE_PURE.  */
#if !_GL_CONFIG_H_INCLUDED
# error "Please include config.h first."
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif


char const *c_file_type (struct stat const *) _GL_ATTRIBUTE_PURE;
char const *file_type (struct stat const *) _GL_ATTRIBUTE_PURE;


#ifdef __cplusplus
}
#endif

#endif /* FILE_TYPE_H */
