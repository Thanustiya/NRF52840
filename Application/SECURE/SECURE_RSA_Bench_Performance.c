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

File        : SECURE_RSA_Bench_Performance.c
Purpose     : Benchmark emSecure-RSA performance.

Additional information:
  Preparations:
    None.

  Expected behavior:
    Measures the sign and verify performance using RSA signatures.
    For more detailed information see the emSecure RSA manual.

  Sample output:
    (c) 2014-2018 SEGGER Microcontroller GmbH    www.segger.com
    emSecure-RSA Performance Benchmark compiled Jan 28 2019 14:11:32

    Compiler: gcc 7.3.1
    System:   Processor speed             = 204.000 MHz
    Config:   CRYPTO_VERSION              = 23000 [2.30]
    Config:   SECURE_RSA_VERSION          = 23800 [2.38]
    Config:   CRYPTO_MPI_BITS_PER_LIMB    = 32
    Config:   SECURE_RSA_MAX_KEY_LENGTH   = 2048 bits
    Config:   SECURE_RSA_HASH_FUNCTION    = SHA1
    Config:   SECURE_RSA_SIGNATURE_SCHEME = PSS

    Sign/Verify Performance
    =======================

    +----------+----------+----------+----------+
    |  Modulus |  Message |     Sign |   Verify |
    |    /bits |   /bytes |      /ms |      /ms |
    +----------+----------+----------+----------+
    |      512 |        0 |    26.49 |     1.89 |
    |      512 |     1024 |    26.73 |     2.27 |
    |      512 |   102400 |    59.58 |    35.15 |
    +----------+----------+----------+----------+
    |     1024 |        0 |   136.93 |     5.40 |
    |     1024 |     1024 |   137.16 |     5.77 |
    |     1024 |   102400 |   169.98 |    38.68 |
    +----------+----------+----------+----------+
    |     2048 |        0 |   882.76 |    20.64 |
    |     2048 |     1024 |   881.14 |    20.94 |
    |     2048 |   102400 |   914.51 |    53.77 |
    +----------+----------+----------+----------+

    Benchmark complete

    STOP.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SECURE_RSA.h"
#include "SECURE_RSA_PrivateKey_512b.h"
#include "SECURE_RSA_PrivateKey_1024b.h"
#include "SECURE_RSA_PrivateKey_2048b.h"
#include "SECURE_RSA_PublicKey_512b.h"
#include "SECURE_RSA_PublicKey_1024b.h"
#include "SECURE_RSA_PublicKey_2048b.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define STRINGIZE(X)   #X
#define STRINGIZEX(X)  STRINGIZE(X)

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

typedef struct {
  const CRYPTO_RSA_PRIVATE_KEY * pPrivateKey;
  const CRYPTO_RSA_PUBLIC_KEY  * pPublicKey;
  const U8                     * pMesssage;
  unsigned                       MessageLen;
} BENCH_PARA;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const U8 _aMessage_100k[100*1024] = {
  0x00,
};

static const BENCH_PARA _aBenchKeys[] = {
  { &_SECURE_RSA_PrivateKey_512b,  &_SECURE_RSA_PublicKey_512b,  _aMessage_100k,        0u },
  { &_SECURE_RSA_PrivateKey_512b,  &_SECURE_RSA_PublicKey_512b,  _aMessage_100k,     1024u },
  { &_SECURE_RSA_PrivateKey_512b,  &_SECURE_RSA_PublicKey_512b,  _aMessage_100k, 100*1024u },
  { NULL,                          NULL,                         NULL,                  0u },
  { &_SECURE_RSA_PrivateKey_1024b, &_SECURE_RSA_PublicKey_1024b, _aMessage_100k,        0u },
  { &_SECURE_RSA_PrivateKey_1024b, &_SECURE_RSA_PublicKey_1024b, _aMessage_100k,     1024u },
  { &_SECURE_RSA_PrivateKey_1024b, &_SECURE_RSA_PublicKey_1024b, _aMessage_100k, 100*1024u },
  { NULL,                          NULL,                         NULL,                  0u },
  { &_SECURE_RSA_PrivateKey_2048b, &_SECURE_RSA_PublicKey_2048b, _aMessage_100k,        0u },
  { &_SECURE_RSA_PrivateKey_2048b, &_SECURE_RSA_PublicKey_2048b, _aMessage_100k,     1024u },
  { &_SECURE_RSA_PrivateKey_2048b, &_SECURE_RSA_PublicKey_2048b, _aMessage_100k, 100*1024u }
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static union {
  //
  // Temporary memory used for signing
  //
  CRYPTO_MPI_LIMB       aWs[5][CRYPTO_MPI_LIMBS_REQUIRED(SECURE_RSA_MAX_KEY_LENGTH) + 2];
  //
  // Temporary memory used for verifing
  //
  CRYPTO_MPI_LIMB       aWv[4][(CRYPTO_MPI_LIMBS_REQUIRED(SECURE_RSA_MAX_KEY_LENGTH*2) + 2)];
} _Workspace;

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
*       _BenchmarkSignVerify()
*
*  Function description
*    Count the number of signs and verifies completed in one second.
*
*  Parameters
*    pPara - Pointer to benchmark parameters.
*/
static void _BenchmarkSignVerify(const BENCH_PARA *pPara) {
  U8    aSignature[270];
  U64   OneSecond;
  U64   T0;
  U64   Elapsed;
  int   SignatureLen;
  int   Loops;
  int   Status;
  float Time;
  SEGGER_MEM_CHUNK_HEAP Heap;
  SEGGER_MEM_CONTEXT    Context;
  //
  SEGGER_SYS_IO_Printf("| %8d | %8u | ",
                       CRYPTO_MPI_BitCount(&pPara->pPublicKey->N),
                       pPara->MessageLen);
  //
  Loops = 0;
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    SEGGER_MEM_CHUNK_HEAP_Init(&Context, &Heap, _Workspace.aWs, SEGGER_COUNTOF(_Workspace.aWs), sizeof(_Workspace.aWs[0]));
    SignatureLen = SECURE_RSA_SignEx(pPara->pPrivateKey,
                                     NULL,             0,
                                     pPara->pMesssage, pPara->MessageLen,
                                     aSignature,       sizeof(aSignature),
                                     &Context);
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (SignatureLen >= 0 && Elapsed < OneSecond);
  //
  Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
  if (SignatureLen < 0) {
    SEGGER_SYS_IO_Printf("%8s | ", "-Fail-");
  } else {
    SEGGER_SYS_IO_Printf("%8.2f | ", Time);
  }
  //
  if (SignatureLen < 0) {
    SEGGER_SYS_IO_Printf("%8s |\n", "-Skip-");
  } else {
    Loops = 0;
    OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
    T0 = SEGGER_SYS_OS_GetTimer();
    do {
      SEGGER_MEM_CHUNK_HEAP_Init(&Context, &Heap, _Workspace.aWv, SEGGER_COUNTOF(_Workspace.aWv), sizeof(_Workspace.aWv[0]));
      Status = SECURE_RSA_VerifyEx(pPara->pPublicKey,
                                   NULL,             0,
                                   pPara->pMesssage, pPara->MessageLen,
                                   aSignature,       SignatureLen,
                                   &Context);
      Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
      ++Loops;
    } while (Status >= 0 && Elapsed < OneSecond);
    //
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (Status <= 0) {
      SEGGER_SYS_IO_Printf("%8s |\n", "-Fail-");
    } else {
      SEGGER_SYS_IO_Printf("%8.2f |\n", Time);
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
*       MainTask()
*
*  Function description
*    Main entry point for application to run all the tests.
*/
void MainTask(void);
void MainTask(void) {
  unsigned i;
  //
  SECURE_RSA_Init();
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", SECURE_RSA_GetCopyrightText());
  SEGGER_SYS_IO_Printf("emSecure-RSA Performance Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed             = %.3f MHz\n",
                         (float)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION              = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_VERSION          = %u [%s]\n", SECURE_RSA_VERSION, SECURE_RSA_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB    = %u\n",      CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_MAX_KEY_LENGTH   = %u bits\n", SECURE_RSA_MAX_KEY_LENGTH);
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_HASH_FUNCTION    = %s\n",      STRINGIZEX(SECURE_RSA_HASH_FUNCTION));
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_SIGNATURE_SCHEME = %s\n",      STRINGIZEX(SECURE_RSA_SIGNATURE_SCHEME));
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Sign/Verify Performance\n");
  SEGGER_SYS_IO_Printf("=======================\n\n");
  //
  SEGGER_SYS_IO_Printf("+----------+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("|  Modulus |  Message |     Sign |   Verify |\n");
  SEGGER_SYS_IO_Printf("|    /bits |   /bytes |      /ms |      /ms |\n");
  SEGGER_SYS_IO_Printf("+----------+----------+----------+----------+\n");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    if (_aBenchKeys[i].pPublicKey == NULL) {
      SEGGER_SYS_IO_Printf("+----------+----------+----------+----------+\n");
    } else {
      _BenchmarkSignVerify(&_aBenchKeys[i]);
    }
  }
  SEGGER_SYS_IO_Printf("+----------+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
