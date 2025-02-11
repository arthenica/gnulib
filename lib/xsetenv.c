/* Setting environment variables, with out-of-memory checking.
   Copyright (C) 2001-2002, 2005-2007, 2009-2025 Free Software Foundation, Inc.

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

/* Specification.  */
#include "xsetenv.h"

#include <stdlib.h>

#include <error.h>
#include "gettext.h"

#define _(msgid) dgettext ("gnulib", msgid)


/* Set NAME to VALUE in the environment.
   If REPLACE is nonzero, overwrite an existing value.
   With error checking.  */
void
xsetenv (const char *name, const char *value, int replace)
{
  if (setenv (name, value, replace) < 0)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
}
