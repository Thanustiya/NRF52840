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

File        : CRYPTO_Demo_ECDSA_Verify_P256_MinSize.c
Purpose     : Minimum data size required for ECDSA verification using P-256.

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
#define CRYPTO_ECDSA_1LIMB_ALLOC        (CRYPTO_MPI_LIMBS_REQUIRED(CRYPTO_ECDSA_MAX_KEY_LENGTH))
#if CRYPTO_CONFIG_ECDSA_TWIN_MULTIPLY > 0
  #define CRYPTO_ECDSA_VERIFY_2LIMB_REQD  25
#else
  #define CRYPTO_ECDSA_VERIFY_2LIMB_REQD  18
#endif

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
  SEGGER_MEM_CONTEXT     Context;
  SEGGER_MEM_CHUNK_HEAP  Heap;
  CRYPTO_MPI_LIMB        aW[CRYPTO_ECDSA_VERIFY_2LIMB_REQD][CRYPTO_ECDSA_2LIMB_ALLOC];
  CRYPTO_MPI_LIMB        aT[2][CRYPTO_ECDSA_1LIMB_ALLOC];
  CRYPTO_ECDSA_SIGNATURE Signature;
  U8                     aDigest[32];
} GLOBAL_DATA;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const U8 _aMessage_1k[1024] = {
  0x00,
};

static const U8 _aSignature_P256_1k[] = {
  0xB2, 0x3F, 0x46, 0x0B, 0x84, 0xE3, 0x46, 0x18,
  0x8C, 0x68, 0x0F, 0x30, 0xD9, 0x0A, 0x1C, 0xAE,
  0x38, 0xC0, 0xF1, 0xB4, 0x6F, 0xDA, 0xC0, 0xF9,
  0xFE, 0xC5, 0x98, 0x60, 0x88, 0x3E, 0x79, 0xB5,
  0x6B, 0xF6, 0xEF, 0xC8, 0x78, 0xD6, 0x56, 0xE1,
  0xF8, 0x40, 0x97, 0x60, 0x34, 0x66, 0xBE, 0xC1,
  0x24, 0x3C, 0x87, 0xD0, 0xD1, 0x27, 0xAB, 0xED,
  0x5C, 0x17, 0xCA, 0xC6, 0xB1, 0xE3, 0x75, 0x87,
};

static const CRYPTO_MPI_LIMB _ECDSA_PublicKey_P256_YX_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x28, 0x59, 0x5E, 0x86),
  CRYPTO_MPI_LIMB_DATA4(0xAD, 0xED, 0x67, 0xC9),
  CRYPTO_MPI_LIMB_DATA4(0x82, 0x0A, 0x6B, 0x47),
  CRYPTO_MPI_LIMB_DATA4(0xA9, 0x44, 0x41, 0xC1),
  CRYPTO_MPI_LIMB_DATA4(0xD6, 0x46, 0xEE, 0x03),
  CRYPTO_MPI_LIMB_DATA4(0xB4, 0x69, 0x77, 0xDD),
  CRYPTO_MPI_LIMB_DATA4(0xF4, 0x82, 0x22, 0xB5),
  CRYPTO_MPI_LIMB_DATA4(0xA7, 0xE3, 0x75, 0xB9)
};

static const CRYPTO_MPI_LIMB _ECDSA_PublicKey_P256_YY_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x28, 0xC5, 0xF6, 0xF9),
  CRYPTO_MPI_LIMB_DATA4(0x1D, 0x5F, 0xAB, 0x1C),
  CRYPTO_MPI_LIMB_DATA4(0x3C, 0x93, 0x5E, 0x34),
  CRYPTO_MPI_LIMB_DATA4(0x29, 0xB1, 0xB5, 0x94),
  CRYPTO_MPI_LIMB_DATA4(0xE1, 0xB7, 0x45, 0x2F),
  CRYPTO_MPI_LIMB_DATA4(0x2B, 0x4E, 0xD9, 0xAD),
  CRYPTO_MPI_LIMB_DATA4(0x38, 0xA1, 0xAE, 0x27),
  CRYPTO_MPI_LIMB_DATA4(0x57, 0x97, 0x7E, 0x5C)
};

static const CRYPTO_ECDSA_PUBLIC_KEY _ECDSA_PublicKey_P256 = { {
  { CRYPTO_MPI_INIT_RO(_ECDSA_PublicKey_P256_YX_aLimbs) },
  { CRYPTO_MPI_INIT_RO(_ECDSA_PublicKey_P256_YY_aLimbs) },
  { CRYPTO_MPI_INIT_RO_ZERO },
  { CRYPTO_MPI_INIT_RO_ZERO },
  },
  &CRYPTO_EC_CURVE_secp256r1
};

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
*       _CRYPTO_ECDSA_Offload()
*
*  Function description
*    Move an MPI from dynamic storage into fixed memory.
* 
*  Parameters 
*    pOffload - Pointer to buffer that will receive the offloaded limb data.
*    pValue   - The multiprecision integer to offload into fixed memory.
*/
static void _CRYPTO_ECDSA_Offload(CRYPTO_MPI_LIMB *pOffload, CRYPTO_MPI *pValue) {
  CRYPTO_MEMCPY(pOffload, pValue->aLimbs, sizeof(CRYPTO_MPI_LIMB) * pValue->LimbCnt);
  SEGGER_MEM_Free(pValue->pStore, pValue->aLimbs);
  pValue->aLimbs   = pOffload;
  pValue->ReadOnly = 1;
  pValue->pStore   = NULL;  // Fixed memory, cannot be resized.
}

/*********************************************************************
*
*       _ECDSA_VerifyDigest()
*
*  Function description
*    Verify message digest.
*
*  Parameters
*    pPublic      - Pointer to public key used to verify the message.
*    pDigest      - Pointer to digest of the original message to be verified.
*    pSignature   - Pointer to signature to verify.
*    SignatureLen - Octet length of the signature.
*
*  Return value
*    <  0 - Processing error verifying signature, signature is not verified.
*    == 0 - Processing successful, signature is not verified.
*    >  0 - Processing successful, signature is verified.
*
*  Additional information
*    The signature buffer must be exactly twice the number of bytes
*    required for the curve's underlying prime field.  For instance, the P-256
*    prime is 256 bits (32 bytes) hence the signature will require
*    64 bytes. The P-521 prime requires 66 bytes and the signature
*    132 bytes.
*/
static int _ECDSA_VerifyDigest(const CRYPTO_ECDSA_PUBLIC_KEY * pPublic,
                               const U8                      * pDigest,
                               const U8                      * pSignature,
                                     int                       SignatureLen) {
  int PrimeSize;
  int Status;
  //
  if (SignatureLen != 2*(int)CRYPTO_MPI_ByteCount(&pPublic->pCurve->P)) {
    return 0;
  }
  //
  // Load signature.  The signature components are the same size as the
  // underlying field's prime, so move that off-heap into local storage
  // so we don't burn double the memory on it.
  //
  PrimeSize = CRYPTO_MPI_ByteCount(&pPublic->pCurve->P);
  CRYPTO_MPI_Init(&_Globals.Signature.R, &_Globals.Context);
  Status = CRYPTO_MPI_LoadBytes(&_Globals.Signature.R, pSignature, PrimeSize);
  if (Status >= 0) {
    _CRYPTO_ECDSA_Offload(_Globals.aT[0], &_Globals.Signature.R);
    CRYPTO_MPI_Init(&_Globals.Signature.S, &_Globals.Context);
    Status = CRYPTO_MPI_LoadBytes(&_Globals.Signature.S, pSignature + PrimeSize, PrimeSize);
    if (Status >= 0) {
      _CRYPTO_ECDSA_Offload(_Globals.aT[1], &_Globals.Signature.S);
      Status = CRYPTO_ECDSA_VerifyDigest(pPublic->pCurve, pPublic, pDigest, CRYPTO_SHA256_DIGEST_BYTE_COUNT, &_Globals.Signature, &_Globals.Context);
    }
  }
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
*    Prepare keys and sign digest.
*/
void MainTask(void);
void MainTask(void) {
  int Status;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_CHUNK_HEAP_Init(&_Globals.Context, &_Globals.Heap, _Globals.aW, SEGGER_COUNTOF(_Globals.aW), sizeof(_Globals.aW[0]));
  //
  // Verify 1k of zeros.
  //
  CRYPTO_ECDSA_InitSignature (&_Globals.Signature, &_Globals.Context);
  CRYPTO_SHA256_Calc(_Globals.aDigest, sizeof(_Globals.aDigest), _aMessage_1k, sizeof(_aMessage_1k));
  CRYPTO_CHECK(_ECDSA_VerifyDigest(&_ECDSA_PublicKey_P256, _Globals.aDigest, _aSignature_P256_1k, sizeof(_aSignature_P256_1k)));
  SEGGER_SYS_IO_Printf("Global data required = %u bytes\n", sizeof(_Globals));
  if (Status > 0) {
    SEGGER_SYS_IO_Printf("Signature verified without error");
  } else {
    SEGGER_SYS_IO_Printf("Signature did not verify!");
  }
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
  //
Finally:
  SEGGER_SYS_IO_Printf("Exception: %d\n", Status);
  SEGGER_SYS_OS_Halt(Status);
}

/*************************** End of file ****************************/
