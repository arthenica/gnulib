# link.m4
# serial 12
dnl Copyright (C) 2009-2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_LINK],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CHECK_FUNCS_ONCE([link])
  if test $ac_cv_func_link = no; then
    HAVE_LINK=0
  else
    AC_CACHE_CHECK([whether link obeys POSIX],
      [gl_cv_func_link_works],
      [touch conftest.a
       # Assume that if we have lstat, we can also check symlinks.
       if test $ac_cv_func_lstat = yes; then
         ln -s conftest.a conftest.lnk
       fi
       AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
           [[#include <unistd.h>
             #include <stdio.h>
           ]],
           [[int result = 0;
             if (!link ("conftest.a", "conftest.b/"))
               result |= 1;
#if HAVE_LSTAT
             if (!link ("conftest.lnk/", "conftest.b"))
               result |= 2;
             if (rename ("conftest.a", "conftest.b"))
               result |= 4;
             if (!link ("conftest.b", "conftest.lnk"))
               result |= 8;
#endif
             return result;
           ]])],
         [gl_cv_func_link_works=yes], [gl_cv_func_link_works=no],
         [case "$host_os" in
                             # Guess yes on Linux systems.
            linux-* | linux) gl_cv_func_link_works="guessing yes" ;;
                             # Guess yes on systems that emulate the Linux system calls.
            midipix*)        gl_cv_func_link_works="guessing yes" ;;
                             # Guess yes on glibc systems.
            *-gnu* | gnu*)   gl_cv_func_link_works="guessing yes" ;;
                             # If we don't know, obey --enable-cross-guesses.
            *)               gl_cv_func_link_works="$gl_cross_guess_normal" ;;
          esac
         ])
       rm -f conftest.a conftest.b conftest.lnk])
    case "$gl_cv_func_link_works" in
      *yes) ;;
      *)
        REPLACE_LINK=1
        ;;
    esac
  fi
])
