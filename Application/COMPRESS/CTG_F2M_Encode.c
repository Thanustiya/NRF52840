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

File    : CTG_F2M_Encode.c
Purpose : Compress function to memory
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

typedef struct {
  const U8 * pInput;
  unsigned   InputLen;
  unsigned   Cursor;
} CTG_RD_CONTEXT;

static int _Rd(U8 *pData, unsigned DataLen, void *pRdCtx) {
  CTG_RD_CONTEXT * pCtx;
  unsigned         N;
  //
  pCtx = (CTG_RD_CONTEXT *)pRdCtx;
  for (N = 0; N < DataLen; ++N) {
    if (pCtx->Cursor >= pCtx->InputLen) {
      break;
    }
    *pData++ = pCtx->pInput[pCtx->Cursor++];
  }
  return N;
}

void MainTask(void);
void MainTask(void) {
  CTG_RD_CONTEXT RdCtx;
  int            Status;
  U8             aWork[CTG_COMPRESS_WS_SIZE(CTG_FLAG_WINDOW_SIZE_1K)];  /*emDoc #1*/
  //
  RdCtx.pInput   = &_aInput[0];
  RdCtx.InputLen = sizeof(_aInput);
  RdCtx.Cursor   = 0;
  //
  Status = CTG_CompressF2M(_Rd, &RdCtx,  /*emDoc #2*/
                           &_aOutput[0], sizeof(_aOutput),
                           &aWork[0], sizeof(aWork),  /*emDoc #3*/
                           CTG_FLAG_WINDOW_SIZE_1K);  /*emDoc #4*/
  if (Status >= 0) {
    printf("Compressed %u to %d bytes.\n", sizeof(_aInput), Status);
  } else {
    printf("Compression error.\n");
  }
}
