/* Normalization of UTF-32 strings.
   Copyright (C) 2009-2025 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "uninorm.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "unistr.h"
#include "unictype.h"
#include "normalize-internal.h"
#include "uninorm/decompose-internal.h"

#define FUNC u32_normalize
#define UNIT uint32_t
#define U_MBTOUC_UNSAFE u32_mbtouc_unsafe
#define U_UCTOMB u32_uctomb
#define U_CPY u32_cpy
#include "u-normalize-internal.h"
