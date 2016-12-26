dnl $Id$
dnl config.m4 for extension atom

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(atom, for atom support,
dnl Make sure that the comment is aligned:
dnl [  --with-atom             Include atom support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(atom, whether to enable atom support,
dnl Make sure that the comment is aligned:
[  --enable-atom           Enable atom support])

if test "$PHP_ATOM" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-atom -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/atom.h"  # you most likely want to change this
  dnl if test -r $PHP_ATOM/$SEARCH_FOR; then # path given as parameter
  dnl   ATOM_DIR=$PHP_ATOM
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for atom files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       ATOM_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$ATOM_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the atom distribution])
  dnl fi

  dnl # --with-atom -> add include path
  dnl PHP_ADD_INCLUDE($ATOM_DIR/include)

  dnl # --with-atom -> check for lib and symbol presence
  dnl LIBNAME=atom # you may want to change this
  dnl LIBSYMBOL=atom # you most likely want to change this

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $ATOM_DIR/$PHP_LIBDIR, ATOM_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_ATOMLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong atom lib version or lib not found])
  dnl ],[
  dnl   -L$ATOM_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(ATOM_SHARED_LIBADD)

  PHP_NEW_EXTENSION(atom, atom.c shm.c spinlock.c, $ext_shared)
fi
