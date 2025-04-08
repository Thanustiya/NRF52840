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

File        : CRYPTO_Bench_EdDSA.c
Purpose     : Benchmark EdDSA sign and verify.

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
*       Defines, fixed
*
**********************************************************************
*/

#define CRYPTO_ASSERT(X)               { if (!(X)) { CRYPTO_PANIC(); } }  // I know this is low-rent
#define CRYPTO_CHECK(X)                /*lint -e{717,801,9036} */ do { if ((Status = (X)) < 0) goto Finally; } while (0)

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define MAX_CHUNKS             31

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*448)+3];  // +3 as one of the EdDSA divisors is not normalized

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static MPI_UNIT                 _aUnits[MAX_CHUNKS];
static SEGGER_MEM_CONTEXT       _MemContext;
static SEGGER_MEM_SELFTEST_HEAP _Heap;
static int                      _ShowMemory = 0;

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
*       _BenchmarkEd25519Sign()
*
*  Function description
*    Benchmark Ed25519 sign.
*/
static void _BenchmarkEd25519Sign(void) {
  CRYPTO_EdDSA_PRIVATE_KEY Private;
  CRYPTO_EdDSA_PUBLIC_KEY  Public;
  CRYPTO_EdDSA_SIGNATURE   Signature;
  U8                       aMsg[] = { "SEGGER: It simply works!" };
  U64                      Limit;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 PeakBytes;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  //
  SEGGER_SYS_IO_Printf("| %-12s |", "Ed25519");
  //
  CRYPTO_MEMSET(aMsg, 0, sizeof(aMsg));
  CRYPTO_EdDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_EdDSA_InitPublicKey (&Public,    &_MemContext);
  CRYPTO_EdDSA_InitSignature (&Signature, &_MemContext);
  //
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_GenKeys(&Private, &Public, &_MemContext));
  //
  Limit = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_Sign(&Private, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
    CRYPTO_EdDSA_KillSignature(&Signature);
    //
    PeakBytes = _Heap.Stats.NumInUseMax * sizeof(MPI_UNIT);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < Limit);
  //
Finally:
  CRYPTO_EdDSA_KillPrivateKey(&Private);
  CRYPTO_EdDSA_KillPublicKey (&Public);
  CRYPTO_EdDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%13s |", "-Fail-");
  } else if (_ShowMemory) {
    SEGGER_SYS_IO_Printf("%13d |", PeakBytes);
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (_ShowMemory) {
      SEGGER_SYS_IO_Printf("%8d |", PeakBytes);
    } else {
      SEGGER_SYS_IO_Printf("%13.2f |", Time);
    }
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _BenchmarkEd448Sign()
*
*  Function description
*    Benchmark Ed448 sign.
*/
static void _BenchmarkEd448Sign(void) {
  CRYPTO_EdDSA_PRIVATE_KEY Private;
  CRYPTO_EdDSA_SIGNATURE   Signature;
  U8                       aMsg[] = { "SEGGER: It simply works!" };
  U64                      Limit;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 PeakBytes;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  //
  SEGGER_SYS_IO_Printf("| %-12s |", "Ed448");
  //
  CRYPTO_MEMSET(aMsg, 0, sizeof(aMsg));
  CRYPTO_EdDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_EdDSA_InitSignature (&Signature, &_MemContext);
  //
  Limit = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_EdDSA_Ed448_Sign(&Private, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
    CRYPTO_EdDSA_KillSignature(&Signature);
    //
    PeakBytes = _Heap.Stats.NumInUseMax * sizeof(MPI_UNIT);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < Limit);
  //
Finally:
  CRYPTO_EdDSA_KillPrivateKey(&Private);
  CRYPTO_EdDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%13s |", "-Fail-");
  } else if (_ShowMemory) {
    SEGGER_SYS_IO_Printf("%13d |", PeakBytes);
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (_ShowMemory) {
      SEGGER_SYS_IO_Printf("%8d |", PeakBytes);
    } else {
      SEGGER_SYS_IO_Printf("%13.2f |", Time);
    }
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _BenchmarkEd25519Verify()
*
*  Function description
*    Benchmark Ed25519 verify.
*/
static void _BenchmarkEd25519Verify(void) {
  CRYPTO_EdDSA_PRIVATE_KEY Private;
  CRYPTO_EdDSA_PUBLIC_KEY  Public;
  CRYPTO_EdDSA_SIGNATURE   Signature;
  U8                       aMsg[] = { "SEGGER: It simply works!" };
  U64                      Limit;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 PeakBytes;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  //
  SEGGER_SYS_IO_Printf("| %-12s |", "Ed25519");
  //
  CRYPTO_MEMSET(aMsg, 0, sizeof(aMsg));
  CRYPTO_EdDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_EdDSA_InitPublicKey (&Public,    &_MemContext);
  CRYPTO_EdDSA_InitSignature (&Signature, &_MemContext);
  //
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_GenKeys(&Private, &Public, &_MemContext));
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_Sign(&Private, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
  //
  Limit = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_EdDSA_Ed25519_Verify(&Public, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
    //
    PeakBytes = _Heap.Stats.NumInUseMax * sizeof(MPI_UNIT);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < Limit);
  //
Finally:
  CRYPTO_EdDSA_KillPrivateKey(&Private);
  CRYPTO_EdDSA_KillPublicKey (&Public);
  CRYPTO_EdDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%13s |", "-Fail-");
  } else if (_ShowMemory) {
    SEGGER_SYS_IO_Printf("%13d |", PeakBytes);
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (_ShowMemory) {
      SEGGER_SYS_IO_Printf("%8d |", PeakBytes);
    } else {
      SEGGER_SYS_IO_Printf("%13.2f |", Time);
    }
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _BenchmarkEd448Verify()
*
*  Function description
*    Benchmark Ed448 verify.
*/
static void _BenchmarkEd448Verify(void) {
  CRYPTO_EdDSA_PRIVATE_KEY Private;
  CRYPTO_EdDSA_PUBLIC_KEY  Public;
  CRYPTO_EdDSA_SIGNATURE   Signature;
  U8                       aMsg[] = { "SEGGER: It simply works!" };
  U64                      Limit;
  U64                      T0;
  U64                      Elapsed;
  int                      Loops;
  int                      Status;
  unsigned                 PeakBytes;
  float                    Time;
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  //
  SEGGER_SYS_IO_Printf("| %-12s |", "Ed448");
  //
  CRYPTO_MEMSET(aMsg, 0, sizeof(aMsg));
  CRYPTO_EdDSA_InitPrivateKey(&Private,   &_MemContext);
  CRYPTO_EdDSA_InitPublicKey (&Public,    &_MemContext);
  CRYPTO_EdDSA_InitSignature (&Signature, &_MemContext);
  //
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed448_CalcPublicKey(&Private, &Public, &_MemContext));
  CRYPTO_CHECK(CRYPTO_EdDSA_Ed448_Sign(&Private, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
  //
  Limit = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_EdDSA_Ed448_Verify(&Public, &aMsg[0], sizeof(aMsg), &Signature, &_MemContext));
    //
    PeakBytes = _Heap.Stats.NumInUseMax * sizeof(MPI_UNIT);
    //
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < Limit);
  //
Finally:
  CRYPTO_EdDSA_KillPrivateKey(&Private);
  CRYPTO_EdDSA_KillPublicKey (&Public);
  CRYPTO_EdDSA_KillSignature (&Signature);
  //
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("%13s |", "-Fail-");
  } else if (_ShowMemory) {
    SEGGER_SYS_IO_Printf("%13d |", PeakBytes);
  } else {
    Loops *= 2;  // Two agreements per loop
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (_ShowMemory) {
      SEGGER_SYS_IO_Printf("%8d |", PeakBytes);
    } else {
      SEGGER_SYS_IO_Printf("%13.2f |", Time);
    }
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
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_SELFTEST_HEAP_Init(&_MemContext, &_Heap, _aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("EdDSA Sign and Verify Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
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
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("| Curve        | ms/Sign      |\n");
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  //
  _BenchmarkEd25519Sign();
  _BenchmarkEd448Sign();
  //
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("| Curve        | ms/Verify    |\n");
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  //
  _BenchmarkEd25519Verify();
  _BenchmarkEd448Verify();
  //
  SEGGER_SYS_IO_Printf("+--------------+--------------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
