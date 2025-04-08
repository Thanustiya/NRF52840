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

File    : CTG_BenchEncode.c
Purpose : Application to benchmark compression performance.

Additional information:
  Sample output:
    Memory to Memory compression
    Successfully compressed 20895 Bytes to 9225 Bytes.

    Function to Memory compression
    Successfully compressed 20895 Bytes to 9225 Bytes.

    Memory to Function compression
    Successfully compressed 20895 Bytes to 9225 Bytes.

    Function to Function compression
    Successfully compressed 20895 Bytes to 9225 Bytes.
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

static U8  _aEncoded[sizeof(DECODED_FILE)];
static U16 _aWorkMem[CTG_FCOMPRESS_M_WS_SIZE(CTG_FLAG_WINDOW_SIZE_512) / 2];

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
*    Print the status of the compression and on success the ratio
*    between uncompressed and compressed data.
*
*  Parameters
*    Status   - Return value of the compression.
*
*  Return value
*    Number of bytes read.
*/
static void _PrintStatus(int Status) {
  if (Status < 0) {
    printf("Failed [system error %d].\n", Status);
  } else if (Status != sizeof(ENCODED_FILE)) {
    printf("Failed [size error].\n");
  } else if (memcmp(&_aEncoded[0], &ENCODED_FILE, sizeof(ENCODED_FILE)) != 0) {
    printf("Failed [bitstream error].\n");
  } else {
    printf("Successfully compressed %u Bytes to %d Bytes.\n\n", sizeof(DECODED_FILE), Status);
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
  CTG_RD_CONTEXT   RdCtx;
  CTG_WR_CONTEXT   WrCtx;
  CTG_COMPRESS_CTX Ctx;
  CTG_STREAM       Stream;
  int              Status;
  //
  // Run compressor from memory to memory.
  //
  printf("Memory to Memory compression\n");
  Status = CTG_CompressM2M(&DECODED_FILE[0], sizeof(DECODED_FILE),
                           &_aEncoded[0], sizeof(_aEncoded),
                           CTG_FLAG_WINDOW_SIZE_512);
  _PrintStatus(Status);
  //
  // Run compressor from function to memory.
  //
  printf("Function to Memory compression\n");
  RdCtx.pInput   = &DECODED_FILE[0];
  RdCtx.InputLen = sizeof(DECODED_FILE);
  RdCtx.Cursor   = 0;
  Status = CTG_CompressF2M(_Rd, &RdCtx,
                           &_aEncoded[0], sizeof(_aEncoded),
                           (U8 *)&_aWorkMem[0], sizeof(_aWorkMem),
                           CTG_FLAG_WINDOW_SIZE_512);
  _PrintStatus(Status);
  //
  // Run compressor from memory to function.
  //
  printf("Memory to Function compression\n");
  WrCtx.pOutput   = &_aEncoded[0];
  WrCtx.OutputLen = sizeof(_aEncoded);
  WrCtx.Cursor    = 0;
  Status = CTG_CompressM2F(&DECODED_FILE[0], sizeof(DECODED_FILE),
                           _Wr, &WrCtx,
                           CTG_FLAG_WINDOW_SIZE_512);
  _PrintStatus(Status);
  //
  // Run compressor from function to function.
  //
  printf("Function to Function compression\n");
  RdCtx.pInput    = &DECODED_FILE[0];
  RdCtx.InputLen  = sizeof(DECODED_FILE);
  RdCtx.Cursor    = 0;
  WrCtx.pOutput   = &_aEncoded[0];
  WrCtx.OutputLen = sizeof(_aEncoded);
  WrCtx.Cursor    = 0;
  Status = CTG_CompressF2F(_Rd, &RdCtx,
                           _Wr, &WrCtx,
                           (U8 *)&_aWorkMem[0], sizeof(_aWorkMem),
                           CTG_FLAG_WINDOW_SIZE_512);
  _PrintStatus(Status);
  //
  // Run compressor using stream API.
  //
  printf("Stream compression\n");
  CTG_CompressInit(&Ctx, (U8 *)&_aWorkMem[0], sizeof(_aWorkMem), CTG_FLAG_WINDOW_SIZE_512);
  Stream.pIn      = &DECODED_FILE[0];
  Stream.AvailIn  = sizeof(DECODED_FILE);
  Stream.Flush    = 1;
  Stream.pOut     = &_aEncoded[0];
  Stream.AvailOut = sizeof(_aEncoded);
  Status = CTG_Compress(&Ctx, &Stream);
  if (Status == 0) {
    Status = CTG_STATUS_OUTPUT_OVERFLOW;
  }
  if (Status > 0) {
    Status = sizeof(_aEncoded) - Stream.AvailOut;
  }
  _PrintStatus(Status);
  //
  // Run fast compressor from memory to memory.
  //
  printf("Fast Memory to Memory compression\n");
  Status = CTG_FastCompressM2M(&DECODED_FILE[0], sizeof(DECODED_FILE),
                               &_aEncoded[0], sizeof(_aEncoded),
                               &_aWorkMem[0], sizeof(_aWorkMem),
                               CTG_FLAG_WINDOW_SIZE_512);
  _PrintStatus(Status);
}
