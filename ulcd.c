/*
 * Copyright (c) 2003,2004 Bernd Walter Computer Technology
 * http://www.bwct.de
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $URL$
 * $Date$
 * $Author$
 * $Rev$
 */

/*
 * ./ulcd -i -c 40 -c 14 -c 6
 * 
 * -i initialisiert das Display - genaugenommen passiert damit nichts und
 * ist für zukünftige Erweiterungen gedacht.
 * 
 * -c zahl die folgende Zahl als Komando zum  Display.
 * 40 ist dabei 4bit Betrieb, 2 Zeilen, 5x8 Zeichmatrix.
 *    Der Converter betreibt das Display im 4 bit Modus, damit im Falle
 *    einer freien Verkabelung weniger Leitungen zu legen sind.
 * 14 ist Display an und nicht blinkender Cursor.
 * 6 ist Cursor nach rechts bewegen - Displayinhalt nicht bewegen.
 * 
 * Im Detail im HD44780 Datenblatt nachzulesen.
 * Datenblatt als PDF ist unter http://www.bwct.de/hd44780.pdf hinterlegt.
 * Die Tabelle der Komandos und Beschreibungen stehen ab Seite 191.
 * 
 * ./ulcd -c 128 -s "USB LCD V1.0    "
 * 
 * -c 128 setzt die Cursorposition auf die 1. Zeile 1. Zeichen.
 * -s gibt den folgenden String ab der Cursorposition aus.
 * 
 * ./ulcd -c 192 -s "(c) 2004 by BWCT"
 * 
 * -c 192 setzt die Cursorposition auf die 2. Zeile 1. Zeichen.
 * -s gibt den folgenden String ab der Cursorposition aus.
 * 
 * ./ulcd -b 30
 * 
 * -b setzt den Kontrast entsprechend der nachfolgenden Zahl.
 */

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <usb.h>

#include "ulcd.h"

int main(int argc, char *argv[]);
void usage(void);

extern int usb_debug;

int
main(int argc, char *argv[]) {
	int res;
	char* tmp;
	int ch;
	int port;
	struct usb_bus *busses;
	struct usb_bus *bus;
	int c, i, a;
	usb_dev_handle *lcd;

#if 0
	usb_debug = 2;
#endif

	usb_init();
	usb_find_busses();
	usb_find_devices();
	busses = usb_get_busses();
	
	lcd = NULL;
	for (bus = busses; bus; bus = bus->next) {
		struct usb_device *dev;

		for (dev = bus->devices; dev; dev = dev->next) {
			/* Check if this device is a BWCT device */
			if (dev->descriptor.idVendor != 0x03da) {
				continue;
			}
			/* Loop through all of the configurations */
			for (c = 0; c < dev->descriptor.bNumConfigurations; c++) {
				/* Loop through all of the interfaces */
				for (i = 0; i < dev->config[c].bNumInterfaces; i++) {
					/* Loop through all of the alternate settings */
					for (a = 0; a < dev->config[c].interface[i].num_altsetting; a++) {
						/* Check if this interface is a ulcd */
						if ((dev->config[c].interface[i].altsetting[a].bInterfaceClass == 0xff &&
						    dev->config[c].interface[i].altsetting[a].bInterfaceSubClass == 0x01) ||
						    dev->descriptor.idProduct == 0x0002) {
							lcd = usb_open(dev);
							usb_claim_interface(lcd, i);
							goto done;
						}
					}
				}
			}
		}
	}
	done:
	if (lcd == NULL) {
		printf("failed to open LCD device\n");
		exit(1);
	}

	port = 0;

	while ((ch = getopt(argc, argv, "ib:c:d:p:s:")) != -1) {
		switch (ch) {
		case 'i':	/* init LCD */
			res = usb_control_msg(lcd, USB_TYPE_VENDOR, VENDOR_LCD_RESET, 0, port, NULL, 0, 1000);
			if (res < 0) {
				printf("USB request failed\n");
				exit(1);
			}
			break;
		case 'b':	/* set contrast */
			res = usb_control_msg(lcd, USB_TYPE_VENDOR, VENDOR_LCD_CONTRAST, atol(optarg), port, NULL, 0, 1000);
			if (res < 0) {
				printf("USB request failed\n");
				exit(1);
			}
			break;
		case 'c':	/* command byte */
			res = usb_control_msg(lcd, USB_TYPE_VENDOR, VENDOR_LCD_CMD, atol(optarg), port, NULL, 0, 1000);
			if (res < 0) {
				printf("USB request failed\n");
				exit(1);
			}
			break;
		case 'd':	/* data byte */
			res = usb_control_msg(lcd, USB_TYPE_VENDOR, VENDOR_LCD_DATA, atol(optarg), port, NULL, 0, 1000);
			if (res < 0) {
				printf("USB request failed\n");
				exit(1);
			}
			break;
		case 's':	/* string of data bytes */
			for (tmp = optarg; *tmp != '\0'; tmp++) {
				res = usb_control_msg(lcd, USB_TYPE_VENDOR, VENDOR_LCD_DATA, *tmp, port, NULL, 0, 1000);
				if (res < 0) {
					printf("USB request failed\n");
					exit(1);
				}
			}
			break;
		case 'p':	/* setup port */
			port = atol(optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (lcd != NULL) {
		usb_release_interface(lcd, i);
		usb_close(lcd);
	}
	return (0);
}

void
usage(void) {

	printf("usage: ulcd [args]\n");
	exit(1);
}

