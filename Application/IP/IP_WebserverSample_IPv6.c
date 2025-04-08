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
-------------------------- END-OF-HEADER -----------------------------

Purpose : Demonstrates starting the emWeb server with IPv6 only.

Additional information:
  For further details about the sample itself and its configuration
  parameters, please refer to the main sample included by this wrapper.
*/

/*********************************************************************
*
*       Main sample configuration and include
*
**********************************************************************
*/

//
// Include and configure the webserver main sample.
//
#define APP_ENABLE_IPV4_PLAIN  (0)
#include "IP_WebserverSample.c"
#undef  APP_ENABLE_IPV4_PLAIN

/*************************** End of file ****************************/
