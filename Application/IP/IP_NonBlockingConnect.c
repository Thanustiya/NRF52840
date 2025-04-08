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
          Demonstrates connecting to a TCP server with a non-blocking
          socket used for connect() . The sample will try to connect
          and disconnect a server in a loop and reports about the
          success rate after each try. This sample can be used with
          the SpeedTestServer.exe Windows sample. The IP addr. of the
          server host has to be configured below.

          The following is a sample of the output to the terminal window:

          0:000 MainTask - INIT: Init started. Version 2.13.11
          0:002 MainTask - DRIVER: Found PHY with Id 0x181 at addr 0x1F
          0:003 MainTask - INIT: Link is down
          0:003 MainTask - INIT: Init completed
          0:003 IP_Task - INIT: IP_Task started
          3:000 IP_Task - LINK: Link state changed: Full duplex, 100 MHz
          4:000 IP_Task - DHCPc: Sending discover!
          4:506 IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          5:000 IP_Task - DHCPc: IP addr. checked, no conflicts
          5:000 IP_Task - DHCPc: Sending Request.
          5:001 IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          5:004 Client - Connecting using non-blocking socket...
          5:005 Client - Successfully connected after 1ms!
          5:505 Client - 1 of 1 tries were successful.

          7:505 Client - Connecting using non-blocking socket...
          7:506 Client - Successfully connected after 1ms!
          8:006 Client - 2 of 2 tries were successful.
          .
          .
          .
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

#define USE_RX_TASK     0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Client sample.
//
#define SERVER_IP_ADDR  "192.168.11.88"                 // IP addr., for example 192.168.5.1 .
#define SERVER_PORT     1234                            // Remote port the server listens on.
#define TIMEOUT         5000                            // Timeout for connect() [ms].

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_CLIENT = 150
  ,TASK_PRIO_IP_TASK          // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK       // Must be the highest priority of all IP related tasks.
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
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];               // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                      // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];          // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                    // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _ClientStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Stack of the client task.
static OS_TASK         _ClientTCB;                                                  // Task-Control-Block of the client task.

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
*       _ConnectWithTimeout()
*
* Function description
*   Tries to connect to the TCP server with a non-blocking socket.
*   The three way connection handshake typically is slower than the
*   application. Therefore it is likely that for a non-blocking
*   socket connect() returns before the handshake has been completed.
*   Therefore, you have to check the socket error to decide about
*   the state of the connecting process.
*
* Parameters
*   hSock : Socket handle to use for connect.
*   pAddr : Pointer to configured structure of type sockaddr_in .
*   pLoops: Pointer where to store the number of loops until
*           a successful connect has been done.
*   ms    : Connect timeout [ms].
*
* Return value
*   Connected: 0 .
*   Timeout  : 1 .
*   Error    : SOCKET_ERROR .
*/
static int _ConnectWithTimeout(int hSock, struct sockaddr_in *pAddr, unsigned* pLoops, U32 ms) {
  I32      Timeout;
  unsigned Loops;
  int      SoError;
  int      Status;
  int      r;

  r       = 0;  // Assume we can connect.
  Loops   = 1u;
  Timeout = (I32)(IP_OS_GET_TIME() + ms);
  for (;; Loops++) {
    Status = connect(hSock, (struct sockaddr*)pAddr, sizeof(struct sockaddr_in));
    if (Status == 0) {
      break;                // Successfully connected
    }
    getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    if (SoError == 0) {
      break;                // Successful late connect.
    }
    if (SoError != IP_ERR_IN_PROGRESS) {
      r = SOCKET_ERROR;     // Error, not in progress and not successful.
      IP_Logf_Application("Socket error: %s", IP_Err2Str(SoError));
      break;
    }
    if (IP_IsExpired(Timeout)) {
      r = 1;                // Error, timeout.
      break;
    }
    OS_Delay(1);            // Grant lower prior tasks some time.
  }
  *pLoops = Loops;
  return r;
}

/*********************************************************************
*
*       _ClientTask()
*
* Function description
*   Initializes a socket and tries to connect to a server with
*   timeout. Reports a summary of the number of successful and
*   total connection attempts and disconnect from the server.
*/
static void _ClientTask(void) {
  struct sockaddr_in Addr;
  int hSock;
  int Status;
  int SoError;
  int t0;
  int t1;
  int r;
  U32 ConnectionCnt;
  U32 SuccessCnt;
  IP_ADDR TargetAddr;
  unsigned Loops;

  //
  // Wait until link is up and interface is configured.
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  //
  // Initialize statistics.
  //
  ConnectionCnt = 0;
  SuccessCnt    = 0;
  //
  // Start connect() test loop.
  //
  while(1) {
    //
    // Open socket.
    //
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if (hSock <= 0) {
      IP_Logf_Application("Error opening socket.");
      getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
      IP_Logf_Application("Socket error: %s", IP_Err2Str(SoError));
      while (1) {
        BSP_ToggleLED(0);
        OS_Delay(20);
      }
    } else {
      //
      // Connect to server
      //
      Addr.sin_family      = AF_INET;
      Addr.sin_port        = htons(SERVER_PORT);
      r = IP_IPV4_ParseIPv4Addr(SERVER_IP_ADDR, &TargetAddr);
      if (r < 0) {
        IP_PANIC("Illegal IP Address.");
      }
      Addr.sin_addr.s_addr = TargetAddr;
      setsockopt(hSock, SOL_SOCKET, SO_NBIO, NULL, 0);   // Set socket non-blocking.
      IP_Logf_Application("Connecting using non-blocking socket...");
      ConnectionCnt++;
      t0 = IP_OS_GET_TIME();
      Status = _ConnectWithTimeout(hSock, &Addr, &Loops, TIMEOUT);
      t1 = IP_OS_GET_TIME() - t0;
      if        (Status == 0) {
        IP_Logf_Application("Succesfully connected after %dms and %u loops!", t1, Loops);
        setsockopt(hSock, SOL_SOCKET, SO_BIO, NULL, 0);  // Set socket blocking.
        BSP_SetLED(1);
        OS_Delay(500);
        BSP_ClrLED(1);
        SuccessCnt++;
      } else if (Status == SOCKET_ERROR) {
        IP_Logf_Application("connect() failed after %dms!", t1);
        getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
        IP_Logf_Application("Socket error: %s", IP_Err2Str(SoError));
      } else {
        IP_Logf_Application("connect() timeout.");
      }
    }
    IP_Logf_Application("%lu of %lu tries were successful.\n", SuccessCnt, ConnectionCnt);
    closesocket(hSock);
    OS_Delay(2000);
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                                 // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                         // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB    , "IP_Task"  , IP_Task    , TASK_PRIO_IP_TASK   , _IPStack);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB  , "IP_RxTask", IP_RxTask  , TASK_PRIO_IP_RX_TASK, _IPRxStack);    // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_ClientTCB, "Client"   , _ClientTask, TASK_PRIO_IP_CLIENT , _ClientStack);  // Start the client task.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                  // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                      // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                       // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
