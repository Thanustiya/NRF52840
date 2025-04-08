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

File    : USB_MTP_Start.c
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
#include "USB.h"
#include "USB_MTP.h"
#include "FS.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
//
// Set to 1 to use emFile BIGFAT storage layer which allows transfer of files > 4GB.
//
#define USE_FS_BIGFAT             0

#define NUM_BYTES_OBJECT_LIST     (4 * 1024)
#define NUM_BYTES_DATA_BUFFER     (4 * USB_HS_BULK_MAX_PACKET_SIZE)
#define NUM_BYTES_RECEIVE_BUFFER  (2 * USB_HS_BULK_MAX_PACKET_SIZE)

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
  0x8765,           // VendorId
  0x1001,           // ProductId
  "Vendor",         // VendorName
  "MTP device",     // ProductName
  "000013245678"    // SerialNumber. Should be 12 character or more for compliance with Mass Storage Device For Bootability spec.
};
//
// MTP device information.
//
static USB_MTP_INFO _MTPInfo = {
  "Vendor",         // Manufacturer
  "Storage device", // Model
  "1.0",            // DeviceVersion
  "0123456789ABCDEF0123456789ABCDEF" // SerialNumber. It must be exactly 32 characters long.
};
/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U8  _acReceiveBuffer[NUM_BYTES_RECEIVE_BUFFER];              // Receive buffer for the USB device
static U32 _aDataBuffer[NUM_BYTES_DATA_BUFFER / sizeof(U32)];       // Send/receive buffer for MTP transactions
static U32 _aObjectList[NUM_BYTES_OBJECT_LIST / sizeof(U32)];       // Memory pool for the MTP object list

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _FSTest
*/
static void _FSTest(void) {
  FS_FILE    * pFile;
  unsigned     NumBytes;
  const char * sInfo = "This sample is based on the SEGGER emUSB-Device software with the MTP component.\r\nFor further information please visit: www.segger.com\r\n";
  unsigned     NumVolumes;
  unsigned     i;
  char         acVolumeName[32];

  NumBytes   = strlen(sInfo);
  NumVolumes = FS_GetNumVolumes();
  for (i = 0; i < NumVolumes; i++) {
    FS_GetVolumeName(i, &acVolumeName[0], sizeof(acVolumeName));
    if (FS_IsLLFormatted(acVolumeName) == 0) {
      FS_X_Log("Low-level format\n");
      FS_FormatLow(acVolumeName);         // Erase and low-level format the volume.
    }
    if (FS_IsHLFormatted(acVolumeName) == 0) {
      FS_X_Log("High-level formatting\n");
      FS_Format(acVolumeName, NULL);      // High-level format the volume.
    }
    strcat(acVolumeName, "\\Readme.txt");
    pFile = FS_FOpen(acVolumeName, "w");
    if (pFile) {
      FS_Write(pFile, sInfo, NumBytes);
      FS_FClose(pFile);
    }
  }
}

/*********************************************************************
*
*       _cbObjectAllocFail
*
*  Function description
*    Callback which is called when the MTP object list is full.
*/
static void _cbObjectAllocFail(U32 NumBytesRequested, U32 NumBytesAvail, const char * pFilePath, const char * pFileName) {
  USBD_Logf_Application("MTP: Could not allocate object, bytes needed: %d, free space in the object list: %d", NumBytesRequested, NumBytesAvail);
  USBD_Logf_Application("MTP: for file %s%s", pFilePath, pFileName);
}

/*********************************************************************
*
*       _AddMTP
*
*  Function description
*    Adds MTP component to USB stack. This is a 2 step operation.
*    First the MTP component is added to USB stack. Then the one or more
*    storage drivers are added to MTP component.
*
*  Notes
*     (1) The interrupt endpoint is used to report events to USB host.
*         It must be allocated even when the device does not support events.
*/
static void _AddMTP(void) {
  USB_MTP_INIT_DATA InitData;
  USB_MTP_INST_DATA InstData;
  USB_ADD_EP_INFO   EPBulkIn;
  USB_ADD_EP_INFO   EPBulkOut;
  USB_ADD_EP_INFO   EPIntIn;

  //
  // Add the MTP component to USB stack.
  //
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
  InitData.EPOut = USBD_AddEPEx(&EPBulkOut, _acReceiveBuffer, sizeof(_acReceiveBuffer));

  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (64 * 125us)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);    // Note 1

  InitData.pObjectList         = _aObjectList;
  InitData.NumBytesObjectList  = sizeof(_aObjectList);
  InitData.pDataBuffer         = _aDataBuffer;
  InitData.NumBytesDataBuffer  = sizeof(_aDataBuffer);
  InitData.pMTPInfo            = &_MTPInfo;
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_MTP_Add(&InitData);
  USBD_MTP_SetObjectAllocFailCb(_cbObjectAllocFail);
  //
  // Add a storage driver to MTP component.
  //
  USB_MEMSET(&InstData, 0, sizeof(InstData));
#if USE_FS_BIGFAT
  InstData.pAPI                = &USB_MTP_StorageFS_BIGFAT;
#else
  InstData.pAPI                = &USB_MTP_StorageFS;
#endif
  InstData.sDescription        = "MTP volume";
  InstData.sVolumeId           = "0123456789";
  InstData.DriverData.pRootDir = "";
//  InstData.DriverData.Protect  = MTP_STORAGE_PROTECT_ALL;
  USBD_MTP_AddStorage(&InstData);
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
  FS_FAT_SupportLFN();
  _FSTest();
  _AddMTP();
  USBD_Start();
  while (1) {
    //
    // Wait for the host to configure the USB device.
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    USBD_MTP_Task();
  }
}

/**************************** end of file ***************************/
