# udev-net-notify - Notify about udev net events

CC	:= gcc
CFLAGS	+= $(shell pkg-config --cflags --libs libudev) \
	   $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: udev-net-notify.c
	$(CC) $(CFLAGS) -o udev-net-notify udev-net-notify.c \
		-DVERSION="\"$(VERSION)\""

clean:
	/bin/rm -f *.o *~ udev-net-notify
