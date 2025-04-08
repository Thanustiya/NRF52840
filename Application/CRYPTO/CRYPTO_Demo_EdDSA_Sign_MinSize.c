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

File        : CRYPTO_Demo_EdDSA_Sign_MinSize.c
Purpose     : Minimum data size required for Ed25519 signing.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "CRYPTO.h"
#include "SEGGER_MEM.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define MAX_CHUNKS             20

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define CRYPTO_ASSERT(X)               { if (!(X)) { CRYPTO_PANIC(); } }  // I know this is low-rent
#define CRYPTO_CHECK(X)                /*lint -e{717,801,9036} */ do { if ((Status = (X)) < 0) goto Finally; } while (0)

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*256)+3];  // +3 as one of the EdDSA divisors is not normalized

typedef struct {
  MPI_UNIT                 aUnits[MAX_CHUNKS];
  SEGGER_MEM_CONTEXT       MemContext;
  SEGGER_MEM_CHUNK_HEAP    Heap;
  CRYPTO_EdDSA_PRIVATE_KEY Private;
  CRYPTO_EdDSA_PUBLIC_KEY  Public;
  CRYPTO_EdDSA_SIGNATURE   Signature;
} GLOBALS;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static GLOBALS _Globals;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Ed25519_Sign()
*
*  Function description
*    Benchmark Ed25519 sign.
*/
static int _Ed25519_Sign(void) {
  U8  aMsg[] = { "SEGGER: It simply works!" };
  int Status;
  //
  CRYPTO_EdDSA_InitPrivateKey(&_Globals.Private,   &_Globals.MemContext);
  CRYPTO_EdDSA_InitPublicKey (&_Globals.Public,    &_Globals.MemContext);
  CRYPTO_EdDSA_InitSignature (&_Globals.Signature, &_Globals.MemContext);
  //
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_GenKeys(&_Globals.Private, &_Globals.Public, &_Globals.MemContext));
  CRYPTO_EdDSA_KillPublicKey (&_Globals.Public);
  //
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_Sign(&_Globals.Private, &aMsg[0], sizeof(aMsg), &_Globals.Signature, &_Globals.MemContext));
  //
Finally:
  CRYPTO_EdDSA_KillPrivateKey(&_Globals.Private);
  CRYPTO_EdDSA_KillPublicKey (&_Globals.Public);
  CRYPTO_EdDSA_KillSignature (&_Globals.Signature);
  //
  return Status;
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
*    Main entry point for application to run all the tests.
*/
void MainTask(void);
void MainTask(void) {
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_CHUNK_HEAP_Init(&_Globals.MemContext, &_Globals.Heap, _Globals.aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  //
  if (_Ed25519_Sign() >= 0) {
    SEGGER_SYS_IO_Printf("Global data required = %u bytes\n", sizeof(_Globals));
    SEGGER_SYS_IO_Printf("Signed without error");
  } else {
    SEGGER_SYS_IO_Printf("Exception");
  }
  //
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
