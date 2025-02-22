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
#ifndef __LIB_USBLP_H__
#define __LIB_USBLP_H__

//------------------------------------------------------------------------------
extern int  usblp_print_err (const char *msg1, const char *msg2, const char *msg3, int ch);
extern int  usblp_print_mac (char *msg, int ch);
extern int  usblp_config    (void);

#endif  //  #define __USBLP_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
