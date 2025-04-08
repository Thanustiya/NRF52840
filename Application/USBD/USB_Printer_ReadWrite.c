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

File    : USB_Printer_ReadWrite.c
Purpose : This sample demonstrates the use of the MTP component
          together with emFile.

Additional information:
  Preparations:
    For MTP the correct emFile configuration file has
    to be included in the project. Depending on the hardware
    it can be one of the following:
    * FS_ConfigRAMDisk_23k.c
    * FS_ConfigNAND_*.c
    * FS_ConfigMMC_CardMode_*.c
    * FS_ConfigNAND_*.c
    * FS_USBH_MSDConfig.c


  Expected behavior:
    This sample will format the storage medium if necessary and
    create a "Readme.txt" file in the root of the storage
    medium. After the formatting is done and the USB cable has
    been connected to a PC a new MTP volume will show up with
    a "Readme.txt" file in the root directory.

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
  0x2115,                 // ProductId. Should be unique for this sample
  "Vendor",               // VendorName
  "Printer",              // ProductName
  "12345678901234567890"  // SerialNumber
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U8 _acData[64 + 1]; // +1 for the terminating zero character

/*********************************************************************
*
*       Static code
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
    USB_PRINTER_Receive(_acData, sizeof(_acData) - 1);
    _acData[sizeof(_acData) - 1] = 0;
    puts((const char *)_acData);
//    USB_PRINTER_Write("Ok", 2);
  }
}

/**************************** end of file ***************************/
