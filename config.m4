dnl config.m4 for extension bsdiff

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary.

dnl If your extension references something external, use 'with':

dnl PHP_ARG_WITH([bsdiff],
dnl   [for bsdiff support],
dnl   [AS_HELP_STRING([--with-bsdiff],
dnl     [Include bsdiff support])])

dnl Otherwise use 'enable':

PHP_ARG_ENABLE([bsdiff],
  [whether to enable bsdiff support],
  [AS_HELP_STRING([--enable-bsdiff],
    [Enable bsdiff support])],
  [no])

PHP_ARG_WITH([bz2],
  [to specify installation directory of BZip2],
  [AS_HELP_STRING([[--with-bz2=DIR]],
    [Specify installation directory of BZip2])])

if test "$PHP_BSDIFF" != "no"; then
  dnl Write more examples of tests here...

  dnl Remove this code block if the library does not support pkg-config.
  dnl PKG_CHECK_MODULES([LIBFOO], [foo])
  dnl PHP_EVAL_INCLINE($LIBFOO_CFLAGS)
  dnl PHP_EVAL_LIBLINE($LIBFOO_LIBS, BSDIFF_SHARED_LIBADD)

  dnl If you need to check for a particular library version using PKG_CHECK_MODULES,
  dnl you can use comparison operators. For example:
  dnl PKG_CHECK_MODULES([LIBFOO], [foo >= 1.2.3])
  dnl PKG_CHECK_MODULES([LIBFOO], [foo < 3.4])
  dnl PKG_CHECK_MODULES([LIBFOO], [foo = 1.2.3])

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-bsdiff -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/bsdiff.h"  # you most likely want to change this
  dnl if test -r $PHP_BSDIFF/$SEARCH_FOR; then # path given as parameter
  dnl   BSDIFF_DIR=$PHP_BSDIFF
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for bsdiff files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       BSDIFF_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$BSDIFF_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the bsdiff distribution])
  dnl fi

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-bsdiff -> add include path
  dnl PHP_ADD_INCLUDE($BSDIFF_DIR/include)

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-bsdiff -> check for lib and symbol presence
  dnl LIBNAME=BSDIFF # you may want to change this
  dnl LIBSYMBOL=BSDIFF # you most likely want to change this

  if test "$PHP_BZ2" != "no"; then
    if test -r $PHP_BZ2/include/bzlib.h; then
      BZIP_DIR=$PHP_BZ2
    else
      AC_MSG_CHECKING(for BZip2 in default path)
      for i in /usr/local /usr; do
        if test -r $i/include/bzlib.h; then
          BZIP_DIR=$i
          AC_MSG_RESULT(found in $i)
          break
        fi
      done
    fi

    if test -z "$BZIP_DIR"; then
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR(Please reinstall the BZip2 distribution)
    fi

    PHP_CHECK_LIBRARY(bz2, BZ2_bzWriteOpen,
    [
      PHP_ADD_INCLUDE($BZIP_DIR/include)
      PHP_ADD_LIBRARY_WITH_PATH(bz2, $BZIP_DIR/$PHP_LIBDIR, BSDIFF_SHARED_LIBADD)
      AC_DEFINE(HAVE_BZ2,1,[ ])
    ], [
      AC_MSG_ERROR(php-bsdiff requires libbz2 >= 1.0.0)
    ], [
      -L$BZIP_DIR/$PHP_LIBDIR
    ])
    PHP_SUBST(BSDIFF_SHARED_LIBADD)
  fi

  dnl In case of no dependencies
  AC_DEFINE(HAVE_BSDIFF, 1, [ Have bsdiff support ])

  PHP_NEW_EXTENSION(bsdiff, php_bsdiff.c bsdiff.c bspatch.c, $ext_shared,,-I@ext_srcdir@)
fi
