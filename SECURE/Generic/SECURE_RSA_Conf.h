/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSecure * Digital signature toolkit                         *
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
*       emSecure version: V2.48.0                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SECURE_RSA_Conf.h
Purpose     : Configurable emSecure-RSA preprocessor configuration

*/

#ifndef SECURE_RSA_CONF_H
#define SECURE_RSA_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       SECURE_RSA_MAX_KEY_LENGTH
*
*  Description
*    Maximum key length. Default: 2048
*
*    Define the maximum used key length to optimize ROM and Stack usage.
*    Longer keys require more memory.
*/
#define SECURE_RSA_MAX_KEY_LENGTH                     (2048)

#endif

/*************************** End of file ****************************/
