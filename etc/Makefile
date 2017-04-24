PREFIX ?= /usr/local
INCLUDEDIR ?= include
LIBDIR ?= lib
VERSION = 3.13.30001.0

LIBS = libatblkbx.so \
	   libatcl_bitflow.so \
	   libatcore.so \
	   libatdevapogee.so \
	   libatdevregcam.so \
	   libatdevsimcam.so \
	   libatusb_libusb10.so \
	   libatutility.so

HEADERS = atcore.h atutility.h

.PHONY: install uninstall

install:
	install -d $(DESTDIR)$(PREFIX)/$(LIBDIR) $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)
	for lib in $(LIBS); do \
		install -t $(DESTDIR)$(PREFIX)/$(LIBDIR) x86_64/"$$lib".$(VERSION) ; \
		ln -s $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib".$(VERSION) $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib" ; \
	done
	for header in $(HEADERS); do \
		install -t $(DESTDIR)$(PREFIX)/$(INCLUDEDIR) inc/"$$header" ; \
	done

uninstall:
	for lib in $(LIBS); do \
		rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib" ; \
		rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/"$$lib".$(VERSION) ; \
	done
	for header in $(HEADERS); do \
		rm -f $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)/"$$header" ; \
	done
