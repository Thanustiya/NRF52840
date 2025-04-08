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

Purpose : Demonstrates usage of the HID component as a multi-purpose HID
device.

Additional information:
  The different purposes are defined by different REPORT IDs.
  For output and input reports there are defined the IDs differently.
  In order to simplify this, we will use for the similiar purposes the
  same ID on both IN and OUT:
  +-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | Dir |  Report ID  |      Purpose      |   Bitmap                                                                                                                       |
  +-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | In  |      1      |  Vendor Specific  | [ReportId = 1] + 32 x [any byte data]                                                                                          |
  |-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | Out |      1      |  Vendor Specific  | [ReportId = 1] + 32 x [any byte data]                                                                                          |
  |-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | In  |      2      |  Telephony        |                  ByteOff   |     7     |     6     |     5     |     4     |     3     |     2     |     1     |     0     |   |
  |     |             |                   | [ReportId = 2] +    0      | Alt Func  |  Hook sw  |        Volume         |                Telephony Key Pad              |   |
  |     |             |                   |                +    1      | --------------------- | Phone Mute| Spk phone |    Hold   |   Drop    | Transfer  |  Conf     |   |
  |-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | Out |      2      |  Telephony        |                  ByteOff   |     7     |     6     |     5     |     4     |     3     |     2     |     1     |     0     |   |
  |     |             |   LEDs            | [ReportId = 2] +    0      | --------------------------------- |        Volume         |      Message LED      | Alt Func  |   |
  |-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  | In  |      3      |  Consumer Ctrl    |                  ByteOff   |     7     |     6     |     5     |     4     |     3     |     2     |     1     |     0     |   |
  |     |             |                   | [ReportId = 2] +    0      |  Rnd Play |   Repeat  |  Prev Trk |  Next Trk | Play/Pause|    Mute   |  Volume - |  Volume + |   |
  |-----+-------------+-------------------+--------------------------------------------------------------------------------------------------------------------------------+
  Expected behavior:
    The device will be enumerated as an audio control device.
    If an telephone dial application such as Skype is available
    the device will dial the number 02173993120 and will hook up (dial).
    Then it will increase the telephone speaker volume, one second later
    it will decrease the volume.
    Optional there is an #if 0 switch which allows to play around
    the multimedia switches, it will mute and un-mute the volume of the
    whole system and decreases the volume.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines configurable
*
**********************************************************************
*/
#define INPUT_REPORT_SIZE   32    // Defines the input (device -> host) report size
#define OUTPUT_REPORT_SIZE  32    // Defines the output (Host -> device) report size
#define VENDOR_PAGE_ID      0x12  // Defines the vendor specific page that
                                  // shall be used, allowed values 0x00 - 0xff
                                  // This value must be identical to HOST application
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
  void USBD_HID_MMControl_Init(void);
  void USBD_HID_MMControl_RunTask(void *);
#ifdef __cplusplus
}
#endif


/*********************************************************************
*
*       Type definition
*
**********************************************************************
*/
typedef enum {
  REPORT_VENDOR_SPECIFIC = 0x01u,
  REPORT_TELEPHONY = 0x02u,
  REPORT_CONSUMER = 0x03
} REPORT_ID;
//
// Each report consists of 2 bytes: The report ID (0x01) and a bit mask
// containing 8 control events:
//
typedef enum {
  TELEPHONE_KEY_0 = 0x01u,
  TELEPHONE_KEY_1 = 0x02u,
  TELEPHONE_KEY_2 = 0x03u,
  TELEPHONE_KEY_3 = 0x04u,
  TELEPHONE_KEY_4 = 0x05u,
  TELEPHONE_KEY_5 = 0x06u,
  TELEPHONE_KEY_6 = 0x07u,
  TELEPHONE_KEY_7 = 0x08u,
  TELEPHONE_KEY_8 = 0x09u,
  TELEPHONE_KEY_9 = 0x0Au,
  TELEPHONE_KEY_STAR = 0x0Bu,
  TELEPHONE_KEY_POUND = 0x0Cu
} TELEPHONE_KEYS;

typedef enum {
  VOLUME_INCREMENT = 1,
  VOLUME_DECREMENT = -1,
} VOLUME_KEYS;

typedef enum {
  INDICATOR_ON = 0u,
  INDICATOR_BLINKING = 1u,
  INDICATOR_OFF = 2u,
} LED_MESSAGE_INDICATOR_TYPE;

typedef struct {
  U8 ReportId : 8;
  union {
    U8 Keys;
    struct {
      U8 VolumeUp : 1;
      U8 VolumeDown : 1;
      U8 Mute : 1;
      U8 PlayPause : 1;
      U8 ScanNextTrack : 1;
      U8 ScanPrevTrack : 1;
      U8 Repeat : 1;
      U8 RandomPlay : 1;
    } b;
  } Input;
} CONSUMER_INPUT_REPORT;

typedef struct {
  U8 ReportId : 8;
  union {
    U8 Data;
    struct {
      U8  TelephonyKey : 4;
      I8  Volume : 2;
      U8  HookSwitch : 1;
      U8  AlternateFunc : 1;
    } b;
  } Input0;
  union {
    U8 Data;
    struct {
      U8 Conference : 1;
      U8 Transfer : 1;
      U8 Drop : 1;
      U8 Hold : 1;
      U8 Speakerphone : 1;
      U8 PhoneMute : 1;
      U8 Reserved : 2;
    } b;
  } Input1;
} TELEPHONE_INPUT_REPORT;

typedef struct {
  REPORT_ID ReportId;
  union {
    U8 Data;
    struct {
      U8  AlternateFunc : 1;
      U8  MessageIndicator : 2;
      U8  Volume : 2;
      U8  Reserved : 3;
    } b;
  } Output;
} TELEPHONE_OUTPUT_REPORT;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
/*********************************************************************
*
*       Information that are used during enumeration
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,               // VendorId
  0x111B,               // ProductId
  "Vendor",             // VendorName
  "HID audio/telephony control",  // ProductName
  "12345678"            // SerialNumber
};
#endif

/*********************************************************************
*
*       _aHIDReport
*
*  This report is generated according to HID spec and
*  HID Usage Tables specifications.
*/
static const U8 _aHIDReport[] = {
    0x06, VENDOR_PAGE_ID, 0xff,    // USAGE_PAGE (Vendor Page)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, INPUT_REPORT_SIZE,       //   REPORT_COUNT (INPUT_REPORT_SIZE)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, OUTPUT_REPORT_SIZE,      //   REPORT_COUNT (OUTPUT_REPORT_SIZE)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0,                          // END_COLLECTION
    0x05, 0x0b,                    // USAGE_PAGE (Telephony Devices)
    0x09, 0x01,                    // USAGE (Phone)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x05, 0x0b,                    //   USAGE_PAGE (Telephony Devices)
    0x09, 0x06,                    //   USAGE (Telephony Key Pad)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x19, 0xb0,                    //     USAGE_MINIMUM (Phone Key 0)
    0x29, 0xbb,                    //     USAGE_MAXIMUM (Phone Key Pound)
    0x15, 0x01,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0c,                    //     LOGICAL_MAXIMUM (12)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0xc0,                          //   END_COLLECTION
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0xff,                    //   LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x05, 0x0b,                    //   USAGE_PAGE (Telephony Devices)
    0x09, 0x20,                    //   USAGE (Hook Switch)
    0x09, 0x29,                    //   USAGE (Alternate Function)
    0x09, 0x2c,                    //   USAGE (Conference)
    0x09, 0x25,                    //   USAGE (Transfer)
    0x09, 0x26,                    //   USAGE (Drop)
    0x09, 0x23,                    //   USAGE (Hold)
    0x09, 0x2b,                    //   USAGE (Speaker Phone)
    0x09, 0x2f,                    //   USAGE (Phone Mute)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x81, 0x40,                    //   INPUT (Data,Ary,Abs,Null)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x81, 0x05,                    //   INPUT (Cnst,Ary,Rel)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x09, 0x3b,                    //   USAGE (Usage In Use Indicator)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x0b,                    //     USAGE_PAGE (Telephony Devices)
    0x09, 0x29,                    //     USAGE (Alternate Function)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x09, 0x3c,                    //   USAGE (Usage Multi Mode Indicator)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x0b,                    //     USAGE_PAGE (Telephony Devices)
    0x09, 0x73,                    //     USAGE (Message)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x0b, 0x3d, 0x00, 0x08, 0x00,  //       USAGE (LEDs:Indicator On)
    0x0b, 0x40, 0x00, 0x08, 0x00,  //       USAGE (LEDs:Indicator Fast Blink)
    0x0b, 0x41, 0x00, 0x08, 0x00,  //       USAGE (LEDs:Indicator Off)
    0x75, 0x02,                    //       REPORT_SIZE (2)
    0x91, 0x40,                    //       OUTPUT (Data,Ary,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0xff,                    //   LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x91, 0x06,                    //   OUTPUT (Data,Var,Rel)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x91, 0x07,                    //   OUTPUT (Cnst,Var,Rel)
    0xc0,                          // END_COLLECTION
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x03,                    //   REPORT_ID (3)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0xe9,                    //   USAGE (Volume Up)
    0x09, 0xea,                    //   USAGE (Volume Down)
    0x09, 0xe2,                    //   USAGE (Mute)
    0x09, 0xcd,                    //   USAGE (Play/Pause)
    0x09, 0xb5,                    //   USAGE (Scan Next Track)
    0x09, 0xb6,                    //   USAGE (Scan Previous Track)
    0x09, 0xbc,                    //   USAGE (Repeat)
    0x09, 0xb9,                    //   USAGE (Random Play)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
 };
//
// Number to dial: 02173993120 -> call SEGGER ;)
//
static const TELEPHONE_KEYS _aTelephoneNumber[] = { TELEPHONE_KEY_0, TELEPHONE_KEY_2,
                                                    TELEPHONE_KEY_1, TELEPHONE_KEY_7, TELEPHONE_KEY_3,
                                                    TELEPHONE_KEY_9, TELEPHONE_KEY_9, TELEPHONE_KEY_3,
                                                    TELEPHONE_KEY_1, TELEPHONE_KEY_2, TELEPHONE_KEY_0 };

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static USB_HID_HANDLE  _hInst;
static U8              _abOutBuffer[USB_HS_INT_MAX_PACKET_SIZE];


/*********************************************************************
*
*       _AddHID
*
*  Function description
*    Add HID mouse class to USB stack
*/
void USBD_HID_MMControl_Init(void) {
  USB_HID_INIT_DATA_EX InitData;
  USB_ADD_EP_INFO   EPIntIn;
  USB_ADD_EP_INFO   EPIntOut;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Report size.
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);
  EPIntOut.Flags           = 0;                             // Flags not used.
  EPIntOut.InDir           = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPIntOut.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntOut.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Report size.
  EPIntOut.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPOut = USBD_AddEPEx(&EPIntOut, _abOutBuffer, sizeof(_abOutBuffer));

  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport = sizeof(_aHIDReport);
  InitData.pInterfaceName = "Multimedia Control";
  _hInst = USBD_HID_AddEx(&InitData);
}

/*********************************************************************
*
*       USBD_HID_Echo1_RunTask
*
*  Function description
*    Performs the HID echo1 operation
*/
void USBD_HID_MMControl_RunTask(void * pPara) {
  CONSUMER_INPUT_REPORT aConsumerInput[2];
  TELEPHONE_INPUT_REPORT TelephoneInput;
  U8 Buffer[64];
  int r;

  USB_USE_PARA(pPara);
  USB_MEMSET(&aConsumerInput, 0, sizeof(aConsumerInput));
  USB_MEMSET(&TelephoneInput, 0, sizeof(TelephoneInput));
  for (;;) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    r = USBD_HID_ReceivePoll(_hInst, Buffer, sizeof(Buffer), 1500);
    if (r > 0) {
      //
      // Handle incoming data
      //
      if (Buffer[0] == REPORT_VENDOR_SPECIFIC) {
        //
        // Vendor defined report -> echo
        //
        Buffer[1]++;
        USBD_HID_Write(_hInst, Buffer, r, 0);
        continue;
      }
      if (Buffer[0] == REPORT_TELEPHONY) {
        //
        // Handle Telephony LEDs here
        //
      }
    }
#if 0 // Enable this to support consumer control and perform some mute/volume action!
    //
    // Setup the Consumer Report
    // Since we have different HID usages, we have to provide the ReportID of the specific usage.
    //
    aConsumerInput[0].ReportId = REPORT_CONSUMER;
    //
    // We will toggle the mute state
    //
    aConsumerInput[0].Input.b.Mute ^= 1;
    //
    // And we will decrease the volume too in another report
    //
    aConsumerInput[1].ReportId = REPORT_CONSUMER;
    aConsumerInput[1].Input.b.VolumeDown = 1;
    //
    // Both reports will be sent to the host.
    //
    USBD_HID_Write(_hInst, &aConsumerInput, sizeof(aConsumerInput), 0);
#endif
    //
    // Setup the Telephony Report
    //
    TelephoneInput.ReportId = REPORT_TELEPHONY;
    //
    // Let send to the host we want to dial a number
    // and will enable the hook to call to dialed number
    //
    for (unsigned i = 0; i < SEGGER_COUNTOF(_aTelephoneNumber); i++) {
      TelephoneInput.Input0.b.TelephonyKey = _aTelephoneNumber[i];
      USBD_HID_Write(_hInst, &TelephoneInput, sizeof(TelephoneInput), 0);
    }
    //
    // Reset report data and send the hook on switch state
    //
    TelephoneInput.Input0.Data = 0;
    TelephoneInput.Input0.b.HookSwitch = 1;
    USBD_HID_Write(_hInst, &TelephoneInput, sizeof(TelephoneInput), 0);
    //
    // Reset report once again and tell the host that we want to increase the volume
    //
    TelephoneInput.Input0.Data = 0;
    TelephoneInput.Input0.b.Volume = VOLUME_INCREMENT;
    USBD_HID_Write(_hInst, &TelephoneInput, sizeof(TelephoneInput), 0);
    TelephoneInput.Input0.Data = 0;
    USB_OS_Delay(1000);
    TelephoneInput.Input0.b.Volume = VOLUME_DECREMENT;
    USBD_HID_Write(_hInst, &TelephoneInput, sizeof(TelephoneInput), 0);
    TelephoneInput.Input0.Data = 0;
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
* Function description
*   USB handling task.
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
void MainTask(void) {
  USBD_Init();
  USBD_HID_MMControl_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  USBD_HID_MMControl_RunTask(NULL);
}
#endif
/**************************** end of file ***************************/
