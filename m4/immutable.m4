# immutable.m4
# serial 1
dnl Copyright (C) 2021-2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_IMMUTABLE],
[
  AC_REQUIRE([gl_FUNC_MPROTECT_WORKS])
  AC_REQUIRE([AC_C_INLINE])
])
