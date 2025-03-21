/* Formatted output to a file descriptor.
   Copyright (C) 2009-2025 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include <stdio.h>

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "full-write.h"
#include "intprops.h"
#include "vasnprintf.h"

off64_t
dzprintf (int fd, const char *format, ...)
{
  va_list args;
  char buf[2000];
  char *output;
  size_t len;
  size_t lenbuf = sizeof (buf);

  va_start (args, format);
  output = vasnprintf (buf, &lenbuf, format, args);
  len = lenbuf;
  va_end (args);

  if (!output)
    return -1;

  if (len > TYPE_MAXIMUM (off64_t))
    {
      /* We could write the (huge) output, but then could not return len, as it
         would be negative.  Since we want to use the error code ENOMEM, it is
         better to treat this case as if vasnprintf had already encountered an
         out-of-memory situation.  */
      if (output != buf)
        free (output);
      errno = ENOMEM;
      return -1;
    }

  if (full_write (fd, output, len) < len)
    {
      if (output != buf)
        free (output);
      return -1;
    }

  if (output != buf)
    free (output);

  return len;
}
