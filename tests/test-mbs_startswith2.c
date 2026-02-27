/* Test of mbs_startswith() function.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2026.  */

#include <config.h>

#include <string.h>

#include <locale.h>
#include <stdlib.h>

#include "macros.h"

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  ASSERT (mbs_startswith ("", ""));
  ASSERT (mbs_startswith ("abc", ""));

  ASSERT (!mbs_startswith ("", "a"));
  ASSERT (!mbs_startswith ("x", "a"));
  ASSERT (mbs_startswith ("a", "a"));
  ASSERT (mbs_startswith ("abc", "a"));

  ASSERT (!mbs_startswith ("", "xyz"));
  ASSERT (!mbs_startswith ("x", "xyz"));
  ASSERT (!mbs_startswith ("a", "xyz"));
  ASSERT (!mbs_startswith ("abc", "xyz"));
  ASSERT (mbs_startswith ("xyz", "xyz"));
  ASSERT (mbs_startswith ("xyzzy", "xyz"));

  ASSERT (mbs_startswith ("", ""));
  ASSERT (mbs_startswith ("\303\244\306\200\303\247", "")); /* "äƀç" */

  ASSERT (!mbs_startswith ("", "\303\244")); /* "ä" */
  ASSERT (!mbs_startswith ("\341\272\213", "\303\244")); /* "ẋ" "ä" */
  ASSERT (mbs_startswith ("\303\244", "\303\244")); /* "ä" "ä" */
  ASSERT (mbs_startswith ("\303\244\306\200\303\247", "\303\244")); /* "äƀç" "ä" */
  /* Test a prefix that ends in an incomplete character.  */
  ASSERT (!mbs_startswith ("\303\244\306\200\303\247", "\303")); /* "äƀç" */

  ASSERT (!mbs_startswith ("", "\341\272\213\303\277\341\272\221")); /* "ẋÿẑ" */
  ASSERT (!mbs_startswith ("\341\272\213", "\341\272\213\303\277\341\272\221")); /* "ẋ" "ẋÿẑ" */
  ASSERT (!mbs_startswith ("\303\244", "\341\272\213\303\277\341\272\221")); /* "ä" "ẋÿẑ" */
  ASSERT (!mbs_startswith ("\303\244\306\200\303\247", "\341\272\213\303\277\341\272\221")); /* "äƀç" "ẋÿẑ" */
  ASSERT (mbs_startswith ("\341\272\213\303\277\341\272\221", "\341\272\213\303\277\341\272\221")); /* "ẋÿẑ" "ẋÿẑ" */
  ASSERT (mbs_startswith ("\341\272\213\303\277\341\272\221\341\272\221\303\277", "\341\272\213\303\277\341\272\221")); /* "ẋÿẑẑÿ" "ẋÿẑ" */
  /* Test a prefix that ends in an incomplete character.  */
  ASSERT (!mbs_startswith ("\341\272\213\303\277\341\272\221\341\272\221\303\277", "\341\272")); /* "ẋÿẑẑÿ" */
  ASSERT (!mbs_startswith ("\341\272\213\303\277\341\272\221\341\272\221\303\277", "\341")); /* "ẋÿẑẑÿ" */

  /* Test cases with invalid or incomplete characters.  */

  /* A valid character should not match an invalid character.  */
  ASSERT (!mbs_startswith ("\303\247", "\301\247"));
  ASSERT (!mbs_startswith ("\301\247", "\303\247"));

  /* A valid character should not match an incomplete character.  */
  ASSERT (!mbs_startswith ("\303\247", "\343\247"));
  ASSERT (!mbs_startswith ("\343\247", "\303\247"));

  /* An invalid character should not match an incomplete character.  */
  ASSERT (!mbs_startswith ("\301\247", "\343\247"));
  ASSERT (!mbs_startswith ("\343\247", "\301\247"));

  /* Two invalid characters should match only if they are identical.  */
  ASSERT (!mbs_startswith ("\301\246", "\301\247"));
  ASSERT (!mbs_startswith ("\301\247", "\301\246"));
  ASSERT (mbs_startswith ("\301\247", "\301\247"));

  /* Two incomplete characters should match only if they are identical.  */
  ASSERT (!mbs_startswith ("\343\246", "\343\247"));
  ASSERT (!mbs_startswith ("\343\247", "\343\246"));
  ASSERT (mbs_startswith ("\343\247", "\343\247"));

  return test_exit_status;
}
