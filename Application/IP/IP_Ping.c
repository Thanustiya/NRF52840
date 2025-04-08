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

Purpose : Sample program for embOS & emNet
          Demonstrates use of the IP stack to PING a host and get
          notified about incoming ICMP (PING) packets.
Notes   : For compatibility with interfaces that need to connect in
          any way this sample calls connect and disconnect routines
          that might not be needed in all cases.

          This sample can be used for Ethernet and dial-up interfaces
          and is configured to use the last registered interface as
          its main interface.
Example output:
          .
          .
          .
          5:275 MainTask - Sending PING to IP addr. 192.168.11.151 .
          5:275 IP_Task - ICMP echo reply received!
          5:475 MainTask - Sending PING to IP addr. 192.168.11.151 .
          5:475 IP_Task - ICMP echo reply received!
          <PC sends a ping to the device>
          5:481 IP_Task - ICMP echo request received!
          5:675 MainTask - Sending PING to IP addr. 192.168.11.151 .
          5:675 IP_Task - ICMP echo reply received!
          5:875 MainTask - Sending PING to IP addr. 192.168.11.151 .
          5:875 IP_Task - ICMP echo reply received!
          .
          .
          .
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

#define USE_RX_TASK   0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// PING sample.
//
#define HOST_TO_PING  "192.168.5.1" // IP addr. of host that will be periodically PINGed by this sample.

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
*       Static const
*
**********************************************************************
*/

static const char *_ICMPContent = "This is a PING";

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

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
*       _OnRxICMP()
*
* Function description
*   Callback that will be notified once an ICMP packet has been
*   received. The packet is evaluated and the type of the message
*   will be output.
*
* Parameters
*   pPacket: Pointer to packet received. pData points to the IP header.
*
* Return value
*   0    : Packet will be processed by the stack.
*   Other: Packet will be freed     by the stack.
*/
static int _OnRxICMP(IP_PACKET* pPacket) {
  const U8* pData;

  pData = IP_GetIPPacketInfo(pPacket);
  if(*pData == IP_ICMP_TYPE_ECHO_REQUEST) {
    IP_Logf_Application("ICMP echo request received!");
  }
  if(*pData == IP_ICMP_TYPE_ECHO_REPLY) {
    IP_Logf_Application("ICMP echo reply received!");
  }
  return 0;
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
  U16      SequenceNumber;
  IP_ADDR  TargetAddr;
  char     ac[16];
  int      r;

  SequenceNumber = 0;

  IP_Init();
  IP_AddLogFilter(IP_MTYPE_APPLICATION);
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_ICMP_SetRxHook(_OnRxICMP);                                                        // Register hook to be notified of incoming ICMP packets.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }

  r = IP_IPV4_ParseIPv4Addr(HOST_TO_PING, &TargetAddr);
  if (r < 0) {
    IP_PANIC("Illegal IP Address.");
  }
  IP_PrintIPAddr(ac, htonl(TargetAddr), sizeof(ac));

  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
    IP_Logf_Application("Sending PING to IP addr. %s .", ac);
    IP_SendPingEx(_IFaceId, TargetAddr, (char*)_ICMPContent, IP_STRLEN(_ICMPContent), SequenceNumber++);
  }
}

/*************************** End of file ****************************/
