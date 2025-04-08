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

File        : CRYPTO_Bench_ECDSA.c
Purpose     : Benchmark ECDSA sign and verify.

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

#define MAX_CHUNKS            30  // For twin multiplication

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

// Maximum prime size is 521 bits, but require additional 63 bits
// for underlying fast prime field reduction.
typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*521+63)+2];

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

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

static MPI_UNIT                 _aUnits[MAX_CHUNKS];
static SEGGER_MEM_CONTEXT       _MemContext;
static SEGGER_MEM_SELFTEST_HEAP _Heap;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ConvertTicksToSeconds()
*
*  Function description
*    Convert ticks to seconds.
*
*  Parameters
*    Ticks - Number of ticks reported by SEGGER_SYS_OS_GetTimer().
*
*  Return value
*    Number of seconds corresponding to tick.
*/
static float _ConvertTicksToSeconds(U64 Ticks) {
  return SEGGER_SYS_OS_ConvertTicksToMicros(Ticks) / 1000000.0f;
}

/*********************************************************************
*
*       _BenchmarkECDSASign()
*
*  Function description
*    Benchmark ECDSA sign.
*
*  Parameters
*    pCurve - Pointer to elliptic curve.
*/
static void _BenchmarkECDSASign(const CRYPTO_EC_CURVE *pCurve) {
  CRYPTO_ECDSA_PRIVATE_KEY Private;
  CRYPTO_ECDSA_PUBLIC_KEY  Public;
  CRYPTO_ECDSA_SIGNATURE   Signature;
  U64                      OneSecond;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 UnitSize;
  unsigned                 PeakBytes;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  UnitSize  = CRYPTO_MPI_BYTES_REQUIRED(2*CRYPTO_MPI_BitCount(&pCurve->P)+63) + 2*CRYPTO_MPI_BYTES_PER_LIMB;
  //
  CRYPTO_ECDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_ECDSA_InitPublicKey (&Public,    &_MemContext);
  CRYPTO_ECDSA_InitSignature (&Signature, &_MemContext);
  //
  CRYPTO_ECDSA_GenKeys       (pCurve, &Private, &Public, &_MemContext);
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_ECDSA_SignDigest(pCurve, &Private, &_aDigest[0], sizeof(_aDigest), &Signature, &_MemContext));
    if (Status == 0) {
      SEGGER_SYS_IO_Printf("ERROR - Did not sign digest\n");
      SEGGER_SYS_OS_Halt(100);
    }
    //
    PeakBytes = SEGGER_MAX(PeakBytes, _Heap.Stats.NumInUseMax * UnitSize);
    //
    CRYPTO_ECDSA_KillSignature(&Signature);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
Finally:
  CRYPTO_ECDSA_KillPrivateKey(&Private);
  CRYPTO_ECDSA_KillPublicKey (&Public);
  CRYPTO_ECDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%10s |", "-Fail-");
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    SEGGER_SYS_IO_Printf("%10.2f |", Time);
    SEGGER_SYS_IO_Printf("%10d |", PeakBytes);
  }
}

/*********************************************************************
*
*       _BenchmarkECDSAVerify()
*
*  Function description
*    Benchmark ECDSA verify.
*
*  Parameters
*    pCurve - Pointer to elliptic curve.
*/
static void _BenchmarkECDSAVerify(const CRYPTO_EC_CURVE *pCurve) {
  CRYPTO_ECDSA_PRIVATE_KEY Private;
  CRYPTO_ECDSA_PUBLIC_KEY  Public;
  CRYPTO_ECDSA_SIGNATURE   Signature;
  U64                      OneSecond;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 PeakBytes;
  unsigned                 UnitSize;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  UnitSize  = CRYPTO_MPI_BYTES_REQUIRED(2*CRYPTO_MPI_BitCount(&pCurve->P)+63) + 2*CRYPTO_MPI_BYTES_PER_LIMB;
  //
  CRYPTO_ECDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_ECDSA_InitPublicKey (&Public,    &_MemContext);
  CRYPTO_ECDSA_InitSignature (&Signature, &_MemContext);
  //
  CRYPTO_ECDSA_GenKeys       (pCurve, &Private, &Public, &_MemContext);
  //
  CRYPTO_CHECK(CRYPTO_ECDSA_SignDigest(pCurve, &Private, &_aDigest[0], sizeof(_aDigest), &Signature, &_MemContext));
  if (Status == 0) {
    SEGGER_SYS_IO_Printf("ERROR - Did not sign digest\n");
    SEGGER_SYS_OS_Halt(100);
  }
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_ECDSA_VerifyDigest(pCurve, &Public, &_aDigest[0], sizeof(_aDigest), &Signature, &_MemContext));
    if (Status == 0) {
      SEGGER_SYS_IO_Printf("ERROR - Did not verify digest\n");
      SEGGER_SYS_OS_Halt(100);
    }
    //
    PeakBytes = SEGGER_MAX(PeakBytes, _Heap.Stats.NumInUseMax * UnitSize);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
Finally:
  CRYPTO_ECDSA_KillPrivateKey(&Private);
  CRYPTO_ECDSA_KillPublicKey (&Public);
  CRYPTO_ECDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%10s |", "-Fail-");
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    SEGGER_SYS_IO_Printf("%10.2f |", Time);
    SEGGER_SYS_IO_Printf("%10d |", PeakBytes);
  }
}

/*********************************************************************
*
*       _BenchmarkECDSA()
*
*  Function description
*    Benchmark ECDSA sign and verify.
*
*  Parameters
*    pCurve - Pointer to elliptic curve.
*/
static void _BenchmarkECDSA(const CRYPTO_EC_CURVE *pCurve) {
  SEGGER_SYS_IO_Printf("| %-16s |", pCurve->aCurveName);
  _BenchmarkECDSASign  (pCurve);
  _BenchmarkECDSAVerify(pCurve);
  SEGGER_SYS_IO_Printf("\n");
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
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_SELFTEST_HEAP_Init(&_MemContext, &_Heap, _aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("ECDSA Sign and Verify Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed                   = %.3f MHz\n", (double)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   Static heap size                  = %u bytes\n", sizeof(_aUnits));
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                    = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB          = %u\n", CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_ECDSA_TWIN_MULTIPLY = %u\n", CRYPTO_CONFIG_ECDSA_TWIN_MULTIPLY);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("+------------------+-----------+-----------+-----------+-----------+\n");
  SEGGER_SYS_IO_Printf("|                  |      Sign |     Sign  |    Verify |    Verify |\n");
  SEGGER_SYS_IO_Printf("| Curve            |        ms |     bytes |        ms |     bytes |\n");
  SEGGER_SYS_IO_Printf("+------------------+-----------+-----------+-----------+-----------+\n");
  //
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp192r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp192k1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp224r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp224k1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp256r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp256k1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp384r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_secp521r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP160r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP160t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP192r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP192t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP224r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP224t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP256r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP256t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP320r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP320t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP384r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP384t1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP512r1);
  _BenchmarkECDSA(&CRYPTO_EC_CURVE_brainpoolP512t1);
  //
  SEGGER_SYS_IO_Printf("+------------------+-----------+-----------+-----------+-----------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
