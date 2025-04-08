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

File        : CRYPTO_Bench_SHA1.c
Purpose     : Benchmark SHA-1 implementation.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "CRYPTO.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const U8 _aTestMessage[65536] = { 0 };

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
static double _ConvertTicksToSeconds(U64 Ticks) {
  return SEGGER_SYS_OS_ConvertTicksToMicros(Ticks) / 1000000.0;
}

/*********************************************************************
*
*       _HashBenchmark()
*
*  Function description
*    Benchmarks a hash implementation.
*
*  Parameters
*    sAlgorithm - Hash algorithm name.
*    pAPI       - Pointer to hash API.
*/
static void _HashBenchmark(const char *sAlgorithm, const CRYPTO_HASH_API *pAPI) {
  CRYPTO_SHA512_CONTEXT C; // big enough for most things...
  U64                   T0;
  U64                   OneSecond;
  unsigned              n;
  //
  SEGGER_SYS_IO_Printf("| %-12s | ", sAlgorithm);
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  if (pAPI->pfClaim) {
    pAPI->pfClaim();
  }
  pAPI->pfInit(&C);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    pAPI->pfAdd(&C, &_aTestMessage[0], sizeof(_aTestMessage));
    n += sizeof(_aTestMessage);
  }
  pAPI->pfKill(&C);
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%9.2f |\n", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
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
  const CRYPTO_HASH_API * pHWAPI;
  const CRYPTO_HASH_API * pSWAPI;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("SHA-1 Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed                = %.3f MHz\n", (double)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                 = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA1_OPTIMIZE    = %d\n",      CRYPTO_CONFIG_SHA1_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA1_HW_OPTIMIZE = %d\n\n",    CRYPTO_CONFIG_SHA1_HW_OPTIMIZE);
  //
  SEGGER_SYS_IO_Printf("+--------------+-----------+\n");
  SEGGER_SYS_IO_Printf("| Algorithm    | Hash MB/s |\n");
  SEGGER_SYS_IO_Printf("+--------------+-----------+\n");
  //
  _HashBenchmark("SHA-1", &CRYPTO_HASH_SHA1_SW);
  CRYPTO_SHA1_QueryInstall(&pHWAPI, &pSWAPI);
  if (pHWAPI && pHWAPI != &CRYPTO_HASH_SHA1_SW) {
    _HashBenchmark("SHA-1 (HW)", pHWAPI);
  }
  SEGGER_SYS_IO_Printf("+--------------+-----------+\n");
  //
  SEGGER_SYS_IO_Printf("\nBenchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
