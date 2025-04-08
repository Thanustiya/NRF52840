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
*       emSSL * Embedded Transport Layer Security                    *
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
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------
File        : CRYPTO_X_Config_SSL_CM.c
Purpose     : Configure CRYPTO for full SSL with no hardware
              accelerators and a dummy, insecure, random number
              generator.

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
*       Local functions
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
*    Configure no hardware assist for CRYPTO component.
*/
void CRYPTO_X_Config(void) {
  //
  static const CRYPTO_RNG_API _RNG = {
    NULL,
    _RNG_Get,
    NULL,
    NULL
  };
  //
  // Install pure software implementations.
  //
  CRYPTO_MD5_Install      (&CRYPTO_HASH_MD5_SW,        NULL);
  CRYPTO_SHA1_Install     (&CRYPTO_HASH_SHA1_SW,       NULL);
  CRYPTO_SHA224_Install   (&CRYPTO_HASH_SHA224_SW,     NULL);
  CRYPTO_SHA256_Install   (&CRYPTO_HASH_SHA256_SW,     NULL);
  CRYPTO_SHA512_Install   (&CRYPTO_HASH_SHA512_SW,     NULL);
  CRYPTO_AES_Install      (&CRYPTO_CIPHER_AES_SW,      NULL);
  CRYPTO_TDES_Install     (&CRYPTO_CIPHER_TDES_SW,     NULL);
  CRYPTO_ARIA_Install     (&CRYPTO_CIPHER_ARIA_SW,     NULL);
  CRYPTO_SEED_Install     (&CRYPTO_CIPHER_SEED_SW,     NULL);
  CRYPTO_CAMELLIA_Install (&CRYPTO_CIPHER_CAMELLIA_SW, NULL);
  //
  // Install RNG using Hash_DRBG-SHA256 with "random" data from
  // RAM.
  //
  CRYPTO_RNG_InstallEx(&CRYPTO_RNG_DRBG_HASH_SHA256, &_RNG);
  //
  // Install small modular exponentiation functions.
  //
  CRYPTO_MPI_SetPublicModExp (CRYPTO_MPI_ModExp_Basic_Fast);
  CRYPTO_MPI_SetPrivateModExp(CRYPTO_MPI_ModExp_Basic_Fast);
}

/*************************** End of file ****************************/
