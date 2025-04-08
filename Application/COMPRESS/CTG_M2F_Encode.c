/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2017 - 2024     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emCompress-ToGo * Compression library                        *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product.                          *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emCompress-ToGo version: V3.40.2                             *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : CTG_M2F_Encode.c
Purpose : Compress memory to function.
*/

#include "CTG.h"
#include <stdio.h>

static const U8 _aInput[] = {
  "Jabberwocky\n  BY LEWIS CARROLL\n\n\n"
  "'Twas brillig, and the slithy toves\n   Did gyre and gimble in the wabe:\n"
  "All mimsy were the borogoves,\n   And the mome raths outgrabe.\n\n"
  "\"Beware the Jabberwock, my son!\n   The jaws that bite, the claws that catch!\n"
  "Beware the Jubjub bird, and shun\n   The frumious Bandersnatch!\"\n\n"
  "He took his vorpal sword in hand;\n   Long time the manxome foe he sought-\n"
  "So rested he by the Tumtum tree\n   And stood awhile in thought.\n\n"
  "And, as in uffish thought he stood,\n   The Jabberwock, with eyes of flame,\n"
  "Came whiffling through the tulgey wood,\n   And burbled as it came!\n\n"
  "One, two! One, two! And through and through\n   The vorpal blade went snicker-snack!\n\n"
  "He left it dead, and with its head\n   He went galumphing back.\n\n"
  "\"And hast thou slain the Jabberwock?\n   Come to my arms, my beamish boy!\n"
  "O frabjous day! Callooh! Callay!\"\n\n   He chortled in his joy.\n"
  "'Twas brillig, and the slithy toves\n   Did gyre and gimble in the wabe:\n"
  "All mimsy were the borogoves,\n   And the mome raths outgrabe.\n"
};

static U8 _aOutput[sizeof(_aInput)*2];

typedef struct {  /*emDoc #1*/
  U8       * pOutput;
  unsigned   OutputLen;
  unsigned   Cursor;
} CTG_WR_CONTEXT;

static int _Wr(const U8 *pData, unsigned DataLen, void *pWrCtx) {  /*emDoc #2*/
  CTG_WR_CONTEXT * pCtx;
  unsigned         N;
  //
  pCtx = (CTG_WR_CONTEXT *)pWrCtx;  /*emDoc #3*/
  for (N = 0; N < DataLen; ++N) {
    if (pCtx->Cursor >= pCtx->OutputLen) {  /*emDoc #4*/
      return CTG_STATUS_OUTPUT_OVERFLOW;
    }
    pCtx->pOutput[pCtx->Cursor++] = *pData++;
  }
  return N;
}

void MainTask(void);
void MainTask(void) {
  CTG_WR_CONTEXT WrCtx;
  int            EncodeLen;
  //
  WrCtx.pOutput   = &_aOutput[0];  /*emDoc #5*/
  WrCtx.OutputLen = sizeof(_aOutput);
  WrCtx.Cursor    = 0;
  EncodeLen = CTG_CompressM2F(&_aInput[0],  sizeof(_aInput),  /*emDoc #6*/
                              _Wr, &WrCtx,
                              0);
  if (EncodeLen < 0) {
    printf("Compression error.\n");
  } else {
    printf("Compressed %u to %d bytes.\n", sizeof(_aInput), EncodeLen);
  }
}
