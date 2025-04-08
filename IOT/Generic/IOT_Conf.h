/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2019  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       IoT Toolkit * HTTP Client and JSON Parser                    *
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
-------------------------- END-OF-HEADER -----------------------------

File        : IOT_Conf.h
Purpose     : Configuration file for configurable defines in IoT module

*/

#ifndef IOT_CONF_H
#define IOT_CONF_H

//
// The ESPRO library has been built with following defines set
//
#ifndef IOT_OS_DO_NOT_INLINE_CALLS
  #define IOT_OS_DO_NOT_INLINE_CALLS  1
#endif

//
// Define IOT_DEBUG: Debug level for IoT product
//                  0: No checks                      (Smallest and fastest code)
//                  1: Warnings & Panic checks
//                  2: Warnings, logs, & panic checks (Seriously bigger code)
//
#if defined(DEBUG) && DEBUG
  #ifndef   IOT_DEBUG
    #define IOT_DEBUG      2      // Default for debug builds
  #endif
#else
  #ifndef   IOT_DEBUG
    #define IOT_DEBUG      0      // Default for release builds
  #endif
#endif

//
// Inline OS function calls for higher performance in release builds
//
#if !defined(IOT_OS_DO_NOT_INLINE_CALLS) && !defined(WIN32) && !defined(__linux__)
  #if IOT_DEBUG > 0
    #include "RTOS.h"
    extern OS_RSEMA IOT_OS_RSema;
    #define IOT_OS_DISABLE_INTERRUPT()   OS_DI()
    #define IOT_OS_ENABLE_INTERRUPT()    OS_RestoreI()
    #define IOT_OS_GET_TIME()            OS_GetTime32()
    #define IOT_OS_LOCK()                OS_Use(&IOT_OS_RSema)
    #define IOT_OS_UNLOCK()              OS_Unuse(&IOT_OS_RSema)
  #endif
#endif

#endif

//
// Compatibility macros.
//
#ifndef     IOT_STRTOULL
  #ifdef _WIN32
  #if _MSC_VER < 1800  // strtoull() is only supported by Visual Studio 2013 and above.
    #define IOT_STRTOULL  _strtoui64
  #endif
  #endif
#endif

/*************************** End of file ****************************/
