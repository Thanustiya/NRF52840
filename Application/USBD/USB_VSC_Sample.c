/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device * USB Device stack for embedded applications    *
*                                                                    *
*       Please note: Knowledge of this file may under no             *
*       circumstances be used to write a similar product.            *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device version: V3.66.0                                *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

Purpose : USB VSC sample showing different ways of data transfers
          including benchmark testing.

Additional information:
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\BULK\SampleApplication\TestV

    On Windows this sample requires the WinUSB driver.
    This driver, if not already installed, is retrieved via
    Windows Update. If Windows Update is disabled you can install
    the driver manually, see \Windows\USB\BULK\WinUSBInstall .

    On Linux either root or udev rules are required to access
    the bulk device, see \Windows\USB\BULK\USBBULK_API_Linux .

    On macOS bulk devices can be accessed without additional
    changes, see \Windows\USB\BULK\USBBULK_API_MacOSX .

  Expected behavior:
    When the Echo1 sample is started on the PC it should display
    information about the connected bulk device.
    The sample should transmit the user specified number of bytes
    to the target and back.

  Sample output:
    Output of the PC program:
      Found 1 device
      Found the following device 0:
        Vendor Name:      Vendor
        Product Name:     VSC Sample
        Serial no.:       12345678
        NumAltInterfaces: 0
          Alt(0)
                0x81: BULK - 200(512)
                0x01: BULK - 200(512)
      Speed: HIGH speed
      To which device do you want to connect?
      Please type in device number (e.g. '0' for the first device, q/a for abort):
      Choose your operation:
        a) Perform simple echo operation.
        b) Perform simple echo operation using the receive read method.
        c) Perform big data read operation.
        d) Perform big data write operation.
        e) Perform benchmark read operation.
        f) Perform benchmark write operation.
        g) Reconnect to a device
        q) Quit.
      All read and write operation are seen from the target side perspective
      Your choice:
      [....]
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "USB_VSC.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USE_CUSTOME_MSOS_DESC        1    // Can be used to give a self define MS OS Descriptor
#define BUFFER_SIZE               8192
#define CRC_POLYNOM               0xEDB88320uL

/*********************************************************************
*
*       Typedefs
*
**********************************************************************
*/
typedef struct {
  USB_VSC_HANDLE hInst;
  U8             hWriteEP;
  U8             hReadEP;
} VSC_SAMPLE_INST;

typedef enum {
  VSC_CMD_ECHO_VAR_RECEIVE,
  VSC_CMD_ECHO_VAR_RECEIVE_POLL,
  VSC_CMD_BIG_DATA_READ,
  VSC_CMD_BIG_DATA_WRITE,
  VSC_CMD_BENCH_WRITE,
  VSC_CMD_BENCH_READ,
  VSC_CMD_INVALID = 0x7FFFFFFF
} VSC_CMD;

typedef struct {
  VSC_CMD Cmd;
  U32 Para1;
  U16 Para2;
  U16 Para3;
} VSC_CMD_INFO;

typedef struct {
  U32 Res1;
  U32 Res2;
  U16 Res3;
} VSC_RESULT;

typedef struct {
  VSC_SAMPLE_INST     * pInst;
  VSC_CMD_INFO          TestModeCmd;
  VSC_RESULT            TestResult;
  U32                   Seed;
  U32                   Result;
  volatile U32          BytesToWrite;
  volatile U32          BytesToRead;
  U32                   BytesProcessed;
  U8                    *pBuff;
  U8                    *pBuff1;
  U8                    Buff [BUFFER_SIZE];
  U8                    Buff1[BUFFER_SIZE];
  U8                    BuffExt[64];             // Space for buffer alignment
} VSC_CONTEXT;

/*********************************************************************
*
*       Static const
*
**********************************************************************
*/
/*********************************************************************
*
*       Information that are used during enumeration
*/
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x2000,         // ProductId
  "Vendor",       // VendorName
  "VSC Sample",   // ProductName
  "12345678"      // SerialNumber
};

#if USE_CUSTOME_MSOS_DESC
// static const U32 _IsEnabled   = 1uL;
// static const U32 _IdleTimeout = 10000uL;  // Time given in ms.

static const USB_MS_OS_EXT_PROP _aProperties[] = {
    {USB_MSOS_EXT_PROPTYPE_REG_SZ,    "DeviceInterfaceGUID",      "{C78607E8-DE76-458B-B7C1-5C14A6F3A1D2}", 0},
//
// Here are additional settings that can be used for WinUSB under Windows
// Per default these are commented out. Once the device has been enumerated to a Windows PC
// The values are cached and Windows will never ask for it again. In order to do so,
// the steps in the Wiki [https://wiki.segger.com/MS_OS_Descriptors] need to be followed.
// Refer to
//
    //{USB_MSOS_EXT_PROPTYPE_REG_DWORD, "DefaultIdleTimeout",       &_IdleTimeout, 0},
    //{USB_MSOS_EXT_PROPTYPE_REG_DWORD, "DefaultIdleState",         &_IsEnabled, 0},
    //{USB_MSOS_EXT_PROPTYPE_REG_DWORD, "DeviceSelectiveSuspended", &_IsEnabled, 0},
    //{USB_MSOS_EXT_PROPTYPE_REG_DWORD, "UserSetDeviceIdleEnabled", &_IsEnabled, 0},
    //{USB_MSOS_EXT_PROPTYPE_REG_DWORD, "SystemWakeEnabled",        &_IsEnabled, 0},
};

static const USB_VSC_MSOSDESC_INFO _MSDescInfo = {
  .sCompatibleID = "WINUSB",
  .sSubCompatibleID = NULL,
  .NumProperties = SEGGER_COUNTOF(_aProperties),
  .pProperties = _aProperties
};
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static  VSC_SAMPLE_INST _Inst;
static VSC_CONTEXT      _VscContext;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ErrorStop
*/
static void _ErrorStop(void) {
  USB_PANIC("Failed to continue, command flow wrong!");
}

/*********************************************************************
*
*       _AlignBuff
*/
static U8 * _AlignBuff(U8 *pBuff, unsigned AlignVal, unsigned BufferAdd) {
  PTR_ADDR  Addr;

  if (AlignVal != 0u && AlignVal <= BufferAdd) {
    Addr = SEGGER_PTR2ADDR(pBuff);
    Addr = (Addr + AlignVal - 1u) & ~(AlignVal - 1u);
    if (AlignVal < 8) {
      Addr |= AlignVal;     // make sure, buffer is not aligned to 2*AlignVal
    }
    pBuff = SEGGER_ADDR2PTR(U8, Addr);
  }
  return pBuff;
}

/*********************************************************************
*
*       _Rand
*/
static int _Rand(U32 *pSeed) {
  return (((*pSeed = *pSeed * 214013L + 2531011L) >> 16) & 0x7fff);
}

/*********************************************************************
*
*       _CRC_Calc32
*/
static U32 _CRC_Calc32(const U8 *pBuffer, unsigned BufferLen, U32 Crc) {
  static U32 _aTable[256];
  static int _HaveTable = 0;
  U32 Remainder;
  U8 ByteItem;
  int i;
  int j;
  const U8 * pItem;
  const U8 * pEnd;

  //
  // Generate a table to be used later to generate the CRC. To optimize this, we may use the pre-generated CRC table.
  //
  if (_HaveTable == 0) {
    /* Calculate CRC table. */
    for (i = 0; i < 256; i++) {
      Remainder = i;  /* remainder from polynomial division */
      for (j = 0; j < 8; j++) {
        if (Remainder & 1) {
          Remainder >>= 1;
          Remainder ^= CRC_POLYNOM;
        } else {
          Remainder >>= 1;
        }
      }
      _aTable[i] = Remainder;
     }
    _HaveTable = 1;
  }

  Crc = ~Crc;
  pEnd = pBuffer + BufferLen;
  for (pItem = pBuffer; pItem < pEnd; pItem++) {
    ByteItem = *pItem;
    Crc = (Crc >> 8) ^ _aTable[(Crc & 0xff) ^ ByteItem];
  }
  return ~Crc;
}

/*********************************************************************
*
*       _CRC_Calc_Rand
*/
static U32 _CRC_Calc_Rand(VSC_CONTEXT * pCtx, unsigned NumBytes) {
  U32 Crc;
  U32 Cnt;
  U32 i;
  U32 SeedSave;
  U8  buff[100];

  SeedSave = pCtx->Seed;
  Crc = 0xFFFFFFFF;
  while (NumBytes > 0) {
    if (NumBytes > sizeof(buff)) {
      Cnt = sizeof(buff);
    } else {
      Cnt = NumBytes;
    }
    NumBytes -= Cnt;
    for (i = 0; i < Cnt; i++) {
      buff[i] = _Rand(&pCtx->Seed);
    }
    Crc = _CRC_Calc32(buff, Cnt, Crc);
  }
  pCtx->Seed = SeedSave;
  return Crc;
}

/*********************************************************************
*
*       _CheckCmd
*
*  Function description
*    Check if acBuff contains new test type command
*/
static int _CheckCmd(VSC_CONTEXT * pContext, int NumBytes) {
  U8 Answ[4];
  U32 Cmd;
  if (NumBytes >= 12 && pContext->pBuff[0] == '#' && pContext->pBuff[1] == '#' && pContext->pBuff[2] == '#') {
    Cmd                         = USBD_GetU32LE((U8 *)pContext->pBuff);
    pContext->TestModeCmd.Para1 = USBD_GetU32LE((U8 *)pContext->pBuff + 4);
    pContext->TestModeCmd.Para2 = USBD_GetU16LE((U8 *)pContext->pBuff + 8);
    pContext->TestModeCmd.Para3 = USBD_GetU16LE((U8 *)pContext->pBuff + 10);
    //
    // acknowledge command
    //
    USBD_StoreU32LE(Answ, (U32)pContext->TestModeCmd.Cmd ^ 0xFFFFFFFF);
    USBD_VSC_Write(pContext->pInst->hWriteEP, Answ, 4, 0, 0);
    pContext->TestModeCmd.Cmd = (VSC_CMD)(Cmd >> 24);
    memset(&pContext->TestResult, 0, sizeof(pContext->TestResult));
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       _SendResult
*
*  Function description
*    Send test result to host
*/
static void _SendResult(VSC_CONTEXT * pContext) {
  U8 Buff[10];

  USBD_StoreU32LE(Buff, pContext->TestResult.Res1);
  USBD_StoreU32LE(Buff + 4, pContext->TestResult.Res2);
  USBD_StoreU16LE(Buff + 8, pContext->TestResult.Res3);
  USBD_VSC_Write(pContext->pInst->hWriteEP, Buff, sizeof(Buff), 0, 0);
  pContext->TestModeCmd.Cmd = VSC_CMD_ECHO_VAR_RECEIVE;
}

/*********************************************************************
*
*       _CheckEnd
*
*  Function description
*    Check if acBuff contains new test type command
*/
static int _CheckEnd(VSC_CONTEXT * pContext, int NumBytes) {
  int i;

  for (i = 0; i < 4 && i < NumBytes; i++) {
    if (pContext->pBuff[i] != '#') {
      return 0;
    }
  }
  pContext->TestResult.Res2 = 0;
  _SendResult(pContext);
  return 1;
}

/*********************************************************************
*
*       _CommandLoop
*
*/
static int _CommandLoop(VSC_CONTEXT * pContext) {
  int  i;
  int  NumBytesReceived;
  int  BytesToRead;
  int  BytesToWrite;
  int  BytesAtOnce;
  int  tStart;
  int  BuffNo;
  int  Toogle;
  U8  *p;
  U32  MaxTransfer;

  pContext->BytesToWrite = 0;
  switch(pContext->TestModeCmd.Cmd) {

  case VSC_CMD_BIG_DATA_READ:
    //
    // big data read test using BULK_Read()
    //
    BytesToRead = pContext->TestModeCmd.Para1;
    MaxTransfer = pContext->TestModeCmd.Para2;
    if (MaxTransfer > BUFFER_SIZE) {
      _ErrorStop();
    }
    if (MaxTransfer == 0) {
      MaxTransfer = 2 * BUFFER_SIZE;
    }
    pContext->TestResult.Res1 = 0xFFFFFFFF;
    while (BytesToRead > 0) {
      BytesAtOnce = MaxTransfer;
      if (BytesAtOnce > BytesToRead) {
        BytesAtOnce = BytesToRead;
      }
      if (BytesAtOnce != USBD_VSC_Read(pContext->pInst->hReadEP, pContext->pBuff, BytesAtOnce, 0, 0)) {
        _ErrorStop();
      }
      pContext->TestResult.Res1 = _CRC_Calc32(pContext->pBuff, BytesAtOnce, pContext->TestResult.Res1);
      BytesToRead -= BytesAtOnce;
    }
    pContext->TestResult.Res2 = 0;
    _SendResult(pContext);
    break;

  case VSC_CMD_BIG_DATA_WRITE:
    //
    // big data write test using BULK_Write()
    //
    BytesToWrite = pContext->TestModeCmd.Para1;
    MaxTransfer  = pContext->TestModeCmd.Para2;
    if (MaxTransfer > BUFFER_SIZE) {
      _ErrorStop();
    }
    Toogle = 1;
    BuffNo = 0;
    if (MaxTransfer == 0) {
      MaxTransfer = 2 * BUFFER_SIZE;
      Toogle = 0;
      BuffNo = 1;
    }
    //
    // We first calculate the CRC to speed up write operation
    //
    pContext->TestResult.Res1 = _CRC_Calc_Rand(pContext, BytesToWrite);
    while (BytesToWrite > 0) {
      BytesAtOnce = MaxTransfer;
      if (BytesAtOnce > BytesToWrite) {
        BytesAtOnce = BytesToWrite;
      }
      if (BuffNo) {
        p = pContext->pBuff;
      } else {
        p = pContext->pBuff1;
      }
      BuffNo ^= Toogle;
      for (i = 0; i < BytesAtOnce; i++) {
        p[i] = _Rand(&pContext->Seed);
      }
      if ((USBD_VSC_Write(pContext->pInst->hWriteEP, p, BytesAtOnce, -1, 0)) < 0) {
        _ErrorStop();
      }
      if (BuffNo) {
        USBD_VSC_WaitForTXReady(pContext->pInst->hWriteEP, 0);
      }
      BytesToWrite -= BytesAtOnce;
    }
    pContext->TestResult.Res2 = 0;
    _SendResult(pContext);
    break;
  case VSC_CMD_BENCH_READ:
    //
    // Read benchmark
    //
    BytesToRead = pContext->TestModeCmd.Para1;
    p = _AlignBuff(pContext->Buff, 64, sizeof(pContext->BuffExt));
    tStart = USB_OS_GetTickCnt();
    while (BytesToRead > 0) {
      BytesAtOnce = BUFFER_SIZE + BUFFER_SIZE;
      if (BytesAtOnce > BytesToRead) {
        BytesAtOnce = BytesToRead;
      }
      if (BytesAtOnce != USBD_VSC_Read(pContext->pInst->hReadEP, p, BytesAtOnce, 0, 0)) {
        _ErrorStop();
      }
      BytesToRead -= BytesAtOnce;
    }
    pContext->TestResult.Res1 = USB_OS_GetTickCnt() - tStart;
    _SendResult(pContext);
    break;

  case VSC_CMD_BENCH_WRITE:
    //
    // Write benchmark
    //
    BytesToWrite = pContext->TestModeCmd.Para1;
    p = _AlignBuff(pContext->Buff, 64, sizeof(pContext->BuffExt));
    memset(p, 0xAA, BUFFER_SIZE + BUFFER_SIZE);
    tStart = USB_OS_GetTickCnt();
    while (BytesToWrite > 0) {
      BytesAtOnce = BUFFER_SIZE + BUFFER_SIZE;
      if (BytesAtOnce > BytesToWrite) {
        BytesAtOnce = BytesToWrite;
      }
      if (USBD_VSC_WaitForTXReady(pContext->pInst->hWriteEP, 5000) != 0 ||
          (USBD_VSC_Write(pContext->pInst->hWriteEP, p, BytesAtOnce, -1, 0)) < 0) {
        _ErrorStop();
      }
      BytesToWrite -= BytesAtOnce;
    }
    USBD_VSC_WaitForTXReady(pContext->pInst->hWriteEP, 0);
    pContext->TestResult.Res1 = USB_OS_GetTickCnt() - tStart;
    _SendResult(pContext);
    break;

  case VSC_CMD_ECHO_VAR_RECEIVE:
    //
    // normal echo test with Receive()
    //
    NumBytesReceived = USBD_VSC_Read(pContext->pInst->hReadEP, pContext->pBuff, BUFFER_SIZE, 0, USB_VSC_READ_FLAG_RECEIVE);
    if (NumBytesReceived > 0) {
      if (_CheckCmd(pContext, NumBytesReceived)) {
        break;
      }
      USBD_VSC_Write(pContext->pInst->hWriteEP, pContext->pBuff, NumBytesReceived, 0, 0);
      pContext->Seed += NumBytesReceived + pContext->pBuff[0];
    }
    break;

  case VSC_CMD_ECHO_VAR_RECEIVE_POLL:
    //
    // normal echo test with ReceivePoll()
    //
    NumBytesReceived = USBD_VSC_Read(pContext->pInst->hReadEP, pContext->pBuff, BUFFER_SIZE, 2, USB_VSC_READ_FLAG_POLL);
    if (NumBytesReceived > 0) {
      if (_CheckEnd(pContext, NumBytesReceived)) {
        break;
      }
      USBD_VSC_Write(pContext->pInst->hWriteEP, pContext->pBuff, NumBytesReceived, 0, 0);
    }
    break;

  default:
    _ErrorStop();
  }
  return 0;
}

/*********************************************************************
*
*       _AddBULK
*
*  Function description
*    Add generic USB BULK interface to USB stack
*/
static VSC_SAMPLE_INST * _AddVSC(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_VSC_INIT_DATA     InitData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;
  USB_VSC_HANDLE        hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPIn.Flags          = 0;                             // Flags not used.
  EPIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.aEP[0]     = USBD_AddEPEx(&EPIn, NULL, 0);

  EPOut.Flags         = 0;                             // Flags not used.
  EPOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.aEP[1]     = USBD_AddEPEx(&EPOut, _abOutBuffer, sizeof(_abOutBuffer));
  InitData.NumEPs     = 2;
  InitData.InterfaceClass = 0xffu;
  InitData.InterfaceSubClass = 0xffu;
  InitData.InterfaceProtocol = 0x01u;
#if USE_CUSTOME_MSOS_DESC
  InitData.Flags |= USB_VSC_USE_CUSTOM_MSOSDESC;
  InitData.pMSDescInfo = &_MSDescInfo;
#endif
  hInst = USBD_VSC_Add(&InitData);
  _Inst.hInst    = hInst;
  _Inst.hWriteEP = InitData.aEP[0];
  _Inst.hReadEP  = InitData.aEP[1];
  return &_Inst;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*
* Function description
*   USB handling task.
*   Modify to implement the desired protocol
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  VSC_SAMPLE_INST  * pInst;
  USBD_Init();
  pInst = _AddVSC();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  //
  // Loop: Receive data byte by byte, send back (data + 1)
  //
  while (1) {

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    _VscContext.pInst  = pInst;
    _VscContext.pBuff  = _VscContext.Buff;
    _VscContext.pBuff1 = _VscContext.Buff1;
    _CommandLoop(&_VscContext);
  }
}
