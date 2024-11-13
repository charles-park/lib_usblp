//------------------------------------------------------------------------------
/**
 * @file main.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief usb label printer test app.
 * @version 0.1
 * @date 2024-11-07
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "lib_usblp.h"

#if defined (__LIB_USBLP_APP__)
#endif

//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
	int i, c = strlen(p);

	for (i = 0; i < c; i++, p++)
		*p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
	int i, c = strlen(p);

	for (i = 0; i < c; i++, p++)
		*p = toupper(*p);
}

//------------------------------------------------------------------------------
static void print_usage(const char *prog)
{
	printf("Usage: %s [-DawhItd]\n", prog);
	puts("  -D --device        device name. (default /dev/i2c-0).\n"
		 "  -a --i2c_addr      i2c chip address. (default 0x3f).\n"
		 "  -w --width         lcd width.(default w = 16)\n"
		 "  -h --height        lcd height.(default h = 2)\n"
		 "  -t --time_offset   Display current time & time offset.(default false)\n"
		 "  -d --delay         Display Switching delay (time & net info, default = 1)\n"
	);
	exit(1);
}

//------------------------------------------------------------------------------
static char		*OPT_DEVICE_NAME = "/dev/i2c-0";
static char		OPT_WIDTH = 16, OPT_HEIGHT = 2;
static uchar_t	OPT_DEVICE_ADDR = 0x3f;
static bool		OPT_LCD_SHIELD = true, OPT_TIME_DISPLAY = false;;
static int 		OPT_TIME_OFFSET = 0, OPT_DISPLAY_DELAY = 1;

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device_name",	1, 0, 'D' },
			{ "device_addr",	1, 0, 'a' },
			{ "width",			1, 0, 'w' },
			{ "height",			1, 0, 'h' },
			{ "time_offset",	1, 0, 't' },
			{ "delay",			1, 0, 'd' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:a:w:h:t:d:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			tolowerstr (optarg);
			OPT_DEVICE_NAME = optarg;
			break;
		case 'a':
			OPT_LCD_SHIELD = false;
			OPT_DEVICE_ADDR = strtol(optarg, NULL, 16) & 0xFF;
			break;
		case 'w':
			OPT_WIDTH = atoi(optarg);
			break;
		case 'h':
			OPT_HEIGHT = atoi(optarg);
			break;
		case 't':
			OPT_TIME_DISPLAY = true;
			OPT_TIME_OFFSET = atoi(optarg);
			break;
		case 'd':
			OPT_DISPLAY_DELAY = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_net_info (const unsigned char *eth_name,
							char *my_ip, int *speed, char *link)
{
	int fd;
	struct ifreq ifr;
	struct ethtool_cmd ecmd;

	/* this entire function is almost copied from ethtool source code */
	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		fprintf(stdout,"Cannot get control socket");
		return 0;
	}
	strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		fprintf(stdout, "SIOCGIFADDR ioctl Error!!\n");
		goto out;
	} else {
		inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, my_ip, sizeof(struct sockaddr));
		fprintf(stdout, "myOwn IP Address is %s\n", my_ip);
	}

	/* Pass the "get info" command to eth tool driver */
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	/* ioctl failed: */
	if (ioctl(fd, SIOCETHTOOL, &ifr))
	{
		fprintf(stdout,"Cannot get device settings");
		close(fd);
		goto out;
	}
	close(fd);

	fprintf(stdout, "LinkSpeed = %d MB/s", ecmd.speed);
	*speed = ecmd.speed;

	switch (ecmd.duplex)
	{
		case DUPLEX_FULL:
			strncpy(link, "FULL", 4);
			fprintf(stdout," Full Duplex\n");
		break;
		case DUPLEX_HALF:
			strncpy(link, "FULL", 4);
			fprintf(stdout," Half Duplex\n");
		break;
		default:
			strncpy(link, "????", 4);
			fprintf(stdout," Duplex reading faulty\n");
		break;
	}
	return 1;
out:
	return 0;
}

//------------------------------------------------------------------------------
static int system_init(void)
{
	int fd;

	// LCD Init
	fd = lcdInit(BOARD_LCD_ROW, BOARD_LCD_COL, BOARD_LCD_BUS,
				PORT_LCD_RS, PORT_LCD_E,
				PORT_LCD_D4, PORT_LCD_D5,
				PORT_LCD_D6, PORT_LCD_D7, 0, 0, 0, 0);

	if(fd < 0) {
		fprintf(stderr, "%s : lcdInit failed!\n", __func__);
		return -1;
	}

	// Button Pull Up Enable.
	pinMode (PORT_BUTTON1, INPUT);
	pullUpDnControl (PORT_BUTTON1, PUD_UP);

	pinMode (PORT_BUTTON2, INPUT);
	pullUpDnControl (PORT_BUTTON2, PUD_UP);

	pinMode (PORT_LED1, OUTPUT);	digitalWrite(PORT_LED1, 0);
	pinMode (PORT_LED2, OUTPUT);	digitalWrite(PORT_LED2, 0);
	pinMode (PORT_LED3, OUTPUT);	digitalWrite(PORT_LED3, 0);
	pinMode (PORT_LED4, OUTPUT);	digitalWrite(PORT_LED4, 0);
	pinMode (PORT_LED5, OUTPUT);	digitalWrite(PORT_LED5, 0);
	pinMode (PORT_LED6, OUTPUT);	digitalWrite(PORT_LED6, 0);
	pinMode (PORT_LED7, OUTPUT);	digitalWrite(PORT_LED7, 0);
	return  fd;
}

//------------------------------------------------------------------------------
static int lcd_clear_line (int fd, int line)
{
	if (line < 0) {
		int i;
		for (i = 0; i < OPT_HEIGHT; i++)
			lcd_puts (fd, 0, i, "                ");
	}
	lcd_puts (fd, 0, line, "                ");
	return 1;
}

//------------------------------------------------------------------------------
static int lcd_put_line (int fd, int x, int y, char *fmt, ...)
{
	char buf[OPT_WIDTH +1], len, i;
	va_list va;

	memset(buf, 0x00, sizeof(buf));

	va_start(va, fmt);
	len = vsprintf(buf, fmt, va);
	va_end(va);

	lcdPosition (fd, x, y);

	for (i = 0; i < len; i++)
		lcdPutchar(fd, buf[i]);

	return 1;
}

//------------------------------------------------------------------------------
static void time_display (int fd, int toffset)
{
	time_t t;
	char buf[40], len;

	time(&t);
	// time offset
	t += (toffset * 60 * 60);

	memset(buf, 0, sizeof(buf));
	len = sprintf (buf, "Time %s", ctime(&t));

	buf[len-1] = ' ';
	lcd_clr (fd, -1);
	lcd_puts (fd, 0, 0, "%s", &buf[0]);
	lcd_puts (fd, 0, 1, "%s", &buf[16]);
	fprintf(stdout, "Time = %s\n", buf);
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int fd, speed = 0, net_alive = 0;
	char my_net_ip[17], net_link[5];

	parse_opts(argc, argv);

	// 16x2 IO Shield Used
	if (OPT_LCD_SHIELD) {

		wiringPiSetup();

		if ((fd = system_init() < 0)) {
			fprintf (stderr, "%s: System Init failed\n", __func__);
			return 0;
		}
		lcd_puts = lcd_put_line;
		lcd_clr  = lcd_clear_line;

	} else {

		if ((fd = lcd_open(OPT_DEVICE_NAME,	OPT_DEVICE_ADDR)) == false) {
			err ("i2c-lcd init fail!\n");
			err ("Device Name = %s, Device Addr = 0x%02x\n",
						OPT_DEVICE_NAME, OPT_DEVICE_ADDR);
			return 0;
		}
		if (!lcd_init (fd, OPT_WIDTH, OPT_HEIGHT, true)) {
			err ("LCD Init Error!\n");
			err ("LCD Width = %d, Height = %d\n", OPT_WIDTH, OPT_HEIGHT);
			return 0;
		}
		lcd_puts = lcd_printf;
		lcd_clr  = lcd_clear;
	}

	// usb label printer search & setup
	usblp_reconfig ();

	while (true) {
		if (net_alive)
			net_alive = is_net_alive();
		else {
			speed = 0;
			memset (my_net_ip, 0, sizeof(my_net_ip));
			memset (net_link,  0, sizeof(net_link));
			net_alive = get_net_info ("eth0", my_net_ip, &speed, net_link);
		}

		lcd_clr(fd, -1);
		if (net_alive) {
			lcd_puts (fd, 0, 0, "%s", my_net_ip);
			lcd_puts (fd, 0, 1, "Speed=%d, %s", speed, net_link);
		} else {
			lcd_puts (fd, 0, 0, "Network Error! ");
			lcd_puts (fd, 0, 1, "Check ETH Cable");
		}
		sleep(OPT_DISPLAY_DELAY);

		if (OPT_TIME_DISPLAY) {
			time_display (fd, OPT_TIME_OFFSET);
			sleep(OPT_DISPLAY_DELAY);
		}

		if (!digitalRead(PORT_BUTTON1) || !digitalRead(PORT_BUTTON2)) {
			lcd_clr(fd, -1);
			lcd_puts (fd, 0, 0, "Reconfigure    ");
			lcd_puts (fd, 0, 1, "  Label Printer");
			if (usblp_reconfig()) {
				lcd_clr(fd, -1);
				lcd_puts (fd, 0, 0, "Label Printer  ");
				lcd_puts (fd, 0, 1, "Setup complete ");
			} else {
				lcd_clr(fd, -1);
				lcd_puts (fd, 0, 0, "Can't found    ");
				lcd_puts (fd, 0, 1, "  Label Printer");
			}
			sleep(OPT_DISPLAY_DELAY);
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
