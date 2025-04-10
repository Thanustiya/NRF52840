/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2007 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet * TCP/IP stack for embedded applications               *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product for in-house use.         *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet version: V3.60.0                                       *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : WEBS_Conf.h
Purpose : Webserver add-on configuration file
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _WEBS_CONF_H_
#define _WEBS_CONF_H_ 1

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   DEBUG
  #define DEBUG  0
#endif

//
// Logging
//
#if (DEBUG != 0)
  #if defined(__linux__) || defined(__APPLE__)
    #include <stdio.h>
    #define WEBS_WARN(p)  printf p ; \
                          printf("\n");
    #define WEBS_LOG(p)   printf p ; \
                          printf("\n");
  #elif defined(_WIN32)
    //
    // - Microsoft Visual Studio
    //
    #ifdef __cplusplus
    extern "C" {     /* Make sure we have C-declarations in C++ programs */
    #endif
    void WIN32_OutputDebugStringf(const char * sFormat, ...);
    #ifdef __cplusplus
    }
    #endif
    #define WEBS_WARN(p)  WIN32_OutputDebugStringf p
    #define WEBS_LOG(p)   WIN32_OutputDebugStringf p
  #elif defined(USB_WEBSERVER)
    //
    // - Webserver using emUSB instead of an IP stack
    //
    #include "USB.h"
    #define WEBS_WARN(p)  USBD_Logf_Application p
    #define WEBS_LOG(p)   USBD_Logf_Application p
  #elif   (defined(__ICCARM__) || defined(__ICCRX__) || defined(__GNUC__) || defined(__SEGGER_CC__))
    //
    // - IAR ARM
    // - IAR RX
    // - GCC based
    // - SEGGER
    //
    #include "IP.h"
    #define WEBS_WARN(p)  IP_Warnf_Application p
    #define WEBS_LOG(p)   IP_Logf_Application  p
  #else
    //
    // Other toolchains
    //
    #define WEBS_WARN(p)
    #define WEBS_LOG(p)
  #endif
#else
  //
  // Release builds
  //
  #define   WEBS_WARN(p)
  #define   WEBS_LOG(p)
#endif

//
// Webserver buffer sizes
//
#define WEBS_IN_BUFFER_SIZE              256  // Buffer size should be at least 256 bytes.
#define WEBS_OUT_BUFFER_SIZE            1460  // Buffer size can be decreased. To small buffers will result in a
                                              // lack of performance and decreases the quality of service.
                                              // Default is max. MTU(1500 bytes) - best case IPv4/TCP headers (40 bytes).
#define WEBS_PARA_BUFFER_SIZE            256  // Required for dynamic content parameter handling.
#define WEBS_FILENAME_BUFFER_SIZE         64

//
// Maximum allowed root path length of the Web server
// in multiples of a CPU native unit (typically int).
// If the root path of the Web server is the root of
// your media you can comment out this define or set
// it to zero.
// Example:
//   For the root path "/httpdocs" the define needs
//   to be set to at least 9 . As this is not a multiple
//   of an int, set it to 12.
//
#define WEBS_MAX_ROOT_PATH_LEN  12

//
// Switch to select if the upload code will be linked in.
//
#ifndef   WEBS_SUPPORT_UPLOAD
  #define WEBS_SUPPORT_UPLOAD  0
#endif

//
// Switch to select if standard plain text basic authentication will be
// used for protected paths or MD5 encrypted digest authentication.
//
#ifndef   WEBS_USE_AUTH_DIGEST
  #define WEBS_USE_AUTH_DIGEST  0
#endif

//
// Switches to select if the bigger samples should be included in the read-only file system
//
#ifndef   INCLUDE_SHARE_SAMPLE
  #define INCLUDE_SHARE_SAMPLE            0
#endif

#ifndef   INCLUDE_PRESENTATION_SAMPLE
  #define INCLUDE_PRESENTATION_SAMPLE     0
#endif

//
// Switch to activate the IP configuration sample (DHCP or Manual)
//
#ifndef     INCLUDE_IP_CONFIG_SAMPLE
  #if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    #define INCLUDE_IP_CONFIG_SAMPLE      0
  #else
    #define INCLUDE_IP_CONFIG_SAMPLE      1
  #endif
#endif

//
// Turn on templating support? It is off by default.
//
#ifndef WEBS_ENABLE_TEMPLATING
  #define WEBS_ENABLE_TEMPLATING          0
#endif

//
// Switches the sample that should be used
//
#ifndef   WEBS_USE_SAMPLE_2018
  #define WEBS_USE_SAMPLE_2018            0
#endif

#ifndef   WEBS_USE_SAMPLE_TEMPLATING
  #define WEBS_USE_SAMPLE_TEMPLATING      0
#endif


#endif     // Avoid multiple inclusion

/*************************** End of file ****************************/
