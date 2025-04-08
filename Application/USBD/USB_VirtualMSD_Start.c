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

File    : USB_VirtualMSD_Start.c
Purpose : This sample shows the basic usage of the VirtualMSD component.

Additional information:
  Preparations:
    None.

  Expected behavior:
    When the device running this sample is
    connected to a USB host the sample shows which files
    are read and written from the host.
    This information can be seen in the debug terminal
    output of your IDE.
    The sample also shows a couple of constant files
    on the volume.

  Sample output:

    <...>

    1:387 MainTask - VMSD_APP: _cbOnRead(): File 'VIRTFILETXT', Offset 0
    1:388 MainTask - VMSD_APP: _cbOnRead(): File 'VIRTFILETXT', Offset 512
    1:388 MainTask - VMSD_APP: _cbOnRead(): File 'VIRTFILETXT', Offset 1024
    27:628 MainTask - VMSD_APP: _cbOnWrite(): File 'SEGGER~1SWP', Offset 0
    27:628 MainTask - VMSD_APP: _cbOnWrite(): File 'SEGGER~1SWX', Offset 0
    37:056 MainTask - VMSD_APP: _cbOnWrite(): File 'SEGGER  TXT', Offset 0
    37:057 MainTask - VMSD_APP: _cbOnWrite(): File 'unknown', Offset 0
    37:057 MainTask - VMSD_APP: _cbOnWrite(): File 'SEGGER  TXT', Offset 0
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "BSP.h"
#include "RTOS.h"
#include "USB.h"
#include "USB_MSD.h"
#include "USB_VirtualMSD.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define _VMSD_NUM_SECTORS (8000)

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
  0x1000,         // ProductId
  "Vendor",       // VendorName
  "VirtualMSD device",  // ProductName
  "0123456789AB"  // SerialNumber. Should be 12 character or more for compliance with Mass Storage Device For Bootability spec.
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
//
// String information used when inquiring the volume 1.
//
static const USB_MSD_LUN_INFO _Lun1Info = {
  "Vendor",     // MSD VendorName
  "MSD Volume", // MSD ProductName
  "1.00",       // MSD ProductVer
  "134657891"   // MSD SerialNo
};

//
// Remaining bytes of sector are filled with 0s on read, if a file does not occupy complete sectors
//
static const U8 _abFile_ReadmeTxt[] = {
  0x54, 0x68, 0x69, 0x73, 0x20, 0x73, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x73, 0x68, 0x6F, 0x77,
  0x73, 0x20, 0x68, 0x6F, 0x77, 0x20, 0x73, 0x69, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x61, 0x6E, 0x64,
  0x20, 0x65, 0x61, 0x73, 0x79, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x6F, 0x20, 0x68,
  0x61, 0x76, 0x65, 0x20, 0x61, 0x6E, 0x20, 0x4D, 0x53, 0x44, 0x20, 0x64, 0x72, 0x69, 0x76, 0x65,
  0x0D, 0x0A, 0x63, 0x6F, 0x6D, 0x70, 0x6C, 0x65, 0x74, 0x65, 0x6C, 0x79, 0x20, 0x76, 0x69, 0x72,
  0x74, 0x75, 0x61, 0x6C, 0x69, 0x7A, 0x65, 0x64, 0x2E, 0x20, 0x49, 0x74, 0x20, 0x63, 0x61, 0x6E,
  0x20, 0x62, 0x65, 0x20, 0x75, 0x73, 0x65, 0x64, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x66, 0x69, 0x72,
  0x6D, 0x77, 0x61, 0x72, 0x65, 0x20, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x73, 0x0D, 0x0A, 0x66,
  0x69, 0x6C, 0x65, 0x20, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x72, 0x20, 0x6F, 0x72, 0x20, 0x6C,
  0x6F, 0x67, 0x2D, 0x64, 0x61, 0x74, 0x61, 0x20, 0x70, 0x72, 0x6F, 0x76, 0x69, 0x64, 0x65, 0x72,
  0x2E, 0x20, 0x46, 0x6F, 0x72, 0x20, 0x66, 0x75, 0x72, 0x74, 0x68, 0x65, 0x72, 0x20, 0x69, 0x6E,
  0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x70, 0x6C, 0x65, 0x61, 0x73, 0x65,
  0x20, 0x76, 0x69, 0x73, 0x69, 0x74, 0x20, 0x6F, 0x75, 0x72, 0x20, 0x77, 0x65, 0x62, 0x73, 0x69,
  0x74, 0x65, 0x3A, 0x0D, 0x0A, 0x77, 0x77, 0x77, 0x2E, 0x73, 0x65, 0x67, 0x67, 0x65, 0x72, 0x2E,
  0x63, 0x6F, 0x6D, 0x20, 0x6F, 0x72, 0x20, 0x68, 0x61, 0x76, 0x65, 0x20, 0x61, 0x20, 0x63, 0x6C,
  0x6F, 0x73, 0x65, 0x72, 0x20, 0x6C, 0x6F, 0x6F, 0x6B, 0x20, 0x69, 0x6E, 0x74, 0x6F, 0x20, 0x74,
  0x68, 0x65, 0x20, 0x65, 0x6D, 0x55, 0x53, 0x42, 0x2D, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20,
  0x6D, 0x61, 0x6E, 0x75, 0x61, 0x6C
};

static const U8 _abFile_SeggerHTML[] = {
 0x3c,  0x68,  0x74,  0x6d,  0x6c,  0x3e,  0x3c,  0x68,  0x65,  0x61,
 0x64,  0x3e,  0x3c,  0x6d,  0x65,  0x74,  0x61,  0x20,  0x68,  0x74,
 0x74,  0x70,  0x2d,  0x65,  0x71,  0x75,  0x69,  0x76,  0x3d,  0x22,
 0x72,  0x65,  0x66,  0x72,  0x65,  0x73,  0x68,  0x22,  0x20,  0x63,
 0x6f,  0x6e,  0x74,  0x65,  0x6e,  0x74,  0x3d,  0x22,  0x30,  0x3b,
 0x20,  0x75,  0x72,  0x6c,  0x3d,  0x68,  0x74,  0x74,  0x70,  0x73,
 0x3a,  0x2f,  0x2f,  0x77,  0x77,  0x77,  0x2e,  0x73,  0x65,  0x67,
 0x67,  0x65,  0x72,  0x2e,  0x63,  0x6f,  0x6d,  0x22,  0x2f,  0x3e,
 0x3c,  0x74,  0x69,  0x74,  0x6c,  0x65,  0x3e,  0x53,  0x45,  0x47,
 0x47,  0x45,  0x52,  0x20,  0x53,  0x68,  0x6f,  0x72,  0x74,  0x63,
 0x75,  0x74,  0x3c,  0x2f,  0x74,  0x69,  0x74,  0x6c,  0x65,  0x3e,
 0x3c,  0x2f,  0x68,  0x65,  0x61,  0x64,  0x3e,  0x3c,  0x62,  0x6f,
 0x64,  0x79,  0x3e,  0x3c,  0x2f,  0x62,  0x6f,  0x64,  0x79,  0x3e,
 0x3c,  0x2f,  0x68,  0x74,  0x6d,  0x6c,  0x3e
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
//
// Constant files which should be displayed on the virtual volume.
//
static USB_VMSD_CONST_FILE _aConstFiles0[] = {
//     sName                     pData                       FileSize                      Flags
  { "readme.txt",             _abFile_ReadmeTxt,          sizeof(_abFile_ReadmeTxt),          0, },
  { "Segger.html",            _abFile_SeggerHTML,         sizeof(_abFile_SeggerHTML),         0, },
};
static USB_VMSD_CONST_FILE _aConstFiles1[] = {
//     sName                     pData                       FileSize                       Flags
  { "readme.txt",             _abFile_ReadmeTxt,          sizeof(_abFile_ReadmeTxt),          0, },
  { "VIRTFILE.TXT",           NULL,                       10 * 1024,                          0, },
};

static U32                      _aMEMBuffer[6 * 1024 / sizeof(U32)];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbOnRead
*
*  Parameters
*    Lun       LUN ID
*    pData     Data which will be sent to the host
*    Off       Offset of the current file requested by the host
*    NumBytes  Number of bytes to read
*    pFile     *Optional* Additional information about the file being written (RootDir entry etc.)
*/
static int _cbOnRead(unsigned Lun, U8* pData, U32 Off, U32 NumBytes, const USB_VMSD_FILE_INFO* pFile) {
  const char *pName;
  unsigned   Len;
  char Buff[50];

  (void)Lun;
  if (pFile != NULL) {
    pName = (char *)pFile->pDirEntry->acFilename;
  } else {
    pName = "unknown";
  }
  USBD_Logf_Application("VMSD_APP: _cbOnRead(): File '%.11s', Offset %u", pName, Off);
  if (memcmp(pName, "VIRTFILE", 8) == 0) {
    for (;;) {
      SEGGER_snprintf(Buff, sizeof(Buff), "VIRTFILE, offset %u\n", Off);
      Len = strlen(Buff);
      if (NumBytes < Len) {
        break;
      }
      memcpy(pData, Buff, Len);
      pData    += Len;
      NumBytes -= Len;
      Off      += Len;
    }
    while (NumBytes-- > 0) {
      *pData++ = '\n';
    }
  } else {
    memset(pData, 0, NumBytes);
  }
  return 0;
}

/*********************************************************************
*
*       _cbOnWrite
*
*  Parameters
*    Lun       LUN ID
*    pData     Data to be written
*    Off       Offset into current file to be written
*    NumBytes  Number of bytes to write into the file
*    pFile     *Optional* Additional information about the file being written (RootDir entry etc.)
*/
static int _cbOnWrite(unsigned Lun, const U8* pData, U32 Off, U32 NumBytes, const USB_VMSD_FILE_INFO* pFile) {
  const char *pName;

  (void)Lun;
  (void)pData;
  (void)NumBytes;
  if (pFile != NULL) {
    pName = (char *)pFile->pDirEntry->acFilename;
  } else {
    pName = "unknown";
  }
  USBD_Logf_Application("VMSD_APP: _cbOnWrite(): File '%.11s', Offset %u", pName, Off);
  return 0;
}

static const USB_VMSD_USER_FUNC_API _UserFuncAPI = {
  _cbOnRead,     // pfOnRead    -> Is called when a sector of a given file is read.
  _cbOnWrite,    // pfOnWrite   -> Is called when a sector of a given file is written.
  NULL,          // pfMemAlloc  -> Optional, can be set in order to allow the VirtualMSD to share the mem alloc function of a system.
  NULL           // pfMemFree   -> Optional, can be set in order to allow the VirtualMSD to share the mem free function of a system.
};

/*********************************************************************
*
*       USB_VMSD_X_Config
*
*  Function description
*    This function is called by the USB MSD Module during USBD_VirtualMSD_Init() and initializes the VirtualMSD volume.
*/
void USB_VMSD_X_Config(void) {
  //
  // Global configuration
  //
  USBD_VMSD_AssignMemory(&_aMEMBuffer[0], sizeof(_aMEMBuffer));
  USBD_VMSD_SetUserAPI(&_UserFuncAPI);
  //
  // Setup LUN0
  //
  USBD_VMSD_SetNumSectors(0, _VMSD_NUM_SECTORS);
  USBD_VMSD_SetSectorsPerCluster(0, 32);    // Anywhere from 1 ... 128, but needs to be a Power of 2
  USBD_VMSD_SetNumRootDirSectors(0, 2);
  USBD_VMSD_SetVolumeInfo(0, "Virt0.MSD", &_Lun0Info);   // Add volume ID
  USBD_VMSD_AddConstFiles(0, &_aConstFiles0[0], SEGGER_COUNTOF(_aConstFiles0));  // Push const file to the volume
  //
  // Setup LUN1
  //
  USBD_VMSD_SetNumSectors(1, _VMSD_NUM_SECTORS / 2);
  USBD_VMSD_SetSectorsPerCluster(1, 32);    // Anywhere from 1 ... 128, but needs to be a Power of 2
  USBD_VMSD_SetNumRootDirSectors(1, 2);
  USBD_VMSD_SetVolumeInfo(1, "Virt1.MSD", &_Lun1Info);   // Add volume ID
  USBD_VMSD_AddConstFiles(1, &_aConstFiles1[0], SEGGER_COUNTOF(_aConstFiles1));  // Push const file to the volume
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
#if defined(__cplusplus)
extern "C"      /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);

void MainTask(void) {
  USBD_Init();
  //
  // Initialize Smart device (also calls USB_VMSD_X_Config())
  //
  USBD_VMSD_Add();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  while (1) {
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    USBD_MSD_Task();
  }
}

/*************************** End of file ****************************/
