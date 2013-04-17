/*
 * (C) 2011-2013 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libnotify/notify.h>
#include <libudev.h>

#define PROGNAME	"udev-net-notify"

#define NOTIFICATION_TIMEOUT	10000
#ifndef DEBUG
#define DEBUG	0
#endif

#define ICON_ADD	"network-wired"
#define ICON_REMOVE	"network-wired-disconnected"
#define ICON_MOVE	"dialog-information"
#define ICON_CHANGE	"dialog-warning"
#define ICON_DEFAULT	"dialog-warning"

#define TEXT_TOPIC	"Udev Net Notification"
#define TEXT_ADD	"Device <b>%s appeared</b>."
#define TEXT_REMOVE	"Device <b>%s disappeared</b>."
#define TEXT_MOVE	"Device <b>%s</b> was <b>renamed</b>."
#define TEXT_CHANGE	"Anything for device %s changed."
#define TEXT_DEFAULT	"Anything happend to %s... Don't know."

char * newstr(char *text, char *device) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(device));
	sprintf(notifystr, text, device);

	return notifystr;
}

int main (int argc, char ** argv) {
	char *device = NULL, *icon = NULL, *notifystr = NULL;
	fd_set readfds;
	GError *error = NULL;
	int fdcount, errcount = 0;
        NotifyNotification *notification;
	struct udev_device *dev = NULL;
   	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;

	printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__ ")\n", argv[0], PROGNAME, VERSION);

	if(!notify_init(PROGNAME)) {
		fprintf(stderr, "%s: Can't create notify.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	udev = udev_new();
	if(!udev) {
		fprintf(stderr, "%s: Can't create udev.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "net", NULL);
	udev_monitor_enable_receiving(mon);

	while (1) {
                FD_ZERO(&readfds);
                if (mon != NULL)
                        FD_SET(udev_monitor_get_fd(mon), &readfds);

                fdcount = select(udev_monitor_get_fd(mon) + 1, &readfds, NULL, NULL, NULL);

                if ((mon != NULL) && FD_ISSET(udev_monitor_get_fd(mon), &readfds)) {
			dev = udev_monitor_receive_device(mon);
			if(dev) {
				device = (char *) udev_device_get_sysname(dev);

				switch(udev_device_get_action(dev)[0]) {
					case 'a':
						// a: add
						notifystr = newstr(TEXT_ADD, device);
						icon = ICON_ADD;
						break;
					case 'r':
						// r: remove
						notifystr = newstr(TEXT_REMOVE, device);
						icon = ICON_REMOVE;
						break;
					case 'm':
						// m: move
						notifystr = newstr(TEXT_MOVE, device);
						icon = ICON_MOVE;
						break;
					case 'c':
						// c: change
						notifystr = newstr(TEXT_CHANGE, device);
						icon = ICON_CHANGE;
						break;
					default:
						// we should never get here I think...
						notifystr = newstr(TEXT_DEFAULT, device);
						icon = ICON_DEFAULT;
				}
#if DEBUG
				printf("%s: %s\n", argv[0], notifystr);
#endif
				notification = notify_notification_new(TEXT_TOPIC, notifystr, icon);
	
			        notify_notification_set_timeout(notification, NOTIFICATION_TIMEOUT);
				notify_notification_set_category(notification, PROGNAME);
				notify_notification_set_urgency (notification, NOTIFY_URGENCY_NORMAL);
	
				while(!notify_notification_show(notification, &error)) {
					if (errcount > 1) {
						fprintf(stderr, "%s: Looks like we can not reconnect to notification daemon... Exiting.\n", argv[0]);
						exit(EXIT_FAILURE);
					} else {
						g_printerr("%s: Error \"%s\" while trying to show notification. Trying to reconnect.\n", argv[0], error->message);
						errcount++;
						
						g_error_free(error);
						error = NULL;
	
						notify_uninit();

						usleep(500 * 1000);

						if(!notify_init(PROGNAME)) {
							fprintf(stderr, "%s: Can't create notify.\n", argv[0]);
							exit(EXIT_FAILURE);
						}
					}
				}
				errcount = 0;
	
				free(notifystr);
				udev_device_unref(dev);
			}
	
			// This is not really needed... But we want to make shure not to eat 100% CPU if anything breaks. ;)
			usleep(50 * 1000);
		}
	}

	udev_unref(udev);
	return EXIT_SUCCESS;
}
