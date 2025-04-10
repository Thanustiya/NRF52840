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

File        : SECURE_RSA_Bench_Verify.c
Purpose     : Benchmark emSecure-RSA verify operation.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SECURE_RSA.h"
#include "SECURE_RSA_PublicKey_512b.h"
#include "SECURE_RSA_PublicKey_1024b.h"
#include "SECURE_RSA_PublicKey_2048b.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

typedef struct {
  const CRYPTO_RSA_PUBLIC_KEY * pKey;
  const U8                    * pMesssage;
  unsigned                      MessageLen;
  const U8                    * pSignature;
  unsigned                      SignatureLen;
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

static const U8 _aSignature_512b_0k[] = {
  0x3D, 0xB1, 0xB3, 0xA5, 0xBB, 0x27, 0x57, 0xCF,
  0x3A, 0x99, 0xF4, 0x6D, 0xA0, 0x65, 0xFF, 0x9C,
  0x85, 0x0C, 0x67, 0x1C, 0x29, 0x16, 0x75, 0xB9,
  0xB2, 0x0C, 0x0D, 0xD9, 0xDA, 0x82, 0xC8, 0x6A,
  0xDD, 0xFB, 0xFC, 0xC2, 0xDF, 0x27, 0xC1, 0x7A,
  0xBF, 0x01, 0x20, 0x68, 0xC6, 0x55, 0x4B, 0x97,
  0xC4, 0x47, 0x7B, 0x68, 0x99, 0xBC, 0x99, 0x16,
  0xA0, 0xF4, 0x40, 0xB9, 0x69, 0xE8, 0x14, 0xD1
};

static const U8 _aSignature_512b_1k[] = {
  0x4D, 0xDF, 0xB3, 0xAB, 0xB8, 0x2A, 0x65, 0xB9,
  0xC4, 0x1C, 0xD1, 0x2E, 0xCC, 0xBB, 0xA7, 0xAC,
  0xAD, 0x28, 0x89, 0xD1, 0x4B, 0xB5, 0x2F, 0x37,
  0xB5, 0x35, 0xA2, 0xE0, 0x6A, 0xC9, 0xF9, 0x43,
  0xE9, 0x98, 0x6A, 0xB8, 0x2A, 0x40, 0xBC, 0x2F,
  0x3D, 0x48, 0xA5, 0xBF, 0x69, 0xC5, 0x29, 0x46,
  0x64, 0xC0, 0xEA, 0xA1, 0x68, 0xA5, 0x7E, 0x21,
  0xE8, 0x5B, 0x7E, 0xD8, 0xAE, 0xF6, 0xE0, 0x41,
};

static const U8 _aSignature_512b_100k[] = {
  0x93, 0x57, 0xB1, 0xB2, 0x26, 0x3A, 0x07, 0x98,
  0xA5, 0xED, 0x7E, 0xFD, 0x42, 0x27, 0xFA, 0x00,
  0x0C, 0x40, 0x94, 0x9A, 0xB6, 0xFB, 0xFA, 0xAC,
  0x6E, 0xE1, 0xA7, 0x10, 0xC7, 0xD8, 0x44, 0xE4,
  0x98, 0x31, 0xC4, 0x87, 0xF3, 0xAF, 0x8D, 0x72,
  0xB1, 0x4B, 0x03, 0x10, 0x2D, 0xF0, 0x8A, 0x8B,
  0xE1, 0x5A, 0xA4, 0x26, 0x1D, 0x65, 0xFE, 0xD3,
  0x0E, 0x61, 0xBB, 0x50, 0x91, 0x83, 0x9C, 0x53,
};

static const U8 _aSignature_1024b_0k[] = {
  0x74, 0x33, 0x96, 0xD5, 0x2F, 0x88, 0x48, 0x59,
  0x66, 0x4D, 0xE8, 0xDA, 0x8F, 0x78, 0x54, 0x52,
  0xD1, 0x24, 0x11, 0x50, 0x98, 0xC7, 0x6A, 0xF5,
  0x2D, 0xCB, 0x9B, 0x97, 0xF3, 0xD3, 0xBC, 0x27,
  0xE3, 0x51, 0x65, 0x64, 0x29, 0x26, 0x94, 0xF0,
  0x0D, 0x59, 0xED, 0xA0, 0xC6, 0xC9, 0x3D, 0x14,
  0x72, 0x76, 0xBF, 0xB6, 0x4B, 0x22, 0xFE, 0xB7,
  0xE1, 0x49, 0x96, 0x9A, 0x18, 0xF2, 0x29, 0xB2,
  0x1F, 0x8B, 0x66, 0xDC, 0x66, 0x12, 0x1B, 0xBA,
  0x2E, 0x04, 0xC9, 0x99, 0x59, 0xB0, 0xD5, 0x32,
  0x8F, 0x49, 0xCC, 0x9D, 0xF2, 0x78, 0xFD, 0x55,
  0x5F, 0x5B, 0xE2, 0x26, 0x01, 0x3D, 0xC9, 0x3D,
  0x95, 0xF7, 0x3E, 0x87, 0xFE, 0xDD, 0xF6, 0x5B,
  0xCB, 0xAA, 0x5D, 0x88, 0xDC, 0x1A, 0x72, 0x0D,
  0x3D, 0xD9, 0x0C, 0x5D, 0xA4, 0x7D, 0x81, 0x11,
  0x1A, 0xE2, 0xD8, 0x06, 0x45, 0x7D, 0x89, 0x4F,
};

static const U8 _aSignature_1024b_1k[] = {
  0x6A, 0x99, 0x33, 0x8D, 0xEA, 0x6E, 0x66, 0xF6,
  0x2F, 0xD2, 0xAD, 0x42, 0x62, 0xFF, 0x7C, 0xDE,
  0xEC, 0x2C, 0x6C, 0x49, 0x42, 0x33, 0x93, 0xB0,
  0x91, 0x1F, 0x18, 0x07, 0x67, 0x81, 0xAE, 0x24,
  0xD1, 0xE7, 0x54, 0x88, 0xFB, 0x8F, 0x16, 0x7F,
  0xFB, 0x37, 0x0D, 0x4C, 0x19, 0x9F, 0xD4, 0x56,
  0xA7, 0x79, 0xD9, 0x2D, 0x60, 0xC2, 0x3C, 0xD2,
  0xBC, 0x54, 0x3A, 0x9F, 0xFC, 0x82, 0x45, 0x77,
  0xF2, 0x29, 0x01, 0x0C, 0xBC, 0x74, 0x29, 0x52,
  0x9A, 0x53, 0x59, 0xB8, 0x6E, 0x3A, 0xF7, 0x92,
  0xF6, 0x5C, 0xFC, 0x87, 0x47, 0x0B, 0x69, 0x0E,
  0xD5, 0x2E, 0x1D, 0xD7, 0x26, 0x1D, 0x4E, 0xE0,
  0xEB, 0x2B, 0xFE, 0xB3, 0x1D, 0x09, 0x1E, 0xAB,
  0x74, 0x41, 0xDB, 0x3B, 0x4D, 0x9A, 0xC9, 0xAF,
  0x5C, 0xAD, 0x79, 0xAF, 0xD8, 0xC0, 0xE5, 0x43,
  0x0F, 0xE8, 0xD7, 0x87, 0xF9, 0x65, 0x66, 0x3C,
};

static const U8 _aSignature_1024b_100k[] = {
  0x7D, 0x80, 0x54, 0x0D, 0x82, 0x1B, 0xD3, 0x1F,
  0x4A, 0x0F, 0x0D, 0x36, 0x63, 0xA2, 0x4C, 0xE4,
  0x9E, 0x38, 0xD4, 0x78, 0x42, 0x35, 0x56, 0x1B,
  0x12, 0xDA, 0x20, 0x1E, 0xF5, 0x95, 0x51, 0xC2,
  0xF5, 0x01, 0x16, 0xF2, 0xB8, 0xA2, 0x93, 0x1C,
  0xB6, 0xE9, 0x78, 0xEF, 0xC0, 0xBF, 0x14, 0xA8,
  0x4B, 0x80, 0x4B, 0xD1, 0x16, 0x65, 0x1C, 0x3F,
  0xAC, 0x7C, 0x19, 0x08, 0x7C, 0x27, 0x9E, 0xC2,
  0xE3, 0x1D, 0xE3, 0xA0, 0xA3, 0x1C, 0x0F, 0x64,
  0x5F, 0x86, 0x93, 0xA0, 0x90, 0x48, 0x38, 0x5A,
  0x1B, 0xE2, 0xDA, 0x75, 0x19, 0x4E, 0xC1, 0x16,
  0x94, 0x92, 0xFA, 0xAA, 0xD8, 0xB2, 0xDF, 0x48,
  0x9C, 0xE2, 0x76, 0xE0, 0xEF, 0xC5, 0xCA, 0x97,
  0xDC, 0x9D, 0x31, 0x90, 0x18, 0xE9, 0x0B, 0x94,
  0xFA, 0x46, 0x1B, 0xB3, 0x89, 0xD1, 0x3C, 0x1B,
  0x23, 0xB2, 0x00, 0x02, 0x32, 0x6D, 0xA5, 0xF4,
};

static const U8 _aSignature_2048b_0k[] = {
  0x22, 0x78, 0x2E, 0xE4, 0xB9, 0x3A, 0x8C, 0x57,
  0x8F, 0xD9, 0x91, 0x8E, 0x5C, 0x5C, 0x5F, 0xED,
  0x39, 0xEF, 0xB8, 0x84, 0x92, 0x2F, 0x77, 0x9B,
  0x8D, 0x0D, 0xD9, 0xAE, 0x0C, 0xCE, 0x51, 0x82,
  0x2C, 0xCD, 0x7E, 0x68, 0x40, 0x70, 0xD4, 0xF6,
  0x10, 0x2F, 0x12, 0xFA, 0x59, 0x4B, 0xF0, 0x88,
  0xF8, 0xA0, 0x4D, 0x0A, 0xC3, 0xEB, 0xA9, 0xDD,
  0x05, 0xD3, 0x87, 0x2B, 0xF7, 0x61, 0x3B, 0xEF,
  0xC2, 0xCF, 0x50, 0x0A, 0x30, 0xE7, 0x64, 0x02,
  0x46, 0x4D, 0x9B, 0x71, 0x47, 0x98, 0x16, 0x42,
  0xF2, 0xF8, 0x4F, 0x39, 0xE3, 0xFF, 0xFC, 0x95,
  0xCA, 0x2E, 0xBA, 0x3F, 0xFF, 0x08, 0x98, 0xDE,
  0x8C, 0x60, 0x04, 0xB2, 0x43, 0x40, 0x87, 0xF8,
  0x45, 0xC6, 0x4F, 0x5C, 0xF8, 0x49, 0xCD, 0x0A,
  0xBB, 0x98, 0x46, 0xF2, 0xCE, 0x74, 0xA6, 0x90,
  0xFD, 0x87, 0xE2, 0xEC, 0xC6, 0x5F, 0xBA, 0x2A,
  0x3B, 0x38, 0x90, 0x2B, 0x7D, 0x15, 0xB2, 0xCD,
  0x93, 0x91, 0x4F, 0xD9, 0x76, 0xE0, 0xCB, 0x6A,
  0x58, 0x5E, 0xCC, 0x57, 0x2D, 0x00, 0xA6, 0xB2,
  0x27, 0xD3, 0xED, 0x9F, 0xF6, 0x6D, 0x4E, 0xC5,
  0xFE, 0xAC, 0xF3, 0xEF, 0x03, 0x7A, 0x93, 0xF1,
  0x7C, 0x60, 0xDB, 0x2D, 0xC0, 0x55, 0x0B, 0xB5,
  0x82, 0x0C, 0x1A, 0xF0, 0x4B, 0x14, 0xD0, 0x09,
  0xEA, 0xCE, 0x2E, 0x8B, 0xB1, 0x2E, 0x40, 0x4D,
  0x72, 0x36, 0x83, 0x84, 0xCA, 0x5B, 0xDA, 0x5B,
  0xC0, 0x6E, 0x67, 0xAF, 0x80, 0x5B, 0xDD, 0x7F,
  0xF7, 0x9C, 0x28, 0x64, 0x80, 0x5B, 0xDD, 0x1E,
  0xD0, 0x5D, 0x5D, 0x2E, 0x31, 0x62, 0x73, 0xA2,
  0x3E, 0x13, 0x47, 0x4B, 0xEB, 0xC9, 0xC2, 0x1C,
  0xE8, 0x15, 0x45, 0xCE, 0x0D, 0xBC, 0x42, 0xD0,
  0x5E, 0xD4, 0xCC, 0x2B, 0x2A, 0x7F, 0x0B, 0x73,
  0xCD, 0x42, 0xBD, 0x27, 0x78, 0x8D, 0xC0, 0xA9,
};

static const U8 _aSignature_2048b_1k[] = {
  0x80, 0xFC, 0x26, 0x0E, 0xCA, 0x0F, 0x11, 0xB0,
  0x3C, 0xA0, 0xA6, 0xEA, 0xF0, 0xB6, 0x04, 0xAC,
  0xE7, 0xD6, 0xF5, 0x2B, 0xCB, 0x7D, 0x1C, 0x5A,
  0x28, 0x3E, 0xCD, 0x93, 0x68, 0xC2, 0x82, 0xAC,
  0x5E, 0x01, 0x94, 0x49, 0xD0, 0xAD, 0x71, 0xCB,
  0x3D, 0x1B, 0xEA, 0x66, 0xD4, 0xBB, 0x2B, 0xA7,
  0xF6, 0x8A, 0xF3, 0x4B, 0x01, 0xA9, 0x6B, 0xCA,
  0x73, 0x4D, 0x2B, 0x08, 0xD2, 0xE2, 0x97, 0x96,
  0x71, 0xCA, 0xF0, 0x33, 0x94, 0xB5, 0xAC, 0x12,
  0xF3, 0xB5, 0xFE, 0xA6, 0x07, 0x37, 0x9E, 0x2E,
  0xEC, 0xCC, 0x3D, 0xFC, 0xFD, 0x1F, 0x62, 0x5F,
  0x0E, 0x52, 0x7B, 0x22, 0x5A, 0xC5, 0xE9, 0x18,
  0x4C, 0x2C, 0x57, 0x6A, 0x3C, 0x8C, 0xC0, 0xB5,
  0xE3, 0xFE, 0x66, 0xE9, 0x04, 0x49, 0x9C, 0x9A,
  0x3F, 0x2F, 0x8B, 0x12, 0x37, 0x33, 0xC8, 0x7F,
  0x86, 0xEC, 0xD6, 0xED, 0x41, 0x63, 0x0F, 0xD9,
  0xA0, 0xBD, 0x58, 0xFF, 0xA6, 0xC8, 0x38, 0x38,
  0x27, 0xF2, 0xF2, 0xC7, 0xED, 0x4E, 0x8A, 0x06,
  0x2A, 0x67, 0xCD, 0x02, 0x39, 0x86, 0x3F, 0x4D,
  0xAD, 0x17, 0x78, 0x72, 0xB2, 0x79, 0x40, 0x2D,
  0x25, 0xC7, 0x43, 0xB2, 0xA3, 0x64, 0x42, 0xF2,
  0x23, 0x50, 0xD4, 0xF3, 0xD7, 0x85, 0x20, 0x2E,
  0x72, 0xF1, 0xEE, 0xAE, 0xE2, 0xC2, 0x72, 0x2E,
  0x68, 0x9D, 0x9E, 0x6B, 0x0A, 0xBE, 0x21, 0x22,
  0x78, 0x0E, 0x76, 0x1B, 0x43, 0x38, 0xB6, 0xC6,
  0x61, 0x4B, 0xB3, 0x11, 0x42, 0x5F, 0x5A, 0x49,
  0x9C, 0xB6, 0x35, 0x59, 0xC0, 0x92, 0x0A, 0x4B,
  0xEB, 0x18, 0x42, 0x9D, 0x28, 0xD9, 0x21, 0x8A,
  0x8E, 0xF6, 0xA3, 0x10, 0xAC, 0xE8, 0x1C, 0xF8,
  0x14, 0xC5, 0x54, 0xD1, 0x47, 0xCC, 0x3A, 0xE1,
  0xF7, 0xDE, 0xEE, 0x0B, 0x50, 0x81, 0x85, 0x7E,
  0x24, 0xBD, 0x31, 0x11, 0x64, 0x78, 0x82, 0x95,
};

static const U8 _aSignature_2048b_100k[] = {
  0x5A, 0x75, 0x62, 0xF9, 0xA9, 0x0C, 0xC3, 0xFD,
  0x93, 0x78, 0x35, 0x33, 0x0B, 0x12, 0xD8, 0xA5,
  0x40, 0xAD, 0x43, 0x23, 0xE6, 0xE0, 0xE4, 0x9A,
  0xA5, 0xCC, 0x9A, 0x94, 0xE7, 0x02, 0xE9, 0xAF,
  0x73, 0xE2, 0x3B, 0xC4, 0xEE, 0xCF, 0xA3, 0xE5,
  0x94, 0x31, 0x82, 0x82, 0xDB, 0x81, 0x90, 0x2F,
  0x46, 0x93, 0x02, 0x39, 0x28, 0xDF, 0x9D, 0x9F,
  0xAE, 0xF7, 0x40, 0x4D, 0x89, 0xF3, 0x06, 0x85,
  0x33, 0xC6, 0x18, 0x77, 0x29, 0x16, 0x72, 0x38,
  0xC7, 0x26, 0xCC, 0x78, 0xC0, 0x46, 0xB3, 0x6E,
  0x4C, 0x0A, 0x8B, 0x74, 0xC1, 0x4C, 0xBF, 0xD2,
  0x64, 0x0A, 0x8E, 0xE3, 0xEF, 0xBB, 0x99, 0x8F,
  0xA3, 0x8E, 0x0D, 0x9D, 0xF8, 0xE5, 0xF3, 0x09,
  0x2D, 0x82, 0x93, 0x0C, 0x89, 0x31, 0x85, 0xA0,
  0xCD, 0x9B, 0x67, 0xC4, 0x59, 0xB0, 0x37, 0x48,
  0x26, 0x44, 0x7A, 0xE0, 0x77, 0x68, 0x90, 0xFF,
  0x8D, 0xDB, 0x1E, 0x4B, 0x1E, 0xAB, 0x4F, 0xC4,
  0x2F, 0xBF, 0x3F, 0x92, 0x09, 0xD1, 0x15, 0xA5,
  0x79, 0xCF, 0xD1, 0x1A, 0xC8, 0x8D, 0x1B, 0x4A,
  0xCC, 0x0F, 0xC7, 0x4B, 0x77, 0x95, 0xAB, 0x9A,
  0xBE, 0xE6, 0xBE, 0x3C, 0x09, 0x52, 0x44, 0x91,
  0x1E, 0x91, 0xB3, 0x2F, 0xE0, 0x9B, 0x38, 0x53,
  0x3C, 0x24, 0xA8, 0x11, 0xFD, 0x04, 0x91, 0xDB,
  0x3A, 0x8B, 0x97, 0x2F, 0x5C, 0x18, 0xD0, 0x0D,
  0xA6, 0xB1, 0x83, 0x38, 0x83, 0x70, 0xFE, 0xE8,
  0x6D, 0x08, 0xC1, 0x26, 0x92, 0x75, 0x7A, 0x94,
  0xA0, 0xDF, 0xF3, 0x73, 0xB5, 0xF4, 0xA5, 0x1A,
  0x69, 0xF7, 0x41, 0xC1, 0x76, 0xF7, 0xFB, 0x59,
  0x5A, 0x75, 0x07, 0xD5, 0x81, 0x94, 0xDE, 0x84,
  0x4A, 0xFA, 0x7E, 0x70, 0xF1, 0x1A, 0x66, 0x00,
  0xDC, 0x5A, 0x72, 0xC9, 0x56, 0x81, 0xDF, 0xBF,
  0x86, 0xB5, 0x4A, 0xC0, 0x20, 0x5A, 0x94, 0x13,
};

static const BENCH_PARA _aBenchKeys[] = {
  { &_SECURE_RSA_PublicKey_512b,  _aMessage_100k,        0u, _aSignature_512b_0k,    sizeof(_aSignature_512b_0k)    },
  { &_SECURE_RSA_PublicKey_512b,  _aMessage_100k,     1024u, _aSignature_512b_1k,    sizeof(_aSignature_512b_1k)    },
  { &_SECURE_RSA_PublicKey_512b,  _aMessage_100k, 100*1024u, _aSignature_512b_100k,  sizeof(_aSignature_512b_100k)  },
  { NULL,                         NULL,                  0u, NULL,                   0u                             },
  { &_SECURE_RSA_PublicKey_1024b, _aMessage_100k,        0u, _aSignature_1024b_0k,   sizeof(_aSignature_1024b_0k)   },
  { &_SECURE_RSA_PublicKey_1024b, _aMessage_100k,     1024u, _aSignature_1024b_1k,   sizeof(_aSignature_1024b_1k)   },
  { &_SECURE_RSA_PublicKey_1024b, _aMessage_100k, 100*1024u, _aSignature_1024b_100k, sizeof(_aSignature_1024b_100k) },
  { NULL,                         NULL,                  0u, NULL,                   0u                             },
  { &_SECURE_RSA_PublicKey_2048b, _aMessage_100k,        0u, _aSignature_2048b_0k,   sizeof(_aSignature_2048b_0k)   },
  { &_SECURE_RSA_PublicKey_2048b, _aMessage_100k,     1024u, _aSignature_2048b_1k,   sizeof(_aSignature_2048b_1k)   },
  { &_SECURE_RSA_PublicKey_2048b, _aMessage_100k, 100*1024u, _aSignature_2048b_100k, sizeof(_aSignature_2048b_100k) }
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
*       _BenchmarkVerify()
*
*  Function description
*    Count the number of signings completed in one second.
*
*  Parameters
*    pPara - Pointer to benchmark parameters.
*/
static void _BenchmarkVerify(const BENCH_PARA *pPara) {
  U64   OneSecond;
  U64   T0;
  U64   Elapsed;
  int   Loops;
  int   Status;
  float Time;
  //
  Loops = 0;
  OneSecond = SEGGER_SYS_OS_ConvertMicrosToTicks(1000000);
  T0 = SEGGER_SYS_OS_GetTimer();
  do {
    Status = SECURE_RSA_Verify(pPara->pKey, NULL, 0, pPara->pMesssage, pPara->MessageLen, pPara->pSignature, pPara->SignatureLen);
    Elapsed = SEGGER_SYS_OS_GetTimer() - T0;
    ++Loops;
  } while (Status >= 0 && Elapsed < OneSecond);
  //
  Time = 1000.0f * _ConvertTicksToSeconds(Elapsed) / Loops;
  if (Status <= 0) {
    SEGGER_SYS_IO_Printf("| %8d | %8u | %8s |\n", CRYPTO_MPI_BitCount(&pPara->pKey->N), pPara->MessageLen, "-Fail-");
  } else {
    SEGGER_SYS_IO_Printf("| %8d | %8u | %8.2f |\n", CRYPTO_MPI_BitCount(&pPara->pKey->N), pPara->MessageLen, Time);
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
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", SECURE_RSA_GetCopyrightText());
  SEGGER_SYS_IO_Printf("emSecure-RSA Verify Benchmark compiled " __DATE__ " " __TIME__ "\n\n");
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
  SEGGER_SYS_IO_Printf("Verify Performance\n");
  SEGGER_SYS_IO_Printf("==================\n\n");
  //
  SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("|  Modulus |  Message |   Verify |\n");
  SEGGER_SYS_IO_Printf("|    /bits |   /bytes |      /ms |\n");
  SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  for (i = 0; i < SEGGER_COUNTOF(_aBenchKeys); ++i) {
    if (_aBenchKeys[i].pKey == NULL) {
      SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
    } else {
      _BenchmarkVerify(&_aBenchKeys[i]);
    }
  }
  SEGGER_SYS_IO_Printf("+----------+----------+----------+\n");
  SEGGER_SYS_IO_Printf("\n");
  //
  SEGGER_SYS_IO_Printf("Benchmark complete\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
