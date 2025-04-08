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

File        : CRYPTO_Bench_PointMul.c
Purpose     : Benchmark EC point multiplication.

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

#define MAX_CHUNKS             244

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

typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*521+63)+2];

typedef int (*POINTMUL_FUNC)(CRYPTO_EC_POINT *pSelf, const CRYPTO_MPI *pK, const CRYPTO_EC_CURVE *pCurve, CRYPTO_MEM_CONTEXT *pMem);

typedef struct {
  const char    * pText;          // Description of algorithm
  POINTMUL_FUNC   pfPointMul;
} BENCH_ALG;

typedef struct {
  const CRYPTO_EC_CURVE * pCurve;
  const char            * sName;
} BENCH_KEY;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*       Benchmark parameterization.
*/

static const BENCH_ALG _aBenchAlgs[] = {
  { "Binary, Basic", CRYPTO_EC_Mul_Basic  },
  { NULL,            NULL                 },
  { "2b, FW",        CRYPTO_EC_Mul_2b_FW  },
  { "3b, FW",        CRYPTO_EC_Mul_3b_FW  },
  { "4b, FW",        CRYPTO_EC_Mul_4b_FW  },
  { "5b, FW",        CRYPTO_EC_Mul_5b_FW  },
  { "6b, FW",        CRYPTO_EC_Mul_6b_FW  },
  { NULL,            NULL                 },
  { "2b, RM",        CRYPTO_EC_Mul_2b_RM  },
  { "3b, RM",        CRYPTO_EC_Mul_3b_RM  },
  { "4b, RM",        CRYPTO_EC_Mul_4b_RM  },
  { "5b, RM",        CRYPTO_EC_Mul_5b_RM  },
  { "6b, RM",        CRYPTO_EC_Mul_6b_RM  },
  { NULL,            NULL                 },
  { "2b, NAF",       CRYPTO_EC_Mul_2w_NAF },
  { "3b, NAF",       CRYPTO_EC_Mul_3w_NAF },
  { "4b, NAF",       CRYPTO_EC_Mul_4w_NAF },
  { "5b, NAF",       CRYPTO_EC_Mul_5w_NAF },
  { "6b, NAF",       CRYPTO_EC_Mul_6w_NAF },
  { NULL,            NULL                 },
  { "Configured",    CRYPTO_EC_Mul        }
};

static const CRYPTO_EC_CURVE * _aPrimeCurves[] = {
  &CRYPTO_EC_CURVE_secp192r1,
  &CRYPTO_EC_CURVE_secp224r1,
  &CRYPTO_EC_CURVE_secp256r1,
  &CRYPTO_EC_CURVE_secp384r1,
  &CRYPTO_EC_CURVE_secp521r1
};

static const CRYPTO_EC_CURVE * _aKoblitzCurves[] = {
  &CRYPTO_EC_CURVE_secp192k1,
  &CRYPTO_EC_CURVE_secp224k1,
  &CRYPTO_EC_CURVE_secp256k1
};

static const CRYPTO_EC_CURVE * _aBrainpoolCurves[] = {
//&CRYPTO_EC_CURVE_brainpoolP160r1,
//&CRYPTO_EC_CURVE_brainpoolP192r1,
  &CRYPTO_EC_CURVE_brainpoolP224r1,
  &CRYPTO_EC_CURVE_brainpoolP256r1,
  &CRYPTO_EC_CURVE_brainpoolP320r1,
  &CRYPTO_EC_CURVE_brainpoolP384r1,
  &CRYPTO_EC_CURVE_brainpoolP512r1
};

static const CRYPTO_EC_CURVE * _aBrainpoolTwistedCurves[] = {
//&CRYPTO_EC_CURVE_brainpoolP160t1,
//&CRYPTO_EC_CURVE_brainpoolP192t1,
  &CRYPTO_EC_CURVE_brainpoolP224t1,
  &CRYPTO_EC_CURVE_brainpoolP256t1,
  &CRYPTO_EC_CURVE_brainpoolP320t1,
  &CRYPTO_EC_CURVE_brainpoolP384t1,
  &CRYPTO_EC_CURVE_brainpoolP512t1
};

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
static unsigned                 _AlgIndex;
static unsigned                 _KeyIndex;
static float                    _aBaseline[SEGGER_COUNTOF(_aBrainpoolCurves)];

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
*       _BenchmarkSinglePointMul()
*
*  Function description
*    Count the number of point multiplications completed in one second.
*
*  Parameters
*    pfPointMul - Pointer to point multiply implementation.
*    pCurve     - Pointer to EC group.
*/
static void _BenchmarkSinglePointMul(POINTMUL_FUNC pfPointMul, const CRYPTO_EC_CURVE *pCurve) {
  CRYPTO_EC_POINT Point;
  CRYPTO_MPI      Scalar;
  U64             OneSecond;
  U64             T0;
  U64             Elapsed;
  int             Loops;
  int             Status;
  unsigned        PeakBytes;
  float           Multiplier;
  float           Time;
  //
  PeakBytes = 0;
  Loops     = 0;
  //
  CRYPTO_EC_InitPoint(&Point,  &_MemContext);
  CRYPTO_MPI_Init    (&Scalar, &_MemContext);
  //
  CRYPTO_EC_AssignPoint(&Point, &pCurve->G);
  CRYPTO_EC_MakeProjective(&Point);
  CRYPTO_MPI_LoadHex(&Scalar, "3243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89452821E638D01377BE5466CF34E90C6CC0AC29B7C97C50DD3F84D5B5B54709179216D5D9897", NULL);  // Hex digits of Pi
  CRYPTO_MPI_TrimBits(&Scalar, CRYPTO_MPI_BitCount(&pCurve->P)-1);
  //
  // Count number of modular exponentiations completed in 1s.
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    CRYPTO_CHECK(pfPointMul(&Point, &Scalar, pCurve, &_MemContext));
    PeakBytes = _Heap.Stats.NumInUseMax * sizeof(MPI_UNIT);
    ++Loops;
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
  } while (Elapsed < OneSecond);
  //
Finally:
  Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
  //
  CRYPTO_EC_KillPoint(&Point);
  CRYPTO_MPI_Kill(&Scalar);
  //
  Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
  if (_AlgIndex == 0) {
    _aBaseline[_KeyIndex] = Time;
  } 
  Multiplier = _aBaseline[_KeyIndex] / (float)Time;
  if (Status < 0) {
    SEGGER_SYS_IO_Printf("         -Fail- |");
  } else {
    if (_ShowMemory) {
      SEGGER_SYS_IO_Printf("       %8d |", PeakBytes);
    } else {
      SEGGER_SYS_IO_Printf("%9.2f %4.2fx |", Time, Multiplier);
    }
  }
}

/*********************************************************************
*
*       _PrintFooter()
*
*  Function description
*    Print column footer for table.
*
*  Parameters
*    NumCurves - Number of curves in group.
*/
static void _PrintFooter(unsigned NumCurves) {
  unsigned i;
  //
  SEGGER_SYS_IO_Printf("+-----------------+");
  for (i = 0; i < NumCurves; ++i) {
    SEGGER_SYS_IO_Printf("----------------+");
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _PrintHeader()
*
*  Function description
*    Print column headers for table.
*
*  Parameters
*    ppCurve   - Pointer to curve group array.
*    NumCurves - Number of curves in group.
*/
static void _PrintHeader(const CRYPTO_EC_CURVE * const * const ppCurve, unsigned NumCurves) {
  unsigned i;
  //
  SEGGER_SYS_IO_Printf("+-----------------+");
  for (i = 0; i < NumCurves; ++i) {
    SEGGER_SYS_IO_Printf("----------------+");
  }
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("| Algorithm       |");
  for (i = 0; i < NumCurves; ++i) {
    SEGGER_SYS_IO_Printf("%15s |", ppCurve[i]->aCurveName);
  }
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("+-----------------+");
  for (i = 0; i < NumCurves; ++i) {
    SEGGER_SYS_IO_Printf("----------------+");
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _BenchmarkGroup()
*
*  Function description
*    Benchmark a group of curves.
*
*  Parameters
*    sGroupName - Zero-terminate group name.
*    pCurve     - Pointer to curve group array.
*    NumCurves  - Number of curves in group.
*/
static void _BenchmarkGroup(const char *sGroupName, const CRYPTO_EC_CURVE * const * const pCurve, unsigned NumCurves) {
  SEGGER_SYS_IO_Printf("*** %s ***\n\n", sGroupName);
  //
  _PrintHeader(pCurve, NumCurves);
  for (_AlgIndex = 0; _AlgIndex < SEGGER_COUNTOF(_aBenchAlgs); ++_AlgIndex) {
    if (_aBenchAlgs[_AlgIndex].pfPointMul == NULL) {
      _PrintFooter(NumCurves);
    } else {
      SEGGER_SYS_IO_Printf("| %-15s |", _aBenchAlgs[_AlgIndex].pText);
      for (_KeyIndex = 0; _KeyIndex < NumCurves; ++_KeyIndex) {
        _BenchmarkSinglePointMul(_aBenchAlgs[_AlgIndex].pfPointMul, pCurve[_KeyIndex]);
      }
      SEGGER_SYS_IO_Printf("\n");
    }
  }
  _PrintFooter(NumCurves);
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
  SEGGER_SYS_IO_Printf("Point Multiplication Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
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
  SEGGER_SYS_IO_Printf("All times in ms\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  _BenchmarkGroup("Prime curves",             _aPrimeCurves,            SEGGER_COUNTOF(_aPrimeCurves));
  _BenchmarkGroup("Koblitz curves",           _aKoblitzCurves,          SEGGER_COUNTOF(_aKoblitzCurves));
  _BenchmarkGroup("Brainpool curves",         _aBrainpoolCurves,        SEGGER_COUNTOF(_aBrainpoolCurves));
  _BenchmarkGroup("Twisted Brainpool curves", _aBrainpoolTwistedCurves, SEGGER_COUNTOF(_aBrainpoolTwistedCurves));
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
