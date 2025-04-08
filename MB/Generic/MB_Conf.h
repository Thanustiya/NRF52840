/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2013 - 2018  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       emModbus * Modbus stack for embedded applications            *
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
----------------------------------------------------------------------
File    : MB_Conf.h
Purpose : Configuration settings for Modbus
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef _MB_CONF_H_
#define _MB_CONF_H_  1

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

//
// The ESPRO library has been built with following defines set
//
#ifdef MB_OS_DO_NOT_INLINE_CALLS
  #define MB_OS_DO_NOT_INLINE_CALLS  1
#endif

//
// Define MB_DEBUG: Debug level for Modbus stack
//                  0: No checks, no statistics      (Smallest and fastest code)
//                  1: Statistics & "Panic" checks
//                  2: Statistics, warn, log, panic  (Seriously bigger code)
//
#ifndef DEBUG
  #define DEBUG  0
#endif

#if DEBUG
  #ifndef   MB_DEBUG
    #define MB_DEBUG  2  // Default for debug builds
  #endif
#else
  #ifndef   MB_DEBUG
    #define MB_DEBUG  0  // Default for release builds
  #endif
#endif

#ifndef   USE_RTT
  #define USE_RTT     0
#endif

//
// Inline OS function calls for higher performance in release builds.
//
#ifndef MB_OS_DO_NOT_INLINE_CALLS
  #if (!MB_DEBUG && !defined(WIN32))
    #include "RTOS.h"
    #define MB_OS_DISABLE_INTERRUPT()  OS_DI()
    #define MB_OS_ENABLE_INTERRUPT()   OS_RestoreI()
    #define MB_OS_GET_TIME()           OS_GetTime32()
  #endif
#endif

//
// IAR ARM compiler related macros
//
#ifdef __ICCARM__
  #if ((__TID__ >> 4) & 0x0F) < 6                                                      // For any ARM CPU core < v7, we will use optimized routines.
    #include "SEGGER.h"
    #define MB_MEMCPY(pDest, pSrc, NumBytes) SEGGER_ARM_memcpy(pDest, pSrc, NumBytes)  // Speed optimization: Our memcpy is much faster!
  #endif
  #ifndef   MB_IS_BIG_ENDIAN
    #define MB_IS_BIG_ENDIAN  (1 - __LITTLE_ENDIAN__)
  #endif
#endif

//
// GCC related macros
//
#ifdef __GNUC__
  #if defined (__ARM_ARCH_4T__) || defined (__ARM_ARCH_5T__)|| defined (__ARM_ARCH_5TE__)  // For any ARM CPU core < v7, we will use optimized routines.
    #include "SEGGER.h"
    #define MB_MEMCPY(pDest, pSrc, NumBytes)  SEGGER_ARM_memcpy(pDest, pSrc, NumBytes)     // Speed optimization: Our memcpy is much faster!
  #endif
#endif

//
// Default value is little endian
//
#ifndef   MB_IS_BIG_ENDIAN
  #define MB_IS_BIG_ENDIAN  0
#endif


#ifdef __cplusplus
  }
#endif

#endif     // Avoid multiple inclusion

/*************************** End of file ****************************/
