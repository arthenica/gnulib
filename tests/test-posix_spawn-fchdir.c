/* Test of posix_spawn() function with 'fchdir' action.
   Copyright (C) 2008-2025 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2018.  */

#include <config.h>

#include <spawn.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "findprog.h"
#include "qemu.h"
#include "xvasprintf.h"

static bool is_qemu;

static int
fd_safer (int fd)
{
  if (0 <= fd && fd <= 2)
    {
      int f = fd_safer (dup (fd));
      int e = errno;
      close (fd);
      errno = e;
      fd = f;
    }

  return fd;
}

static void
test (const char *pwd_prog)
{
  char *argv[2] = { (char *) "pwd", NULL };
  /* The name of a directory that most likely is accessible.  */
  #if defined __ANDROID__
    #define KNOWNDIR "/proc"
  #else
    #define KNOWNDIR "/"
  #endif
  int knownfd;
  int ifd[2];
  sigset_t blocked_signals;
  sigset_t fatal_signal_set;
  posix_spawn_file_actions_t actions;
  bool actions_allocated;
  posix_spawnattr_t attrs;
  bool attrs_allocated;
  int err;
  pid_t child;
  int fd;
  FILE *fp;
  char line[80];
  int status;
  int exitstatus;

  knownfd = open (KNOWNDIR, O_RDONLY);
  if (knownfd < 0)
    {
      perror ("cannot open directory");
      exit (1);
    }
  if (pipe (ifd) < 0 || (ifd[0] = fd_safer (ifd[0])) < 0)
    {
      perror ("cannot create pipe");
      exit (1);
    }
  sigprocmask (SIG_SETMASK, NULL, &blocked_signals);
  sigemptyset (&fatal_signal_set);
  sigaddset (&fatal_signal_set, SIGINT);
  sigaddset (&fatal_signal_set, SIGTERM);
  #ifdef SIGHUP
  sigaddset (&fatal_signal_set, SIGHUP);
  #endif
  #ifdef SIGPIPE
  sigaddset (&fatal_signal_set, SIGPIPE);
  #endif
  sigprocmask (SIG_BLOCK, &fatal_signal_set, NULL);
  actions_allocated = false;
  attrs_allocated = false;
  if ((err = posix_spawn_file_actions_init (&actions)) != 0
      || (actions_allocated = true,
          (err = posix_spawn_file_actions_adddup2 (&actions, ifd[1], STDOUT_FILENO)) != 0
          || (err = posix_spawn_file_actions_addclose (&actions, ifd[1])) != 0
          || (err = posix_spawn_file_actions_addclose (&actions, ifd[0])) != 0
          || (err = posix_spawn_file_actions_addopen (&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0)) != 0
          || (err = posix_spawn_file_actions_addfchdir (&actions, knownfd)) != 0
          || (err = posix_spawnattr_init (&attrs)) != 0
          || (attrs_allocated = true,
              #if defined _WIN32 && !defined __CYGWIN__
              0
              #else
              (err = posix_spawnattr_setsigmask (&attrs, &blocked_signals)) != 0
              || (err = posix_spawnattr_setflags (&attrs, POSIX_SPAWN_SETSIGMASK)) != 0
              #endif
             )
          || (err = posix_spawnp (&child, pwd_prog, &actions, &attrs, argv, environ)) != 0))
    {
      if (actions_allocated)
        posix_spawn_file_actions_destroy (&actions);
      if (attrs_allocated)
        posix_spawnattr_destroy (&attrs);
      sigprocmask (SIG_UNBLOCK, &fatal_signal_set, NULL);
      errno = err;
      perror ("subprocess failed");
      exit (1);
    }
  posix_spawn_file_actions_destroy (&actions);
  posix_spawnattr_destroy (&attrs);
  sigprocmask (SIG_UNBLOCK, &fatal_signal_set, NULL);
  close (ifd[1]);
  fd = ifd[0];
  fp = fdopen (fd, "r");
  if (fp == NULL)
    {
      fprintf (stderr, "fdopen() failed\n");
      exit (1);
    }
  if (fread (line, 1, 80, fp) < 2)
    {
      fprintf (stderr, "could not read expected output\n");
      exit (1);
    }
  /* For a process running under QEMU user-mode, knownfd points to the directory
     that is the value of the QEMU_LD_PREFIX environment variable or of the -L
     command-line option, and the line produced by 'pwd' is that directory, not
     "/".  */
  if (!is_qemu)
    {
      if (memcmp (line, KNOWNDIR "\n", strlen (KNOWNDIR) + 1) != 0)
        {
          fprintf (stderr, "read output is not the expected output\n");
          exit (1);
        }
    }
  fclose (fp);
  status = 0;
  while (waitpid (child, &status, 0) != child)
    ;
  if (!WIFEXITED (status))
    {
      fprintf (stderr, "subprocess terminated with unexpected wait status %d\n", status);
      exit (1);
    }
  exitstatus = WEXITSTATUS (status);
  if (exitstatus != 0)
    {
      fprintf (stderr, "subprocess terminated with unexpected exit status %d\n", exitstatus);
      exit (1);
    }
}

int
main ()
{
  is_qemu = is_running_under_qemu_user ();

  test ("pwd");

  /* Verify that if a program is given as a relative file name with at least one
     slash, it is interpreted w.r.t. the current directory after fchdir has been
     executed.  */
  if (!is_qemu)
    {
      const char *abs_pwd_prog = find_in_path ("pwd");

      if (abs_pwd_prog != NULL
          && abs_pwd_prog[0] == '/'
          && abs_pwd_prog[1] != '0' && abs_pwd_prog[1] != '/')
        {
          /* Determine the location of the 'pwd' program, relative to
             KNOWNDIR.  */
          const char *relative_pwd_prog;
          #if defined __ANDROID__
          relative_pwd_prog = xasprintf ("..%s", abs_pwd_prog);
          #else
          relative_pwd_prog = &abs_pwd_prog[1];
          #endif

          test (relative_pwd_prog);
        }
    }

  return 0;
}
