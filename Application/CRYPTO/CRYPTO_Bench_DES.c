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

File        : CRYPTO_Bench_DES.c
Purpose     : Benchmark DES implementation.

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

static const U8 _aKey[32] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U8 _aTestMessage[65536] = { 0 };

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
  CRYPTO_TDES_CONTEXT Context;
  U8                  aIV[16];
  U64                 T0;
  U64                 OneSecond;
  unsigned            n;
  //
  SEGGER_SYS_IO_Printf("| %-12s | %4d | ", sAlgorithm, KeySize*8);
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, _aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    pAPI->pfEncrypt(&Context, &_aTestMessage[0], &_aTestMessage[0]);
    n += pAPI->BlockSize;
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%6.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitDecrypt(&Context, _aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    pAPI->pfDecrypt(&Context, &_aTestMessage[0], &_aTestMessage[0]);
    n += pAPI->BlockSize;
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%6.2f | ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CBC encrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitEncrypt(&Context, _aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_CBC_Encrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), aIV, pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%6.2f ", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
  //
  // CBC decrypt
  //
  T0 = SEGGER_SYS_OS_GetTimer();
  n = 0;
  pAPI->pfInitDecrypt(&Context, _aKey, KeySize);
  while (SEGGER_SYS_OS_GetTimer() - T0 < OneSecond) {
    CRYPTO_CIPHER_CBC_Decrypt(&Context, &_aTestMessage[0], &_aTestMessage[0], sizeof(_aTestMessage), aIV, pAPI);
    n += sizeof(_aTestMessage);
  }
  T0 = SEGGER_SYS_OS_GetTimer() - T0;
  SEGGER_SYS_IO_Printf("%6.2f |\n", (double)n / (1024.0*1024.0) / _ConvertTicksToSeconds(T0));
}

/*********************************************************************
*
*       _GetTDESHardwareAssist()
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
static const CRYPTO_CIPHER_API *_GetTDESHardwareAssist(unsigned KeySize) {
  const CRYPTO_CIPHER_API *pHWAPI;
  const CRYPTO_CIPHER_API *pSWAPI;
  const CRYPTO_CIPHER_API *pChosen;
  //
  pChosen = 0;
  CRYPTO_TDES_QueryInstall(&pHWAPI, &pSWAPI);
  if (pHWAPI != &CRYPTO_CIPHER_TDES_SW) {
    pChosen = pHWAPI->pfClaim(KeySize);
    if (pChosen) {
      pHWAPI->pfUnclaim(0);
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
  SEGGER_SYS_IO_Printf("DES Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed               = %.3f MHz\n", SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION                = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_CONFIG_DES_OPTIMIZE    = %d\n",      CRYPTO_CONFIG_DES_OPTIMIZE);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("+--------------+------+---------------+---------------+\n");
  SEGGER_SYS_IO_Printf("|              |      | ECB      MB/s | CBC      MB/s |\n");
  SEGGER_SYS_IO_Printf("| Cipher       | Bits |    Enc    Dec |    Enc    Dec |\n");
  SEGGER_SYS_IO_Printf("+--------------+------+---------------+---------------+\n");
  //
 _CipherBenchmark_ECB_CBC("DES", &CRYPTO_CIPHER_TDES_SW, CRYPTO_TDES_1KEY_SIZE);
  if ((pAssist = _GetTDESHardwareAssist(CRYPTO_TDES_1KEY_SIZE)) != 0) {
    _CipherBenchmark_ECB_CBC("DES (HW)", pAssist, CRYPTO_TDES_1KEY_SIZE);
  }
  _CipherBenchmark_ECB_CBC("DES", &CRYPTO_CIPHER_TDES_SW, CRYPTO_TDES_2KEY_SIZE);
  if ((pAssist = _GetTDESHardwareAssist(CRYPTO_TDES_2KEY_SIZE)) != 0) {
    _CipherBenchmark_ECB_CBC("DES (HW)", pAssist, CRYPTO_TDES_2KEY_SIZE);
  }
  _CipherBenchmark_ECB_CBC("DES", &CRYPTO_CIPHER_TDES_SW, CRYPTO_TDES_3KEY_SIZE);
  if ((pAssist = _GetTDESHardwareAssist(CRYPTO_TDES_3KEY_SIZE)) != 0) {
    _CipherBenchmark_ECB_CBC("DES (HW)", pAssist, CRYPTO_TDES_3KEY_SIZE);
  }
  SEGGER_SYS_IO_Printf("+--------------+------+---------------+---------------+\n");
  //
  SEGGER_SYS_IO_Printf("\n* Note: key sizes include parity bits\n");
  //
  SEGGER_SYS_IO_Printf("\nBenchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
