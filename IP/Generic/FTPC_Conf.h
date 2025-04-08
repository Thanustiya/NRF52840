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
File    : FTPC_Conf.h
Purpose : FTP client add-on configuration file
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FTPS_CONF_H_
#define _FTPS_CONF_H_ 1

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
// Selection to our best knowledge.
//
#if defined(__linux__)
  #include <stdio.h>
  #define FTPC_FUNC_WARN(p)  printf p ; \
                             printf("\n");
  #define FTPC_FUNC_LOG(p)   printf p ; \
                             printf("\n");
#elif defined(WIN32)
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
  #define FTPC_FUNC_WARN(p)  WIN32_OutputDebugStringf p
  #define FTPC_FUNC_LOG(p)   WIN32_OutputDebugStringf p
#elif (defined(__ICCARM__) || defined(__ICCRX__) || defined(__GNUC__) || defined(__SEGGER_CC__))
  //
  // - IAR ARM
  // - IAR RX
  // - GCC based
  // - SEGGER
  //
  #include "IP.h"
  #define FTPC_FUNC_WARN(p)  IP_Warnf_Application p
  #define FTPC_FUNC_LOG(p)   IP_Logf_Application  p
#else
  //
  // Other toolchains
  //
  #define FTPC_FUNC_WARN(p)
  #define FTPC_FUNC_LOG(p)
#endif

//
// Final selection that can be overridden.
//
#ifndef       FTPC_WARN
  #if (DEBUG != 0)
    //
    // Debug builds
    //
    #define   FTPC_WARN(p)      FTPC_FUNC_WARN(p)
  #else
    //
    // Release builds
    //
    #define   FTPC_WARN(p)
  #endif
#endif

#ifndef       FTPC_LOG
  #if (DEBUG != 0)
    //
    // Debug builds
    //
    #define   FTPC_LOG(p)       FTPC_FUNC_LOG(p)
  #else
    //
    // Release builds
    //
    #define   FTPC_LOG(p)
  #endif
#endif

#ifndef       FTPC_APP_WARN
  #define     FTPC_APP_WARN(p)  FTPC_FUNC_WARN(p)
#endif

#ifndef       FTPC_APP_LOG
  #define     FTPC_APP_LOG(p)   FTPC_FUNC_LOG(p)
#endif

#define FTPC_BUFFER_SIZE               512
#define FTPC_CTRL_BUFFER_SIZE          256
#define FTPC_SERVER_REPLY_BUFFER_SIZE  128  // Only required in debug builds with enabled logging.

#endif     // Avoid multiple inclusion

/*************************** End of file ****************************/
