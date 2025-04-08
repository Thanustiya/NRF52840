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

Purpose : Sample application to test BULK (vendor) class and
          measures the transfer speed.

Additional information:
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\BULK\SampleApplication\

    On Windows this sample requires the WinUSB driver.
    This driver, if not already installed, is retrieved via
    Windows Update automatically when a device running this
    sample is connected. If Windows Update is disabled you can
    install the driver manually, see \Windows\USB\BULK\WinUSBInstall .

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
#include "USB_Bulk.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#ifndef USBD_SAMPLE_NO_MAINTASK
  #define USBD_SAMPLE_NO_MAINTASK  0
#endif

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
void USBD_BULK_Test_Init(void);
void USBD_BULK_Test_RunTask(void *);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Information that are used during enumeration
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1240,         // ProductId
  "Vendor",       // VendorName
  "Bulk test",    // ProductName
  "13245678"      // SerialNumber
};
#endif

static const char _CmdReadSpeed[]  = "@Test-Read-Speed@";
static const char _CmdWriteSpeed[] = "@Test-Write-Speed@";
static const char _CmdEchoFix[]    = "@n@";

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static USB_BULK_HANDLE   _hInst;
// Size of _ac must be a power of 2
#if defined(__GNUC__) || defined(__clang__) || defined(__SEGGER_CC__)
static U32               _ac[0x4000 / 4] __attribute__ ((aligned (32)));
#else
static U32               _ac[0x4000 / 4];
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBD_BULK_Test_Init
*
*  Function description
*    Add generic USB BULK interface to USB stack
*/
void USBD_BULK_Test_Init(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_BULK_INIT_DATA    InitData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;

  memset(&InitData, 0, sizeof(InitData));
  EPIn.Flags          = 0;                             // Flags not used.
  EPIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPIn, NULL, 0);

  EPOut.Flags         = 0;                             // Flags not used.
  EPOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPOut, _abOutBuffer, sizeof(_abOutBuffer));

  _hInst = USBD_BULK_Add(&InitData);
  USBD_BULK_SetMSDescInfo(_hInst);
}

/*********************************************************************
*
*       USBD_BULK_Test_RunTask
*
*  Function description
*    USB handling task.
*/
void USBD_BULK_Test_RunTask(void *dummy) {
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

    r = USBD_BULK_Receive(_hInst, _ac, sizeof(_ac), 0);
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
      USBD_BULK_Write(_hInst, "ok", 2, 0);
      //
      // Perform test
      //
      do {
        NumBytesAtOnce = SEGGER_MIN(DataSize, sizeof(_ac));
        if (USBD_BULK_Read(_hInst, _ac, NumBytesAtOnce, 0) != (int)NumBytesAtOnce) {
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
      USBD_BULK_Write(_hInst, "ok", 2, 0);
      //
      // Perform test
      //
      USB_MEMSET(_ac, 0x55, sizeof(_ac));
      do {
        NumBytesAtOnce = SEGGER_MIN(DataSize, sizeof(_ac));
        if (USBD_BULK_Write(_hInst, _ac, NumBytesAtOnce, 1000) != (int)NumBytesAtOnce) {
          break;
        }
        DataSize -= NumBytesAtOnce;
      } while (DataSize > 0);
      continue;
    }
    if (r == sizeof(_CmdEchoFix) + 1 && USB_MEMCMP(_ac, _CmdEchoFix, sizeof(_CmdEchoFix) - 1) == 0) {
      //
      // Perform echo with fixed size
      //
      pBuff = (unsigned char *)&_ac[0];
      DataSize = (pBuff[sizeof(_CmdEchoFix) - 1] << 8) | pBuff[sizeof(_CmdEchoFix)];
      if (USBD_BULK_Read(_hInst, _ac, DataSize, 500) == (int)DataSize) {
        USBD_BULK_Write(_hInst, _ac, DataSize, 500);
      }
      continue;
    }
    //
    // No test, just echo received data with first byte incremented
    //
    _ac[0]++;
    USBD_BULK_Write(_hInst, _ac, r, 500);
  }
}

/*********************************************************************
*
*       MainTask
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
void MainTask(void) {
  USBD_Init();
  USBD_BULK_Test_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  USBD_BULK_Test_RunTask(NULL);
}
#endif

