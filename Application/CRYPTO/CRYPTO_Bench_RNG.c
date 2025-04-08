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

File        : CRYPTO_Bench_RNG.c
Purpose     : Benchmark RNG (DRBG) implementation.

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
*       Static data
*
**********************************************************************
*/

static U8                     _aTestMessage[65536];

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
*       _RNG_Benchmark()
*
*  Function description
*    Benchmarks a RNG implementation.
*
*  Parameters
*    sAlgorithm - RNG algorithm name.
*    pAPI       - Pointer to RNG API.
*/
static void _RNG_Benchmark(const char *sAlgorithm, const CRYPTO_RNG_API *pAPI) {
  U64      T0;
  U64      OneSecond;
  unsigned n;
  //
  SEGGER_SYS_IO_Printf("| %-21s | ", sAlgorithm);
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  //
  // ECB encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  if (pAPI->pfInit != NULL) {
    pAPI->pfInit();
  }
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    pAPI->pfGet(&_aTestMessage[0], sizeof(_aTestMessage));
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f |\n", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
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
  const CRYPTO_RNG_API *pEntropyAPI;
  const CRYPTO_RNG_API *pRngAPI;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  //
  CRYPTO_RNG_QueryInstallEx(&pRngAPI, &pEntropyAPI);
  if (pEntropyAPI == NULL) {
    SEGGER_SYS_IO_Printf("\nBenchmark complete\n");
    SEGGER_SYS_OS_PauseBeforeHalt();
    SEGGER_SYS_OS_Halt(100);
  }
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("RNG Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed               = %.3f MHz\n", (double)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA1_OPTIMIZE   = %d\n", CRYPTO_CONFIG_SHA1_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA256_OPTIMIZE = %d\n", CRYPTO_CONFIG_SHA256_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_SHA512_OPTIMIZE = %d\n", CRYPTO_CONFIG_SHA512_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_DES_OPTIMIZE    = %d\n", CRYPTO_CONFIG_DES_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_AES_OPTIMIZE    = %d\n", CRYPTO_CONFIG_AES_OPTIMIZE);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("RNG Performance\n");
  SEGGER_SYS_IO_Printf("===============\n\n");
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  SEGGER_SYS_IO_Printf("| Algorithm             |    MB/s |\n");
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  //
  _RNG_Benchmark("Entropy source",        pEntropyAPI);
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  if (CRYPTO_SHA1_IsInstalled()) {
    _RNG_Benchmark("Hash_DRBG-SHA-1",       &CRYPTO_RNG_DRBG_HASH_SHA1);
  }
  if (CRYPTO_SHA224_IsInstalled()) {
    _RNG_Benchmark("Hash_DRBG-SHA-224",     &CRYPTO_RNG_DRBG_HASH_SHA224);
  }
  if (CRYPTO_SHA256_IsInstalled()) {
    _RNG_Benchmark("Hash_DRBG-SHA-256",     &CRYPTO_RNG_DRBG_HASH_SHA256);
  }
  if (CRYPTO_SHA512_IsInstalled()) {
    _RNG_Benchmark("Hash_DRBG-SHA-384",     &CRYPTO_RNG_DRBG_HASH_SHA384);
    _RNG_Benchmark("Hash_DRBG-SHA-512",     &CRYPTO_RNG_DRBG_HASH_SHA512);
    _RNG_Benchmark("Hash_DRBG-SHA-512/224", &CRYPTO_RNG_DRBG_HASH_SHA512_224);
    _RNG_Benchmark("Hash_DRBG-SHA-512/256", &CRYPTO_RNG_DRBG_HASH_SHA512_256);
  }
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  if (CRYPTO_SHA1_IsInstalled()) {
    _RNG_Benchmark("HMAC_DRBG-SHA-1",       &CRYPTO_RNG_DRBG_HMAC_SHA1);
  }
  if (CRYPTO_SHA224_IsInstalled()) {
    _RNG_Benchmark("HMAC_DRBG-SHA-224",     &CRYPTO_RNG_DRBG_HMAC_SHA224);
  }
  if (CRYPTO_SHA256_IsInstalled()) {
    _RNG_Benchmark("HMAC_DRBG-SHA-256",     &CRYPTO_RNG_DRBG_HMAC_SHA256);
  }
  if (CRYPTO_SHA512_IsInstalled()) {
    _RNG_Benchmark("HMAC_DRBG-SHA-384",     &CRYPTO_RNG_DRBG_HMAC_SHA384);
    _RNG_Benchmark("HMAC_DRBG-SHA-512",     &CRYPTO_RNG_DRBG_HMAC_SHA512);
    _RNG_Benchmark("HMAC_DRBG-SHA-512/224", &CRYPTO_RNG_DRBG_HMAC_SHA512_224);
    _RNG_Benchmark("HMAC_DRBG-SHA-512/256", &CRYPTO_RNG_DRBG_HMAC_SHA512_256);
  }
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  if (CRYPTO_TDES_IsInstalled()) {
    _RNG_Benchmark("CTR-DRBG-TDES",         &CRYPTO_RNG_DRBG_CTR_TDES);
  }
  if (CRYPTO_AES_IsInstalled()) {
    _RNG_Benchmark("CTR-DRBG-AES-128",      &CRYPTO_RNG_DRBG_CTR_AES128);
    _RNG_Benchmark("CTR-DRBG-AES-192",      &CRYPTO_RNG_DRBG_CTR_AES192);
    _RNG_Benchmark("CTR-DRBG-AES-256",      &CRYPTO_RNG_DRBG_CTR_AES256);
  }
  //
  SEGGER_SYS_IO_Printf("+-----------------------+---------+\n");
  //
  SEGGER_SYS_IO_Printf("\nBenchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
