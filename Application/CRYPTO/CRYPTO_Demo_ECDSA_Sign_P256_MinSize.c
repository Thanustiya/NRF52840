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

File        : CRYPTO_Demo_ECDSA_Sign_P256_MinSize.c
Purpose     : Minimum data size required for ECDSA signing using P-256.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SEGGER.h"
#include "SEGGER_MEM.h"
#include "SEGGER_SYS.h"
#include "CRYPTO.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// Number of bits in the key; in this case we use P-256.
//
#define CRYPTO_ECDSA_MAX_KEY_LENGTH  256

//
// Number of limbs to hold an MPI (double-length plus overhead to
// hold product for division) and total number of simultaneous MPIs
// needed when signing.
//
#define CRYPTO_ECDSA_2LIMB_ALLOC        (CRYPTO_MPI_LIMBS_REQUIRED(2*CRYPTO_ECDSA_MAX_KEY_LENGTH) + 2)
#define CRYPTO_ECDSA_SIGN_2LIMB_REQD    15

//
// Handle exceptions.
//
#define CRYPTO_ASSERT(X)               { if (!(X)) { CRYPTO_PANIC(); } }  // I know this is low-rent
#define CRYPTO_CHECK(X)                /*lint -e{717,801,9036} */ do { if ((Status = (X)) < 0) goto Finally; } while (0)

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  SEGGER_MEM_CHUNK_HEAP    Heap;
  SEGGER_MEM_CONTEXT       Context;
  CRYPTO_MPI_LIMB          aWorkspace[CRYPTO_ECDSA_SIGN_2LIMB_REQD][CRYPTO_ECDSA_2LIMB_ALLOC];
  CRYPTO_ECDSA_SIGNATURE   Signature;
  U8                       aSignature[2*CRYPTO_ECDSA_MAX_KEY_LENGTH/8];
} GLOBAL_DATA;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const CRYPTO_MPI_LIMB _ECDSA_PrivateKey_P256_X_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x91, 0xC9, 0x0B, 0x85),
  CRYPTO_MPI_LIMB_DATA4(0x0F, 0x58, 0xD4, 0x46),
  CRYPTO_MPI_LIMB_DATA4(0x15, 0xA1, 0x3B, 0x4C),
  CRYPTO_MPI_LIMB_DATA4(0x17, 0x00, 0xEC, 0xE9),
  CRYPTO_MPI_LIMB_DATA4(0xC8, 0xB1, 0x62, 0x5A),
  CRYPTO_MPI_LIMB_DATA4(0x9F, 0x8D, 0x88, 0x8A),
  CRYPTO_MPI_LIMB_DATA4(0x52, 0x1F, 0x6C, 0xB5),
  CRYPTO_MPI_LIMB_DATA4(0x1E, 0x8D, 0x1B, 0xE0)
};

static const CRYPTO_ECDSA_PRIVATE_KEY _ECDSA_PrivateKey_P256 = {
  { CRYPTO_MPI_INIT_RO(_ECDSA_PrivateKey_P256_X_aLimbs) },
  &CRYPTO_EC_CURVE_secp256r1
};

static const U8 _aDigest[32] = { ' ', 'S', 'E', 'G', 'G', 'E', 'R', ' ',
                                 ' ', 'S', 'E', 'G', 'G', 'E', 'R', ' ',
                                 ' ', 'S', 'E', 'G', 'G', 'E', 'R', ' ',
                                 ' ', 'S', 'E', 'G', 'G', 'E', 'R', ' ' };


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static GLOBAL_DATA _Globals;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ECDSA_SignDigest()
*
*  Function description
*    Sign message digest.
*
*  Parameters
*    pPrivate     - Pointer to the private key to sign the digest with.
*    pDigest      - Pointer to octet string that is the digest of the message.
*    DigestLen    - Octet length of the digest octet string.
*    pSignature   - Pointer to buffer that receives the generated signature.
*    SignatureLen - Octet length of the signature buffer.
*
*  Return value
*    <= 0 - Signature failure (signature buffer too small).
*    >  0 - Success, number of bytes written to the the
*           signature buffer that constitute the signature.
*
*  Additional information
*    The signature buffer must be at least twice the number of bytes
*    required for curve's underlying prime field.  For instance, the P-256
*    prime is 256 bits (32 bytes) hence the signature will require
*    64 bytes. The P-521 prime requires 66 bytes and the signature
*    132 bytes.
*/
static int _ECDSA_SignDigest(const CRYPTO_ECDSA_PRIVATE_KEY * pPrivate, const U8 *pDigest, unsigned DigestLen, U8 * pSignature, int SignatureLen) {
  int PrimeSize;
  int Status;
  //
  PrimeSize = CRYPTO_MPI_ByteCount(&pPrivate->pCurve->P);
  if (SignatureLen < 2 * PrimeSize) {
    return CRYPTO_ERROR_BAD_LENGTH;  // Signature will not fit.
  }
  //
  CRYPTO_ECDSA_InitSignature(&_Globals.Signature, &_Globals.Context);
  Status = CRYPTO_ECDSA_SignDigest(pPrivate->pCurve, pPrivate, pDigest, DigestLen, &_Globals.Signature, &_Globals.Context);
  if (Status > 0) {
    CRYPTO_MPI_StoreBytes(&_Globals.Signature.R, pSignature, PrimeSize);
    CRYPTO_MPI_StoreBytes(&_Globals.Signature.S, pSignature + PrimeSize, PrimeSize);
    Status = 2 * PrimeSize;
  }
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
*    Prepare keys and sign digest.
*/
void MainTask(void);
void MainTask(void) {
  int Status;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_CHUNK_HEAP_Init(&_Globals.Context, &_Globals.Heap, _Globals.aWorkspace, SEGGER_COUNTOF(_Globals.aWorkspace), sizeof(_Globals.aWorkspace[0]));
  //
  CRYPTO_ECDSA_InitSignature(&_Globals.Signature, &_Globals.Context);
  CRYPTO_CHECK(_ECDSA_SignDigest(&_ECDSA_PrivateKey_P256, _aDigest, sizeof(_aDigest), _Globals.aSignature, sizeof(_Globals.aSignature)));
  //
  SEGGER_SYS_IO_Printf("Global data required = %u bytes\n", sizeof(_Globals));
  SEGGER_SYS_IO_Printf("Signed without error");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
  //
Finally:
  SEGGER_SYS_IO_Printf("Exception: %d\n", Status);
  SEGGER_SYS_OS_Halt(Status);
}

/*************************** End of file ****************************/
