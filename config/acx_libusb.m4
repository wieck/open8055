dnl Checking for libusb features
dnl
AC_DEFUN([ACX_LIBUSB], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_C

have_libusb_strerror=0
AC_CHECK_LIB(usb, libusb_strerror, HAVE_LIBUSB_STRERROR=1)
if test -n "$HAVE_LIBUSB_STRERROR" ; then
	AC_DEFINE(HAVE_LIBUSB_STRERROR,1,[libusb has libusb_strerror()])
fi

])dnl ACX_LIBUSB

