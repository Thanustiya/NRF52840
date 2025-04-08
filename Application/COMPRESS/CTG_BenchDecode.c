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

File    : CTG_BenchDecode.c
Purpose : Application to benchmark decompression performance.

Additional information:
  Sample output:
    Memory to Memory decompression
    Successfully expanded 9225 Bytes to 20895 Bytes.

    Function to Memory decompression
    Successfully expanded 9225 Bytes to 20895 Bytes.

    Memory to Function decompression
    Successfully expanded 9225 Bytes to 20895 Bytes.

    Function to Function decompression
    Successfully expanded 9225 Bytes to 20895 Bytes.
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CTG.h"
#include "CTG_Blog_Compressed.h"
#include "CTG_Blog_Uncompressed.h"
#include <stdio.h>

/*********************************************************************
*
*       Defines, configurable (if other files included)
*
**********************************************************************
*/

#define DECODED_FILE   ctg_blog_uncompressed_h_file
#define ENCODED_FILE   ctg_blog_compressed_h_file

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  const U8 * pInput;
  unsigned   InputLen;
  unsigned   Cursor;
} CTG_RD_CONTEXT;

typedef struct {
  U8       * pOutput;
  unsigned   OutputLen;
  unsigned   Cursor;
} CTG_WR_CONTEXT;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U8 _aDecoded[sizeof(DECODED_FILE)];
static U8 _aWork   [CTG_DECOMPRESS_WS_SIZE(CTG_FLAG_WINDOW_SIZE_512)];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _PrintStatus()
*
*  Function description
*    Print the status of the decompression and on success the ratio
*    between compressed and decompressed data.
*
*  Parameters
*    Status   - Return value of the decompression.
*
*  Return value
*    Number of bytes read.
*/
static void _PrintStatus(int Status) {
  if (Status < 0) {
    printf("Failed [system error].\n");
  } else if (Status != sizeof(DECODED_FILE)) {
    printf("Failed [size error].\n");
  } else if (memcmp(&_aDecoded[0], &DECODED_FILE, sizeof(DECODED_FILE)) != 0) {
    printf("Failed [bitstream error].\n");
  } else {
    printf("Successfully expanded %u Bytes to %d Bytes.\n\n", sizeof(ENCODED_FILE), Status);
  }
}

/*********************************************************************
*
*       _Rd()
*
*  Function description
*    Read data.
*
*  Parameters
*    pData   - Pointer to object that receives the data.
*    DataLen - Maximum number of bytes to read into receiving object.
*    pRdCtx  - Pointer to user-provided context.
*
*  Return value
*    Number of bytes read.
*/
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

/*********************************************************************
*
*       _Wr()
*
*  Function description
*    Write data.
*
*  Parameters
*    pData   - Pointer to object that contains the data to write.
*    DataLen - Number of bytes to write.
*    pWrCtx  - Pointer to user-provided context.
*
*  Return value
*    Number of bytes read.
*/
static int _Wr(const U8 *pData, unsigned DataLen, void *pWrCtx) {
  CTG_WR_CONTEXT * pCtx;
  unsigned         N;
  //
  pCtx = (CTG_WR_CONTEXT *)pWrCtx;
  for (N = 0; N < DataLen; ++N) {
    if (pCtx->Cursor >= pCtx->OutputLen) {
      return -1;  // Buffer overflow
    }
    pCtx->pOutput[pCtx->Cursor++] = *pData++;
  }
  return 1;
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
*    Main task.
*/
void MainTask(void);
void MainTask(void) {
  CTG_RD_CONTEXT     RdCtx;
  CTG_WR_CONTEXT     WrCtx;
  CTG_DECOMPRESS_CTX Ctx;
  CTG_STREAM         Stream;
  int                Status;
  //
  // Run decompression from memory to memory.
  //
  printf("Memory to Memory decompression\n");
  Status = CTG_DecompressM2M(&ENCODED_FILE[0], sizeof(ENCODED_FILE),
                             &_aDecoded[0], sizeof(_aDecoded));
  _PrintStatus(Status);
  //
  // Run decompression from function to memory.
  //
  printf("Function to Memory decompression\n");
  RdCtx.pInput   = &ENCODED_FILE[0];
  RdCtx.InputLen = sizeof(ENCODED_FILE);
  RdCtx.Cursor   = 0;
  Status = CTG_DecompressF2M(_Rd, &RdCtx,
                             &_aDecoded[0], sizeof(_aDecoded));
  _PrintStatus(Status);
  //
  // Run decompression from memory to function.
  //
  printf("Memory to Function decompression\n");
  WrCtx.pOutput   = &_aDecoded[0];
  WrCtx.OutputLen = sizeof(_aDecoded);
  WrCtx.Cursor    = 0;
  Status = CTG_DecompressM2F(&ENCODED_FILE[0], sizeof(ENCODED_FILE),
                             _Wr, &WrCtx,
                             &_aWork[0], sizeof(_aWork));
  _PrintStatus(Status);
  //
  // Run decompression from function to function.
  //
  printf("Function to Function decompression\n");
  RdCtx.pInput    = &ENCODED_FILE[0];
  RdCtx.InputLen  = sizeof(ENCODED_FILE);
  RdCtx.Cursor    = 0;
  WrCtx.pOutput   = &_aDecoded[0];
  WrCtx.OutputLen = sizeof(_aDecoded);
  WrCtx.Cursor    = 0;
  Status = CTG_DecompressF2F(_Rd, &RdCtx,
                             _Wr, &WrCtx,
                             &_aWork[0], sizeof(_aWork));
  _PrintStatus(Status);
  //
  // Run decompressor using stream API.
  //
  printf("Stream decompression\n");
  CTG_DecompressInit(&Ctx, &_aWork[0], sizeof(_aWork));
  Stream.pIn      = &ENCODED_FILE[0];
  Stream.AvailIn  = sizeof(ENCODED_FILE);
  Stream.Flush    = 1;
  Stream.pOut     = &_aDecoded[0];
  Stream.AvailOut = sizeof(_aDecoded);
  Status = CTG_Decompress(&Ctx, &Stream);
  if (Status == 0) {
    Status = CTG_STATUS_OUTPUT_OVERFLOW;
  }
  if (Status > 0) {
    Status = sizeof(_aDecoded) - Stream.AvailOut;
  }
  _PrintStatus(Status);
}
