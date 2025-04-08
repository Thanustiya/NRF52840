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

Purpose : This sample shows a simple echo server realized with
          the HID protocol.

Additional information:
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\HID\SampleApp\Exe\

  Expected behavior:
    When the HIDEcho1 sample is started on the PC it should display
    information about the connected HID device.
    The sample should transmit the user specified number of bytes
    to the target and back.

  Sample output:
    The target side does not produce terminal output.
    HID PC counterpart output:
    Device 0:
    Productname: HID generic sample
    VID        : 0x8765
    PID        : 0x1114
    ReportSizes:
    Input     : 64 bytes
    Output    : 64 bytes
    Starting Echo...
    Enter the number of echoes to be sent to the echo client: 500

    <...>

    500 echoes successfully transferred.
    Communication with USB HID device was successful!
    Press enter to exit.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines configurable
*
**********************************************************************
*/
#define INPUT_REPORT_SIZE   64    // Defines the input (device -> host) report size
#define OUTPUT_REPORT_SIZE  64    // Defines the output (Host -> device) report size
#define VENDOR_PAGE_ID      0x12  // Defines the vendor specific page that
                                  // shall be used, allowed values 0x00 - 0xff
                                  // This value must be identical to HOST application
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
  void USBD_HID_Echo1_Init(void);
  void USBD_HID_Echo1_RunTask(void *);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Static const data
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
  0x1114,         // ProductId. Should be unique for this sample
  "Vendor",       // VendorName
  "HID generic sample",  // ProductName
  "12345678"      // SerialNumber
};
#endif

static const U8 _aHIDReport[] = {
    0x06, VENDOR_PAGE_ID, 0xFF,    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x06,                    //   USAGE_PAGE (Generic Device)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, INPUT_REPORT_SIZE,       //   REPORT_COUNT (64)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x05, 0x06,                    //   USAGE_PAGE (Generic Device)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, OUTPUT_REPORT_SIZE,      //   REPORT_COUNT (64)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static char                 _ac[USB_HS_INT_MAX_PACKET_SIZE];
static USB_HID_HANDLE       _hInst;

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       USBD_HID_Echo1_Init
*
*  Function description
*    Add an HID class to USB stack
*/
void USBD_HID_Echo1_Init(void) {
  static U8            _abOutBuffer[USB_HS_INT_MAX_PACKET_SIZE];
  USB_HID_INIT_DATA_EX InitData;
  USB_ADD_EP_INFO      EPIntIn;
  USB_ADD_EP_INFO      EPIntOut;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 1;                             // Interval of 125 us (1 ms in full-speed)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  EPIntOut.Flags          = 0;                             // Flags not used.
  EPIntOut.InDir          = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPIntOut.Interval       = 1;                             // Interval of 125 us (1 ms in full-speed)
  EPIntOut.MaxPacketSize  = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntOut.TransferType   = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPOut = USBD_AddEPEx(&EPIntOut, _abOutBuffer, sizeof(_abOutBuffer));

  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport  = sizeof(_aHIDReport);
  InitData.pInterfaceName  = "HID raw";
  _hInst = USBD_HID_AddEx(&InitData);
}

/*********************************************************************
*
*       USBD_HID_Echo1_RunTask
*
*  Function description
*    Performs the HID echo1 operation
*/
void USBD_HID_Echo1_RunTask(void * pPara) {
  //
  // Loop: Receive data byte by byte, send back (data + 1)
  //
  USB_USE_PARA(pPara);
  for(;;) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    USBD_HID_Read(_hInst, &_ac[0], OUTPUT_REPORT_SIZE, 0);
    _ac[0]++;
    USBD_HID_Write(_hInst, &_ac[0], INPUT_REPORT_SIZE, 0);
    USB_OS_Delay(50);
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
  USBD_HID_Echo1_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  USBD_HID_Echo1_RunTask(NULL);
}
#endif

/**************************** end of file ***************************/
