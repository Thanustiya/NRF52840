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

Purpose : Demonstrates usage of the HID component of the USB stack as
          a mouse and keyboard.

Additional information:
  Preparations:
    It is advised to open a notepad application before
    connecting the USB cable.

  Expected behavior:
    After connecting the USB cable the sample
    makes the mouse jump left & right as well as types
    a predefined string like from a regular keyboard.

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
#include <ctype.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
//
// Specifies whether the return key should be sent in this sample.
//
#define SEND_RETURN 0

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
  0x1116,         // ProductId. Should be unique for this sample
  "Vendor",       // VendorName
  "HID mouse/keyboard sample",  // ProductName
  "12345678"      // SerialNumber
};

/*********************************************************************
*
*       Local data definitions
*
**********************************************************************
*/
typedef struct {
  U16 KeyCode;
  char cCharacter;
} SCANCODE_TO_DESC;

/*********************************************************************
*
*       _aHIDReportKeyboard
*
*  This report is generated according to HID spec and
*  HID Usage Tables specifications.
*/
const U8 _aHIDReportKeyboard[] = {
  USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
  USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_KEYBOARD,
  USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_APPLICATION,
    USB_HID_GLOBAL_USAGE_PAGE + 1, 7,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 224,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 231,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 8,
    USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE,
    USB_HID_MAIN_INPUT + 1, 1,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 0,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 8,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 6,
    USB_HID_MAIN_INPUT + 1, 0,
    USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_LEDS,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 1,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 5,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 5,
    USB_HID_MAIN_OUTPUT + 1, 2,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 3,
    USB_HID_MAIN_OUTPUT + 1, 1,
  USB_HID_MAIN_ENDCOLLECTION
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
*       Static data
*
**********************************************************************
*/
static const  SCANCODE_TO_DESC _aScanCode2StringTable[] = {
  { 0x04, 'a'},
  { 0x05, 'b'},
  { 0x06, 'c'},
  { 0x07, 'd'},
  { 0x08, 'e'},
  { 0x09, 'f'},
  { 0x0A, 'g'},
  { 0x0B, 'h'},
  { 0x0C, 'i'},
  { 0x0D, 'j'},
  { 0x0E, 'k'},
  { 0x0F, 'l'},
  { 0x10, 'm'},
  { 0x11, 'n'},
  { 0x12, 'o'},
  { 0x13, 'p'},
  { 0x14, 'q'},
  { 0x15, 'r'},
  { 0x16, 's'},
  { 0x17, 't'},
  { 0x18, 'u'},
  { 0x19, 'v'},
  { 0x1A, 'w'},
  { 0x1B, 'x'},
  { 0x1C, 'y'},
  { 0x1D, 'z'},
  { 0x1E, '1'},
  { 0x1F, '2'},
  { 0x20, '3'},
  { 0x21, '4'},
  { 0x22, '5'},
  { 0x23, '6'},
  { 0x24, '7'},
  { 0x25, '8'},
  { 0x26, '9'},
  { 0x27, '0'},
  { 0x2C, ' '},
  { 0x37, '.'}
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

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
  USB_HID_HANDLE    hMouse;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  InitData.pReport = _aHIDReportMouse;
  InitData.NumBytesReport = sizeof(_aHIDReportMouse);
  hMouse = USBD_HID_Add(&InitData);
  return hMouse;
}

/*********************************************************************
*
*       _AddHIDKeyboard
*
*  Function description
*    Add HID keyboard class to USB stack
*/
static USB_HID_HANDLE _AddHIDKeyboard(void) {
  USB_HID_INIT_DATA InitData;
  USB_ADD_EP_INFO   EPIntIn;
  USB_HID_HANDLE    hKeyboard;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  InitData.pReport = _aHIDReportKeyboard;
  InitData.NumBytesReport = sizeof(_aHIDReportKeyboard);
  hKeyboard = USBD_HID_Add(&InitData);
  return hKeyboard;
}


/*********************************************************************
*
*       _Output
*
*  Function description
*    Outputs a string
*/
static void _SendText(USB_HID_HANDLE hKeyboard, const char * sString) {
  U8   ac[8];
  char cTemp;
  unsigned int i;
  unsigned int j;

  memset(ac, 0, sizeof(ac));
  for (i = 0; sString[i] != 0; i++) {
    //
    // A character is uppercase if it's hex value is less than 0x61 ('a')
    // and greater or equal to 0x41 ('A'), therefore we set the
    // LeftShiftUp bit for those characters
    //
    if (sString[i] < 0x61 && sString[i] >= 0x41) {
      ac[0] = (1 << 1); // LeftShiftUp bit.
      cTemp = tolower((int)sString[i]);
    } else {
      cTemp = sString[i];
    }
    for (j = 0; j < SEGGER_COUNTOF(_aScanCode2StringTable); j++) {
      if (_aScanCode2StringTable[j].cCharacter == cTemp) {
        ac[2] = _aScanCode2StringTable[j].KeyCode;
      }
    }
    USBD_HID_Write(hKeyboard, &ac[0], 8, 0);
    memset(ac, 0, sizeof(ac));
    //
    // Send a 0 field packet to tell the host that the key has been released
    //
    USBD_HID_Write(hKeyboard, &ac[0], 8, 0);
    USB_OS_Delay(50);
  }
}

#if (SEND_RETURN == 1)
/*********************************************************************
*
*       _SendReturnCharacter
*
*  Function description
*    Outputs a return character
*    It is an optional function.
*    Has been make public in order to avoid,
*    static function is not used.
*/
static void _SendReturnCharacter(USB_HID_HANDLE hKeyboard) {
  U8 ac[8];

  memset(ac, 0, sizeof(ac));
  ac[2] = 0x28;
  USBD_HID_Write(hKeyboard, &ac[0], 8, 0);
  memset(ac, 0, sizeof(ac));
  //
  // Send a 0 field packet to tell the host that the key has been released
  //
  USBD_HID_Write(hKeyboard, &ac[0], 8, 0);
  USB_OS_Delay(50);
}
#endif

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
  U8 ac[3];
  const char * sInfo0 = "Hello world! ";
  USB_HID_HANDLE hMouse;
  USB_HID_HANDLE hKeyboard;

  USBD_Init();
  hKeyboard = _AddHIDKeyboard();
  hMouse    = _AddHIDMouse();
  USBD_SetDeviceInfo(&_DeviceInfo);
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
    memset (ac, 0, sizeof(ac));
    //
    // Move the mouse to the right
    //
    ac[1] = 20;   // To the right !
    USBD_HID_Write(hMouse, &ac[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
    USB_OS_Delay(100);
    //
    // Move mouse to the left
    //
    ac[1] = (U8)-20;  // To the left !
    USBD_HID_Write(hMouse, &ac[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
    USB_OS_Delay(100);
    //
    // Type some text via keyboard
    //
    _SendText(hKeyboard, sInfo0);
    //
    // The "_SendReturnCharacter()" line can be added if desired. Please set
    // the SEND_RETURN define to 1 in the "Defines, configurable" section of this file.
    // This function will send a Return/Enter key to the host.
    // Usually this is not wanted as a return key may have undesired behavior.
    //
#if (SEND_RETURN == 1)
    _SendReturnCharacter(hKeyboard);
#endif
  }
}

/**************************** end of file ***************************/

