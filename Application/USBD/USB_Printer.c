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

File    : USB_Printer.c
Purpose : Sample implementation of USB printer device class.

Additional information:
  Preparations:
    On Windows a printer driver may need to be installed.
    Windows Update does this automatically when the sample
    is connected.

  Expected behavior:
    After connecting the USB cable the PC registers a new printer.
    Normally the printer is displayed as "HP-LaserJet-6MP".
    The name may vary slightly depending on the operating system.
    Any document printed to the new printer should show up in
    the debug terminal. The printed data includes all PostScript
    raw command data.

  Sample output:
    20:070 MainTask - %!PS-Adobe-3.0
    %Produced by poppler pdftops version: 0.62.0 (htt
    20:070 MainTask - p://poppler.freedesktop.org)
    %%LanguageLevel: 2
    %%DocumentSuppli
    20:070 MainTask - edResources: (atend)
    %%DocumentMedia: A4 595 842 0 () ()
    %%For:
    20:070 MainTask - (user)
    %%Title: (Untitled Document 1)
    %RBINumCopies: 1
    %%Pages: (
    20:071 MainTask - atend)
    %%BoundingBox: (atend)
    %%EndComments
    %%BeginProlog

    <...>
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include "USB_PrinterClass.h"
#include "BSP.h"

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
  0x8765,                 // VendorId
  0x2114,                 // ProductId. Should be unique for this sample
  "Vendor",               // VendorName
  "Printer",              // ProductName
  "12345678901234567890"  // SerialNumber
};

/*********************************************************************
*
*       static data
*
**********************************************************************
*/
static U8 _acData[64 + 1]; // +1 for the terminating zero character

/*********************************************************************
*
*       static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetDeviceIdString
*
*/
static const char * _GetDeviceIdString(void) {
  const char * s = "CLASS:PRINTER;"
                   "MODEL:HP LaserJet 6MP;"
                   "MANUFACTURER:Hewlett-Packard;"
                   "DESCRIPTION:Hewlett-Packard LaserJet 6MP Printer;"
                   "COMMAND SET:PJL,MLC,PCLXL,PCL,POSTSCRIPT;";
  return s;
}
/*********************************************************************
*
*       _GetHasNoError
*
*/
static U8 _GetHasNoError(void) {
  return 1;
}
/*********************************************************************
*
*       _GetIsSelected
*
*/
static U8 _GetIsSelected(void) {
  return 1;
}

/*********************************************************************
*
*       _GetIsPaperEmpty
*
*/
static U8 _GetIsPaperEmpty(void) {
  return 0;
}

/*********************************************************************
*
*       _OnDataReceived
*
*/
static int _OnDataReceived(const U8 * pData, unsigned NumBytes) {
  while (NumBytes > sizeof(_acData) - 1) {
    memcpy(_acData, pData, sizeof(_acData) - 1);
    _acData[sizeof(_acData) - 1] = 0;
    USBD_Logf_Application("%s", _acData);
    pData    += sizeof(_acData) - 1;
    NumBytes -= sizeof(_acData) - 1;
  }
  memcpy(_acData, pData, NumBytes);
  _acData[NumBytes] = 0;
  USBD_Logf_Application("%s", _acData);
  return 0;
}

/*********************************************************************
*
*       _OnReset
*
*/
static void _OnReset(void) {
  USBD_Logf_Application("Received Reset.");
}

static USB_PRINTER_API _PrinterAPI = {
  _GetDeviceIdString,
  _OnDataReceived,
  _GetHasNoError,
  _GetIsSelected,
  _GetIsPaperEmpty,
  _OnReset
};

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
  USBD_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USB_PRINTER_Init(&_PrinterAPI);

#ifdef HANDLE_RECEIVE_DATA_IN_INTERRUPT

  USB_PRINTER_ConfigIRQProcessing();
  USBD_Start();
  for (;;) {
    USB_OS_Delay(1000);     // do nothing, everything is handled in interrupt.
  }

#else

  USBD_Start();
  while (1) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    //
    // Use USB_PRINTER_Task() to receive data.
    //
    USB_PRINTER_Task();
  }

#endif
}

/**************************** end of file ***************************/

