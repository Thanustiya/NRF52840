/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2021    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSH * Embedded Secure Shell                                *
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

File        : SSH_Conf.h
Purpose     : Configuration file for configurable defines in SSH module

*/

#ifndef SSH_CONF_H
#define SSH_CONF_H

//
// The ESPRO library has been built with following defines set
//
#ifndef SSH_OS_DO_NOT_INLINE_CALLS
  #define SSH_OS_DO_NOT_INLINE_CALLS  1
#endif

//
// Define SSH_DEBUG: Debug level for SSH product
//                  0: No checks                      (Smallest and fastest code)
//                  1: Warnings & Panic checks
//                  2: Warnings, logs, & panic checks (Seriously bigger code)
//
#if defined(DEBUG) && DEBUG
  #ifndef   SSH_DEBUG
    #define SSH_DEBUG      2      // Default for debug builds
  #endif
#else
  #ifndef   SSH_DEBUG
    #define SSH_DEBUG      0      // Default for release builds
  #endif
#endif

//
// Inline OS function calls for higher performance in release builds
//
#if !defined(SSH_OS_DO_NOT_INLINE_CALLS) && !defined(WIN32) && !defined(__linux__)
  #if SSH_DEBUG > 0
    #include "RTOS.h"
    extern OS_RSEMA SSH_OS_RSema;
    #define SSH_OS_DISABLE_INTERRUPT()   OS_DI()
    #define SSH_OS_ENABLE_INTERRUPT()    OS_RestoreI()
    #define SSH_OS_GET_TIME()            OS_GetTime32()
    #define SSH_OS_LOCK()                OS_Use(&SSH_OS_RSema)
    #define SSH_OS_UNLOCK()              OS_Unuse(&SSH_OS_RSema)
  #endif
#endif

#ifndef SSH_SCP_CONFIG_VIRTUAL_ROOT
  #ifdef WIN32
    #define SSH_SCP_CONFIG_VIRTUAL_ROOT   "C:/SEGGER"
  #endif
#endif

#ifndef   SSH_SNPRINTF
  #define SSH_SNPRINTF                    SEGGER_snprintf
#endif

#ifndef   SSH_VSNPRINTF
  #define SSH_VSNPRINTF                   SEGGER_vsnprintf
#endif

#endif

/*************************** End of file ****************************/
