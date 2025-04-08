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

File        : CRYPTO_Bench_ECDH.c
Purpose     : Benchmark ECDH key agreement performance.

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

#define MAX_CHUNKS             22

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
*       Local data types
*
**********************************************************************
*/

// Maximum prime size is 521 bits, but require additional 63 bits
// for underlying fast prime field reduction.
typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*521+63)+2];

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
*       _LFSR_Get()
*
*  Function description
*    Get pseudo-random data.
*
*  Parameters
*    pData   - Pointer to object the receives the random data.
*    DataLen - Octet length of the random data.
*
*  Additional information
*    This LFSR PRNG does not need to be cryptographically strong as
*    its purpose is only to deliver repeatable pseudo-random data
*    that does not depend upon a nondeterministic source (such as
*    a hardware RNG or the availability of hardware ciphering and
*    hashing in the DRBG code).  Therefore, this RNG is suitable
*    for deterministic benchmarking across compilers and systems.
*/
static void _LFSR_Get(U8 *pData, unsigned DataLen) {
  static U32 State32 = 0xFEDCBA8uL;
  static U32 State31 = 0x1234567uL;
  //
  while (DataLen > 0) {
    if (State32 & 1) {
      State32 >>= 1;
      State32 ^= 0xB4BCD35CuL;
    } else {
      State32 >>= 1;
    }
    if (State32 & 1) {
      State31 >>= 1;
      State31 ^= 0x7A5BC2E3uL;
    } else {
      State31 >>= 1;
    }
    if (DataLen >= 2) {
      CRYPTO_WRU16LE(pData, (U16)(State31 ^ State32));
      pData += 2;
      DataLen   -= 2;
    } else {
      *pData = (U8)(State31 ^ State32);
      DataLen -= 1;
    }
  }
}

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
*       _BenchmarkECDHKeyAgreement()
*
*  Function description
*    Benchmark ECDH key agreement, both sides.
*
*  Parameters
*    pCurve - Pointer to elliptic curve.
*/
static void _BenchmarkECDHKeyAgreement(const CRYPTO_EC_CURVE *pCurve) {
  CRYPTO_ECDH_KA_CONTEXT Us;
  CRYPTO_ECDH_KA_CONTEXT Them;
  U64                    OneSecond;
  U64                    T0;
  U64                    Elapsed;
  int                    Loops;
  int                    Status;
  unsigned               PeakBytes;
  unsigned               UnitSize;
  float                  Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  UnitSize  = CRYPTO_MPI_BYTES_REQUIRED(2*CRYPTO_MPI_BitCount(&pCurve->P)+63) + 2*CRYPTO_MPI_BYTES_PER_LIMB;
  //
  SEGGER_SYS_IO_Printf("| %-16s |", pCurve->aCurveName);
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_ECDH_KA_Init(&Us,   &_MemContext);
    CRYPTO_ECDH_KA_Init(&Them, &_MemContext);
    //
    CRYPTO_CHECK(CRYPTO_ECDH_KA_Start(&Us,   pCurve, &_MemContext));
    CRYPTO_CHECK(CRYPTO_ECDH_KA_Start(&Them, pCurve, &_MemContext));
    //
    CRYPTO_CHECK(CRYPTO_ECDH_KA_Agree(&Us,   &Them.Public.Y, &_MemContext));
    CRYPTO_CHECK(CRYPTO_ECDH_KA_Agree(&Them, &Us.Public.Y,   &_MemContext));
    //
    CRYPTO_ASSERT(CRYPTO_MPI_IsEqual(&Us.K.X, &Them.K.X));
    //
    CRYPTO_ECDH_KA_Kill(&Us);
    CRYPTO_ECDH_KA_Kill(&Them);
    //
    PeakBytes = _Heap.Stats.NumInUseMax * UnitSize;
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
Finally:
  CRYPTO_ECDH_KA_Kill(&Us);
  CRYPTO_ECDH_KA_Kill(&Them);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%13s |%13s | ", "-Fail-", "-Fail-");
  } else {
    Loops     *= 2;    // Two agreements per loop
    PeakBytes /= 2;    // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    SEGGER_SYS_IO_Printf("%13.2f |%13d |", Time, PeakBytes);
  }
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
  static const CRYPTO_RNG_API LFSR = { NULL, _LFSR_Get, NULL, NULL };
  //
  CRYPTO_Init();
  CRYPTO_RNG_Install(&LFSR);
  SEGGER_SYS_Init();
  SEGGER_MEM_SELFTEST_HEAP_Init(&_MemContext, &_Heap, _aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("ECDH Key Agreement Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed          = %.3f MHz\n", (double)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   Static heap size         = %u bytes\n", sizeof(_aUnits));
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION           = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB = %u\n", CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("This benchmarks both ends of an ECDH key agreement, but\n");
  SEGGER_SYS_IO_Printf("timing is reported as the time for one end's calculation.\n");
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("+------------------+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("| Curve            | ms/Agreement |       Memory |\n");
  SEGGER_SYS_IO_Printf("+------------------+--------------+--------------+\n");
  //
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp192r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp192k1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp224r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp224k1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp256r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp256k1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp384r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_secp521r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP160r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP160t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP192r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP192t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP224r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP224t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP256r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP256t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP320r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP320t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP384r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP384t1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP512r1);
  _BenchmarkECDHKeyAgreement(&CRYPTO_EC_CURVE_brainpoolP512t1);
  //
  SEGGER_SYS_IO_Printf("+------------------+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
