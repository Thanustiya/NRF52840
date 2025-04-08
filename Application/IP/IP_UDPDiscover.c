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
          target(s) which are available in the subnet.
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
   TASK_PRIO_IP_DISCOVER = 150
  ,TASK_PRIO_IP_TASK            // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK         // Must be the highest priority of all IP related tasks.
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

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                               // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                                      // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];                          // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                                    // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _DiscoverStack[(1024 + PACKET_SIZE + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Stack of the discover server.
static OS_TASK         _DiscoverTCB;                                                                // Task-Control-Block of the discover server.

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
*       _DiscoverTask()
*
* Function description
*   Server task that receives incoming discover packets on a specific
*   UDP port and sends back an answer to identify itself.
*/
static void _DiscoverTask(void) {
  struct sockaddr_in Addr;
  char acBuffer[256];
  int  hSock;
  int  AddrLen;
  int  NumBytes;
  U32  IPAddr;
  int Error;
  int r;

  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  //
  // Loop until we get a socket.
  //
  do {
    hSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (hSock != SOCKET_ERROR) {
      //
      // Bind socket to UDP port.
      //
      IP_MEMSET(&Addr, 0, sizeof(Addr));
      Addr.sin_family      = AF_INET;
      Addr.sin_port        = htons(PORT);
      Addr.sin_addr.s_addr = INADDR_ANY;
      bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
      break;
    }
    OS_Delay(100);  // Try again after delay.
  } while (1);
  //
  // Wait for incoming UDP discover packets.
  //
  while (1) {
    AddrLen = sizeof(Addr);
    NumBytes = recvfrom(hSock, &acBuffer[0], sizeof(acBuffer), 0, (struct sockaddr*)&Addr, &AddrLen);  // Senders addr. and port will be stored in Addr.
    if (NumBytes > 0) {
      if (IP_MEMCMP(&acBuffer[0], "Discover target", 16) == 0) {
        //
        // Create answer packet, containing IPAddr, MacAddr, S/N, Name
        //
        IPAddr = htonl(IP_GetIPAddr(_IFaceId));             // There is no way to find out on which interface the data was received. Use the last registered interface.
        IP_MEMSET(acBuffer, 0, sizeof(acBuffer));              // Make sure all fieds are cleared.
        IP_STRCPY(acBuffer, "Found");                          // Offset 0x00: Answer string for a discover response.
        IP_MEMCPY((void*)&acBuffer[0x20], (void*)&IPAddr, 4);  // Offset 0x20: IP addr.
        IP_GetHWAddr(0, (U8*)&acBuffer[0x30], 6);           // Offset 0x30: MAC addr.
        IP_MEMCPY((void*)&acBuffer[0x40], "12345678", 8);      // Offset 0x40: S/N.
        IP_STRCPY(&acBuffer[0x50], "MyTarget");                // Offset 0x50: Product name.
        //
        // Send answer.
        //
        r = sendto(hSock, &acBuffer[0], sizeof(acBuffer), 0, (struct sockaddr*)&Addr, sizeof(Addr));

        if (r == 0) {
          IP_Logf_Application("sendto() 0 bytes sent!");
        } else if(r == SOCKET_ERROR) {
          getsockopt(hSock, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
          IP_Logf_Application("sendto() failed with error: %s", IP_Err2Str(Error));
        }
      }
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_DiscoverTCB, "Discover", _DiscoverTask, TASK_PRIO_IP_DISCOVER   , _DiscoverStack);  // Start the discover client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
