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

File        : SECURE_RSA_Bench_Sign.c
Purpose     : Benchmark emSecure-RSA sign operation.

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
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const CRYPTO_RSA_PRIVATE_KEY * const _aBenchKeys[] = {
  &_SECURE_RSA_PrivateKey_512b,
  &_SECURE_RSA_PrivateKey_1024b,
  &_SECURE_RSA_PrivateKey_2048b
};

static const U8 _aMessage_100k[100*1024] = {
  0x00,
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
*       _BenchmarkSign()
*
*  Function description
*    Count the number of signings completed in one second.
*
*  Parameters
*    pKey       - Pointer to RSA private key.
*    pMessage   - Pointer to message to sign.
*    MessageLen - Octet length of the message.
*/
static void _BenchmarkSign(const CRYPTO_RSA_PRIVATE_KEY *pKey, const U8 *pMessage, unsigned MessageLen) {
  U64   OneSecond;
  U64   T0;
  U64   Elapsed;
  int   Loops;
  int   Status;
  float Time;
  U8    aSignature[270];
  //
  Loops = 0;
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    Status = SECURE_RSA_Sign(pKey, NULL, 0, pMessage, MessageLen, aSignature, sizeof(aSignature));
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
  Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
  if (Status < 0) {
    SEGGER_SYS_IO_Printf("| %8d | %8u | %8s |\n", CRYPTO_MPI_BitCount(&pKey->N), MessageLen, "-Fail-");
  } else {
    SEGGER_SYS_IO_Printf("| %8d | %8u | %8.2f |\n", CRYPTO_MPI_BitCount(&pKey->N), MessageLen, Time);
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
  SEGGER_SYS_IO_Printf("emSecure-RSA Sign Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed           = %.3f MHz\n", (float)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION            = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_VERSION        = %u [%s]\n", SECURE_RSA_VERSION, SECURE_RSA_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB  = %u\n",      CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("Config:   SECURE_RSA_MAX_KEY_LENGTH = %u bits\n", SECURE_RSA_MAX_KEY_LENGTH);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Signing Performance\n");
  SEGGER_SYS_IO_Printf("===================\n\n");
  //
  SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("|  Modulus |  Message |     Sign |\n");
  SEGGER_SYS_IO_Printf("|    /bits |   /bytes |      /ms |\n");
  SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    _BenchmarkSign(_aBenchKeys[i], _aMessage_100k, 0);
    _BenchmarkSign(_aBenchKeys[i], _aMessage_100k, 1024);
    _BenchmarkSign(_aBenchKeys[i], _aMessage_100k, 100*1024u);
    SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  }
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
