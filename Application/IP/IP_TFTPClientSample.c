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

Purpose : TFTP client sample program for embOS & emNet

Additional information:
  This sample demonstrates the use of the emNet TFTP client
  which can be used to access files on a TFTP server.

  Preparations:
    The following defines need to be configured:
      SERVER_IP_ADDR:  Server IPv4 address to connect to.
      SERVER_PORT:     Port that the server uses.
    Additionally you can define the following:
      FILE_NAME:       The file that should be retrieved/sent.

Example output:
  ...
  0:001 MainTask - INIT: Init started. Version 2.13.11
  0:003 MainTask - DRIVER: Found PHY with Id 0x181 at addr 0x1F
  0:004 MainTask - INIT: Link is down
  0:004 MainTask - INIT: Init completed
  0:004 IP_Task - INIT: IP_Task started
  3:000 IP_Task - LINK: Link state changed: Full duplex, 100 MHz
  4:000 IP_Task - DHCPc: Sending discover!
  4:508 IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.63, Mask: 255.255.0.0, GW: 192.168.11.1.
  5:000 IP_Task - DHCPc: IP addr. checked, no conflicts
  5:000 IP_Task - DHCPc: Sending Request.
  5:001 IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.63, Mask: 255.255.0.0, GW: 192.168.11.1.
  6:044 TFTP client - TFTP: Send a request.
  6:044 TFTP client - TFTP: Using octet (binary) mode.
  6:060 TFTP client - TFTP: Send a request.
  6:060 TFTP client - TFTP: Using octet (binary) mode.
  6:171 TFTP client - Receive/Send O.K.
  7:180 TFTP client - TFTP: Send a request.
  7:180 TFTP client - TFTP: Using octet (binary) mode.
  7:196 TFTP client - TFTP: Send a request.
  7:196 TFTP client - TFTP: Using octet (binary) mode.
  7:305 TFTP client - Receive/Send O.K.
  ...

Notes   :
  For compatibility with interfaces that need to connect in
  any way this sample calls connect and disconnect routines
  that might not be needed in all cases.

  This sample can be used for Ethernet and dial-up interfaces
  and is configured to use the last registered interface as
  its main interface.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_FS.h"
#include "IP_TFTP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK     0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// TFTP client sample.
//
#define SERVER_IP_ADDR  "192.168.11.64"
#define SERVER_PORT     69
#define FILE_NAME       "text.txt"  // File to retrieve from server and send back to the server.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TFTP = 150
  ,TASK_PRIO_IP_TASK        // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD     0
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

static TFTP_CONTEXT            _Context;        // TFTP context holding settings and states.
static char                    _acBuffer[516];  // Largest possible TFTP packet size is 516 bytes. 512 bytes payload and 4 bytes TFTP header.

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _TFTPStack[2048/sizeof(int)];                        // Stack of the TFTP task.
static OS_TASK         _TFTPTCB;                                            // Task-Control-Block of the TFTP task.

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
*       _TFTPTask()
*
* Function description
*   DNS client task that resolves the configured host name to its
*   IP addr.
*/
static void _TFTPTask(void) {
  IP_ADDR TargetAddr;
  int     Status;
  int     r;

  r = IP_IPV4_ParseIPv4Addr(SERVER_IP_ADDR, &TargetAddr);
  if (r < 0) {
    IP_PANIC("Illegal IP Address.");
  }
  //
  // Wait until link is up and network interface is configured.
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  IP_TFTP_InitContext(&_Context, _IFaceId, &IP_FS_FS, _acBuffer, sizeof(_acBuffer), SERVER_PORT);  // Init the TFTP client.
  while (1) {
    OS_Delay(1000);
    //
    // Receive a file from a TFTP server.
    //
    Status = IP_TFTP_RecvFile(&_Context, _IFaceId, htonl(TargetAddr), SERVER_PORT, FILE_NAME, TFTP_MODE_OCTET);
    if (Status < 0) {
      IP_Warnf_Application("Receive failed.");
      continue;
    }
    //
    // Send a file to a TFTP server.
    //
    Status = IP_TFTP_SendFile(&_Context, _IFaceId, htonl(TargetAddr), SERVER_PORT, FILE_NAME, TFTP_MODE_OCTET);
    if (Status < 0) {
      IP_Warnf_Application("Send failed.");
    } else {
      IP_Logf_Application("Receive/Send O.K.");
    }
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
  IP_Init();
  IP_AddLogFilter(IP_MTYPE_APPLICATION);
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                             // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                     // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"    , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask"  , IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_TFTPTCB, "TFTP client", _TFTPTask, TASK_PRIO_IP_TFTP   , _TFTPStack);  // Start the DNS client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                              // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                  // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                   // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
