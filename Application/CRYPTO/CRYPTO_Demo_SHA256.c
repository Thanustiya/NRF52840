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
*       emCrypt * Cryptographic algorithm library                    *
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
*       emCrypt version: V2.44.1                                     *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File        : CRYPTO_Demo_SHA256.c
Purpose     : Demonstrate how to use a hash function, in this case
              SHA-256.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "CRYPTO.h"
#include <stdio.h>

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const U8 _aMessage[] = {
  "abc"
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

static void _PrintDigest(const U8 *pDigest, unsigned DigestLen) {
  unsigned i;
  //
  printf("Digest: ");
  for (i = 0; i < DigestLen; ++i) {
    printf("%02X", pDigest[i]);
  }
  printf("\n");
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Main entry point for application.
*/
void MainTask(void);
void MainTask(void) {
  CRYPTO_SHA256_CONTEXT C;
  U8                    aDigest[CRYPTO_SHA256_DIGEST_BYTE_COUNT];
  unsigned              i;
  //
  CRYPTO_Init();
  //
  CRYPTO_SHA256_Calc(aDigest, sizeof(aDigest), _aMessage, sizeof(_aMessage));
  _PrintDigest(aDigest, sizeof(aDigest));
  //
  CRYPTO_SHA256_Init (&C);
  CRYPTO_SHA256_Add  (&C, _aMessage, sizeof(_aMessage));
  CRYPTO_SHA256_Final(&C, aDigest, sizeof(aDigest));
  _PrintDigest(aDigest, sizeof(aDigest));
  //
  CRYPTO_SHA256_Init (&C);
  for (i = 0; i < sizeof(_aMessage); ++i) {
    CRYPTO_SHA256_Add(&C, &_aMessage[i], 1);
  }
  CRYPTO_SHA256_Final(&C, aDigest, sizeof(aDigest));
  _PrintDigest(aDigest, sizeof(aDigest));
}

/*************************** End of file ****************************/
