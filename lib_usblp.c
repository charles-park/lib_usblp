//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 * @file lib_usblp.c
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>

#include "lib_usblp.h"

//------------------------------------------------------------------------------
// printer library path & filename
//------------------------------------------------------------------------------
const char *F_LPR      = "/usr/bin/lpr";
const char *F_LPSTAT   = "/usr/bin/lpstat";
const char *F_LPADMIN  = "/usr/sbin/lpadmin";
const char *F_LPINFO   = "/usr/sbin/lpinfo";

//------------------------------------------------------------------------------
#define TEXT_WIDTH  80

const char *EPL_FORM_START = "I8,0,001\nQ78,16\nq240\nrN\nS4\nD15\nZB\nJF\nO\nR304,10\nf100\nN\n";
/*
    "< forum.odroid.com" or "forum.odroid.com >"
    00:1E:06:xx:xx:xx
*/
const char *EPL_FORM_MAC_L = "A%d,0,0,2,1,1,N,\"< forum.odroid.com\"\nA%d,32,0,2,1,1,N,\"%s\"\n";
const char *EPL_FORM_MAC_R = "A%d,0,0,2,1,1,N,\"forum.odroid.com >\"\nA%d,32,0,2,1,1,N,\"%s\"\n";

/* %d = 20, 40, 60, (11 * i) * 2 */
const char *EPL_FORM_ERR   = "A%d,%d,0,2,1,1,N,\"%c%s\"\n";

const char *EPL_FORM_ERR_1 = "A%d,00,0,2,1,1,N,\"%c%s\"\n";
const char *EPL_FORM_ERR_2 = "A%d,22,0,2,1,1,N,\"%c%s\"\n";
const char *EPL_FORM_ERR_3 = "A%d,44,0,2,1,1,N,\"%c%s\"\n";
/* Page end */
const char *EPL_FORM_END   = "P1\n";

//------------------------------------------------------------------------------
const char *ZPL_FORM_START = "^XA\n^CFC\n^LH0,0\n";

/*
    "< forum.odroid.com" or "forum.odroid.com >"
    00:1E:06:xx:xx:xx
*/
const char *ZPL_FORM_MAC_L = "^FO310,25\n^FD< forum.odroid.com^FS\n^FO316,55\n^FD%s^FS\n";
const char *ZPL_FORM_MAC_R = "^FO310,25\n^FDforum.odroid.com >^FS\n^FO316,55\n^FD%s^FS\n";

/* %d = 20, 40, 60, (i * 2) * 10 + 20 */
const char *ZPL_FORM_ERR   = "^FO304,%d\n^FD%s^FS\n";

const char *ZPL_FORM_ERR_1 = "^FO304,20\n^FD%c%s^FS\n";
const char *ZPL_FORM_ERR_2 = "^FO304,40\n^FD%c%s^FS\n";
const char *ZPL_FORM_ERR_3 = "^FO304,60\n^FD%c%s^FS\n";

/* Page end */
const char *ZPL_FORM_END   = "^XZ\n";

const char *ZPL_USB_INIT   = "^XA^JUF^XZ\n";

//------------------------------------------------------------------------------
// EPL Protocol Pos
// MAC ADDR : [A0,0 A6,0 (ZD230D)], [A6,0, A16,0 (GC420D)]
// ERR MSG  : [A0,0 A0,22 A0,44 (ZD230D)], [A4,0 A4,22 A4,44 (GC420D)]
//------------------------------------------------------------------------------
#define USE_PROTOCOL_EPL

static int NLP_MODEL = 0;   /* 0 : GC420D, 1 : ZD230D */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// check the usb label printer connection.
//------------------------------------------------------------------------------
static int check_usblp_connection (void)
{
    FILE *fp;
    char cmd_line[1024];

    memset (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lsusb | grep Zebra 2<&1");

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
            if (strstr (cmd_line, "Zebra") != NULL) {
                pclose (fp);
                return 1;
            }
            memset (cmd_line, 0x00, sizeof(cmd_line));
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
// get the usb label printer info.
//------------------------------------------------------------------------------
static int get_usblp_device (char *lpname)
{
    FILE *fp;
    char cmd_line[1024], *ptr;

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lpinfo -v | grep usb 2<&1");

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
            if ((ptr = strstr (cmd_line, "usb:")) != NULL) {
                strncpy (lpname, ptr, strlen(ptr));
                pclose (fp);
                return 1;
            }
            memset (cmd_line, 0x00, sizeof(cmd_line));
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
// clr the usb label printer info.
//------------------------------------------------------------------------------
static int clr_usblp_device (void)
{
    FILE *fp = NULL;
    char cmd_line[1024];

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "lpadmin -x %s 2<&1", "zebra");

    if ((fp = popen(cmd_line, "w")) != NULL) {
        pclose(fp);
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
// set the usb label printer info.
//------------------------------------------------------------------------------
static int set_usblp_device (char *lpname)
{
    FILE *fp;
    char cmd_line[1024], s_lpname[512];

    memset (s_lpname, 0x00, sizeof(s_lpname));
    {
        int i, pos;
        for (i = 0, pos = 0; i < (int)strlen(lpname); i++, pos++) {
            if ((lpname[i] == '(') || (lpname[i] == ')'))
                s_lpname [pos++] = '\\';
            s_lpname [pos] = lpname[i];
        }
    }

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "lpadmin -p zebra -E -v %s 2<&1", s_lpname);

    if ((fp = popen(cmd_line, "w")) != NULL) {
        pclose(fp);
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
// confirm the usb label printer info.
//------------------------------------------------------------------------------
static int confirm_usblp_device (char *lpname)
{
    FILE *fp;
    char cmd_line[1024], *ptr;

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lpstat -v | grep usb 2<&1");

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
            if ((ptr = strstr (cmd_line, "usb:")) != NULL) {
                if (!strncmp (lpname, ptr, strlen(ptr)-1)) {
                    pclose (fp);
                    return 1;
                }
            }
            memset (cmd_line, 0x00, sizeof(cmd_line));
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
int usblp_print_mac (char *msg, int ch)
{
    FILE *fp = fopen ("usblp_mac.txt", "w");
    char cmd_line [1024];

    if (fp == NULL) {
        fprintf (stdout, "%s : couuld not create file for usblp test. ", __func__);
        return 0;
    }
#if defined (USE_PROTOCOL_EPL)
    fputs  (EPL_FORM_START, fp);
    memset (cmd_line, 0, sizeof(cmd_line));
    if (ch) sprintf(cmd_line, EPL_FORM_MAC_R, NLP_MODEL ? 0:10, NLP_MODEL ? 6:16, msg);
    else    sprintf(cmd_line, EPL_FORM_MAC_L, NLP_MODEL ? 0:10, NLP_MODEL ? 6:16, msg);
    fputs  (cmd_line, fp);
    fputs  (EPL_FORM_END, fp);
    fclose (fp);
#else
    fputs  (ZPL_FORM_START, fp);
    memset (cmd_line, 0, sizeof(cmd_line));
    if (ch) sprintf(cmd_line, ZPL_FORM_MAC_R, msg);
    else    sprintf(cmd_line, ZPL_FORM_MAC_L, msg);
    fputs  (cmd_line, fp);
    fputs  (ZPL_FORM_END, fp);
    fclose (fp);
#endif

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lpr usblp_mac.txt -P zebra 2<&1");

    if ((fp = popen(cmd_line, "w")) != NULL) {
        pclose(fp);
    }
    return 1;
}

//------------------------------------------------------------------------------
int usblp_print_err (const char *msg1, const char *msg2, const char *msg3, int ch)
{
    FILE *fp = fopen ("usblp_err.txt", "w");
    char cmd_line [1024];

    if (fp == NULL) {
        fprintf (stdout, "%s : couuld not create file for usblp test. ", __func__);
        return 0;
    }

#if defined (USE_PROTOCOL_EPL)
    fputs  (EPL_FORM_START, fp);
    memset (cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, EPL_FORM_ERR_1, ch ? '>' : '<', NLP_MODEL ? 0:4, msg1);
    fputs  (cmd_line, fp);
    sprintf(cmd_line, EPL_FORM_ERR_2, ch ? '>' : '<', NLP_MODEL ? 0:4, msg2);
    fputs  (cmd_line, fp);
    sprintf(cmd_line, EPL_FORM_ERR_3, ch ? '>' : '<', NLP_MODEL ? 0:4, msg3);
    fputs  (cmd_line, fp);
    fputs  (EPL_FORM_END, fp);
    fclose (fp);
#else
    fputs  (ZPL_FORM_START, fp);
    memset (cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, ZPL_FORM_ERR_1, ch ? '>' : '<', msg1);
    fputs  (cmd_line, fp);
    sprintf(cmd_line, ZPL_FORM_ERR_2, ch ? '>' : '<', msg2);
    fputs  (cmd_line, fp);
    sprintf(cmd_line, ZPL_FORM_ERR_3, ch ? '>' : '<', msg3);
    fputs  (cmd_line, fp);
    fputs  (ZPL_FORM_END, fp);
    fclose (fp);
#endif

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lpr usblp_err.txt -P zebra 2<&1");

    if ((fp = popen(cmd_line, "w")) != NULL) {
        pclose(fp);
    }
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int usblp_zpl_init (void)
{
    FILE *fp = fopen ("usblp_init.txt", "w");
    char cmd_line [1024];

    if (fp == NULL) {
        fprintf (stdout, "%s : couuld not create file for usblp test. ", __func__);
        return 0;
    }

    fputs  (ZPL_USB_INIT, fp);
    fclose (fp);

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "%s", "lpr usblp_err.txt -P zebra 2<&1");

    if ((fp = popen(cmd_line, "w")) != NULL) {
        pclose(fp);
    }
    return 1;
}

//------------------------------------------------------------------------------
int get_usblp_ptype (void); /* epl, zpl */

//------------------------------------------------------------------------------
int usblp_config (void)
{
    char usblp_device[512];

    /*
        ubuntu printer library(lpr, lpadmin, lpstat) check.
    */
    if ( access (F_LPR,     F_OK) || access (F_LPSTAT,  F_OK) ||
         access (F_LPADMIN, F_OK) || access (F_LPINFO,  F_OK) ) {
        fprintf (stderr, "Error : printer control file not found(lpr, lpstat, lpadmin, lpinfo)\n");
        fprintf (stderr, "Need package : cups cups-bsd\n");
        return 0;
     }

    memset (usblp_device, 0x00, sizeof(usblp_device));
    if ( !check_usblp_connection () ) {
        fprintf (stderr, "Error : Zebra USB Label Printer not found\n");
        return 0;
    }

    if ( !get_usblp_device (usblp_device) ) {
        fprintf (stderr, "Error : Unable to get usblp infomation.\n");
        return 0;
    }

    /* Clears the current configuration if it is already configured for nlp. */
    if ( confirm_usblp_device (usblp_device) )
        fprintf (stderr, "usblp device removed. ret = %s\n", clr_usblp_device() ? "success" : "fail");

    if ( !set_usblp_device (usblp_device) ) {
        fprintf (stderr, "Error : Failed to configure usblp.\n");
        return 0;
    }

    if ( !confirm_usblp_device (usblp_device) ) {
        fprintf (stderr, "Error : The usblp settings have not been changed.\n");
        return 0;
    }

    fprintf (stdout, "*** USB Label Printer setup is complete. ***\n");
    fprintf (stdout, "*** Printer Device Name : %s\n", usblp_device);
    NLP_MODEL = (strstr (usblp_device, "ZPL") != NULL) ? 1 : 0;
    fprintf (stdout, "*** Printer Device Type : %s\n",
                            NLP_MODEL ? "ZPL(ZD230D)" : "EPL(GC420D)");

#if !defined (USE_PROTOCOL_EPL)
    usblp_zpl_init ();
#endif
    return 1;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
