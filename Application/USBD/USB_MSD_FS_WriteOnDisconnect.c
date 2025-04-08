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

File    : USB_MSD_FS_WriteOnDisconnect.c
Purpose : Sample showing how to use the FS and USB mass storage device
          simultaneously.
          This sample creates 2 tasks.
          The USB task controls the mass storage device
          The file system tasks writes into the storage device once per
          second whenever mass storage device is not connected to a host.

Additional information:
  Preparations:
    The correct emFile configuration file has
    to be included in the project. Depending on the hardware
    it can be one of the following:
    * FS_ConfigRAMDisk_23k.c
    * FS_ConfigNAND_*.c
    * FS_ConfigMMC_CardMode_*.c
    * FS_ConfigNAND_*.c

  Expected behavior:
    A new MSD volume is recognized by the PC.
    When the cable is not connected to the PC the applications
    writes the embOS time into "Log.txt" each second.

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
#include "USB.h"
#include "USB_MSD.h"
#include "FS.h"
#include "RTOS.h"
#include "BSP.h"

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
// Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1000,         // ProductId
  "Vendor",       // VendorName
  "MSD device",   // ProductName
  "000013245678"  // SerialNumber. Should be 12 character or more for compliance with Mass Storage Device For Bootability spec.
};
//
// String information used when inquiring the volume 0.
//
static const USB_MSD_LUN_INFO _Lun0Info = {
  "Vendor",     // MSD VendorName
  "MSD Volume", // MSD ProductName
  "1.00",       // MSD ProductVer
  "134657890"   // MSD SerialNo
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int _aUSBStack[512];   // Task stacks
static OS_TASK _USBTCB0;                  // Task-control-blocks

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _FSTask
*/
static void _FSTask(void) {
  FS_FILE * pFile;

  if (FS_IsLLFormatted("") <= 0) {
    USBD_Logf_Application("Low level formatting");
    FS_FormatLow("");  /* Erase & Low-level  format the flash */
  }
  if (FS_IsHLFormatted("") <= 0) {
    USBD_Logf_Application("High level formatting\n");
    FS_Format("", NULL);       /* High-level format the flash */
  }
  FS_SetAutoMount("", 0);
  FS_Mount("");
  while (1) {

    while (1) {
      char ac[30];
      U32 Time;
      int i;
      char *x;

      if (FS_IsVolumeMounted("")) {
        pFile = FS_FOpen("Log.txt", "a+");
        Time = OS_GetTime();
        memcpy(ac, "OS_Time = ", 10);
        x = ac + 10;
        for (i = 8; i >= 0; i--) {
          x[i] = Time % 10 + '0';
          Time /= 10;
        }
        x += 9;
        *x++ = '\r';
        *x++ = '\n';
        FS_Write(pFile, ac, x - ac);
        FS_FClose(pFile);
      }
      OS_Delay(1000);
    }
  }

}

/*********************************************************************
*
*       _AddMSD
*
*  Function description
*    Add mass storage device to USB stack
*
*  Notes:
*   (1)  -   This examples uses the internal driver of the file system.
*            The module intializes the low-level part of the file system if necessary.
*            If FS_Init() was not previously called, none of the high level functions
*            such as FS_FOpen, FS_Write etc will work.
*            Only functions that are driver related will be called.
*            Initialization, sector read/write, retrieve device information.
*            The members of the DriverData are used as follows:
*              DriverData.pStart       = VOLUME_NAME such as "nand:", "mmc:1:".
*              DriverData.NumSectors   = Number of sectors to be used - 0 means auto-detect.
*              DriverData.StartSector  = The first sector that shall be used.
*              DriverData.SectorSize will not be used.
*/
static void _AddMSD(void) {
  static U8         _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_MSD_INIT_DATA InitData;
  USB_MSD_INST_DATA InstData;
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

  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_MSD_Add(&InitData);
  //
  // Add logical unit 0: Use default device.
  //
  memset(&InstData, 0,  sizeof(InstData));
  InstData.pAPI                    = &USB_MSD_StorageByName;
  InstData.DriverData.pStart       = (void *)"";
  InstData.pLunInfo = &_Lun0Info;
  USBD_MSD_AddUnit(&InstData);
}

/*********************************************************************
*
*       _USBTask
*/
static void _USBTask(void) {
  _AddMSD();
  USBD_Start();
  while(1) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    FS_Unmount("");
    USBD_MSD_Task();    // Task, does only return when device is non-configured mode
    FS_Mount("");
  }
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
*  Function description
*    This routine is started as a task from main.
*    It creates a USB task and continues handling the file system.
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
  FS_Init();
  OS_CREATETASK(&_USBTCB0, "USB Task", _USBTask, 150, _aUSBStack);
  _FSTask();
}

/**************************** end of file ***************************/

