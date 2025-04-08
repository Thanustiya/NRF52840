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
          Demonstrates setup of a simple UDP application which waits
          for messages on a UDP port and answers if a discover packet
          has been received. The related host application,
          found at "/Windows/IP/UDPDiscoverGUI", sends UDP
          broadcasts to the UDP port and waits for an answer from the
          target(s) which are available in the subnet. The sample
          uses the zero-copy API that on the one hand is able to
          handle incoming requests without a dedicated task and on
          the other hand is able to directly access and fill the
          packets data part to avoid unnecessary copying of the data
          within the stack.
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

#define USE_RX_TASK   0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// UDP discover sample.
//
#define PORT         50020
#define PACKET_SIZE  256


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
*       _OnRx()
*
* Function description
*   Discover client UDP callback. Called from stack
*   whenever we get a discover request.
*
* Parameters
*   pInPacket: Pointer to incoming packet.
*   pContext : Context set during callback registration.
*
* Return value
*   IP_RX_ERROR: Packet content is invalid for some reason.
*   IP_OK      : Packet content is valid.
*
* Notes
*   (1) Freeing pInPacket:
*       With either return value, the IN-packet is freed by the stack
*       and therefore can not be re-used nor has it to be freed by
*       this routine.
*/
static int _OnRx(IP_PACKET *pInPacket, void *pContext) {
  unsigned    IFaceId;
  U32         IPAddr;
  U32         TargetAddr;
  char      *pOutData;
  char      *pInData;
  IP_PACKET *pOutPacket;

  IP_USE_PARA(pContext);

  IFaceId  = IP_UDP_GetIFIndex(pInPacket);            // Find out the interface that the packet came in.
  pInData  = (char*)IP_UDP_GetDataPtr(pInPacket);     // Get the pointer to the UDP payload.
  if (IP_MEMCMP(pInData, "Discover target", 16) == 0) {  // Check if this is a known discover packet.
    //
    // Alloc packet for sending back an answer.
    //
    pOutPacket = IP_UDP_Alloc(PACKET_SIZE);
    if (pOutPacket) {
      pOutData = (char*)IP_UDP_GetDataPtr(pOutPacket);
      IP_UDP_GetSrcAddr(pInPacket, &TargetAddr, sizeof(TargetAddr));  // Send back as unicast.
//      TargetAddr = 0xFFFFFFFF;                                        // Send back as broadcast.
      //
      // Fill packet with data, containing IPAddr, HWAddr, S/N and name.
      //
      IPAddr = htonl(IP_GetIPAddr(IFaceId));
      IP_MEMSET(pOutData, 0, PACKET_SIZE);                // Make sure all fieds are cleared.
      IP_STRCPY(pOutData + 0x00, "Found");                // Offset 0x00: Answer string for a discover response.
      IP_MEMCPY(pOutData + 0x20, (void*)&IPAddr, 4);      // Offset 0x20: IP addr.
      IP_GetHWAddr(IFaceId, (U8*)pOutData + 0x30, 6);  // Offset 0x30: MAC addr.
      IP_MEMCPY(pOutData + 0x40, "12345678", 8);          // Offset 0x40: S/N.
      IP_STRCPY(pOutData + 0x50, "MyTarget");             // Offset 0x50: Product name.
      //
      // Send packet
      //
      IP_UDP_SendAndFree(IFaceId, TargetAddr, PORT, PORT, pOutPacket);
    }
  }
  return IP_OK;
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_UDP_Open(0uL /* any foreign host */, PORT, PORT, _OnRx, NULL);                    // Register UDP discover callback.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
