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

File    : USB_BULK_ShowDeviceState.c
Purpose : emUSB-Device sample showing the current status
          of the device in the debug output terminal.

Additional information:
  Preparations:
    None.

  Expected behavior:
    Different USB states are shown in the debug terminal depending
    on the connection state.

  Sample output:
    <...>

    0:005 Status Task - Current state =
    0:008 Status Task - Current state = Attached      Suspended
    0:200 Status Task - Current state = Attached   Ready
    0:358 Status Task - Current state = Attached   Ready      Addressed
    0:380 Status Task - Current state = Attached   Ready      Addressed  Configured
    5:771 Status Task - Current state = Attached   Ready      Addressed  Configured Suspended

    <...>

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "BSP.h"
#include "USB_Bulk.h"


/*********************************************************************
*
*       Information that are used during enumeration
*/
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,             // VendorId
  0x1240,             // ProductId
  "Vendor",           // VendorName
  "Bulk device state",// ProductName
  "13245678"          // SerialNumber
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnStatusChange
*
*/
static void _OnStatusChange(void * pContext, U8 NewState) {
  USB_USE_PARA(pContext);
  USBD_Logf_Application("Current state = %s %s %s %s %s", (NewState & USB_STAT_ATTACHED)  ? "Attached  " : "",
                                                          (NewState & USB_STAT_READY)     ? "Ready     " : "",
                                                          (NewState & USB_STAT_ADDRESSED) ? "Addressed " : "",
                                                          (NewState & USB_STAT_CONFIGURED)? "Configured" : "",
                                                          (NewState & USB_STAT_SUSPENDED) ? "Suspended " : ""
                       );
}


/*********************************************************************
*
*       _AddBULK
*
*  Function description
*    Add generic USB BULK interface to USB stack
*/
static USB_BULK_HANDLE _AddBULK(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_BULK_INIT_DATA    InitData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;
  USB_BULK_HANDLE       hInst;

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

  hInst = USBD_BULK_Add(&InitData);
  USBD_BULK_SetMSDescInfo(hInst);
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
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  USB_BULK_HANDLE hInst;
  static USB_HOOK _Hook;

  USBD_Init();
  hInst = _AddBULK();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_RegisterSCHook(&_Hook, _OnStatusChange, NULL);
  USBD_Start();
  for(;;) {
    //
    // Loop: Receive data byte by byte, send back (data + 1)
    //
    for(;;) {
      char c;

      while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
        BSP_ToggleLED(0);
        USB_OS_Delay(50);
      }
      BSP_SetLED(0);
      BSP_SetLED(1);         // LED on to indicate we are waiting for data
      USBD_BULK_Read(hInst, &c, 1, 0);
      BSP_ClrLED(1);
      c++;
      USBD_BULK_Write(hInst, &c, 1, 0);
    }
  }
}

/**************************** end of file ***************************/

