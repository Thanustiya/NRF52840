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

File    : USB_MSD_FS_DisconnectOnWrite.c
Purpose : Sample showing how to use the FS and USB mass storage device
          simultaneously.
          This sample creates 2 tasks.
          The USB task controls the mass storage device
          On a given timeout the FSTask disables access to storage
          device.
          While it is disabled HOST file system can not access
          the storage device. In this period embedded file system
          can access the storage device.
          After the data has been written to storage device, HOST FS
          gains access to storage device.

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
    Any read/write operations will be shown in the debug terminal.
    Periodically the volume will detach itself for a short period,
    after the volume has re-attached and a new line will  be written
    by the application into the file "LOG.TXT" containing the current
    embOS time.

  Sample output:
    <...>

    6:109 USB Task - T6109, Prevent removal
    6:112 USB Task - T6:112, Start Read operation (StartLBA: 0, NumBlocks: 2)
    6:113 USB Task - T6:113, End   Read operation (StartLBA: 0, NumBlocks: 2)
    6:117 USB Task - T6:117, Start Read operation (StartLBA: 62336, NumBlocks: 2)
    6:118 USB Task - T6:117, End   Read operation (StartLBA: 62336, NumBlocks: 2)
    6:118 USB Task - T6:118, Start Read operation (StartLBA: 62366, NumBlocks: 1)
    6:118 USB Task - T6:118, End   Read operation (StartLBA: 62366, NumBlocks: 1)
    6:118 USB Task - T6:118, Start Read operation (StartLBA: 62367, NumBlocks: 1)
    6:119 USB Task - T6:119, End   Read operation (StartLBA: 62367, NumBlocks: 1)
    6:119 USB Task - T6:119, Start Read operation (StartLBA: 2, NumBlocks: 2)
    6:120 USB Task - T6:120, End   Read operation (StartLBA: 2, NumBlocks: 2)

    <...>
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
*       Defines configurable
*
**********************************************************************
*/
#define TIMEOUT_WAIT_HOST    7000     // Max. time to wait for the host to allow disconnection

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
static OS_TASK         _USBTCB0;                  // Task-control-blocks

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _WriteLogFile
*/
static void _WriteLogFile(void) {
  char      ac[30];
  FS_FILE * pFile;
  U32       Time;
  int       i;
  char      *x;

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
}

/*********************************************************************
*
*       _FSTask
*/
static void _FSTask(void) {
  int r;
  if (FS_IsLLFormatted("") <= 0) {
    USBD_Logf_Application("Low level formatting");
    FS_FormatLow("");          // Erase & Low-level  format the flash
  }
  if (FS_IsHLFormatted("") <= 0) {
    USBD_Logf_Application("High level formatting\n");
    FS_Format("", NULL);       // High-level format the flash
  }
  FS_SetAutoMount("", 0);
  FS_Mount("");
  while (1) {
    USBD_MSD_RequestDisconnect(0);
    BSP_SetLED(2);
    r = USBD_MSD_WaitForDisconnection(0, TIMEOUT_WAIT_HOST);
    if (r == 0) {
      USBD_MSD_Disconnect(0); // Force disconnection
    }
    BSP_ClrLED(2);
    BSP_SetLED(3);
    FS_Mount("");
    _WriteLogFile();
    FS_Unmount("");
    BSP_ClrLED(3);
    USBD_MSD_Connect(0);
    OS_Delay(30000);
  }
}

/*********************************************************************
*
*       _OnPreventAllowRemoval
*
*/
static void _OnPreventAllowRemoval(U8 PreventRemoval) {
  if (PreventRemoval) {
    USBD_Logf_Application("T%lu, Prevent removal", USB_OS_GetTickCnt());
    BSP_SetLED(1);
  } else {
    USBD_Logf_Application("T%lu, Allow removal", USB_OS_GetTickCnt());
    BSP_ClrLED(1);
  }
}

/*********************************************************************
*
*       _OnReadWrite
*
*/
static void _OnReadWrite(U8 Lun, U8 IsRead, U8 OnOff, U32 StartLBA, U32 NumBlocks) {
  U32 t;

  BSP_USE_PARA(Lun);

  t = USB_OS_GetTickCnt();
  if (OnOff) {
    USBD_Logf_Application("T%lu:%lu, Start %s operation (StartLBA: %lu, NumBlocks: %lu)", t / 1000, t % 1000 ,IsRead ? "Read" : "Write", StartLBA, NumBlocks);
    BSP_SetLED(3);
  } else {
    USBD_Logf_Application("T%lu:%lu, End   %s operation (StartLBA: %lu, NumBlocks: %lu)", t / 1000, t % 1000, IsRead ? "Read" : "Write", StartLBA, NumBlocks);
    BSP_ClrLED(3);
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
  USBD_MSD_SetPreventAllowRemovalHook(0, _OnPreventAllowRemoval);
  USBD_MSD_SetReadWriteHook(0, _OnReadWrite);
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

