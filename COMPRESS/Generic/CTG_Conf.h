/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2017 - 2024     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emCompress-ToGo * Compression library                        *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product.                          *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emCompress-ToGo version: V3.40.2                             *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : COTG_Conf.h
Purpose     : Configuration settings for emCompress-OTG.

*/

#ifndef COTG_CONF_H
#define COTG_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG
  #define DEBUG 0
#endif

#if DEBUG
  #ifndef   COTG_DEBUG
    #define COTG_DEBUG      1      // Default for debug builds
  #endif
#else
  #ifndef   COTG_DEBUG
    #define COTG_DEBUG      0      // Default for release builds
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
