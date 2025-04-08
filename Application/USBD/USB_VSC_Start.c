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
Purpose : USB VSC sample showing a simple 1-byte echo server.

Additional information:
  This sample can be used as start base for simple applications.
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\BULK\SampleApplication\TestV
    Please use the simple echo operation with transfer size 1 to test it.

    On Windows this sample requires the WinUSB driver.
    This driver, if not already installed, is retrieved via
    Windows Update. If Windows Update is disabled you can install
    the driver manually, see \Windows\USB\BULK\WinUSBInstall .

    On Linux either root or udev rules are required to access
    the bulk device, see \Windows\USB\BULK\USBBULK_API_Linux .

    On macOS bulk devices can be accessed without additional
    changes, see \Windows\USB\BULK\USBBULK_API_MacOSX .

  Expected behavior:
    After running the PC counterpart and connecting the USB cable
    the PC counterpart should start the test automatically.


  Sample output:
    The target side does not produce terminal output.
    PC counterpart output:

    Found 1 device
    Found the following device 0:
      Vendor Name : Vendor
      Product Name: Bulk test
      Serial no.  : 13245678
    To which device do you want to connect?
    Please type in device number (e.g. '0' for the first device, q/a for abort):

    Echo test
    Operation successful!

    Read speed test
    Performance: 6145 ms for 256 MB
              =  42659 kB / second

    Write speed test
    Performance: 6154 ms for 256 MB
              =  42597 kB / second

    Echo test
    Operation successful!

    Communication with USB BULK device was successful!
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

#ifndef USBD_SAMPLE_NO_MAINTASK
  #define USBD_SAMPLE_NO_MAINTASK  0
#endif

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

/*********************************************************************
*
*       Forward declarations
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
void USBD_VSC_Start_Init(void);
void USBD_VSC_Start_RunTask(void *);
#ifdef __cplusplus
}
#endif

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
#if USBD_SAMPLE_NO_MAINTASK == 0
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x2000,         // ProductId
  "Vendor",       // VendorName
  "VSC Start",    // ProductName
  "12345678"      // SerialNumber
};
#endif

static const char _CmdReadSpeed[]  = "@Test-Read-Speed@";
static const char _CmdWriteSpeed[] = "@Test-Write-Speed@";

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static VSC_SAMPLE_INST _Inst;
static U32             _ac[0x4000 / 4];   // size must be a power of 2

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
#if USE_CUSTOME_MSOS_DESC
//static const U32 _IsEnabled   = 1uL;
//static const U32 _IdleTimeout = 10000uL;  // Time given in ms.

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
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBD_VSC_Start_Init
*
*  Function description
*    Add generic USB BULK interface to USB stack
*/
void USBD_VSC_Start_Init(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_VSC_INIT_DATA     InitData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;

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
  _Inst.hInst    = USBD_VSC_Add(&InitData); ;
  _Inst.hWriteEP = InitData.aEP[0];
  _Inst.hReadEP  = InitData.aEP[1];
}

/*********************************************************************
*
*       USBD_VSC_Start_RunTask
*
*  Function description
*    USB handling task.
*/
void USBD_VSC_Start_RunTask(void *dummy) {
  int      r;
  unsigned DataSize;
  unsigned NumBytesAtOnce;
  U8       *pBuff;

  USB_USE_PARA(dummy);
  for (;;) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0); // Toggle LED to indicate configuration
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);

    r = USBD_VSC_Read(_Inst.hReadEP, _ac, sizeof(_ac), 0, USB_VSC_READ_FLAG_RECEIVE);
    if (r < 0) {
      //
      // error occurred
      //
      continue;
    }
    //
    // Check for speed-test
    //
    if (r == sizeof(_CmdReadSpeed) && USB_MEMCMP(_ac, _CmdReadSpeed, sizeof(_CmdReadSpeed) - 1) == 0) {
      //
      // Read speed test requested by the host.
      //
      // Last byte of received command contains data size to be used by the speed-test:
      // '0': 4 MB, '1': 8 MB, '2': 16 MB, ...., '9': 2048 MB
      //
      pBuff = (unsigned char *)&_ac[0];
      DataSize = 0x400000u << (pBuff[r - 1] & 0xFu);
      USBD_Logf_Application("Start read speed test with %u bytes", DataSize);
      USBD_VSC_Write(_Inst.hWriteEP, "ok", 2, 0, 0);
      //
      // Perform test
      //
      do {
        NumBytesAtOnce = SEGGER_MIN(DataSize, sizeof(_ac));
        if (USBD_VSC_Read(_Inst.hReadEP, _ac, NumBytesAtOnce, 0, 0) != (int)NumBytesAtOnce) {
          break;
        }
        DataSize -= NumBytesAtOnce;
      } while (DataSize > 0);
      continue;
    }
    if (r == sizeof(_CmdWriteSpeed) && USB_MEMCMP(_ac, _CmdWriteSpeed, sizeof(_CmdWriteSpeed) - 1) == 0) {
      //
      // Write speed test requested by the host.
      //
      // Last byte of received command contains data size to be used by the speed-test:
      // '0': 4 MB, '1': 8 MB, '2': 16 MB, ...., '9': 2048 MB
      //
      pBuff = (unsigned char *)&_ac[0];
      DataSize = 0x400000u << (pBuff[r - 1] & 0xFu);
      USBD_Logf_Application("Start write speed test with %u bytes", DataSize);
      USBD_VSC_Write(_Inst.hWriteEP, "ok", 2, 0, 0);
      //
      // Perform test
      //
      USB_MEMSET(_ac, 0x55, sizeof(_ac));
      do {
        NumBytesAtOnce = SEGGER_MIN(DataSize, sizeof(_ac));
        if (USBD_VSC_Write(_Inst.hWriteEP, _ac, NumBytesAtOnce, 1000, 0) != (int)NumBytesAtOnce) {
          break;
        }
        DataSize -= NumBytesAtOnce;
      } while (DataSize > 0);
      continue;
    }
    //
    // No test, just echo received data with first byte incremented
    //
    _ac[0]++;
    USBD_VSC_Write(_Inst.hWriteEP, _ac, r, 500, 0);
  }
}

/*********************************************************************
*
*       MainTask
*
* Function description
*   USB handling task.
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
void MainTask(void) {
  USBD_Init();
  USBD_VSC_Start_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  USBD_VSC_Start_RunTask(NULL);
}
#endif
