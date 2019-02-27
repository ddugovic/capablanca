dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.60])
AC_INIT(acconfig.h, [2.2])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER(autoconfig.h)
AC_USE_SYSTEM_EXTENSIONS

dnl This isn't a standard way to configure, but it works.
CHESSD_CFLAGS="-DCHESSDDIR='\"/usr/local/chessd\"'"

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_INSTALL

dnl Checks for libraries.
AC_CHECK_LIB(mcrypt, mcrypt_check_version, [], [exit -1])
AC_CHECK_LIB(crypt, crypt, [], [exit -1])
AC_CHECK_LIB(dl, dlopen, [], [exit -1])
AC_CHECK_LIB(tdb, tdb_open, [], [exit -1])
dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, sqrt, [], [exit -1])

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h strings.h sys/file.h sys/ioctl.h sys/time.h unistd.h, [], [exit -1])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_CACHE_CHECK([for compar_fn_t in stdlib.h],ccache_cv_COMPAR_FN_T, [
    AC_TRY_COMPILE(
[#include <stdlib.h>],
[
void test_fn(void) { qsort(NULL, 0, 0, (__compar_fn_t)NULL); }
],
	ccache_cv_COMPAR_FN_T=yes,ccache_cv_COMPAR_FN_T=no)])
if test x"$ccache_cv_COMPAR_FN_T" = x"yes"; then
   AC_DEFINE(HAVE_COMPAR_FN_T, 1, [ ])
fi

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_FUNC_WAIT3
AC_CHECK_FUNCS(gethostname gettimeofday time select socket strcspn strdup strstr dprintf, [], [exit -1])
AC_CHECK_FUNCS(strnlen strlcpy strlcat)

# We want to check for compiler flag support, but there is no way to make
# clang's "argument unused" warning fatal.  So we invoke the compiler through a
# wrapper script that greps for this message.
saved_CC="$CC"
saved_CXX="$CXX"
saved_LD="$LD"
flag_wrap="$srcdir/scripts/wrap-compiler-for-flag-check"
CC="$flag_wrap $CC"
CXX="$flag_wrap $CXX"
LD="$flag_wrap $LD"

# We use the same hardening flags for C and C++.  We must check that each flag
# is supported by both compilers.
AC_DEFUN([check_cc_cxx_flag],
 [AC_LANG_PUSH(C)
  AX_CHECK_COMPILE_FLAG([$1],
   [AC_LANG_PUSH(C++)
    AX_CHECK_COMPILE_FLAG([$1], [$2], [$3], [-Werror $4])
    AC_LANG_POP(C++)],
   [$3], [-Werror $4])
  AC_LANG_POP(C)])
AC_DEFUN([check_link_flag],
 [AX_CHECK_LINK_FLAG([$1], [$2], [$3], [-Werror $4])])

AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--enable-hardening],
    [Enable compiler and linker options to frustrate memory corruption exploits @<:@yes@:>@])],
  [hardening="$enableval"],
  [hardening="yes"])

HARDEN_CFLAGS=""
HARDEN_LDFLAGS=""
AS_IF([test x"$hardening" != x"no"], [
  check_cc_cxx_flag([-fno-strict-overflow], [HARDEN_CFLAGS="$HARDEN_CFLAGS -fno-strict-overflow"])

  # This one will likely succeed, even on platforms where it does nothing.
  check_cc_cxx_flag([-D_FORTIFY_SOURCE=2], [HARDEN_CFLAGS="$HARDEN_CFLAGS -D_FORTIFY_SOURCE=2"])

  check_cc_cxx_flag([-fstack-protector-all],
   [check_link_flag([-fstack-protector-all],
     [HARDEN_CFLAGS="$HARDEN_CFLAGS -fstack-protector-all"
      check_cc_cxx_flag([-Wstack-protector], [HARDEN_CFLAGS="$HARDEN_CFLAGS -Wstack-protector"],
        [], [-fstack-protector-all])
      check_cc_cxx_flag([--param ssp-buffer-size=1], [HARDEN_CFLAGS="$HARDEN_CFLAGS --param ssp-buffer-size=1"],
        [], [-fstack-protector-all])])])

  # At the link step, we might want -pie (GCC) or -Wl,-pie (Clang on OS X)
  #
  # The linker checks also compile code, so we need to include -fPIE as well.
  check_cc_cxx_flag([-fPIE],
   [check_link_flag([-fPIE -pie],
     [HARDEN_CFLAGS="$HARDEN_CFLAGS -fPIE"
      HARDEN_LDFLAGS="$HARDEN_LDFLAGS -pie"],
     [check_link_flag([-fPIE -Wl,-pie],
       [HARDEN_CFLAGS="$HARDEN_CFLAGS -fPIE"
        HARDEN_LDFLAGS="$HARDEN_LDFLAGS -Wl,-pie"])])])

  check_link_flag([-Wl,-z,relro],
   [HARDEN_LDFLAGS="$HARDEN_LDFLAGS -Wl,-z,relro"
    check_link_flag([-Wl,-z,now], [HARDEN_LDFLAGS="$HARDEN_LDFLAGS -Wl,-z,now"])])])
AC_SUBST([HARDEN_CFLAGS])
AC_SUBST([HARDEN_LDFLAGS])

# End of flag tests.
CC="$saved_CC"
CXX="$saved_CXX"
LD="$saved_LD"

AC_SUBST(CHESSD_CFLAGS)

AC_OUTPUT(Makefile)
