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

File        : CRYPTO_X_Config_SECURE_RSA_CM.c
Purpose     : Configure CRYPTO for emSecure-RSA on Cortex-M devices
              and a dummy, insecure, random number generator.

Additional information:
  The dummy random number generator does not generate secure random 
  numbers, but can be run on any hardware with memory at 0x20000000.
  To provide secure random numbers modify it according to the hardware
  capabilities.

  Random number generators for different hardware is available from 
  SEGGER upon request.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CRYPTO.h"

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _RNG_Get()
*
*  Function description
*    Get random data from RNG.
*
*  Parameters
*    pData   - Pointer to the object that receives the random data.
*    DataLen - Octet length of the random data.
*/
static void _RNG_Get(U8 *pData, unsigned DataLen) {
  if (pData && DataLen) {
    while (DataLen--) {
      *pData++ = *((volatile U8*)0x20000000 + DataLen);
    }
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       CRYPTO_X_Panic()
*
*  Function description
*    Hang when something unexpected happens.
*/
void CRYPTO_X_Panic(void) {
  for (;;) {
    /* Hang */
  }
}

/*********************************************************************
*
*       CRYPTO_X_Config()
*
*  Function description
*    Configure hardware assist for CRYPTO component.
*/
void CRYPTO_X_Config(void) {
  static const CRYPTO_RNG_API _RNG = {
    NULL,
    _RNG_Get,
    NULL,
    NULL
  };
  //
  // Install RNG using insecure random data.  The RNG is only
  // required if you are signing: for verification, the RNG
  // is not required and need not be installed!
  //
  CRYPTO_RNG_InstallEx(&_RNG, NULL);
  //
  // Install modular exponentiation functions.
  //
  CRYPTO_MPI_SetPublicModExp (CRYPTO_MPI_ModExp_Basic_Fast);
  CRYPTO_MPI_SetPrivateModExp(CRYPTO_MPI_ModExp_Basic_Fast);
}

/*************************** End of file ****************************/
