/* Test of cosh() function.
   Copyright (C) 2010-2025 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (cosh, double, (double));

#include <stdio.h>
#include <string.h>

#include "macros.h"

volatile double x;
double y;

int
main ()
{
  /* A particular value.  */
  x = 0.6;
  y = cosh (x);
  ASSERT (y >= 1.185465218 && y <= 1.185465219);

  /* Another particular value.  */
  {
    char buf[80];
    x = 5.152911276257581641;
    y = cosh (x);
    sprintf (buf, "%.2f", y);
    ASSERT (strcmp (buf, "86.47") == 0);
  }

  return test_exit_status;
}
