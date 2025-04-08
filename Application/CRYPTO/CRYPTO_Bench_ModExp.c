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

File        : CRYPTO_Bench_ModExp.c
Purpose     : Benchmark modular exponentiation.

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

#define INCLUDE_SMALL_MODULI   0
#define INCLUDE_PLAIN_PRIVATE  0                  // Include plain private key operations [not really used in practice]
#define INCLUDE_EFM32          0                  // Include EFM32 benchmarks using CRYPTO unit
#define MAX_CHUNKS             75

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
*       Defines, fixed
*
**********************************************************************
*/

#if INCLUDE_EFM32
  #define EFM32(X, Y, Z) X, Y, Z
#else
  #define EFM32(X, Y, Z)
#endif

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

#if INCLUDE_EFM32
typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*2048)+4*128/32];  // 128-bit megadigit implementation
#else
typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*2048)+4];
#endif

typedef int  (*MODEXP_FUNC)(CRYPTO_MPI *pSelf, const CRYPTO_MPI *pExponent, const CRYPTO_MPI *pModulus, SEGGER_MEM_CONTEXT *pMem);

typedef struct {
  const char * pText;          // Description of algorithm
  MODEXP_FUNC  pfModExp;
} BENCH_ALG;

typedef struct {
  const CRYPTO_MPI *pN;
  const CRYPTO_MPI *pE;
  const CRYPTO_MPI *pP;
  const CRYPTO_MPI *pQ;
  const CRYPTO_MPI *pDP;
  const CRYPTO_MPI *pDQ;
  const CRYPTO_MPI *pQInv;
} BENCH_KEY;

typedef void (*BENCH_FUNC)(MODEXP_FUNC pfModExp, const BENCH_KEY *pKey);

typedef struct {
  const char * pText;          // Description of scenario
  BENCH_FUNC   pfBenchFunc;
} BENCH_SCENARIO;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

#if INCLUDE_PLAIN_PRIVATE
static void _BenchmarkModExp_Private_Plain(MODEXP_FUNC pfModExp, const BENCH_KEY *pKey);
#endif
static void _BenchmarkModExp_Private_CRT  (MODEXP_FUNC pfModExp, const BENCH_KEY * pKey);
static void _BenchmarkModExp_Public       (MODEXP_FUNC pfModExp, const BENCH_KEY *pKey);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

#if INCLUDE_SMALL_MODULI

/*********************************************************************
*
*       128-bit modulus
*/

__MPI_LITERAL_BEGIN(static, _RSA128_N)
  __MPI_LITERAL_DATA(0x37, 0x28, 0x3b, 0x11),
  __MPI_LITERAL_DATA(0x68, 0x8d, 0xe5, 0x7c),
  __MPI_LITERAL_DATA(0x47, 0x41, 0x65, 0x41),
  __MPI_LITERAL_DATA(0x22, 0x25, 0xdf, 0xb2)
__MPI_LITERAL_END(static, _RSA128_N, 128)

__MPI_LITERAL_BEGIN(static, _RSA128_E)
  __MPI_LITERAL_DATA(0x01, 0x00, 0x01, 0x00)
__MPI_LITERAL_END(static, _RSA128_E, 17)

__MPI_LITERAL_BEGIN(static, _RSA128_P)
  __MPI_LITERAL_DATA(0x63, 0x30, 0x6a, 0x78),
  __MPI_LITERAL_DATA(0x5e, 0x2c, 0xf8, 0xc2)
__MPI_LITERAL_END(static, _RSA128_P, 64)

__MPI_LITERAL_BEGIN(static, _RSA128_Q)
  __MPI_LITERAL_DATA(0x1d, 0xaf, 0x60, 0xdb),
  __MPI_LITERAL_DATA(0x2f, 0xfb, 0xdc, 0xea)
__MPI_LITERAL_END(static, _RSA128_Q, 64)

__MPI_LITERAL_BEGIN(static, _RSA128_DP)
  __MPI_LITERAL_DATA(0xd1, 0xb2, 0x3b, 0x5f),
  __MPI_LITERAL_DATA(0x62, 0x33, 0x5f, 0xc1)
__MPI_LITERAL_END(static, _RSA128_DP, 64)

__MPI_LITERAL_BEGIN(static, _RSA128_DQ)
  __MPI_LITERAL_DATA(0x85, 0x66, 0x35, 0x1e),
  __MPI_LITERAL_DATA(0xe3, 0xfc, 0xd2, 0x74)
__MPI_LITERAL_END(static, _RSA128_DQ, 63)

__MPI_LITERAL_BEGIN(static, _RSA128_QINV)
  __MPI_LITERAL_DATA(0x38, 0xf1, 0xdc, 0x71),
  __MPI_LITERAL_DATA(0x4e, 0x0a, 0xec, 0xa4)
__MPI_LITERAL_END(static, _RSA128_QINV, 64)

/*********************************************************************
*
*       256-bit modulus
*/

__MPI_LITERAL_BEGIN(static, _RSA256_N)
  __MPI_LITERAL_DATA(0xef, 0xcc, 0xb1, 0x30),
  __MPI_LITERAL_DATA(0xc7, 0x06, 0x36, 0xaf),
  __MPI_LITERAL_DATA(0xe7, 0x14, 0xc9, 0x14),
  __MPI_LITERAL_DATA(0x3c, 0xc0, 0x0f, 0x48),
  __MPI_LITERAL_DATA(0x4b, 0xc6, 0xa6, 0xab),
  __MPI_LITERAL_DATA(0xb3, 0x62, 0xbb, 0x52),
  __MPI_LITERAL_DATA(0xa4, 0xa0, 0xbb, 0x1a),
  __MPI_LITERAL_DATA(0x26, 0xcf, 0xb8, 0x9d)
__MPI_LITERAL_END(static, _RSA256_N, 256)

__MPI_LITERAL_BEGIN(static, _RSA256_E)
  __MPI_LITERAL_DATA(0x01, 0x00, 0x01, 0x00)
__MPI_LITERAL_END(static, _RSA256_E, 17)

__MPI_LITERAL_BEGIN(static, _RSA256_P)
  __MPI_LITERAL_DATA(0x73, 0x10, 0x31, 0xa2),
  __MPI_LITERAL_DATA(0xfe, 0xd9, 0xf7, 0x90),
  __MPI_LITERAL_DATA(0x53, 0x2f, 0x0e, 0x4b),
  __MPI_LITERAL_DATA(0x9d, 0x50, 0xa9, 0xc3)
__MPI_LITERAL_END(static, _RSA256_P, 128)

__MPI_LITERAL_BEGIN(static, _RSA256_Q)
  __MPI_LITERAL_DATA(0x95, 0x5e, 0x43, 0xa8),
  __MPI_LITERAL_DATA(0x96, 0xef, 0x23, 0x3a),
  __MPI_LITERAL_DATA(0x3a, 0xd3, 0x69, 0x41),
  __MPI_LITERAL_DATA(0xfe, 0x52, 0x5c, 0xce)
__MPI_LITERAL_END(static, _RSA256_Q, 128)

__MPI_LITERAL_BEGIN(static, _RSA256_DP)
  __MPI_LITERAL_DATA(0x99, 0x7c, 0x4c, 0x04),
  __MPI_LITERAL_DATA(0xe8, 0x1f, 0xcd, 0x61),
  __MPI_LITERAL_DATA(0xc8, 0x5e, 0x98, 0xcc),
  __MPI_LITERAL_DATA(0xcb, 0xbd, 0x0e, 0xbe)
__MPI_LITERAL_END(static, _RSA256_DP, 128)

__MPI_LITERAL_BEGIN(static, _RSA256_DQ)
  __MPI_LITERAL_DATA(0x39, 0xb0, 0x00, 0x8e),
  __MPI_LITERAL_DATA(0xa4, 0x1e, 0x06, 0xa5),
  __MPI_LITERAL_DATA(0xfe, 0xe6, 0x55, 0x09),
  __MPI_LITERAL_DATA(0x70, 0x1f, 0xa8, 0x0b)
__MPI_LITERAL_END(static, _RSA256_DQ, 124)

__MPI_LITERAL_BEGIN(static, _RSA256_QINV)
  __MPI_LITERAL_DATA(0x82, 0x19, 0xf4, 0x2b),
  __MPI_LITERAL_DATA(0xea, 0x4f, 0xe1, 0xa8),
  __MPI_LITERAL_DATA(0x4b, 0x3b, 0x3f, 0xb1),
  __MPI_LITERAL_DATA(0xe0, 0xc1, 0xb8, 0x94)
__MPI_LITERAL_END(static, _RSA256_QINV, 128)

/*********************************************************************
*
*       512-bit modulus
*/

__MPI_LITERAL_BEGIN(static, _RSA512_N)
  __MPI_LITERAL_DATA(0x59, 0xae, 0x18, 0x11),
  __MPI_LITERAL_DATA(0xc4, 0x8a, 0xe4, 0x73),
  __MPI_LITERAL_DATA(0x24, 0xfd, 0xf3, 0x08),
  __MPI_LITERAL_DATA(0x40, 0x9b, 0x6b, 0x4e),
  __MPI_LITERAL_DATA(0x07, 0x01, 0x94, 0x87),
  __MPI_LITERAL_DATA(0xd8, 0xbf, 0x28, 0x45),
  __MPI_LITERAL_DATA(0x85, 0x8b, 0x65, 0x10),
  __MPI_LITERAL_DATA(0x8f, 0x82, 0x8a, 0x38),
  __MPI_LITERAL_DATA(0x12, 0x6a, 0xb1, 0x48),
  __MPI_LITERAL_DATA(0x09, 0x44, 0xf5, 0xd4),
  __MPI_LITERAL_DATA(0xa9, 0x62, 0x76, 0xd2),
  __MPI_LITERAL_DATA(0x5b, 0xa5, 0x10, 0x15),
  __MPI_LITERAL_DATA(0x9b, 0xa5, 0xa6, 0x36),
  __MPI_LITERAL_DATA(0xf8, 0x8e, 0xbe, 0x5b),
  __MPI_LITERAL_DATA(0x17, 0x59, 0x63, 0x44),
  __MPI_LITERAL_DATA(0xe4, 0x23, 0x35, 0xbe)
__MPI_LITERAL_END(static, _RSA512_N, 512)

__MPI_LITERAL_BEGIN(static, _RSA512_E)
  __MPI_LITERAL_DATA(0x01, 0x00, 0x01, 0x00)
__MPI_LITERAL_END(static, _RSA512_E, 17)

__MPI_LITERAL_BEGIN(static, _RSA512_P)
  __MPI_LITERAL_DATA(0xc1, 0x81, 0x46, 0x93),
  __MPI_LITERAL_DATA(0xf4, 0x07, 0x58, 0xcb),
  __MPI_LITERAL_DATA(0x90, 0x25, 0x97, 0x29),
  __MPI_LITERAL_DATA(0x65, 0x38, 0x27, 0x35),
  __MPI_LITERAL_DATA(0xf4, 0xe3, 0xd6, 0x51),
  __MPI_LITERAL_DATA(0x0e, 0xf5, 0x88, 0x7d),
  __MPI_LITERAL_DATA(0xda, 0x87, 0x3f, 0xd0),
  __MPI_LITERAL_DATA(0x28, 0x43, 0x6a, 0xc9)
__MPI_LITERAL_END(static, _RSA512_P, 256)

__MPI_LITERAL_BEGIN(static, _RSA512_Q)
  __MPI_LITERAL_DATA(0x99, 0xa2, 0x19, 0x42),
  __MPI_LITERAL_DATA(0xcf, 0x0a, 0x50, 0xbf),
  __MPI_LITERAL_DATA(0xec, 0x20, 0x03, 0x5b),
  __MPI_LITERAL_DATA(0x7b, 0x90, 0x11, 0x4c),
  __MPI_LITERAL_DATA(0x92, 0x18, 0xe9, 0x1a),
  __MPI_LITERAL_DATA(0xbd, 0x67, 0x2b, 0x3e),
  __MPI_LITERAL_DATA(0xd0, 0x02, 0xc5, 0x4f),
  __MPI_LITERAL_DATA(0x52, 0x53, 0xc1, 0xf1)
__MPI_LITERAL_END(static, _RSA512_Q, 256)

__MPI_LITERAL_BEGIN(static, _RSA512_DP)
  __MPI_LITERAL_DATA(0x41, 0xbc, 0x3a, 0xac),
  __MPI_LITERAL_DATA(0xd6, 0x34, 0x12, 0x0f),
  __MPI_LITERAL_DATA(0x19, 0x92, 0x7d, 0x5c),
  __MPI_LITERAL_DATA(0x85, 0xde, 0x65, 0xe4),
  __MPI_LITERAL_DATA(0xb9, 0xb3, 0xf9, 0x6e),
  __MPI_LITERAL_DATA(0xcf, 0x38, 0x3d, 0x68),
  __MPI_LITERAL_DATA(0xd8, 0xa5, 0xaf, 0xf9),
  __MPI_LITERAL_DATA(0xa5, 0xf8, 0xa6, 0xa4)
__MPI_LITERAL_END(static, _RSA512_DP, 256)

__MPI_LITERAL_BEGIN(static, _RSA512_DQ)
  __MPI_LITERAL_DATA(0x21, 0xf0, 0x5f, 0x76),
  __MPI_LITERAL_DATA(0x2c, 0x1b, 0x07, 0x6f),
  __MPI_LITERAL_DATA(0x51, 0x8f, 0x81, 0x1e),
  __MPI_LITERAL_DATA(0xdd, 0x6d, 0xce, 0xdd),
  __MPI_LITERAL_DATA(0x1c, 0x81, 0x4d, 0x74),
  __MPI_LITERAL_DATA(0x83, 0xdf, 0x58, 0x28),
  __MPI_LITERAL_DATA(0x01, 0x71, 0x43, 0x04),
  __MPI_LITERAL_DATA(0x2e, 0xe5, 0x8d, 0x63)
__MPI_LITERAL_END(static, _RSA512_DQ, 255)

__MPI_LITERAL_BEGIN(static, _RSA512_QINV)
  __MPI_LITERAL_DATA(0x6f, 0xc3, 0x88, 0x72),
  __MPI_LITERAL_DATA(0x72, 0x2b, 0x9f, 0x29),
  __MPI_LITERAL_DATA(0x02, 0x27, 0x41, 0x6f),
  __MPI_LITERAL_DATA(0xdb, 0xd5, 0xad, 0xb0),
  __MPI_LITERAL_DATA(0x66, 0xd3, 0x16, 0x67),
  __MPI_LITERAL_DATA(0xdc, 0x31, 0x0c, 0x61),
  __MPI_LITERAL_DATA(0x07, 0xfb, 0x2f, 0x88),
  __MPI_LITERAL_DATA(0x2d, 0x86, 0xef, 0x6b)
__MPI_LITERAL_END(static, _RSA512_QINV, 255)

#endif

/*********************************************************************
*
*       1024-bit modulus
*/

__MPI_LITERAL_BEGIN(static, _RSA1024_N)
  __MPI_LITERAL_DATA(0x69, 0x79, 0xab, 0x83),
  __MPI_LITERAL_DATA(0x84, 0x03, 0x2a, 0x64),
  __MPI_LITERAL_DATA(0xbb, 0x79, 0x87, 0xf9),
  __MPI_LITERAL_DATA(0x89, 0x56, 0x97, 0x96),
  __MPI_LITERAL_DATA(0xcc, 0x8c, 0x6f, 0xe2),
  __MPI_LITERAL_DATA(0x86, 0xa9, 0xdf, 0x09),
  __MPI_LITERAL_DATA(0x11, 0x1e, 0x4c, 0x9c),
  __MPI_LITERAL_DATA(0xf9, 0x47, 0xf1, 0xe1),
  __MPI_LITERAL_DATA(0x96, 0x0b, 0x06, 0xfe),
  __MPI_LITERAL_DATA(0xcc, 0x59, 0xcd, 0x24),
  __MPI_LITERAL_DATA(0x08, 0x1c, 0xd6, 0x18),
  __MPI_LITERAL_DATA(0x64, 0xee, 0xaa, 0x8b),
  __MPI_LITERAL_DATA(0x42, 0xb6, 0x7a, 0x80),
  __MPI_LITERAL_DATA(0x76, 0xee, 0x77, 0xc5),
  __MPI_LITERAL_DATA(0x57, 0x4b, 0x7e, 0x04),
  __MPI_LITERAL_DATA(0x83, 0xc0, 0xf4, 0x96),
  __MPI_LITERAL_DATA(0x20, 0x53, 0x39, 0x77),
  __MPI_LITERAL_DATA(0xa0, 0x7a, 0x74, 0x36),
  __MPI_LITERAL_DATA(0x07, 0x25, 0x44, 0xf3),
  __MPI_LITERAL_DATA(0x6e, 0x85, 0x8c, 0x01),
  __MPI_LITERAL_DATA(0xc2, 0x29, 0x6f, 0xcc),
  __MPI_LITERAL_DATA(0x48, 0x18, 0xad, 0xc6),
  __MPI_LITERAL_DATA(0x86, 0x6d, 0xcb, 0x3e),
  __MPI_LITERAL_DATA(0x12, 0x49, 0x53, 0xb6),
  __MPI_LITERAL_DATA(0x26, 0x25, 0xb9, 0xc9),
  __MPI_LITERAL_DATA(0x8c, 0x3b, 0xec, 0x27),
  __MPI_LITERAL_DATA(0x7e, 0xc0, 0x7c, 0x4a),
  __MPI_LITERAL_DATA(0x27, 0xad, 0x0a, 0x64),
  __MPI_LITERAL_DATA(0xf6, 0xd4, 0x5d, 0x4b),
  __MPI_LITERAL_DATA(0xa0, 0xf1, 0x46, 0x96),
  __MPI_LITERAL_DATA(0xcc, 0xc1, 0xc9, 0x0f),
  __MPI_LITERAL_DATA(0x4e, 0xb4, 0x5b, 0xca)
__MPI_LITERAL_END(static, _RSA1024_N, 1024)

__MPI_LITERAL_BEGIN(static, _RSA1024_E)
  __MPI_LITERAL_DATA(0x01, 0x00, 0x01, 0x00)
__MPI_LITERAL_END(static, _RSA1024_E, 17)

__MPI_LITERAL_BEGIN(static, _RSA1024_P)
  __MPI_LITERAL_DATA(0x19, 0x63, 0x9c, 0xf6),
  __MPI_LITERAL_DATA(0xc5, 0x2f, 0x5a, 0x80),
  __MPI_LITERAL_DATA(0xa7, 0x8c, 0x2a, 0x53),
  __MPI_LITERAL_DATA(0x4a, 0x1d, 0x7b, 0x34),
  __MPI_LITERAL_DATA(0x9d, 0x0d, 0x99, 0xfb),
  __MPI_LITERAL_DATA(0x8f, 0x74, 0xa2, 0x28),
  __MPI_LITERAL_DATA(0x96, 0x50, 0x5f, 0x55),
  __MPI_LITERAL_DATA(0x42, 0xe7, 0xb5, 0x3b),
  __MPI_LITERAL_DATA(0x9e, 0x91, 0xb2, 0x8a),
  __MPI_LITERAL_DATA(0x1d, 0xca, 0xf2, 0x5a),
  __MPI_LITERAL_DATA(0xbb, 0xbc, 0x15, 0xe8),
  __MPI_LITERAL_DATA(0xde, 0x2b, 0x58, 0x35),
  __MPI_LITERAL_DATA(0x38, 0xbf, 0xe7, 0x3c),
  __MPI_LITERAL_DATA(0x22, 0x00, 0xd9, 0x7b),
  __MPI_LITERAL_DATA(0xe0, 0xaf, 0xf9, 0xb4),
  __MPI_LITERAL_DATA(0x02, 0x2e, 0x8d, 0xd0)
__MPI_LITERAL_END(static, _RSA1024_P, 512)

__MPI_LITERAL_BEGIN(static, _RSA1024_Q)
  __MPI_LITERAL_DATA(0xd1, 0x62, 0x67, 0x19),
  __MPI_LITERAL_DATA(0xad, 0x64, 0xf7, 0xe3),
  __MPI_LITERAL_DATA(0xf6, 0x77, 0x04, 0xcd),
  __MPI_LITERAL_DATA(0x83, 0xef, 0xf3, 0x4f),
  __MPI_LITERAL_DATA(0xf2, 0x08, 0xc7, 0xeb),
  __MPI_LITERAL_DATA(0xfc, 0x95, 0x3f, 0x0b),
  __MPI_LITERAL_DATA(0x34, 0x06, 0x46, 0xf3),
  __MPI_LITERAL_DATA(0x79, 0xad, 0xe3, 0xf8),
  __MPI_LITERAL_DATA(0xa6, 0x11, 0x4b, 0x66),
  __MPI_LITERAL_DATA(0xd3, 0x51, 0x3e, 0x0c),
  __MPI_LITERAL_DATA(0x0d, 0xa0, 0x28, 0xd8),
  __MPI_LITERAL_DATA(0xf0, 0x38, 0x16, 0xa3),
  __MPI_LITERAL_DATA(0x02, 0xaa, 0x2e, 0x4f),
  __MPI_LITERAL_DATA(0x8e, 0xe9, 0xc9, 0x7b),
  __MPI_LITERAL_DATA(0xd3, 0x33, 0x36, 0x1f),
  __MPI_LITERAL_DATA(0x43, 0xce, 0x65, 0xf8)
__MPI_LITERAL_END(static, _RSA1024_Q, 512)

__MPI_LITERAL_BEGIN(static, _RSA1024_DP)
  __MPI_LITERAL_DATA(0x59, 0x8c, 0x4f, 0xdf),
  __MPI_LITERAL_DATA(0x08, 0xe0, 0xf2, 0xf2),
  __MPI_LITERAL_DATA(0x32, 0x1d, 0x35, 0x49),
  __MPI_LITERAL_DATA(0x8b, 0x5d, 0xe4, 0x25),
  __MPI_LITERAL_DATA(0xe2, 0x21, 0xa8, 0xed),
  __MPI_LITERAL_DATA(0x1e, 0xed, 0xf8, 0x65),
  __MPI_LITERAL_DATA(0x49, 0x3e, 0xe3, 0x00),
  __MPI_LITERAL_DATA(0xba, 0x4a, 0x93, 0x70),
  __MPI_LITERAL_DATA(0x08, 0x88, 0x1a, 0x51),
  __MPI_LITERAL_DATA(0x56, 0x48, 0x8a, 0x9f),
  __MPI_LITERAL_DATA(0x24, 0xec, 0x1e, 0x14),
  __MPI_LITERAL_DATA(0x0d, 0x0f, 0x59, 0xfa),
  __MPI_LITERAL_DATA(0xb0, 0x74, 0x0e, 0x8c),
  __MPI_LITERAL_DATA(0x4a, 0x72, 0x90, 0xc7),
  __MPI_LITERAL_DATA(0x75, 0x35, 0xd1, 0xbf),
  __MPI_LITERAL_DATA(0x73, 0x48, 0xdf, 0x3c)
__MPI_LITERAL_END(static, _RSA1024_DP, 510)

__MPI_LITERAL_BEGIN(static, _RSA1024_DQ)
  __MPI_LITERAL_DATA(0x11, 0x67, 0x61, 0xb7),
  __MPI_LITERAL_DATA(0xf2, 0x78, 0x2c, 0xa1),
  __MPI_LITERAL_DATA(0x52, 0x1c, 0xe4, 0xeb),
  __MPI_LITERAL_DATA(0xfc, 0xa5, 0x8d, 0xdb),
  __MPI_LITERAL_DATA(0xb2, 0xf3, 0xd0, 0x7a),
  __MPI_LITERAL_DATA(0x7a, 0x3f, 0xd4, 0x72),
  __MPI_LITERAL_DATA(0xad, 0x8d, 0xc3, 0x9b),
  __MPI_LITERAL_DATA(0x06, 0x34, 0x35, 0xab),
  __MPI_LITERAL_DATA(0xa2, 0x56, 0x68, 0x8c),
  __MPI_LITERAL_DATA(0x60, 0x1b, 0xfb, 0x27),
  __MPI_LITERAL_DATA(0x12, 0x02, 0x28, 0x4f),
  __MPI_LITERAL_DATA(0x8c, 0xf8, 0xa8, 0xdb),
  __MPI_LITERAL_DATA(0xfb, 0x38, 0x30, 0x24),
  __MPI_LITERAL_DATA(0x17, 0xf0, 0x8c, 0x2e),
  __MPI_LITERAL_DATA(0xb7, 0x98, 0x01, 0x5d),
  __MPI_LITERAL_DATA(0xee, 0x1c, 0x3b, 0xf8)
__MPI_LITERAL_END(static, _RSA1024_DQ, 512)

__MPI_LITERAL_BEGIN(static, _RSA1024_QINV)
  __MPI_LITERAL_DATA(0xd4, 0x63, 0x00, 0xbf),
  __MPI_LITERAL_DATA(0xbc, 0x22, 0x79, 0x05),
  __MPI_LITERAL_DATA(0x86, 0x76, 0x8b, 0x10),
  __MPI_LITERAL_DATA(0xf9, 0xe3, 0x65, 0x64),
  __MPI_LITERAL_DATA(0xbe, 0xff, 0x69, 0xef),
  __MPI_LITERAL_DATA(0x9f, 0x2a, 0xa2, 0x7d),
  __MPI_LITERAL_DATA(0x33, 0xe8, 0xac, 0x7b),
  __MPI_LITERAL_DATA(0x25, 0x03, 0xf1, 0xa2),
  __MPI_LITERAL_DATA(0x1b, 0x31, 0xa3, 0xd7),
  __MPI_LITERAL_DATA(0x6f, 0xd4, 0xdf, 0x37),
  __MPI_LITERAL_DATA(0x6a, 0x8c, 0x59, 0xab),
  __MPI_LITERAL_DATA(0x19, 0x61, 0x3c, 0x62),
  __MPI_LITERAL_DATA(0x20, 0x61, 0xf6, 0x41),
  __MPI_LITERAL_DATA(0x48, 0x9e, 0xfd, 0x7a),
  __MPI_LITERAL_DATA(0xc0, 0x21, 0x1b, 0xde),
  __MPI_LITERAL_DATA(0x80, 0x0a, 0x14, 0x5c)
__MPI_LITERAL_END(static, _RSA1024_QINV, 511)

/*********************************************************************
*
*       2048-bit modulus
*/

static const CRYPTO_MPI_LIMB _RSA2048_N_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0xe7, 0xfe, 0x60, 0x71),
  CRYPTO_MPI_LIMB_DATA4(0x5f, 0x76, 0x91, 0xee),
  CRYPTO_MPI_LIMB_DATA4(0x16, 0x60, 0x98, 0x0a),
  CRYPTO_MPI_LIMB_DATA4(0x5e, 0x71, 0x2f, 0x4f),
  CRYPTO_MPI_LIMB_DATA4(0x17, 0x6b, 0x5d, 0x2a),
  CRYPTO_MPI_LIMB_DATA4(0x22, 0xb2, 0x4f, 0x71),
  CRYPTO_MPI_LIMB_DATA4(0x1d, 0xd1, 0xef, 0x56),
  CRYPTO_MPI_LIMB_DATA4(0x06, 0xf8, 0xe7, 0x80),
  CRYPTO_MPI_LIMB_DATA4(0xfb, 0xa5, 0x93, 0xc2),
  CRYPTO_MPI_LIMB_DATA4(0x4b, 0xc0, 0xeb, 0xfa),
  CRYPTO_MPI_LIMB_DATA4(0x51, 0x6d, 0x9e, 0xb9),
  CRYPTO_MPI_LIMB_DATA4(0x1b, 0xce, 0xe3, 0x57),
  CRYPTO_MPI_LIMB_DATA4(0x46, 0xa5, 0x3e, 0x32),
  CRYPTO_MPI_LIMB_DATA4(0x3e, 0x12, 0x6e, 0xd8),
  CRYPTO_MPI_LIMB_DATA4(0x5c, 0x84, 0x65, 0xce),
  CRYPTO_MPI_LIMB_DATA4(0x31, 0xda, 0x2e, 0x80),
  CRYPTO_MPI_LIMB_DATA4(0xae, 0xfc, 0xda, 0x17),
  CRYPTO_MPI_LIMB_DATA4(0x35, 0x3e, 0xb1, 0xe8),
  CRYPTO_MPI_LIMB_DATA4(0x48, 0xb9, 0x9f, 0xe1),
  CRYPTO_MPI_LIMB_DATA4(0x51, 0xa6, 0xcc, 0xd2),
  CRYPTO_MPI_LIMB_DATA4(0x3b, 0xa5, 0x1a, 0xf9),
  CRYPTO_MPI_LIMB_DATA4(0x5c, 0x8d, 0x7c, 0x05),
  CRYPTO_MPI_LIMB_DATA4(0x25, 0x06, 0x35, 0x15),
  CRYPTO_MPI_LIMB_DATA4(0x26, 0x4e, 0x45, 0xd6),
  CRYPTO_MPI_LIMB_DATA4(0x46, 0x33, 0x31, 0xd6),
  CRYPTO_MPI_LIMB_DATA4(0x97, 0x18, 0xbe, 0x5b),
  CRYPTO_MPI_LIMB_DATA4(0xa1, 0xfa, 0x39, 0x9a),
  CRYPTO_MPI_LIMB_DATA4(0xe4, 0x4f, 0x82, 0xbe),
  CRYPTO_MPI_LIMB_DATA4(0xfe, 0x99, 0xab, 0x5a),
  CRYPTO_MPI_LIMB_DATA4(0xab, 0x33, 0xd9, 0xb6),
  CRYPTO_MPI_LIMB_DATA4(0xa9, 0x4a, 0xf6, 0xa1),
  CRYPTO_MPI_LIMB_DATA4(0x70, 0xa4, 0x1b, 0xae),
  CRYPTO_MPI_LIMB_DATA4(0xf5, 0x87, 0x51, 0x65),
  CRYPTO_MPI_LIMB_DATA4(0xe4, 0x2c, 0x9b, 0x62),
  CRYPTO_MPI_LIMB_DATA4(0x3e, 0xe0, 0x21, 0x5b),
  CRYPTO_MPI_LIMB_DATA4(0x09, 0x41, 0xe5, 0xb5),
  CRYPTO_MPI_LIMB_DATA4(0x92, 0x8c, 0x7c, 0x1b),
  CRYPTO_MPI_LIMB_DATA4(0x8a, 0x3e, 0x10, 0x2a),
  CRYPTO_MPI_LIMB_DATA4(0x81, 0xd5, 0xd4, 0x03),
  CRYPTO_MPI_LIMB_DATA4(0x62, 0x3b, 0x99, 0x4b),
  CRYPTO_MPI_LIMB_DATA4(0x6c, 0xd1, 0x3d, 0x74),
  CRYPTO_MPI_LIMB_DATA4(0x0f, 0x3a, 0x9d, 0xd2),
  CRYPTO_MPI_LIMB_DATA4(0x2a, 0x67, 0xbe, 0x47),
  CRYPTO_MPI_LIMB_DATA4(0x3b, 0x80, 0x4d, 0xe7),
  CRYPTO_MPI_LIMB_DATA4(0xae, 0x5a, 0x8f, 0xb9),
  CRYPTO_MPI_LIMB_DATA4(0xdd, 0x6f, 0x3c, 0x98),
  CRYPTO_MPI_LIMB_DATA4(0xea, 0x38, 0x6c, 0x50),
  CRYPTO_MPI_LIMB_DATA4(0x27, 0x5f, 0xea, 0x19),
  CRYPTO_MPI_LIMB_DATA4(0x7d, 0xd8, 0x9f, 0x00),
  CRYPTO_MPI_LIMB_DATA4(0x78, 0xc5, 0x05, 0x72),
  CRYPTO_MPI_LIMB_DATA4(0x4c, 0x5a, 0x13, 0xaf),
  CRYPTO_MPI_LIMB_DATA4(0xbf, 0x64, 0x79, 0x69),
  CRYPTO_MPI_LIMB_DATA4(0xbd, 0x75, 0x57, 0xae),
  CRYPTO_MPI_LIMB_DATA4(0x4f, 0xd6, 0xce, 0xe9),
  CRYPTO_MPI_LIMB_DATA4(0xd9, 0x81, 0x48, 0x2f),
  CRYPTO_MPI_LIMB_DATA4(0xef, 0x36, 0x86, 0x0c),
  CRYPTO_MPI_LIMB_DATA4(0xd5, 0x5e, 0x29, 0xd4),
  CRYPTO_MPI_LIMB_DATA4(0xb1, 0xa3, 0xee, 0xe0),
  CRYPTO_MPI_LIMB_DATA4(0xba, 0xe0, 0x38, 0xd9),
  CRYPTO_MPI_LIMB_DATA4(0xc6, 0x01, 0xcb, 0xff),
  CRYPTO_MPI_LIMB_DATA4(0xdb, 0x56, 0x09, 0x50),
  CRYPTO_MPI_LIMB_DATA4(0xce, 0x10, 0x3c, 0x23),
  CRYPTO_MPI_LIMB_DATA4(0x99, 0x6c, 0x7f, 0x39),
  CRYPTO_MPI_LIMB_DATA4(0xd4, 0x86, 0xfa, 0xa0)
};

static const CRYPTO_MPI _RSA2048_N = {
  CRYPTO_MPI_INIT_RO(_RSA2048_N_aLimbs)
};

__MPI_LITERAL_BEGIN(static, _RSA2048_E)
  __MPI_LITERAL_DATA(0x01, 0x00, 0x01, 0x00)
__MPI_LITERAL_END(static, _RSA2048_E, 17)

__MPI_LITERAL_BEGIN(static, _RSA2048_P)
  __MPI_LITERAL_DATA(0x29, 0x78, 0x79, 0xe2),
  __MPI_LITERAL_DATA(0x5c, 0x0d, 0xeb, 0xed),
  __MPI_LITERAL_DATA(0x2a, 0x79, 0xaf, 0x00),
  __MPI_LITERAL_DATA(0xe0, 0xe7, 0xc9, 0x58),
  __MPI_LITERAL_DATA(0x11, 0x17, 0x42, 0x0d),
  __MPI_LITERAL_DATA(0xa5, 0x11, 0x99, 0x1b),
  __MPI_LITERAL_DATA(0x26, 0x62, 0x37, 0x3c),
  __MPI_LITERAL_DATA(0xbf, 0xdc, 0xa4, 0x78),
  __MPI_LITERAL_DATA(0x3e, 0x95, 0xd6, 0x8d),
  __MPI_LITERAL_DATA(0x8c, 0x92, 0xde, 0x78),
  __MPI_LITERAL_DATA(0x7a, 0x03, 0xd9, 0xd2),
  __MPI_LITERAL_DATA(0x2d, 0xb1, 0x35, 0x4c),
  __MPI_LITERAL_DATA(0x9e, 0x4b, 0x71, 0x29),
  __MPI_LITERAL_DATA(0xf8, 0x8d, 0x7e, 0x56),
  __MPI_LITERAL_DATA(0x33, 0x42, 0xd7, 0xd7),
  __MPI_LITERAL_DATA(0x1a, 0xe5, 0xcc, 0xb7),
  __MPI_LITERAL_DATA(0x14, 0x78, 0x8d, 0x29),
  __MPI_LITERAL_DATA(0x5d, 0x19, 0xde, 0x8c),
  __MPI_LITERAL_DATA(0x14, 0xd6, 0x51, 0xc5),
  __MPI_LITERAL_DATA(0x34, 0xe7, 0xfe, 0x5b),
  __MPI_LITERAL_DATA(0x37, 0xb8, 0xf4, 0x3f),
  __MPI_LITERAL_DATA(0x29, 0x8d, 0x38, 0xa0),
  __MPI_LITERAL_DATA(0x41, 0xb8, 0xd9, 0x82),
  __MPI_LITERAL_DATA(0x05, 0xf5, 0xd2, 0xf7),
  __MPI_LITERAL_DATA(0x7e, 0x23, 0xf4, 0x46),
  __MPI_LITERAL_DATA(0xde, 0x69, 0x11, 0x45),
  __MPI_LITERAL_DATA(0x22, 0x33, 0x6a, 0xdf),
  __MPI_LITERAL_DATA(0x38, 0x3d, 0xff, 0x14),
  __MPI_LITERAL_DATA(0xaa, 0xd5, 0xb7, 0x17),
  __MPI_LITERAL_DATA(0x4f, 0xc2, 0x40, 0x0f),
  __MPI_LITERAL_DATA(0x67, 0x80, 0x53, 0x55),
  __MPI_LITERAL_DATA(0xbd, 0x37, 0xc2, 0xc6)
__MPI_LITERAL_END(static, _RSA2048_P, 1024)

__MPI_LITERAL_BEGIN(static, _RSA2048_Q)
  __MPI_LITERAL_DATA(0x8f, 0xe0, 0xab, 0xab),
  __MPI_LITERAL_DATA(0x0c, 0xe5, 0xdb, 0x1b),
  __MPI_LITERAL_DATA(0xb8, 0x29, 0x3f, 0x90),
  __MPI_LITERAL_DATA(0x4f, 0x91, 0xee, 0x24),
  __MPI_LITERAL_DATA(0xae, 0xc2, 0x70, 0x7d),
  __MPI_LITERAL_DATA(0x3e, 0x0e, 0xd0, 0x3f),
  __MPI_LITERAL_DATA(0x23, 0x8b, 0x16, 0xef),
  __MPI_LITERAL_DATA(0x5e, 0x1e, 0xb8, 0x1d),
  __MPI_LITERAL_DATA(0x59, 0x6f, 0xdd, 0x12),
  __MPI_LITERAL_DATA(0x62, 0xfb, 0xe8, 0xa0),
  __MPI_LITERAL_DATA(0x04, 0xca, 0xd3, 0x2e),
  __MPI_LITERAL_DATA(0x20, 0xf4, 0x5b, 0xb0),
  __MPI_LITERAL_DATA(0xb9, 0x4f, 0xa6, 0x32),
  __MPI_LITERAL_DATA(0x5d, 0xef, 0x4f, 0x87),
  __MPI_LITERAL_DATA(0x2b, 0x52, 0x87, 0x20),
  __MPI_LITERAL_DATA(0x34, 0x2f, 0xed, 0x6d),
  __MPI_LITERAL_DATA(0x4b, 0x02, 0x61, 0x46),
  __MPI_LITERAL_DATA(0x73, 0x76, 0x1b, 0x44),
  __MPI_LITERAL_DATA(0xd4, 0x5b, 0x64, 0xf7),
  __MPI_LITERAL_DATA(0xff, 0xf7, 0xb3, 0xe6),
  __MPI_LITERAL_DATA(0xce, 0x08, 0x10, 0x8f),
  __MPI_LITERAL_DATA(0xd9, 0x0d, 0x60, 0xbe),
  __MPI_LITERAL_DATA(0xef, 0x62, 0x81, 0x67),
  __MPI_LITERAL_DATA(0xa4, 0x5e, 0x0c, 0xdb),
  __MPI_LITERAL_DATA(0x5b, 0x72, 0x8f, 0x8b),
  __MPI_LITERAL_DATA(0xe3, 0xf0, 0x5a, 0x73),
  __MPI_LITERAL_DATA(0x5f, 0xb4, 0xba, 0xa2),
  __MPI_LITERAL_DATA(0xd8, 0x24, 0x6e, 0x34),
  __MPI_LITERAL_DATA(0xce, 0xe0, 0x95, 0x61),
  __MPI_LITERAL_DATA(0xee, 0xb4, 0xd0, 0x5e),
  __MPI_LITERAL_DATA(0x2a, 0x02, 0x7b, 0x79),
  __MPI_LITERAL_DATA(0x13, 0xeb, 0x56, 0xcf)
__MPI_LITERAL_END(static, _RSA2048_Q, 1024)

__MPI_LITERAL_BEGIN(static, _RSA2048_DP)
  __MPI_LITERAL_DATA(0x79, 0x73, 0x0c, 0xf7),
  __MPI_LITERAL_DATA(0xaa, 0x65, 0xbe, 0x0c),
  __MPI_LITERAL_DATA(0x84, 0x0c, 0x7f, 0x6e),
  __MPI_LITERAL_DATA(0x8a, 0x0d, 0x13, 0x59),
  __MPI_LITERAL_DATA(0x5f, 0x79, 0x04, 0xc8),
  __MPI_LITERAL_DATA(0x42, 0x82, 0x03, 0x72),
  __MPI_LITERAL_DATA(0x45, 0x5c, 0x7f, 0x22),
  __MPI_LITERAL_DATA(0x10, 0xe6, 0x0d, 0x9c),
  __MPI_LITERAL_DATA(0x71, 0x10, 0x07, 0xf4),
  __MPI_LITERAL_DATA(0x5f, 0xff, 0x91, 0x33),
  __MPI_LITERAL_DATA(0x44, 0x14, 0x3e, 0x95),
  __MPI_LITERAL_DATA(0x67, 0xe9, 0x18, 0xc1),
  __MPI_LITERAL_DATA(0xd0, 0xe7, 0xd6, 0x8d),
  __MPI_LITERAL_DATA(0xfa, 0xa5, 0x16, 0xaf),
  __MPI_LITERAL_DATA(0x20, 0xb3, 0x4f, 0x57),
  __MPI_LITERAL_DATA(0x7d, 0xda, 0x1e, 0x95),
  __MPI_LITERAL_DATA(0x19, 0x47, 0x1c, 0x1e),
  __MPI_LITERAL_DATA(0x55, 0x0d, 0xc4, 0x98),
  __MPI_LITERAL_DATA(0xa5, 0x83, 0xdd, 0x5c),
  __MPI_LITERAL_DATA(0xdb, 0x30, 0x5a, 0xba),
  __MPI_LITERAL_DATA(0xb7, 0xb4, 0x60, 0x43),
  __MPI_LITERAL_DATA(0x6c, 0x8f, 0x16, 0x4f),
  __MPI_LITERAL_DATA(0xdc, 0x4b, 0x51, 0xda),
  __MPI_LITERAL_DATA(0xc5, 0xb4, 0xb9, 0x1f),
  __MPI_LITERAL_DATA(0xf9, 0x3b, 0xb9, 0x97),
  __MPI_LITERAL_DATA(0xc7, 0x20, 0xef, 0x85),
  __MPI_LITERAL_DATA(0xbb, 0x4c, 0xff, 0x46),
  __MPI_LITERAL_DATA(0xf5, 0xff, 0xf4, 0x29),
  __MPI_LITERAL_DATA(0x68, 0xf2, 0x4c, 0xf4),
  __MPI_LITERAL_DATA(0x01, 0x9d, 0x8b, 0x9d),
  __MPI_LITERAL_DATA(0xf4, 0xd5, 0xd3, 0xba),
  __MPI_LITERAL_DATA(0x0d, 0xda, 0x79, 0x77)
__MPI_LITERAL_END(static, _RSA2048_DP, 1023)

__MPI_LITERAL_BEGIN(static, _RSA2048_DQ)
  __MPI_LITERAL_DATA(0x9d, 0x5c, 0x6a, 0x09),
  __MPI_LITERAL_DATA(0xe9, 0xf6, 0x40, 0x1d),
  __MPI_LITERAL_DATA(0x18, 0x8f, 0x7c, 0x4d),
  __MPI_LITERAL_DATA(0x5f, 0x3d, 0xe5, 0x78),
  __MPI_LITERAL_DATA(0x6d, 0xbe, 0xb0, 0xa4),
  __MPI_LITERAL_DATA(0x6b, 0x70, 0xc8, 0x48),
  __MPI_LITERAL_DATA(0x3b, 0x5b, 0xee, 0x16),
  __MPI_LITERAL_DATA(0xf0, 0xd2, 0x64, 0xc2),
  __MPI_LITERAL_DATA(0x30, 0xc2, 0x64, 0x9a),
  __MPI_LITERAL_DATA(0x42, 0x84, 0x00, 0xfa),
  __MPI_LITERAL_DATA(0x0b, 0xea, 0x77, 0xe3),
  __MPI_LITERAL_DATA(0x1e, 0x9f, 0xf2, 0xc3),
  __MPI_LITERAL_DATA(0xd0, 0x52, 0x34, 0xb3),
  __MPI_LITERAL_DATA(0x9b, 0x6a, 0x80, 0xb1),
  __MPI_LITERAL_DATA(0x93, 0x7e, 0x68, 0xc0),
  __MPI_LITERAL_DATA(0xbc, 0xc7, 0xd9, 0x35),
  __MPI_LITERAL_DATA(0x89, 0xd8, 0x5e, 0x31),
  __MPI_LITERAL_DATA(0xd7, 0x5f, 0x82, 0xe9),
  __MPI_LITERAL_DATA(0xd1, 0xad, 0xec, 0x4d),
  __MPI_LITERAL_DATA(0xb4, 0x9e, 0x91, 0x28),
  __MPI_LITERAL_DATA(0xe4, 0xee, 0xae, 0x36),
  __MPI_LITERAL_DATA(0x4a, 0x57, 0xe2, 0x42),
  __MPI_LITERAL_DATA(0x4a, 0xe5, 0xb6, 0x54),
  __MPI_LITERAL_DATA(0x1a, 0x4a, 0x0e, 0x04),
  __MPI_LITERAL_DATA(0x31, 0x59, 0x76, 0xaa),
  __MPI_LITERAL_DATA(0xb7, 0x52, 0xd3, 0xf0),
  __MPI_LITERAL_DATA(0xd7, 0xd8, 0xbf, 0x09),
  __MPI_LITERAL_DATA(0xab, 0x15, 0x1f, 0x75),
  __MPI_LITERAL_DATA(0x85, 0x0c, 0xef, 0xa7),
  __MPI_LITERAL_DATA(0xa4, 0x1b, 0xd7, 0xdc),
  __MPI_LITERAL_DATA(0x1e, 0x3f, 0x70, 0x1f),
  __MPI_LITERAL_DATA(0x09, 0x17, 0x33, 0x51)
__MPI_LITERAL_END(static, _RSA2048_DQ, 1023)

__MPI_LITERAL_BEGIN(static, _RSA2048_QINV)
  __MPI_LITERAL_DATA(0x0d, 0xa3, 0x5a, 0xe9),
  __MPI_LITERAL_DATA(0x0f, 0x46, 0x32, 0x97),
  __MPI_LITERAL_DATA(0x59, 0xf1, 0xd0, 0xec),
  __MPI_LITERAL_DATA(0x08, 0xd6, 0x56, 0xd0),
  __MPI_LITERAL_DATA(0xc9, 0x0e, 0x2a, 0x04),
  __MPI_LITERAL_DATA(0x66, 0x0c, 0xa6, 0xbf),
  __MPI_LITERAL_DATA(0x5b, 0xcb, 0x3b, 0x24),
  __MPI_LITERAL_DATA(0x76, 0xab, 0x72, 0xb8),
  __MPI_LITERAL_DATA(0x65, 0x89, 0x79, 0x87),
  __MPI_LITERAL_DATA(0xa8, 0xc7, 0x51, 0xd6),
  __MPI_LITERAL_DATA(0x8a, 0x50, 0x54, 0xda),
  __MPI_LITERAL_DATA(0x07, 0x93, 0x97, 0xa5),
  __MPI_LITERAL_DATA(0x3b, 0x84, 0x56, 0xa7),
  __MPI_LITERAL_DATA(0x4b, 0x5a, 0xab, 0x2d),
  __MPI_LITERAL_DATA(0x79, 0xa7, 0xbf, 0x1a),
  __MPI_LITERAL_DATA(0xc5, 0xf6, 0x29, 0x70),
  __MPI_LITERAL_DATA(0xc3, 0xcd, 0x3a, 0xde),
  __MPI_LITERAL_DATA(0x58, 0x7b, 0x40, 0x52),
  __MPI_LITERAL_DATA(0xd2, 0x10, 0x41, 0x73),
  __MPI_LITERAL_DATA(0xc1, 0x11, 0x02, 0xdf),
  __MPI_LITERAL_DATA(0xd1, 0xba, 0x9d, 0x11),
  __MPI_LITERAL_DATA(0xeb, 0x88, 0xcb, 0xaf),
  __MPI_LITERAL_DATA(0x19, 0x7d, 0x96, 0xac),
  __MPI_LITERAL_DATA(0xeb, 0x3f, 0xc0, 0x58),
  __MPI_LITERAL_DATA(0x92, 0x95, 0xf2, 0xe6),
  __MPI_LITERAL_DATA(0x5d, 0x00, 0x42, 0x59),
  __MPI_LITERAL_DATA(0xd8, 0xc4, 0x00, 0xab),
  __MPI_LITERAL_DATA(0xde, 0x34, 0x3d, 0x4f),
  __MPI_LITERAL_DATA(0xc6, 0xcc, 0xb8, 0xd6),
  __MPI_LITERAL_DATA(0xbe, 0x74, 0xef, 0x6d),
  __MPI_LITERAL_DATA(0xcb, 0x98, 0xba, 0x9d),
  __MPI_LITERAL_DATA(0x00, 0xcd, 0x72, 0x08)
__MPI_LITERAL_END(static, _RSA2048_QINV, 1020)

/*********************************************************************
*
*       Benchmark parameterization.
*/
static const BENCH_ALG _aBenchAlgs[] = {
       { "Basic, fast",                CRYPTO_MPI_ModExp_Basic_Fast                   },
       { "Basic, ladder",              CRYPTO_MPI_ModExp_Basic_Ladder                 },
       { "Basic, 2b, FW",              CRYPTO_MPI_ModExp_Basic_2b_FW                  },
       { "Basic, 3b, FW",              CRYPTO_MPI_ModExp_Basic_3b_FW                  },
       { "Basic, 4b, FW",              CRYPTO_MPI_ModExp_Basic_4b_FW                  },
       { "Basic, 5b, FW",              CRYPTO_MPI_ModExp_Basic_5b_FW                  },
       { "Basic, 6b, FW",              CRYPTO_MPI_ModExp_Basic_6b_FW                  },
       { NULL,                         NULL                                           },
       { "Basic, 2b, RM",              CRYPTO_MPI_ModExp_Basic_2b_RM                  },
       { "Basic, 3b, RM",              CRYPTO_MPI_ModExp_Basic_3b_RM                  },
       { "Basic, 4b, RM",              CRYPTO_MPI_ModExp_Basic_4b_RM                  },
       { "Basic, 5b, RM",              CRYPTO_MPI_ModExp_Basic_5b_RM                  },
       { "Basic, 6b, RM",              CRYPTO_MPI_ModExp_Basic_6b_RM                  },
       { NULL,                         NULL                                           },
       { "Barrett, fast",              CRYPTO_MPI_ModExp_Barrett_Fast                 },
       { "Barrett, ladder",            CRYPTO_MPI_ModExp_Barrett_Ladder               },
       { "Barrett, 2b, FW",            CRYPTO_MPI_ModExp_Barrett_2b_FW                },
       { "Barrett, 3b, FW",            CRYPTO_MPI_ModExp_Barrett_3b_FW                },
       { "Barrett, 4b, FW",            CRYPTO_MPI_ModExp_Barrett_4b_FW                },
       { "Barrett, 5b, FW",            CRYPTO_MPI_ModExp_Barrett_5b_FW                },
       { "Barrett, 6b, FW",            CRYPTO_MPI_ModExp_Barrett_6b_FW                },
       { NULL,                         NULL                                           },
       { "Barrett, 2b, RM",            CRYPTO_MPI_ModExp_Barrett_2b_RM                },
       { "Barrett, 3b, RM",            CRYPTO_MPI_ModExp_Barrett_3b_RM                },
       { "Barrett, 4b, RM",            CRYPTO_MPI_ModExp_Barrett_4b_RM                },
       { "Barrett, 5b, RM",            CRYPTO_MPI_ModExp_Barrett_5b_RM                },
       { "Barrett, 6b, RM",            CRYPTO_MPI_ModExp_Barrett_6b_RM                },
       { NULL,                         NULL                                           },
       { "Montgomery, fast",           CRYPTO_MPI_ModExp_Montgomery_Fast              },
       { "Montgomery, ladder",         CRYPTO_MPI_ModExp_Montgomery_Ladder            },
       { "Montgomery, 2b, FW",         CRYPTO_MPI_ModExp_Montgomery_2b_FW             },
EFM32( { "Montgomery, 2b, FW, EFM32",  CRYPTO_MPI_ModExp_Montgomery_2bFW_EFM32_CRYPTO }, )
       { "Montgomery, 3b, FW",         CRYPTO_MPI_ModExp_Montgomery_3b_FW             },
EFM32( { "Montgomery, 3b, FW, EFM32",  CRYPTO_MPI_ModExp_Montgomery_3bFW_EFM32_CRYPTO }, )
       { "Montgomery, 4b, FW",         CRYPTO_MPI_ModExp_Montgomery_4b_FW             },
EFM32( { "Montgomery, 4b, FW, EFM32",  CRYPTO_MPI_ModExp_Montgomery_4bFW_EFM32_CRYPTO }, )
       { "Montgomery, 5b, FW",         CRYPTO_MPI_ModExp_Montgomery_5b_FW             },
EFM32( { "Montgomery, 5b, FW, EFM32",  CRYPTO_MPI_ModExp_Montgomery_5bFW_EFM32_CRYPTO }, )
       { "Montgomery, 6b, FW",         CRYPTO_MPI_ModExp_Montgomery_6b_FW             },
EFM32( { "Montgomery, 6b, FW, EFM32",  CRYPTO_MPI_ModExp_Montgomery_6bFW_EFM32_CRYPTO }, )
       { NULL,                         NULL                                           },
       { "Montgomery, 2b, RM",         CRYPTO_MPI_ModExp_Montgomery_2b_RM             },
EFM32( { "Montgomery, 2b, RM, EFM32",  CRYPTO_MPI_ModExp_Montgomery_2bRM_EFM32_CRYPTO }, )
       { "Montgomery, 3b, RM",         CRYPTO_MPI_ModExp_Montgomery_3b_RM             },
EFM32( { "Montgomery, 3b, RM, EFM32",  CRYPTO_MPI_ModExp_Montgomery_3bRM_EFM32_CRYPTO }, )
       { "Montgomery, 4b, RM",         CRYPTO_MPI_ModExp_Montgomery_4b_RM             },
EFM32( { "Montgomery, 4b, RM, EFM32",  CRYPTO_MPI_ModExp_Montgomery_4bRM_EFM32_CRYPTO }, )
       { "Montgomery, 5b, RM",         CRYPTO_MPI_ModExp_Montgomery_5b_RM             },
EFM32( { "Montgomery, 5b, RM, EFM32",  CRYPTO_MPI_ModExp_Montgomery_5bRM_EFM32_CRYPTO }, )
       { "Montgomery, 6b, RM",         CRYPTO_MPI_ModExp_Montgomery_6b_RM             },
EFM32( { "Montgomery, 6b, RM, EFM32",  CRYPTO_MPI_ModExp_Montgomery_6bRM_EFM32_CRYPTO }, )
       { NULL,                         NULL                                           },
       { "Configured",                 CRYPTO_MPI_ModExp_Pub                          }
};

static const BENCH_SCENARIO _aBenchScenarios[] = {
  { "CRT private key, exponent length = modulus length",     _BenchmarkModExp_Private_CRT   },
#if INCLUDE_PLAIN_PRIVATE  
  { "Non-CRT private key, exponent length = modulus length", _BenchmarkModExp_Private_Plain },
#endif
  { "Public key, exponent length = 17 bits",                 _BenchmarkModExp_Public        }
};

static const BENCH_KEY _aBenchKeys[] = {
#if INCLUDE_SMALL_MODULI
  { &_RSA128_N,  &_RSA128_E,  &_RSA128_P,  &_RSA128_Q,  &_RSA128_DP,  &_RSA128_DQ,  &_RSA128_QINV  },
  { &_RSA256_N,  &_RSA256_E,  &_RSA256_P,  &_RSA256_Q,  &_RSA256_DP,  &_RSA256_DQ,  &_RSA256_QINV  },
  { &_RSA512_N,  &_RSA512_E,  &_RSA512_P,  &_RSA512_Q,  &_RSA512_DP,  &_RSA512_DQ,  &_RSA512_QINV  },
#endif
  { &_RSA1024_N, &_RSA1024_E, &_RSA1024_P, &_RSA1024_Q, &_RSA1024_DP, &_RSA1024_DQ, &_RSA1024_QINV },
  { &_RSA2048_N, &_RSA2048_E, &_RSA2048_P, &_RSA2048_Q, &_RSA2048_DP, &_RSA2048_DQ, &_RSA2048_QINV }
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
static unsigned                 _AlgIndex;
static unsigned                 _KeyIndex;
static float                    _aBaselinePerf[SEGGER_COUNTOF(_aBenchKeys)];
static float                    _aBaselineMem [SEGGER_COUNTOF(_aBenchKeys)];

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
*       _BenchmarkSingleModExp()
*
*  Function description
*    Count the number of encryptions completed in one second using
*    the public key parameters N, E.
*
*  Parameters
*    pfModExp  - Pointer to modular exponentiation implementation.
*    pN        - Pointer to MPI containing modulus.
*    pExponent - Pointer to MPI containing exponent.
*/
static void _BenchmarkSingleModExp(MODEXP_FUNC pfModExp, const CRYPTO_MPI *pN, const CRYPTO_MPI *pExponent) {
  CRYPTO_MPI Data;
  U64        OneSecond;
  U64        T0;
  U64        Elapsed;
  int        Loops;
  int        Status;
  unsigned   PeakBytes;
  unsigned   ChunkSize;
  float      PerfMultiplier;
  float      MemMultiplier;
  float      Time;
  //
  PeakBytes = 0;
  Loops     = 0;
  //
  ChunkSize = CRYPTO_MPI_BYTES_REQUIRED(2*CRYPTO_MPI_BitCount(pN)+CRYPTO_MPI_BYTES_PER_LIMB-1) + 2*CRYPTO_MPI_BYTES_PER_LIMB;
  CRYPTO_MPI_SetChunkSize(ChunkSize);
  //
  // Create fixed plaintext.
  //
  CRYPTO_MPI_Init   (&Data, &_MemContext);
  CRYPTO_MPI_LoadHex(&Data, "123456789ABCDEF123456789ABCDEF0123456789ABCDEF", 0);
  CRYPTO_MPI_Mod    (&Data, pN, &_MemContext);
  //
  // Count number of modular exponentiations completed in 1s.
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    Status = pfModExp(&Data, pExponent, pN, &_MemContext);
    PeakBytes = SEGGER_MAX(PeakBytes, _Heap.Stats.NumInUseMax * ChunkSize);
    ++Loops;
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
  CRYPTO_MPI_Kill(&Data);
  //
  Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
  if (_AlgIndex == 0) {
    _aBaselinePerf[_KeyIndex] = Time;
    _aBaselineMem [_KeyIndex] = (float)PeakBytes;
  } 
  PerfMultiplier = _aBaselinePerf[_KeyIndex] / (float)Time;
  MemMultiplier  = (float)PeakBytes / _aBaselineMem [_KeyIndex];
  if (Status < 0) {
    SEGGER_SYS_IO_Printf("  -Fail-          -Fail-        |");
  } else {
    SEGGER_SYS_IO_Printf("%8.2f %5.2fx %8u %5.2fx |", Time, PerfMultiplier, PeakBytes, MemMultiplier);
  }
}

/*********************************************************************
*
*       _BenchmarkModExp_Public()
*
*  Function description
*    Benchmark public key operation.
*
*  Parameters
*    pfModExp - Modular exponentiation function to benchmark.
*    pKey     - Pointer to key ti benchmark with.
*/
static void _BenchmarkModExp_Public(MODEXP_FUNC pfModExp, const BENCH_KEY *pKey) {
  _BenchmarkSingleModExp(pfModExp, pKey->pN, pKey->pE);
}

#if INCLUDE_PLAIN_PRIVATE

/*********************************************************************
*
*       _BenchmarkModExp_Private_Plain()
*
*  Function description
*    Benchmark non-CRT private key operation.
*
*  Parameters
*    pfModExp - Modular exponentiation function to benchmark.
*    pKey     - Pointer to key ti benchmark with.
*/
static void _BenchmarkModExp_Private_Plain(MODEXP_FUNC pfModExp, const BENCH_KEY *pKey) {
  CRYPTO_MPI D;
  CRYPTO_MPI P;
  CRYPTO_MPI Q;
  CRYPTO_MPI x;
  //
  // Compute non-CRT form of decryption exponent, d = modinv(e, lcm(p-1, q-1))
  //
  CRYPTO_MPI_Init(&x, &_MemContext);
  CRYPTO_MPI_Init(&D, &_MemContext);
  CRYPTO_MPI_Init(&P, &_MemContext);
  CRYPTO_MPI_Init(&Q, &_MemContext);
  CRYPTO_MPI_Assign(&P, pKey->pP);
  CRYPTO_MPI_Assign(&Q, pKey->pQ);
  CRYPTO_MPI_Dec(&P);
  CRYPTO_MPI_Dec(&Q);
  CRYPTO_MPI_LCM(&x, &P, &Q, &_MemContext);           // x = lcm(p-1, q-1)
  CRYPTO_MPI_ModInvEx(&D, pKey->pE, &x, &_MemContext);  // d = modinv(e, lcm(p-1, q-1))
  CRYPTO_MPI_Kill(&P);
  CRYPTO_MPI_Kill(&Q);
  CRYPTO_MPI_Kill(&x);
  //
  _BenchmarkSingleModExp(pfModExp, pKey->pN, &D);
  //
  CRYPTO_MPI_Kill(&D);
}

#endif

/*********************************************************************
*
*       _BenchmarkModExp_Private_CRT()
*
*  Function description
*    Benchmark private key operation in CRT form.
*
*  Parameters
*    pfModExp - Modular exponentiation function to benchmark.
*    pKey     - Pointer to key ti benchmark with.
*/
static void _BenchmarkModExp_Private_CRT(MODEXP_FUNC pfModExp, const BENCH_KEY *pKey) {
  CRYPTO_MPI Data;
  CRYPTO_MPI a;
  CRYPTO_MPI b;
  U64        OneSecond;
  U64        T0;
  U64        Elapsed;
  int        Loops;
  int        Status;
  unsigned   ChunkSize;
  unsigned   PeakBytes;
  float      Time;
  float      PerfMultiplier;
  float      MemMultiplier;
  //
  ChunkSize = CRYPTO_MPI_BYTES_REQUIRED(2*CRYPTO_MPI_BitCount(pKey->pP)+CRYPTO_MPI_BYTES_PER_LIMB-1) + 2*CRYPTO_MPI_BYTES_PER_LIMB;
  CRYPTO_MPI_SetChunkSize(ChunkSize);
  //
  // Make PC-lint quiet, it's dataflow analysis provides false positives.
  //
  Loops     = 0;
  Elapsed   = 0;
  PeakBytes = 0;
  //
  // Create fixed plaintext.
  //
  CRYPTO_MPI_Init(&Data, &_MemContext);
  CRYPTO_MPI_Init(&a, &_MemContext);
  CRYPTO_MPI_Init(&b, &_MemContext);
  //
  CRYPTO_CHECK(CRYPTO_MPI_LoadHex(&Data, "123456789ABCDEF123456789ABCDEF0123456789ABCDEF", 0));
  CRYPTO_CHECK(CRYPTO_MPI_Mod    (&Data, pKey->pN, &_MemContext));
  //
  // Count number of modular exponentiations completed in 1s.
  //
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    //
    // Apply Chinese Remainder Theorem.  We assume that ciphertext
    // is already reduced modulo p.
    //
    _Heap.Stats.NumInUseMax = _Heap.Stats.NumInUse;
    //
    CRYPTO_CHECK(CRYPTO_MPI_Assign(&a, &Data));
    CRYPTO_CHECK(pfModExp(&a, pKey->pDP, pKey->pP, &_MemContext));   // a = cipher^dp (mod p)
    CRYPTO_CHECK(CRYPTO_MPI_Assign(&b, &Data));
    CRYPTO_CHECK(pfModExp(&b, pKey->pDQ, pKey->pQ, &_MemContext));   // b = cipher^dq (mod q)
    //
    // plaintext = b + q * (((a-b)*u) % p)
    //
    CRYPTO_CHECK(CRYPTO_MPI_Move(&Data, &a));                        // a
    CRYPTO_CHECK(CRYPTO_MPI_Sub(&Data, &b));                         // a-b could be negative as p < q.
    while (CRYPTO_MPI_IsNegative(&Data)) {
      CRYPTO_CHECK(CRYPTO_MPI_Add(&Data, pKey->pP));
    }
    CRYPTO_CHECK(CRYPTO_MPI_ModMul(&Data, pKey->pQInv, pKey->pP, &_MemContext)); // (a-b)*qinv mod p
    CRYPTO_CHECK(CRYPTO_MPI_Mul(&Data, pKey->pQ, &_MemContext));     // q * ((a-b)*qinv mod p)
    CRYPTO_CHECK(CRYPTO_MPI_Add(&Data, &b));                         // b + q * ((a-b)*qinv mod p)
    //
    PeakBytes = SEGGER_MAX(PeakBytes, _Heap.Stats.NumInUseMax * ChunkSize);
    //
    ++Loops;
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
Finally:
  CRYPTO_MPI_Kill(&Data);
  CRYPTO_MPI_Kill(&a);
  CRYPTO_MPI_Kill(&b);
  if (Status < 0 || Loops == 0) {
    SEGGER_SYS_IO_Printf("  -Fail-          -Fail-        |");
  } else {
    Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
    if (_AlgIndex == 0) {
      _aBaselinePerf[_KeyIndex] = Time;
      _aBaselineMem [_KeyIndex] = (float)PeakBytes;
    } 
    PerfMultiplier = _aBaselinePerf[_KeyIndex] / (float)Time;
    MemMultiplier  = (float)PeakBytes / _aBaselineMem[_KeyIndex];
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("  -Fail-          -Fail-        |");
    } else {
      SEGGER_SYS_IO_Printf("%8.2f %5.2fx %8u %5.2fx |", Time, PerfMultiplier, PeakBytes, MemMultiplier);
    }
  }
}

/*********************************************************************
*
*       _PrintSeparator()
*
*  Function description
*    Print row separator for table.
*/
static void _PrintSeparator(void) {
  unsigned i;
  //
  SEGGER_SYS_IO_Printf("+---------------------------+");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    SEGGER_SYS_IO_Printf("--------------------------------+");
  }
  SEGGER_SYS_IO_Printf("\n");
}

/*********************************************************************
*
*       _PrintHeader()
*
*  Function description
*    Print column headers for table.
*/
static void _PrintHeader(void) {
  unsigned i;
  //
  _PrintSeparator();
  SEGGER_SYS_IO_Printf("|                   Modulus |");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    SEGGER_SYS_IO_Printf(" %25d bits |", CRYPTO_MPI_BitCount(_aBenchKeys[i].pN));
  }
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("| Algorithm                 |");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    SEGGER_SYS_IO_Printf(" %7s %6s  %7s %6s |", "Time", "x", "Memory", "x");
  }
  SEGGER_SYS_IO_Printf("\n");
  _PrintSeparator();
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
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_MEM_SELFTEST_HEAP_Init(&_MemContext, &_Heap, _aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  //
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("Modular Exponentiation Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
  //
  SEGGER_SYS_IO_Printf("Compiler: %s\n", SEGGER_SYS_GetCompiler());
  if (SEGGER_SYS_GetProcessorSpeed() > 0) {
    SEGGER_SYS_IO_Printf("System:   Processor speed          = %.3f MHz\n", (double)SEGGER_SYS_GetProcessorSpeed() / 1000000.0f);
  }
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_VERSION           = %u [%s]\n", CRYPTO_VERSION, CRYPTO_GetVersionText());
  SEGGER_SYS_IO_Printf("Config:   CRYPTO_MPI_BITS_PER_LIMB = %u\n",   CRYPTO_MPI_BITS_PER_LIMB);
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Modular Arithmetic Performance\n");
  SEGGER_SYS_IO_Printf("==============================\n\n");
  //
  for (i = 0; i < SEGGER_COUNTOF(_aBenchScenarios); ++i) {
    SEGGER_SYS_IO_Printf("%s, all times in ms\n\n", _aBenchScenarios[i].pText);
    _PrintHeader();
    for (_AlgIndex = 0; _AlgIndex < SEGGER_COUNTOF(_aBenchAlgs); ++_AlgIndex) {
      if (_aBenchAlgs[_AlgIndex].pfModExp == 0) {
        _PrintSeparator();
      } else {
        SEGGER_SYS_IO_Printf("| %-25s |", _aBenchAlgs[_AlgIndex].pText);
        for (_KeyIndex = 0; _KeyIndex < SEGGER_COUNTOF(_aBenchKeys); ++_KeyIndex) {
          _aBenchScenarios[i].pfBenchFunc(_aBenchAlgs[_AlgIndex].pfModExp, &_aBenchKeys[_KeyIndex]);
        }
        SEGGER_SYS_IO_Printf("\n");
      }
    }
    _PrintSeparator();
    SEGGER_SYS_IO_Printf("\n");
  }
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
