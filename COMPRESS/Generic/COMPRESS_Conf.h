/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2018  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       emCompress-Embed * Compression library                       *
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
*       emCompress-Embed version: V2.14                              *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : COMPRESS_Conf.h
Purpose     : Configuration settings for emCompress.
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef COMPRESS_CONF_H
#define COMPRESS_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG
  #define DEBUG 0
#endif

#if DEBUG
  #ifndef   COMPRES_DEBUG
    #define COMPRES_DEBUG      1      // Default for debug builds
  #endif
#else
  #ifndef   COMPRES_DEBUG
    #define COMPRES_DEBUG      0      // Default for release builds
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif

/****** End Of File *************************************************/
