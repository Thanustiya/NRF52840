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
*       emSecure-ECDSA * Digital signature toolkit                   *
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
*       emSecure-ECDSA version: V2.48.0                              *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SECURE_ECDSA_Bench_Performance.c
Purpose     : Benchmark emSecure-ECDSA performance.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SECURE_ECDSA.h"
#include "SECURE_ECDSA_PrivateKey_P192.h"
#include "SECURE_ECDSA_PrivateKey_P224.h"
#include "SECURE_ECDSA_PrivateKey_P256.h"
#include "SECURE_ECDSA_PrivateKey_P384.h"
#include "SECURE_ECDSA_PrivateKey_P521.h"
#include "SECURE_ECDSA_PublicKey_P192.h"
#include "SECURE_ECDSA_PublicKey_P224.h"
#include "SECURE_ECDSA_PublicKey_P256.h"
#include "SECURE_ECDSA_PublicKey_P384.h"
#include "SECURE_ECDSA_PublicKey_P521.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

typedef struct {
  const CRYPTO_ECDSA_PRIVATE_KEY * pPrivateKey;
  const CRYPTO_ECDSA_PUBLIC_KEY  * pPublicKey;
  const U8                       * pMesssage;
  unsigned                         MessageLen;
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
  { &_SECURE_ECDSA_PrivateKey_P192, &_SECURE_ECDSA_PublicKey_P192, _aMessage_100k,        0u },
  { &_SECURE_ECDSA_PrivateKey_P192, &_SECURE_ECDSA_PublicKey_P192, _aMessage_100k,     1024u },
  { &_SECURE_ECDSA_PrivateKey_P192, &_SECURE_ECDSA_PublicKey_P192, _aMessage_100k, 100*1024u },
  { NULL,                           NULL,                          NULL,                  0u },
  { &_SECURE_ECDSA_PrivateKey_P224, &_SECURE_ECDSA_PublicKey_P224, _aMessage_100k,        0u },
  { &_SECURE_ECDSA_PrivateKey_P224, &_SECURE_ECDSA_PublicKey_P224, _aMessage_100k,     1024u },
  { &_SECURE_ECDSA_PrivateKey_P224, &_SECURE_ECDSA_PublicKey_P224, _aMessage_100k, 100*1024u },
  { NULL,                           NULL,                          NULL,                  0u },
  { &_SECURE_ECDSA_PrivateKey_P256, &_SECURE_ECDSA_PublicKey_P256, _aMessage_100k,        0u },
  { &_SECURE_ECDSA_PrivateKey_P256, &_SECURE_ECDSA_PublicKey_P256, _aMessage_100k,     1024u },
  { &_SECURE_ECDSA_PrivateKey_P256, &_SECURE_ECDSA_PublicKey_P256, _aMessage_100k, 100*1024u },
  { NULL,                           NULL,                          NULL,                  0u },
  { &_SECURE_ECDSA_PrivateKey_P384, &_SECURE_ECDSA_PublicKey_P384, _aMessage_100k,        0u },
  { &_SECURE_ECDSA_PrivateKey_P384, &_SECURE_ECDSA_PublicKey_P384, _aMessage_100k,     1024u },
  { &_SECURE_ECDSA_PrivateKey_P384, &_SECURE_ECDSA_PublicKey_P384, _aMessage_100k, 100*1024u },
  { NULL,                           NULL,                          NULL,                  0u },
  { &_SECURE_ECDSA_PrivateKey_P521, &_SECURE_ECDSA_PublicKey_P521, _aMessage_100k,        0u },
  { &_SECURE_ECDSA_PrivateKey_P521, &_SECURE_ECDSA_PublicKey_P521, _aMessage_100k,     1024u },
  { &_SECURE_ECDSA_PrivateKey_P521, &_SECURE_ECDSA_PublicKey_P521, _aMessage_100k, 100*1024u },
};

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
  //
  SEGGER_SYS_IO_Printf("| %9s | %8u | ", pPara->pPublicKey->pCurve->aCurveName, pPara->MessageLen);
  //
  Loops = 0;
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    SignatureLen = SECURE_ECDSA_Sign(pPara->pPrivateKey,
                                     pPara->pMesssage, pPara->MessageLen,
                                     aSignature,       sizeof(aSignature));
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
      Status = SECURE_ECDSA_Verify(pPara->pPublicKey,
                                   pPara->pMesssage, pPara->MessageLen,
                                   aSignature,       SignatureLen);
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
  SECURE_ECDSA_Init();
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", SECURE_ECDSA_GetCopyrightText());
  SEGGER_SYS_IO_Printf("emSecure-ECDSA Performance Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed                   = %.3f MHz\n",
                                    (float)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                    = %u [%s]\n",
                                  CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   SECURE_ECDSA_VERSION              = %u [%s]\n",
                                  SECURE_ECDSA_VERSION, SECURE_ECDSA_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB          = %u\n",
                                  CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_ECDSA_TWIN_MULTIPLY = %u\n",
                                  CRYPTO_CONFIG_ECDSA_TWIN_MULTIPLY);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA256_OPTIMIZE     = %u\n",
                                  CRYPTO_CONFIG_SHA256_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   SECURE_ECDSA_MAX_KEY_LENGTH       = %u bits\n",
                                  SECURE_ECDSA_MAX_KEY_LENGTH);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Sign/Verify Performance\n");
  SEGGER_SYS_IO_Printf("=======================\n\n");
  //
  SEGGER_SYS_IO_Printf("+-----------+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("|     Curve |  Message |     Sign |   Verify |\n");
  SEGGER_SYS_IO_Printf("|           |   /bytes |      /ms |      /ms |\n");
  SEGGER_SYS_IO_Printf("+-----------+----------+----------+----------+\n");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    if (_aBenchKeys[i].pPublicKey == NULL) {
      SEGGER_SYS_IO_Printf("+-----------+----------+----------+----------+\n");
    } else {
      _BenchmarkSignVerify(&_aBenchKeys[i]);
    }
  }
  SEGGER_SYS_IO_Printf("+-----------+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
