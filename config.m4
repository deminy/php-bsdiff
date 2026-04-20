PHP_ARG_ENABLE([bsdiff],
  [whether to enable bsdiff support],
  [AS_HELP_STRING([--enable-bsdiff],
    [Enable bsdiff support])],
  [yes])

PHP_ARG_WITH([bz2],
  [to specify installation directory of BZip2],
  [AS_HELP_STRING([[--with-bz2=DIR]],
    [Specify installation directory of BZip2 (default: auto-detect)])],
  [yes])

if test "$PHP_BSDIFF" != "no"; then
  if test "$PHP_BZ2" = "no"; then
    AC_MSG_ERROR(BZip2 support is required for php-bsdiff)
  fi

  if test "$PHP_BZ2" = "yes"; then
    AC_MSG_CHECKING(for BZip2 in default path)
    for i in /opt/homebrew/opt/bzip2 /usr/local/opt/bzip2 /opt/local /usr/local /usr; do
      if test -r $i/include/bzlib.h; then
        BZIP_DIR=$i
        break
      fi
    done
    if test -n "$BZIP_DIR"; then
      AC_MSG_RESULT(found in $BZIP_DIR)
    else
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR(Please reinstall the BZip2 distribution or specify its path with --with-bz2=DIR)
    fi
  else
    if test -r $PHP_BZ2/include/bzlib.h; then
      BZIP_DIR=$PHP_BZ2
    else
      AC_MSG_ERROR(BZip2 headers not found in $PHP_BZ2/include; please verify the --with-bz2 path)
    fi
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

  AC_DEFINE(HAVE_BSDIFF, 1, [ Have bsdiff support ])

  PHP_NEW_EXTENSION(bsdiff, php_bsdiff.c bsdiff.c bspatch.c, $ext_shared,,-I@ext_srcdir@)
fi
