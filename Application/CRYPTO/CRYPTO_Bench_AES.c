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

File        : CRYPTO_Bench_AES.c
Purpose     : Benchmark AES implementation.

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

static U8 _aTestMessage[1024] = { 0 };
static U8 _aAAD[13]           = { 0 };

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
*       _CipherBenchmark_ECB_CBC()
*
*  Function description
*    Benchmarks a cipher implementation.
*
*  Parameters
*    sAlgorithm - Cipher algorithm name.
*    pAPI       - Pointer to cipher API.
*    KeySize    - Cipher key size in bytes.
*/
static void _CipherBenchmark_ECB_CBC(const char *sAlgorithm, const CRYPTO_CIPHER_API *pAPI, unsigned KeySize) {
  CRYPTO_AES_CONTEXT Context;
  U64                T0;
  U64                OneSecond;
  U8                 aIV [16];
  U8                 aKey[32];
  unsigned           n;
  //
  CRYPTO_MEMZAP(aIV,  sizeof(aIV));
  CRYPTO_MEMZAP(aKey, sizeof(aKey));
  //
  SEGGER_SYS_IO_Printf("| %-12s | %4d | ", sAlgorithm, KeySize*8);
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  //
  // ECB encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_ECB_Encrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // ECB decrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitDecrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_ECB_Decrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f | ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CBC encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_CBC_Encrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), aIV, pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CBC decrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitDecrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_CBC_Decrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), aIV, pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f |\n", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
}

/*********************************************************************
*
*       _CipherBenchmark_GCM_CCM()
*
*  Function description
*    Benchmarks a cipher implementation.
*
*  Parameters
*    sAlgorithm - Cipher algorithm name.
*    pAPI       - Pointer to cipher API.
*    KeySize    - Cipher key size in bytes.
*/
static void _CipherBenchmark_GCM_CCM(const char *sAlgorithm, const CRYPTO_CIPHER_API *pAPI, unsigned KeySize) {
  CRYPTO_AES_CONTEXT Context;
  U64                T0;
  U64                OneSecond;
  U8                 aIV[12];
  U8                 aKey[32];
  U8                 aTag[16];
  unsigned           n;
  //
  CRYPTO_MEMZAP(aIV, sizeof(aIV));
  CRYPTO_MEMZAP(aKey, sizeof(aKey));
  //
  SEGGER_SYS_IO_Printf("| %-12s | %4d | ", sAlgorithm, KeySize*8);
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  //
  // GCM encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_AES_GCM_Encrypt(&Context, &_aTestMessage[0], &aTag[0], sizeof(aTag), &_aTestMessage[0], sizeof(_aTestMessage), _aAAD, sizeof(_aAAD), aIV, sizeof(aIV));
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // GCM decrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_AES_GCM_Decrypt(&Context, &_aTestMessage[0], &aTag[0], sizeof(aTag), &_aTestMessage[0], sizeof(_aTestMessage), _aAAD, sizeof(_aAAD), aIV, sizeof(aIV));
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f | ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CCM encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_AES_CCM_Encrypt(&Context, &_aTestMessage[0], &aTag[0], 16, &_aTestMessage[0], sizeof(_aTestMessage), _aAAD, sizeof(_aAAD), aIV, 12);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CCM decrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_AES_CCM_Decrypt(&Context, &_aTestMessage[0], &aTag[0], 16, &_aTestMessage[0], sizeof(_aTestMessage), _aAAD, sizeof(_aAAD), aIV, 12);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%7.2f |\n", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
}

/*********************************************************************
*
*       _GetHWAPI()
*
*  Function description
*    Returns hardware acceleration API for given key size.
*
*  Parameters
*    KeySize - Key size requested.
*
*  Return value
*    == 0 - No hardware API for the given key size.
*    != 0 - Hardware API for the given key size.
*/
static const CRYPTO_CIPHER_API *_GetHWAPI(unsigned KeySize) {
  const CRYPTO_CIPHER_API *pHWAPI;
  const CRYPTO_CIPHER_API *pSWAPI;
  const CRYPTO_CIPHER_API *pChosen;
  //
  pChosen = 0;
  CRYPTO_AES_QueryInstall(&pHWAPI, &pSWAPI);
  if (pHWAPI != &CRYPTO_CIPHER_AES_SW) {
    pChosen = pHWAPI->pfClaim(KeySize);
    if (pChosen) {
      pHWAPI ->pfUnclaim(0);
    }
  }
  return pChosen;
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
  const CRYPTO_CIPHER_API * pAssist;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("AES Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed               = %.3f MHz\n", SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_AES_OPTIMIZE    = %d\n", CRYPTO_CONFIG_AES_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_AES_HW_OPTIMIZE = %d\n", CRYPTO_CONFIG_AES_HW_OPTIMIZE);
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_GCM_OPTIMIZE    = %d\n", CRYPTO_CONFIG_GCM_OPTIMIZE);
  SEGGER_SYS_IO_Printf("\n");
  //  
  SEGGER_SYS_IO_Printf("+--------------+------+-----------------+-----------------+\n");
  SEGGER_SYS_IO_Printf("|              |      | ECB        MB/s | CBC        MB/s |\n");
  SEGGER_SYS_IO_Printf("| Cipher       | Bits |     Enc     Dec |     Enc     Dec |\n");
  SEGGER_SYS_IO_Printf("+--------------+------+-----------------+-----------------+\n");
  //
  _CipherBenchmark_ECB_CBC("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES128_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES128_KEY_SIZE)) != NULL) {
    _CipherBenchmark_ECB_CBC("AES (HW)", pAssist, CRYPTO_AES128_KEY_SIZE);
  }
  _CipherBenchmark_ECB_CBC("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES192_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES192_KEY_SIZE)) != NULL) {
    _CipherBenchmark_ECB_CBC("AES (HW)", pAssist, CRYPTO_AES192_KEY_SIZE);
  }
  _CipherBenchmark_ECB_CBC("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES256_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES256_KEY_SIZE)) != NULL) {
    _CipherBenchmark_ECB_CBC("AES (HW)", pAssist, CRYPTO_AES256_KEY_SIZE);
  }
  SEGGER_SYS_IO_Printf("+--------------+------+-----------------+-----------------+\n");
  SEGGER_SYS_IO_Printf("|              |      | GCM        MB/s | CCM        MB/s |\n");
  SEGGER_SYS_IO_Printf("| Cipher       | Bits |     Enc     Dec |     Enc     Dec |\n");
  SEGGER_SYS_IO_Printf("+--------------+------+-----------------+-----------------+\n");
  //
  _CipherBenchmark_GCM_CCM("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES128_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES128_KEY_SIZE)) != NULL) {
    _CipherBenchmark_GCM_CCM("AES (HW)", pAssist, CRYPTO_AES128_KEY_SIZE);
  }
  _CipherBenchmark_GCM_CCM("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES192_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES192_KEY_SIZE)) != NULL) {
    _CipherBenchmark_GCM_CCM("AES (HW)", pAssist, CRYPTO_AES192_KEY_SIZE);
  }
  _CipherBenchmark_GCM_CCM("AES", &CRYPTO_CIPHER_AES_SW, CRYPTO_AES256_KEY_SIZE);
  if ((pAssist = _GetHWAPI(CRYPTO_AES256_KEY_SIZE)) != NULL) {
    _CipherBenchmark_GCM_CCM("AES (HW)", pAssist, CRYPTO_AES256_KEY_SIZE);
  }
  //
  SEGGER_SYS_IO_Printf("+--------------+------+-----------------+-----------------+\n");
  //
  SEGGER_SYS_IO_Printf("\nBenchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
