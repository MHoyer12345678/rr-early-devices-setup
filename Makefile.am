ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src

DISTCHECK_CONFIGURE_FLAGS = \
  --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)
  
if HAVE_SYSTEMD
systemdsystemunit_DATA = \
  		files/rr-early-device-setup.service
endif
