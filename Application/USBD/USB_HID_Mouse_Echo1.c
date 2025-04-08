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

File    : USB_HID_Mouse_Echo1.c
Purpose : Demonstrates usage of the HID component.
          A combination of the "USB_HID_Echo1.c" and "USB_HID_Mouse.c"
          samples.

Additional information:
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\HID\SampleApp\Exe\

  Expected behavior:
    When the HIDEcho1 sample is started on the PC it should display
    information about the connected HID device.
    The sample should transmit the user specified number of bytes
    to the target and back.
    Also the mouse cursor constantly jumps from left to right and back.

  Sample output:
    The target side does not produce terminal output.
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

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1117,         // ProductId
  "Vendor",       // VendorName
  "HID mouse and echo sample",  // ProductName
  "12345678"      // SerialNumber
};

/*********************************************************************
*
*       Const data
*
**********************************************************************
*/

const U8 _aHIDReportEcho1[] = {
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
*       _aHIDReportMouse
*
*  This report is generated according to HID spec and
*  HID Usage Tables specifications.
*/
const U8 _aHIDReportMouse[] = {
  USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
  USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_MOUSE,
  USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_APPLICATION,
    USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_POINTER,
    USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_PHYSICAL,
      USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_BUTTON,
      USB_HID_LOCAL_USAGE_MINIMUM + 1, 1,
      USB_HID_LOCAL_USAGE_MAXIMUM + 1, 3,
      USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
      USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
      USB_HID_GLOBAL_REPORT_COUNT + 1, 3,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
      USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE,  // 3 button bits
      USB_HID_GLOBAL_REPORT_COUNT + 1, 1,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 5,
      USB_HID_MAIN_INPUT + 1, USB_HID_CONSTANT,  // 5 bit padding
      USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
      USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_X,
      USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_Y,
      USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, (unsigned char) -127,
      USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 127,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 8,
      USB_HID_GLOBAL_REPORT_COUNT + 1, 2,
      USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE | USB_HID_RELATIVE,
    USB_HID_MAIN_ENDCOLLECTION,
  USB_HID_MAIN_ENDCOLLECTION
};


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
static char _ac[USB_HS_INT_MAX_PACKET_SIZE];

/*********************************************************************
*
*       _AddHID
*
*  Function description
*    Add HID class to USB stack
*/
static USB_HID_HANDLE  _AddHIDEcho1(void) {
  static U8           _abOutBuffer[USB_HS_INT_MAX_PACKET_SIZE];
  USB_HID_INIT_DATA   InitData;
  USB_ADD_EP_INFO     EPIntIn;
  USB_ADD_EP_INFO     EPIntOut;
  USB_HID_HANDLE      hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  EPIntOut.Flags           = 0;                             // Flags not used.
  EPIntOut.InDir           = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPIntOut.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntOut.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntOut.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPOut = USBD_AddEPEx(&EPIntOut, _abOutBuffer, sizeof(_abOutBuffer));

  InitData.pReport = _aHIDReportEcho1;
  InitData.NumBytesReport  = sizeof(_aHIDReportEcho1);
  USBD_SetDeviceInfo(&_DeviceInfo);
  hInst = USBD_HID_Add(&InitData);
  return hInst;
}

/*********************************************************************
*
*       _AddHIDMouse
*
*  Function description
*    Add HID mouse class to USB stack
*/
static USB_HID_HANDLE _AddHIDMouse(void) {
  USB_HID_INIT_DATA InitData;
  USB_ADD_EP_INFO   EPIntIn;
  USB_HID_HANDLE    hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  InitData.pReport = _aHIDReportMouse;
  InitData.NumBytesReport = sizeof(_aHIDReportMouse);
  hInst = USBD_HID_Add(&InitData);
  return hInst;
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
  USB_HID_HANDLE hEcho1;
  USB_HID_HANDLE hMouse;
  unsigned       NumBytes2Read;

  USBD_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  hEcho1 = _AddHIDEcho1();
  hMouse = _AddHIDMouse();
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
    BSP_SetLED(0);
    USBD_HID_StartReadTransfer(hEcho1);
    NumBytes2Read = USBD_HID_GetNumBytesInBuffer(hEcho1);
    //
    // If there are no data to echo, toggle the mouse
    if (NumBytes2Read == 0) {
      U8 acMouse[3] = {0, 0, 0};

      //
      // Move the mouse to the right
      //
      acMouse[1] = 20;   // To the right !
      USBD_HID_Write(hMouse, &acMouse[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
      USB_OS_Delay(100);
      //
      // Move mouse to the left
      //
      acMouse[1] = (U8)-20;  // To the left !
      USBD_HID_Write(hMouse, &acMouse[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
      USB_OS_Delay(100);
    } else {
      USBD_HID_Read(hEcho1, &_ac[0], NumBytes2Read, 0);
      _ac[0]++;
      USBD_HID_Write(hEcho1, &_ac[0], INPUT_REPORT_SIZE, 0);
    }
    USB_OS_Delay(50);
  }
}
