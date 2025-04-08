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

File    : USB_CDC_Echo.c
Purpose : The sample shows how to create a simple CDC-ACM echo server.

Additional information:
  Preparations:
    On Windows 8.1 and below the "usbser" driver is not automatically
    assigned to the CDC-ACM device. To install the "usbser" driver
    see \Windows\USB\CDC . The device can be accessed via COM port
    emulation programs e.g. PuTTY.

    On Linux no drivers are needed, the device should show up as
    /dev/ttyACM0 or similar. "sudo screen /dev/ttyACM0 115200"
    can be used to access the device.

    On macOS no drivers are needed, the device should show up as
    /dev/tty.usbmodem13245678 or similar. The "screen" terminal
    program can be used to access the device.

  Expected behavior:
    After connecting the USB cable the PC registers a new COM port.
    Terminal programs are able to open the COM port.
    Any data sent should be received back from the target.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "BSP.h"
#include "USB_CDC.h"
#include "USB.h"

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
void USBD_CDC_Echo_Init(void);
void USBD_CDC_Echo_RunTask(void *);
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
  0x1111,         // ProductId
  "Vendor",       // VendorName
  "CDC device",   // ProductName
  "13245678"      // SerialNumber
};
#endif

static USB_CDC_HANDLE _hInst;
static char           _ac[USB_HS_BULK_MAX_PACKET_SIZE];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _OnLineCoding
*
*  Function description
*    Called whenever a "SetLineCoding" Packet has been received
*
*  Notes
*    (1) Context
*        This function is called directly from an ISR in most cases.
*/
static void _OnLineCoding(USB_CDC_LINE_CODING * pLineCoding) {
#if 0
  USBD_Logf_Application("DTERate=%u, CharFormat=%u, ParityType=%u, DataBits=%u\n",
          pLineCoding->DTERate,
          pLineCoding->CharFormat,
          pLineCoding->ParityType,
          pLineCoding->DataBits);
#else
  BSP_USE_PARA(pLineCoding);
#endif
}


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBD_CDC_Echo_Init
*
*  Function description
*    Add communication device class to USB stack
*/
void USBD_CDC_Echo_Init(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_CDC_INIT_DATA     InitData;
  USB_ADD_EP_INFO       EPBulkIn;
  USB_ADD_EP_INFO       EPBulkOut;
  USB_ADD_EP_INFO       EPIntIn;

  memset(&InitData, 0, sizeof(InitData));
  EPBulkIn.Flags          = 0;                             // Flags not used.
  EPBulkIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPBulkIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPBulkIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);

  EPBulkOut.Flags         = 0;                             // Flags not used.
  EPBulkOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPBulkOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPBulkOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPBulkOut, _abOutBuffer, sizeof(_abOutBuffer));

  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (64 * 125us)
  EPIntIn.MaxPacketSize   = 16;                            // Maximum packet size for Interrupt.
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);

  _hInst = USBD_CDC_Add(&InitData);
  USBD_CDC_SetOnLineCoding(_hInst, _OnLineCoding);
}

/*********************************************************************
*
*       USBD_CDC_Echo_RunTask
*/
void USBD_CDC_Echo_RunTask(void *pDummy) {
  int  NumBytesReceived;

  USB_USE_PARA(pDummy);
  for (;;) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    //
    // Receive one USB data packet and echo it back.
    //
    NumBytesReceived = USBD_CDC_Receive(_hInst, &_ac[0], sizeof(_ac), 0);
    if (NumBytesReceived > 0) {
      USBD_CDC_Write(_hInst, &_ac[0], NumBytesReceived, 0);
    }
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
  USBD_CDC_Echo_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  USBD_CDC_Echo_RunTask(NULL);
}
#endif

/**************************** end of file ***************************/
