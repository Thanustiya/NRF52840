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
----------------------------------------------------------------------
Purpose : Sample program for embOS & TCP/IP with AutoIP
          Demonstrates use of the IP stack to automatically assign an
          IP address using AutoIP negotiation.

          The sample will reset a configuration received via DHCP client
          if any and will try to negotiate its own IP address with other
          participants in the network. A preferred IP address out of the
          AutoIP pool can be chosen via the define AUTO_IP_PREFERRED_IP .
          The AutoIP module will then try to use the preferred IP address.
          If the preferred IP addr. is already in use and is detected to
          cause an addr. conflict the AutoIP module will negotiate a new
          IP address.
          The following is a sample of the output to the terminal window
          when using a preferred IP addr. that is already in use:

          [Interface no. 0] AutoIP status: Start with preset IP addr.
          [Interface no. 0] AutoIP status: Start of IP addr. conflict check
          [Interface no. 0] AutoIP status: Conflict detected, retry with other IP
          [Interface no. 0] AutoIP status: Start of IP addr. conflict check
          [Interface no. 0] AutoIP status: End of IP addr. conflict check
          [Interface no. 0] AutoIP status: Using IP addr. 169.254.136.19

          To fully test the AutoIP module, configure a PC in your network
          to use the preferred IP address. The AutoIP module will then try
          to use the same IP addr. but will recognize it as being in use and
          will try another generated IP addr. for itself.
--------- END-OF-HEADER --------------------------------------------*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK          0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// AutoIP sample.
//
#define AUTOIP_PREFERRED_IP  "169.254.2.5"                  // Try to use this IP addr. before anything else. If in use
                                                            // by another client we will need to negotiate another IP addr.

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

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;
static U8                      _ConflictCheckActive;

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
*       Static code
*
**********************************************************************
*/

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
*       _ResetIPConfig
*
*  Function description
*    Stops the DHCP client if running and sets the IP address, the
*    subnet mask and the gateway to zero.
*/
static void _ResetIPConfig(U8 IFaceId) {
  int r;

  //
  // Check if DHCP client is running and stop it if necessary.
  //
  r = IP_DHCPC_GetState(IFaceId);
  if (r > 0) {
    IP_DHCPC_Halt(IFaceId);
  }
  //
  // Set the IP address, address mask and gateway to zero.
  //
  IP_SetAddrMaskEx(IFaceId, 0, 0);
  IP_SetGWAddr    (IFaceId, 0);
}

/*********************************************************************
*
*       _OnStatusChange
*
*  Function description
*    Callback function to inform the user about a status change of the
*    AutoIP module.
*
*  Parameter
*    pIFaceId - [IN] Interface ID
*    pStatus  - [IN] Current state of AutoIP
*/
static void _OnStatusChange(U32 IFace, U32 Status) {
  U32  IPAddr;
  char ac[16];

  if (Status == AUTOIP_STATE_UNUSED) {
    _ConflictCheckActive = 0;
    IP_Logf_Application("[Interface no. %lu] AutoIP status: Disabled", IFace);
  }
  if (Status == AUTOIP_STATE_INITREBOOT) {
    IP_Logf_Application("[Interface no. %lu] AutoIP status: Start with preset IP addr.", IFace);
  }
  if (Status == AUTOIP_STATE_INIT) {
    if (_ConflictCheckActive) {
      IP_Logf_Application("[Interface no. %lu] AutoIP status: Conflict detected, retry with other IP", IFace);
    } else {
      IP_Logf_Application("[Interface no. %lu] AutoIP status: Start", IFace);
    }
  }
  if (Status == AUTOIP_STATE_SENDPROBE) {
    _ConflictCheckActive = 1;
    IP_Logf_Application("[Interface no. %lu] AutoIP status: Start of IP addr. conflict check", IFace);
  }
  if (Status == AUTOIP_STATE_CHECKREPLY) {
    _ConflictCheckActive = 0;
    IP_Logf_Application("[Interface no. %lu] AutoIP status: End of IP addr. conflict check", IFace);
  }
  if (Status == AUTOIP_STATE_BOUND) {
    _ConflictCheckActive = 0;
    IPAddr = IP_GetIPAddr(IFace);
    IP_PrintIPAddr(ac, IPAddr, sizeof(ac));
    IP_Logf_Application("[Interface no. %lu] AutoIP status: Using IP addr. %s", IFace, ac);
  }
}

/*********************************************************************
*
*       MainTask
*/
void MainTask(void) {
  IP_ADDR TargetAddr;
  int r;

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  //
  // Reset the configuration that might have been acquired via DHCPc
  //
  _ResetIPConfig(_IFaceId);
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);               // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                   // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                    // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  //
  // Enhance log filter to output AutoIP related status messages in debug builds
  //
//  IP_AddLogFilter(IP_MTYPE_AUTOIP);
  //
  // Enable AutoIP
  //
  IP_AutoIP_SetUserCallback(_IFaceId, _OnStatusChange);
#ifdef AUTOIP_PREFERRED_IP
  r = IP_IPV4_ParseIPv4Addr(AUTOIP_PREFERRED_IP, &TargetAddr);
  if (r < 0) {
    IP_PANIC("Illegal IP Address.");
  }
  IP_AutoIP_SetStartIP(_IFaceId, htonl(TargetAddr));
#endif
  IP_AutoIP_Activate(_IFaceId);
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(100);
  }
  //
  // Application
  //
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay (200);
  }
}

/*************************** End of file ****************************/
