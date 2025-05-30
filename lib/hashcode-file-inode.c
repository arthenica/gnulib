/* Hash functions for file-related triples: name, device, inode.
   Copyright (C) 2007, 2009-2025 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

/* Specification.  */
#include "hashcode-file.h"

#include "same.h"
#include "same-inode.h"

/* Hash an F_triple, without considering the file name.  */
size_t
triple_hash_no_name (void const *x, size_t table_size)
{
  struct F_triple const *p = x;

  /* Ignoring the device number here should be fine.  */
  return p->st_ino % table_size;
}

/* Compare two F_triple structs.  */
bool
triple_compare (void const *x, void const *y)
{
  struct F_triple const *a = x;
  struct F_triple const *b = y;
  return PSAME_INODE (a, b) && same_name (a->name, b->name);
}
