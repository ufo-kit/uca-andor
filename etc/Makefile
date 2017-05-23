PREFIX ?= /usr/local
INCLUDEDIR ?= include
LIBDIR ?= lib

BITFLOW_LIB = libBFSOciLib.9.05.so

ANDOR_VERSION = 3.13.30001.0
ANDOR_HEADERS = atcore.h atutility.h
ANDOR_LIBS = libatblkbx.so \
	   libatcl_bitflow.so \
	   libatcore.so \
	   libatdevapogee.so \
	   libatdevregcam.so \
	   libatdevsimcam.so \
	   libatusb_libusb10.so \
	   libatutility.so

.PHONY: install uninstall

install:
	install -d $(DESTDIR)$(PREFIX)/$(LIBDIR) $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)
	install -t $(DESTDIR)$(PREFIX)/$(LIBDIR) bitflow/64b/lib/$(BITFLOW_LIB)
	for lib in $(ANDOR_LIBS); do \
		install -t $(DESTDIR)$(PREFIX)/$(LIBDIR) x86_64/"$$lib".$(ANDOR_VERSION) ; \
		ln -s $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib".$(ANDOR_VERSION) $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib" ; \
	done
	for header in $(ANDOR_HEADERS); do \
		install -t $(DESTDIR)$(PREFIX)/$(INCLUDEDIR) inc/"$$header" ; \
	done

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(BITFLOW_LIB)
	for lib in $(ANDOR_LIBS); do \
		rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib" ; \
		rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib".$(ANDOR_VERSION) ; \
	done
	for header in $(ANDOR_HEADERS); do \
		rm -f $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)/"$$header" ; \
	done
