dnl Checking for libusb features
dnl
AC_DEFUN([ACX_LIBUSB], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_C

acx_libusb_version=none
for acx_try in '' '-1.0' ; do
	AC_CHECK_LIB(usb$acx_try, libusb_init, [
		acx_libusb_version=$acx_try
		acx_libusb=usb$acx_try
		break
	])
done

if test x$acx_libusb_version = xnone ; then
    AC_MSG_ERROR([libusb not found])
fi

AC_DEFINE(LIBUSB, $acx_libusb, [name of libusb])
AC_SUBST(LIBUSB, $acx_libusb)
if test x$acx_libusb_version != xnone ; then
	CFLAGS="$CFLAGS -I/usr/include/libusb$acx_libusb_version"
fi

AC_CHECK_LIB($acx_libusb, libusb_strerror, HAVE_LIBUSB_STRERROR=1)
if test -n "$HAVE_LIBUSB_STRERROR" ; then
	AC_DEFINE(HAVE_LIBUSB_STRERROR,1,[libusb has libusb_strerror()])
	AC_SUBST(HAVE_LIBUSB_STRERROR, $HAVE_LIBUSB_STRERROR)
fi

])dnl ACX_LIBUSB

