/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2007 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet * TCP/IP stack for embedded applications               *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product for in-house use.         *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet version: V3.60.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

Purpose : Sample program for embOS & TCP/IP & WiFi interface.
          Demonstrates use of the IP stack without any server or
          client program using a WiFi interface.
          To ping the target, use the command line:
          > ping <target-ip>
          Where <target-ip> represents the IP address of the target,
          which depends on the configuration.
          The application regularly scans the available WiFi networks
          and outputs them as list.
Notes   : For compatibility with interfaces that need to connect in
          any way this sample calls connect and disconnect routines
          that might not be needed in all cases.

          This sample can be used for Ethernet and dial-up interfaces
          and is configured to use the last registered interface as
          its main interface.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

typedef struct {
  I32  Rssi;
  U8   abBSSID[6];
  char acSSID[33];  // SSID can be up to 32 characters + 1 byte for string termination.
  U8   Channel;
  U8   Security;
} SCAN_RESULT;

static U8                      _ScanFinished;
static unsigned                _NumNetworks;
static SCAN_RESULT             _aNetwork[20];

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/********************************************************************
*
*       _GlobalScanResult
*
*  Function description
*    Callback used to handle the results from a global scan.
*    Results are saved into a list that can be shown later.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*    pResult: Pointer to IP_WIFI_SCAN_RESULT structure of the scan
*             result for the network to connect to.
*    Status :  1: This is the last (empty) result returned for the current scan.
*              0: Next result returned for the current scan.
*             -1: Error, scan aborted.
*/
static void _GlobalScanResult(unsigned IFaceId, const IP_WIFI_SCAN_RESULT *pResult, int Status) {
  IP_USE_PARA(IFaceId);

  if (pResult) {
    if (_NumNetworks < SEGGER_COUNTOF(_aNetwork)) {
      IP_STRCPY(&_aNetwork[_NumNetworks].acSSID[0] ,  pResult->sSSID);
      IP_MEMCPY(&_aNetwork[_NumNetworks].abBSSID[0], &pResult->abBSSID[0], 6);
      _aNetwork[_NumNetworks].Channel  = pResult->Channel;
      _aNetwork[_NumNetworks].Security = pResult->Security;
      _aNetwork[_NumNetworks].Rssi     = pResult->Rssi;
      _NumNetworks++;
    }
  }
  if (Status != 0) {
    _ScanFinished = 1;
  }
}

/*********************************************************************
*
*       _OnStateChange()
*
* Function description
*   Callback that will be notified once the state of an interface
*   changes.
*
* Parameters
*   IFaceId   : Zero-based interface index.
*   AdminState: Is this interface enabled ?
*   HWState   : Is this interface physically ready ?
*/
static void _OnStateChange(unsigned IFaceId, U8 AdminState, U8 HWState) {
  //
  // Check if this is a disconnect from the peer or a link down.
  // In this case call IP_Disconnect() to get into a known state.
  //
  if (((AdminState == IP_ADMIN_STATE_DOWN) && (HWState == 1)) ||  // Typical for dial-up connection e.g. PPP when closed from peer. Link up but app. closed.
      ((AdminState == IP_ADMIN_STATE_UP)   && (HWState == 0))) {  // Typical for any Ethernet connection e.g. PPPoE. App. opened but link down.
    IP_Disconnect(IFaceId);                                       // Disconnect the interface to a clean state.
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
* Function description
*   Main task executed by the RTOS to create further resources and
*   running the main application.
*/
void MainTask(void) {
  unsigned u;

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  //
  // Scan
  //
ScanAgain:
  //
  // Scan and list available network
  //
  _ScanFinished = 0;
  _NumNetworks  = 0;
  while (IP_WIFI_Scan(0, 5000, _GlobalScanResult, NULL, IP_WIFI_CHANNEL_ALL)) {
    OS_Delay(50);
  }
  while (_ScanFinished == 0) {
    OS_Delay(10);
  }
  if (_NumNetworks) {
    IP_Logf_Application("Found the following WiFi networks (max. %u results):", SEGGER_COUNTOF(_aNetwork));
    for (u = 0; u < _NumNetworks; u++) {
      IP_Logf_Application("#%u - Channel #%d, SSID: %s, Security: %s, BSSID: %2x:%2x:%2x:%2x:%2x:%2x, RSSI: %d", u, _aNetwork[u].Channel, _aNetwork[u].acSSID[0] ? _aNetwork[u].acSSID:"INVISIBLE", IP_WIFI_Security2String(_aNetwork[u].Security), _aNetwork[u].abBSSID[0], _aNetwork[u].abBSSID[1], _aNetwork[u].abBSSID[2], _aNetwork[u].abBSSID[3], _aNetwork[u].abBSSID[4], _aNetwork[u].abBSSID[5], _aNetwork[u].Rssi);
      OS_Delay(10);  // Avoid debug output terminal buffers running full.
    }
  } else {
    IP_Logf_Application("No networks found.");
  }
  OS_Delay(5000);
  if (_NumNetworks != 0xFFFFFFFF) {  // Will not happen naturally but prevents "statement is unreachable" warning of following code
    goto ScanAgain;
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
