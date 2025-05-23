#!/bin/sh
# Test suite for update-copyright.
# Copyright (C) 2009-2025 Free Software Foundation, Inc.
# This file is part of the GNUlib Library.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/init.sh"; path_prepend_ .

# Ensure the update-copyright program gets found.
PATH=$abs_aux_dir:$PATH
export PATH

TMP_BASE=update-copyright.test
trap 'rm -f $TMP_BASE*' EXIT HUP INT QUIT TERM

## --------------------------------- ##
## Skip if user does not have perl.  ##
## --------------------------------- ##

TMP=$TMP_BASE
s=$TMP-script
cat <<\EOF > $s
#!/bin/sh
#! -*-perl-*-
# This prologue allows running a perl script as an executable
# on systems that are compliant to a POSIX version before POSIX:2017.
# On such systems, the usual invocation of an executable through execlp()
# or execvp() fails with ENOEXEC if it is a script that does not start
# with a #! line.  The script interpreter mentioned in the #! line has
# to be /bin/sh, because on GuixSD systems that is the only program that
# has a fixed file name.  The second line is essential for perl and is
# also useful for editing this file in Emacs.  The next two lines below
# are valid code in both sh and perl.  When executed by sh, they re-execute
# the script through the perl program found in $PATH.  The '-x' option
# is essential as well; without it, perl would re-execute the script
# through /bin/sh.  When executed by perl, the next two lines are a no-op.
eval 'exec perl -wSx -pi "$0" "$@"'
     if 0;

s/a/b/
EOF
chmod a+x $s
echo a > $TMP-in
./$s $TMP-in 2>/dev/null && test b = "`cat $TMP-in 2>/dev/null`" ||
  {
    printf '%s\n' "$0: skipping this test;" \
      'your system has insufficient support for Perl' 1>&2
    exit 77
  }

# Skip this test if Perl is too old.  FIXME: 5.8.0 is just a guess.
# We have a report that 5.6.1 is inadequate and that 5.8.0 works.
perl -e 'require 5.8.0' || {
  echo "$0: skipping this test; Perl version is too old" 1>&2
  exit 77
}

# Do not let a different envvar setting perturb results.
UPDATE_COPYRIGHT_MAX_LINE_LENGTH=72
export UPDATE_COPYRIGHT_MAX_LINE_LENGTH

# This assignment to 'ight' is so that data can contain strings
# like "Copy$ight 2024 Free Software Foundation, Inc." without being
# mistakenly updated by "cd .. && make update-copyright".
ight=right

## ----------------------------- ##
## Examples from documentation.  ##
## ----------------------------- ##

TMP=$TMP_BASE-ex
cat > $TMP-1 <<EOF
Copy$ight @copy$ight{} 1990-2005, 2007-2009 Free Software
Foundation, Inc.
EOF
cat > $TMP-2 <<EOF
# Copy$ight (C) 1990-2005, 2007-2009 Free Software
# Foundation, Inc.
EOF
cat > $TMP-3 <<EOF
/*
 * Copy$ight &copy; 90,2005,2007-2009
 * Free Software Foundation, Inc.
 */
EOF
cat > $TMP-4 <<EOF
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.
EOF
cat > $TMP-5 <<EOF
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.
EOF
cat > $TMP-6 <<EOF
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.

Copy$ight (C) 1990-2005, 2007-2009 Free Software Foundation,
Inc.
EOF
cat > $TMP-7 <<EOF
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.

# Copy$ight (C) 1990-2005, 2007-2009 Free Software
# Foundation, Inc.
EOF
cat > $TMP-8 <<EOF
Copy$ight (C) 2008 Free Software Foundation, Inc.
Copy$ight (C) 2008 Free Software Foundation, Inc.
EOF
cat > $TMP-9 <<EOF
# Copy$ight (C) 2008 Free Software Foundation, Inc.

Copy$ight (C) 2007 Free Software Foundation, Inc.

Copy$ight (C) 2008 Free Software Foundation, Inc.
EOF

UPDATE_COPYRIGHT_YEAR=2009 \
  update-copyright $TMP-? 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare - $TMP-stderr <<EOF || exit 1
$TMP-4: warning: copyright statement not found
$TMP-5: warning: copyright statement not found
EOF
compare - $TMP-1 <<EOF || exit 1
Copy$ight @copy$ight{} 1990-2005, 2007-2009 Free Software
Foundation, Inc.
EOF
compare - $TMP-2 <<EOF || exit 1
# Copy$ight (C) 1990-2005, 2007-2009 Free Software
# Foundation, Inc.
EOF
compare - $TMP-3 <<EOF || exit 1
/*
 * Copy$ight &copy; 90,2005,2007-2009
 * Free Software Foundation, Inc.
 */
EOF
compare - $TMP-4 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.
EOF
compare - $TMP-5 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.
EOF
compare - $TMP-6 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.

Copy$ight (C) 1990-2005, 2007-2009 Free Software Foundation,
Inc.
EOF
compare - $TMP-7 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.

# Copy$ight (C) 1990-2005, 2007-2009 Free Software
# Foundation, Inc.
EOF
compare - $TMP-8 <<EOF || exit 1
Copy$ight (C) 2008, 2009 Free Software Foundation, Inc.
Copy$ight (C) 2008, 2009 Free Software Foundation, Inc.
EOF

# Note that expecting "2007, 2009" on the third copyright line below
# looks wrong since the corresponding copyright year was "2008".
# However, the premise of this test (having inconsistent copyright dates
# with the same holder) is not worth handling. What happens is that the
# xform for the preceding line matches any list of year numbers with the
# same prefix and holder.
compare - $TMP-9 <<EOF || exit 1
# Copy$ight (C) 2008, 2009 Free Software Foundation, Inc.

Copy$ight (C) 2007, 2009 Free Software Foundation, Inc.

Copy$ight (C) 2007, 2009 Free Software Foundation, Inc.
EOF

UPDATE_COPYRIGHT_YEAR=2010 UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP-? 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare - $TMP-stderr <<EOF || exit 1
$TMP-4: warning: copyright statement not found
$TMP-5: warning: copyright statement not found
EOF
compare - $TMP-1 <<EOF || exit 1
Copy$ight @copy$ight{} 1990-2005, 2007-2010 Free Software Foundation,
Inc.
EOF
compare - $TMP-2 <<EOF || exit 1
# Copy$ight (C) 1990-2005, 2007-2010 Free Software Foundation, Inc.
EOF
compare - $TMP-3 <<EOF || exit 1
/*
 * Copy$ight &copy; 1990, 2005, 2007-2010 Free Software Foundation, Inc.
 */
EOF
compare - $TMP-4 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.
EOF
compare - $TMP-5 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.
EOF
compare - $TMP-6 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.

Copy$ight (C) 1990-2005, 2007-2010 Free Software Foundation, Inc.
EOF
compare - $TMP-7 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.

# Copy$ight (C) 1990-2005, 2007-2010 Free Software Foundation, Inc.
EOF

UPDATE_COPYRIGHT_YEAR=2010 UPDATE_COPYRIGHT_FORCE=1 \
  update-copyright $TMP-? 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare - $TMP-stderr <<EOF || exit 1
$TMP-4: warning: copyright statement not found
$TMP-5: warning: copyright statement not found
EOF
compare - $TMP-1 <<EOF || exit 1
Copy$ight @copy$ight{} 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010
Free Software Foundation, Inc.
EOF
compare - $TMP-2 <<EOF || exit 1
# Copy$ight (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
# 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010 Free
# Software Foundation, Inc.
EOF
compare - $TMP-3 <<EOF || exit 1
/*
 * Copy$ight &copy; 1990, 2005, 2007, 2008, 2009, 2010 Free Software
 * Foundation, Inc.
 */
EOF
compare - $TMP-4 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.
EOF
compare - $TMP-5 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.
EOF
compare - $TMP-6 <<EOF || exit 1
## Copy$ight (C) 1990-2005, 2007-2009 Free Software
#  Foundation, Inc.

Copy$ight (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010 Free
Software Foundation, Inc.
EOF
compare - $TMP-7 <<EOF || exit 1
Copy$ight (C) 1990-2005, 2007-2009 Acme, Inc.

# Copy$ight (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
# 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010 Free
# Software Foundation, Inc.
EOF

rm $TMP*

## ------------------------ ##
## Examples from man pages  ##
## ------------------------ ##

TMP=$TMP_BASE-man
cat > $TMP.1 <<EOF
'\" Copy$ight (C) 1998-2009 Free Software Foundation, Inc.
EOF
cat > $TMP.2 <<EOF
Copy$ight \(co 1998-2009 Free Software Foundation, Inc.
EOF
cat > $TMP.3 <<EOF
Copy$ight \(co 1998\(en2009 Free Software Foundation, Inc.
EOF
cat > $TMP.4 <<EOF
Copy$ight \(co 2009 Free Software Foundation, Inc.
EOF

UPDATE_COPYRIGHT_YEAR=2010 UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP.? 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP.1 <<EOF || exit 1
'\" Copy$ight (C) 1998\(en2010 Free Software Foundation, Inc.
EOF
compare - $TMP.2 <<EOF || exit 1
Copy$ight \(co 1998\(en2010 Free Software Foundation, Inc.
EOF
compare - $TMP.3 <<EOF || exit 1
Copy$ight \(co 1998\(en2010 Free Software Foundation, Inc.
EOF
compare - $TMP.4 <<EOF || exit 1
Copy$ight \(co 2009\(en2010 Free Software Foundation, Inc.
EOF

rm $TMP*

## -------------- ##
## Current year.  ##
## -------------- ##

TMP=$TMP_BASE-current-year
YEAR=`date +%Y`
cat > $TMP <<EOF
'\" Copy$ight (C) 2006
'\" Free Software Foundation,
'\" Inc.
EOF
update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
'\" Copyright (C) 2006, $YEAR Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
'\" Copy$ight (C) 2006, $YEAR Free Software Foundation, Inc.
EOF
rm $TMP*

## ------------------ ##
## Surrounding text.  ##
## ------------------ ##

TMP=$TMP_BASE-surrounding-text
cat > $TMP <<EOF
    Undisturbed text.
dnl Undisturbed text.
dnl Copy$ight (C) 89
dnl Free Software Foundation, Inc.
dnl   Undisturbed text.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
    Undisturbed text.
dnl Undisturbed text.
dnl Copy$ight (C) 1989, 2010 Free Software Foundation, Inc.
dnl   Undisturbed text.
EOF
rm $TMP*

## --------------- ##
## Widest prefix.  ##
## --------------- ##

TMP=$TMP_BASE-widest-prefix
cat > $TMP <<EOF
#### Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
#### 1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
#### 2008 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
#### Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984,
#### 1985, 1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005,
#### 2006, 2007, 2008, 2010 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2011 UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
#### Copy$ight (C) 1976-1988, 1999-2008, 2010-2011 Free Software
#### Foundation, Inc.
EOF
rm $TMP*

## ------------------- ##
## Prefix too large.  ##
## ------------------- ##

TMP=$TMP_BASE-prefix-too-large
cat > $TMP <<EOF
####  Copyright (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
####  1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
####  2008 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare - $TMP-stderr <<EOF || exit 1
$TMP: warning: copyright statement not found
EOF
compare - $TMP <<EOF || exit 1
####  Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
####  1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
####  2008 Free Software Foundation, Inc.
EOF
rm $TMP*

## ------------- ##
## Blank lines.  ##
## ------------- ##

TMP=$TMP_BASE-blank-lines
cat > $TMP <<EOF
#Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
#
#1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
#2008 Free Software Foundation, Inc.

Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,

1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
2008 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare - $TMP-stderr <<EOF || exit 1
$TMP: warning: copyright statement not found
EOF
compare - $TMP <<EOF || exit 1
#Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
#
#1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
#2008 Free Software Foundation, Inc.

Copy$ight (C) 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,

1986, 1987, 1988, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
2008 Free Software Foundation, Inc.
EOF
rm $TMP*

## -------------- ##
## Leading tabs.  ##
## -------------- ##

TMP=$TMP_BASE-leading-tabs
cat > $TMP <<EOF
	Copy$ight (C) 87, 88, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 98,
	 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free
	Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
	Copy$ight (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996,
	1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
	2007, 2008, 2009, 2010 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2011 UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
	Copy$ight (C) 1987-1988, 1991-2011 Free Software Foundation,
	Inc.
EOF
rm $TMP*

## -------------------- ##
## Unusual whitespace.  ##
## -------------------- ##

TMP=$TMP_BASE-unusual-ws
cat > $TMP <<EOF
		# Copy$ight (C) 87-88, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
		# 98, 1999, 2000, 2001, 2002, 2003,     		  2004, 2005, 2006, 2007, 2008,
		# 2009 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
		# Copy$ight (C) 1987, 1988, 1991, 1992, 1993, 1994,
		# 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
		# 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free Software
		# Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2011 UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
		# Copy$ight (C) 1987-1988, 1991-2011 Free Software
		# Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2011 UPDATE_COPYRIGHT_USE_INTERVALS=2 \
  UPDATE_COPYRIGHT_FORCE=1 update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
		# Copy$ight (C) 1987-2011 Free Software Foundation, Inc.
EOF
rm $TMP*

## --------- ##
## DOS EOL.  ##
## --------- ##

TMP=$TMP_BASE-dos-eol
tr @ '\015' > $TMP <<EOF
Rem Copy$ight (C) 87, 88, 1991, 1992, 1993, 1994, 1995, 1996, 1997,@
Rem 98, 1999, 2000, 2001, 2002, 2003,  2004, 2005, 2006, 2007, 2008,@
Rem 2009 Free Software Foundation, Inc.@
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
tr @ '\015' > $TMP-exp <<EOF
Rem Copy$ight (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997,@
Rem 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,@
Rem 2009, 2010 Free Software Foundation, Inc.@
EOF
compare $TMP-exp $TMP || exit 1
rm $TMP*

## --------------- ##
## Omitted "(C)".  ##
## --------------- ##

TMP=$TMP_BASE-omitted-circle-c
cat > $TMP <<EOF
  Copy$ight 87, 88, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
  98, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
  2009 Free Software Foundation, Inc.
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP <<EOF || exit 1
  Copy$ight 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
  1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
EOF
rm $TMP*

## ------------------ ##
## C-style comments.  ##
## ------------------ ##

TMP=$TMP_BASE-c-style-comments
cat > $TMP.star <<EOF
/*  Copy$ight 87, 88, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
 *  98, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
 *  2009 Free Software Foundation, Inc.  */
EOF
cat > $TMP.space <<EOF
  /*Copy$ight 87, 88, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
    98, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
    2009 Free Software Foundation, Inc.  */
EOF
cat > $TMP.single-line <<EOF
/*   Copy$ight 87, 1991, 1992 Free Software Foundation, Inc.  */
EOF
cat > $TMP.single-line-wrapped <<EOF
 /* Copy$ight 1988, 1991, 1992, 1993 Free Software Foundation, Inc.  */
EOF
cat > $TMP.extra-text-star <<EOF
 /* Copy$ight 1987, 1988, 1991, 1992 Free Software Foundation, Inc.  End
  * More comments.  */
EOF
cat > $TMP.extra-text-space <<EOF
 /* Copy$ight 1987, 1988, 1991, 1992 Free Software Foundation, Inc. ***
    * End of comments. */
EOF
cat > $TMP.two-digit-final-is-substr-of-first <<EOF
 /* Copy$ight 1991, 99 Free Software Foundation, Inc. */
EOF
UPDATE_COPYRIGHT_YEAR=2010 \
  update-copyright $TMP.* 1> $TMP-stdout 2> $TMP-stderr
compare /dev/null $TMP-stdout || exit 1
compare /dev/null $TMP-stderr || exit 1
compare - $TMP.star <<EOF || exit 1
/*  Copy$ight 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
 *  1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
 *  2009, 2010 Free Software Foundation, Inc.  */
EOF
compare - $TMP.space <<EOF || exit 1
  /*Copy$ight 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
    1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
    2009, 2010 Free Software Foundation, Inc.  */
EOF
compare - $TMP.single-line <<EOF || exit 1
/*   Copy$ight 1987, 1991, 1992, 2010 Free Software Foundation, Inc.  */
EOF
compare - $TMP.single-line-wrapped <<EOF || exit 1
 /* Copy$ight 1988, 1991, 1992, 1993, 2010 Free Software Foundation,
  * Inc.  */
EOF
compare - $TMP.extra-text-star <<EOF || exit 1
 /* Copy$ight 1987, 1988, 1991, 1992, 2010 Free Software Foundation,
  * Inc.  End
  * More comments.  */
EOF
compare - $TMP.extra-text-space <<EOF || exit 1
 /* Copy$ight 1987, 1988, 1991, 1992, 2010 Free Software Foundation,
    Inc. ***
    * End of comments. */
EOF
compare - $TMP.two-digit-final-is-substr-of-first <<EOF || exit 1
 /* Copy$ight 1991, 1999, 2010 Free Software Foundation, Inc. */
EOF
rm $TMP*

exit 0
