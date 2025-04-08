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
----------------------------------------------------------------------
File    : USB_UVC_Start.c
Purpose : Demonstrates usage of the UVC component.
          Displays a simple, changing color over the whole screen.
Notes   : To run this sample the info buffer size should be increased,
          this can be done by adding #define USB_DESC_BUFFER_SIZE 512
          to the project's USB_Conf.h file.
          #define USB_SUPPORT_TRANSFER_ISO 1 also must be added.

          Running this sample with USB full-speed is not advised,
          it will be very slow.
          When running this sample with USB full-speed it is possible
          that you might have to decrease the UVC_ISO_EP_SIZE to make
          sure the sample works at all. The total bandwidth on the
          USB full-speed bus for periodic endpoints (interrupt and
          isochronous is 1500). When you connect the sample it will
          require 1023 bytes (+9 bytes protocol overhead) from that
          bandwidth in it's default configuration. If your other
          devices already have a large portion of that bandwidth
          reserved the sample won't work.
--------  END-OF-HEADER  ---------------------------------------------
*/


/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include <stdio.h>
#include "USB.h"
#include "USB_UVC.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USE_BULK_MODE 0 // Use bulk endpoints instead of isochronous endpoints.

#if USE_BULK_MODE == 0
  #if 0 // Change this to 1 if your USB driver supports isochronous high-bandwidth mode.
    #define UVC_ISO_EP_SIZE USB_HS_ISO_HB_MAX_PACKET_SIZE
  #else
    #ifndef UVC_ISO_EP_SIZE
      #define UVC_ISO_EP_SIZE   USB_HS_ISO_MAX_PACKET_SIZE
    #endif
  #endif
#else
  #define UVC_ISO_EP_SIZE 512 // Used for buffer size only
#endif

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
  0x1088,         // ProductId
  "SEGGER",       // VendorName
  "SEGGER video", // ProductName
  "13245678"      // SerialNumber
};

/*********************************************************************
*
*       Const data
*
**********************************************************************
*/


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static USBD_UVC_BUFFER _Buf;
static const USBD_UVC_RESOLUTION aRes[] = {{320,240}, {400,300}, {640,480}, {800,600}};
static unsigned _FrameIndex;
static U32 _acUVCBuffer[USBD_UVC_NUM_BUFFERS][USBD_UVC_DATA_BUFFER_SIZE / 4];
static U32 _acBuffer[UVC_ISO_EP_SIZE / 4];
static USBD_UVC_CONTROLS _Controls;

static USBD_UVC_CONTROL_8 _Auto_Exp_Mode;
static USBD_UVC_CONTROL_8 _PL_Freq;
static U8 _AE_Mode_Value;
static U8 _PL_Freq_Value;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ResChange
*
*  Function description
*    Called when the PC requests a change in the current image resolution.
*/
static void _ResChange(unsigned FrameIndex) {
  _FrameIndex = FrameIndex - 1;
}

/*********************************************************************
*
*       _CbControl_AE_Mode
*
*  Function description
*    Called when the PC changes a control.
*
*  Return value
*     In case of "set":
*    0 - Request was valid, confirm it.
*    1 - Request was invalid, stall it.
*
*     In case of "get":
*    Current value of the control.
*/
static U8 _CbControl_AE_Mode(U8 IsSetRequest, unsigned Control, U8 Value) {
  U8 r;

  if (IsSetRequest) {
    //
    // b0: Manual Mode
    // b1: Auto Mode
    // b2: Shutter Priority Mode manual Exposure Time, auto Iris
    // b3: Aperture Priority Mode auto Exposure Time, manual Iris
    // b4..b7: Reserved
    //
    if ((Value <= 8) && (Value > 0) && !(Value & (Value - 1))) { // Make sure only one bit is set and value is valid.
      _AE_Mode_Value = Value;
      USBD_Logf_Application("Control %d (AE_Mode) changed to %d", Control, Value);
      r = 0;
    } else {
      r = 1;
    }
  } else {
    r = _AE_Mode_Value;
    USBD_Logf_Application("Control %d (AE_Mode) value reported to host is %d", Control, r);
  }
  return r;
}

/*********************************************************************
*
*       _CbControl_PL_Freq
*
*  Function description
*    Called when the PC changes a control.
*/
static U8 _CbControl_PL_Freq(U8 IsSetRequest, unsigned Control, U8 Value) {
  U8 r;

  r = 1;
  if (IsSetRequest) {
    if (Value <= 2u) { // 0: Disabled 1: 50 Hz 2: 60 Hz
      _PL_Freq_Value = Value;
      USBD_Logf_Application("Control %d (PL_Freq) changed to %d", Control, Value);
      r = 0;
    }
  } else {
    r = _PL_Freq_Value;
    USBD_Logf_Application("Control %d (PL_Freq) value reported to host is %d", Control, r);
  }
  return r;
}

/*********************************************************************
*
*       _AddUVC
*
*  Function description
*    Add UVC to USB stack
*/
static int _AddUVC(void) {
  USBD_UVC_INIT_DATA InitData;
  U8 i;
  USB_ADD_EP_INFO EP_Info;

  memset(&InitData, 0, sizeof(InitData));
#if USE_BULK_MODE == 0
  EP_Info.InDir = USB_DIR_IN;
  EP_Info.TransferType = USB_TRANSFER_TYPE_ISO;
  EP_Info.Interval = 1;
  EP_Info.Flags = USB_ADD_EP_FLAG_USE_ISO_SYNC_TYPES;
#if USB_SUPPORT_TRANSFER_ISO
  EP_Info.ISO_Type = USB_ISO_SYNC_TYPE_ASYNCHRONOUS;
#endif
  EP_Info.MaxPacketSize = UVC_ISO_EP_SIZE;
  InitData.EPIn    = USBD_AddEPEx(&EP_Info, NULL, 0);
#else
  InitData.Flags = USBD_UVC_USE_BULK_MODE;
  memset(&InitData, 0, sizeof(InitData));
  EPBulkIn.Flags          = 0;                             // Flags not used.
  EPBulkIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPBulkIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPBulkIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);
#endif
  for (i = 0; i < USBD_UVC_NUM_BUFFERS; i++) {
    _Buf.Buf[i].pData = (U8 *)_acUVCBuffer[i];
  }
  InitData.pBuf    = &_Buf;
  InitData.aResolutions = aRes;
  InitData.NumResolutions = SEGGER_COUNTOF(aRes);

  memset(&_Controls, 0, sizeof(_Controls));

  _Auto_Exp_Mode.Res            = 0x0F; // b0: Manual Mode or b1: Auto Mode or b2: Shutter Priority Mode or b3: Aperture Priority Mode
  _Auto_Exp_Mode.pfCb           = _CbControl_AE_Mode;
  _AE_Mode_Value                = 0x01; // b0: Manual Mode
  _Controls.pAuto_Exp_Mode      = &_Auto_Exp_Mode;

  _PL_Freq.Min                  = 0;    // 0: Disabled 1: 50 Hz 2: 60 Hz
  _PL_Freq.Max                  = 2;
  _PL_Freq.pfCb                 = _CbControl_PL_Freq;
  _PL_Freq_Value                = 0x01; // 1: 50 Hz
  _Controls.pPL_Freq            = &_PL_Freq;

  InitData.Controls = &_Controls;
  USBD_SetDeviceInfo(&_DeviceInfo);
  return USBD_UVC_Add(&InitData);
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
* USB handling task.
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
  unsigned i;
  U8 Color;
  U32 t;

  USBD_Init();
  USBD_EnableIAD();
  _AddUVC();
  USBD_UVC_SetOnResolutionChange(_ResChange);
  USBD_Start();

  Color = 0;
  while (1) {

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    t = 0xFF00FF00 | (Color << 16) | Color;
    for (i = 0; i < sizeof(_acBuffer)/4; i++) {
      USBD_StoreU32BE((U8 *)&_acBuffer[i], t);
    }
    for (i = 0; i < aRes[_FrameIndex].Width * aRes[_FrameIndex].Height * 2; i += (UVC_ISO_EP_SIZE - USBD_UVC_PAYLOAD_HEADER_SIZE)) {
      if (aRes[_FrameIndex].Width * aRes[_FrameIndex].Height * 2 - i <= (UVC_ISO_EP_SIZE - USBD_UVC_PAYLOAD_HEADER_SIZE)) {
        USBD_UVC_Write((U8 *)_acBuffer, aRes[_FrameIndex].Width * aRes[_FrameIndex].Height * 2 - i, USBD_UVC_END_OF_FRAME);
      } else {
        USBD_UVC_Write((U8 *)_acBuffer, UVC_ISO_EP_SIZE - USBD_UVC_PAYLOAD_HEADER_SIZE, 0);
      }
    }
    Color++;
  }
}

/**************************** end of file ***************************/
