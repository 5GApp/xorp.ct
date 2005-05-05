## ----------------------------------- ##
## Check if --with-dmalloc was given.  ##
## From Franc,ois Pinard               ##
## ----------------------------------- ##
## Modified by Pavlin Radoslavov:      ##
##   - can specify dmalloc directory   ##
##   - `-ldmalloc' before `$LIBS'      ##
##   - correct dmalloc URL             ##
## Modified by bms: use AC_HELP_STRING ##

# serial 3

dnl Note: Don't use AC_LANG* macros here, they aren't ready when
dnl this gets included.

AC_DEFUN([XR_WITH_DMALLOC_DIR],
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
	    AC_HELP_STRING([--with-dmalloc=DIR],
			   [build using the dmalloc allocator in DIR]),
[if test "$withval" = no; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC,1,
            [Define if using the dmalloc debugging malloc package])
  if test "$withval" = yes; then
    LIBS="-ldmalloc $LIBS"
  else
    LIBS="-L$withval -L$withval/lib -ldmalloc $LIBS"
    CFLAGS="$CFLAGS -I$withval -I$withval/include"
  fi
  LDFLAGS="$LDFLAGS -g"
fi], [AC_MSG_RESULT(no)])
])

AC_CACHE_SAVE
