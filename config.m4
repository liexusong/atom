dnl $Id$
dnl config.m4 for extension ukey

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(ukey, for ukey support,
dnl Make sure that the comment is aligned:
dnl [  --with-ukey             Include ukey support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(ukey, whether to enable ukey support,
dnl Make sure that the comment is aligned:
[  --enable-ukey           Enable ukey support])

if test "$PHP_UKEY" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-ukey -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/ukey.h"  # you most likely want to change this
  dnl if test -r $PHP_UKEY/$SEARCH_FOR; then # path given as parameter
  dnl   UKEY_DIR=$PHP_UKEY
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for ukey files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       UKEY_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$UKEY_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the ukey distribution])
  dnl fi

  dnl # --with-ukey -> add include path
  dnl PHP_ADD_INCLUDE($UKEY_DIR/include)

  dnl # --with-ukey -> check for lib and symbol presence
  dnl LIBNAME=ukey # you may want to change this
  dnl LIBSYMBOL=ukey # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $UKEY_DIR/lib, UKEY_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_UKEYLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong ukey lib version or lib not found])
  dnl ],[
  dnl   -L$UKEY_DIR/lib -lm -ldl
  dnl ])
  dnl
  dnl PHP_SUBST(UKEY_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ukey, ukey.c spinlock.c shm.c, $ext_shared)
fi
