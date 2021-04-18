ACLOCAL_AMFLAGS=-I m4

bin_PROGRAMS=rr-early-device-setup

rr_early_device_setup_SOURCES =	\
	main.c						\
	Controller.c				\
	Controller.h				\
	jobs.c						\
	jobs.h						\
	gpio_led.c					\
	gpio_led.h					\
	handover.c					\
	handover.h				

rr_early_device_setup_CPPFLAGS = \
		$(SYSTEMD_CFLAGS)


rr_early_device_setup_LDADD	  = \
		$(SYSTEMD_LIBS)
