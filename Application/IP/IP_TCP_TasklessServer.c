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
          Demonstrates accepting a TCP client as server without using
          a parent task by using Taskless Accept.
          A new client gets accepted in the background and a registered
          callback gets notified about the new connection. The callback
          can either already send back small portions of data or dispatch
          the new child socket to a child task as it is done in a traditional
          parent (accepts new connections) and client (processes the connection)
          task behavior.
          To connect to the target, use the command line:
          > telnet <target-ip>
          Where <target-ip> represents the IP address of the target,
          which depends on the configuration used in IP_X_Config() .
          The callback sends back a small greeting and closes the connection.
          Instead of closing the connection, the new connection can also be
          dispatched to another location like a child task.

          The following is a sample of the output to the terminal window:

          0:000 MainTask - INIT: Init started. Version 3.20
          0:002 MainTask - DRIVER: Found PHY with Id 0x181 at addr 0x1F
          0:003 MainTask - INIT: Link is down
          0:003 MainTask - INIT: Init completed
          0:003 IP_Task - INIT: IP_Task started
          3:000 IP_Task - LINK: Link state changed: Full duplex, 100 MHz
          4:000 IP_Task - DHCPc: Sending discover!
          4:516 IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          5:000 IP_Task - DHCPc: IP addr. checked, no conflicts
          5:000 IP_Task - DHCPc: Sending Request.
          5:001 IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          9:554 IP_Task - New connection accepted for server on local port 23.

Additional information:
  For compatibility with interfaces that need to connect in any way
  this sample calls connect and disconnect routines that might not be
  needed in all cases.

  This sample can be used for Ethernet and dial-up interfaces and is
  configured to use the last registered interface as its main interface.
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
// Telnet server sample.
//
#define SERVER_PORT  23

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150    // Priority should be higher than all IP application tasks.
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
static IP_TCP_ACCEPT_HOOK      _AcceptHook;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[(TASK_STACK_SIZE_IP_TASK + APP_TASK_STACK_OVERHEAD + 512)/sizeof(int)];  // Stack of the IP_Task.
                                                                                                         // Unlike in other IP examples, the taskless accept callback runs
                                                                                                         // from down below the IP_Task() up to application level and the
                                                                                                         // send() call in the application runs back down through all layers
                                                                                                         // of TCP and as low as into the driver. Therefore in this sample
                                                                                                         // the IP_Task() might need additional task stack, depending on
                                                                                                         // what is done from within the callback.
static OS_TASK         _IPTCB;                                                                           // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];                               // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                                         // Task-Control-Block of the IP_RxTask.
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
*       _CreateSocket()
*
* Function description
*   Creates a socket for the requested protocol family.
*
* Parameter
*   IPProtVer - Protocol family which should be used (PF_INET or PF_INET6).
*
* Return value
*   O.K. : Socket handle.
*   Error: SOCKET_ERROR .
*/
static int _CreateSocket(unsigned IPProtVer) {
  int hSock;

  hSock = SOCKET_ERROR;
  //
  // Create IPv4 socket
  //
  if (IPProtVer == AF_INET) {
    hSock = socket(AF_INET, SOCK_STREAM, 0);
  }
  return hSock;
}

/*********************************************************************
*
*       _BindAtTcpPort()
*
* Function description
*   Binds a socket to a port.
*
* Parameter
*   IPProtVer - Protocol family which should be used (AF_INET).
*   hSock     - Socket handle
*   Port      - Port which should be to wait for connections.
*
* Return value
*   O.K. : == 0
*   Error: != 0
*/
static int _BindAtTcpPort(unsigned IPProtVer, int hSock, U16 LPort) {
  int r;

  r = -1;
  //
  // Bind it to the port
  //
  if (IPProtVer == AF_INET) {
    struct sockaddr_in Addr;

    IP_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(LPort);
    Addr.sin_addr.s_addr = INADDR_ANY;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
  return r;
}

/*********************************************************************
*
*       _ListenAtTcpPort()
*
* Function description
*   Creates a socket, binds it to a port and sets the socket into
*   listening state.
*
* Parameter
*   IPProtVer - Protocol family which should be used (PF_INET or PF_INET6).
*   Port      - Port which should be to wait for connections.
*
* Return value
*   O.K. : Socket handle.
*   Error: SOCKET_ERROR .
*/
static int _ListenAtTcpPort(unsigned IPProtVer, U16 Port) {
  int hSock;
  int r;

  //
  // Create socket
  //
  hSock = _CreateSocket(IPProtVer);
  if (hSock != SOCKET_ERROR) {
    //
    // Bind it to the port
    //
    r = _BindAtTcpPort(IPProtVer, hSock, Port);
    //
    // Start listening on the socket.
    //
    if (r != 0) {
      hSock = SOCKET_ERROR;
    } else {
      r = listen(hSock, 1);
      if (r != 0) {
        hSock = SOCKET_ERROR;
      }
    }
  }
  return hSock;
}

/*********************************************************************
*
*       _cbAccept()
*
*  Function description
*    Accepts a new connection without using a parent task by using
*    taskless accept.
*
*  Parameters
*    hSock   : Accepted child socket.
*    pInfo   : Connection information.
*    pContext: Custom context as registered with the accept hook.
*
*  Additional information
*    The callback has to prevent any blocking situation.
*    Calling send() for example on a non-blocking socket is fine.
*/
static void _cbAccept(int hSock, IP_TCP_ACCEPT_INFO* pInfo, void* pContext) {
  static const char _acHelloBye[] = "Hello and goodbye...";

  IP_USE_PARA(pContext);

  IP_Logf_Application("New connection accepted for server on local port %lu.", pInfo->LPort);
  setsockopt(hSock, SOL_SOCKET, SO_NBIO, NULL, 0);                  // Set socket non-blocking as we are not allowed to risk a blocking situation in this callback.
  send(hSock, &_acHelloBye[0], SEGGER_strlen(&_acHelloBye[0]), 0);  // Send MOTD.
  closesocket(hSock);                                               // Close connection.
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
  int hSockParent4;

  IP_Init();
  IP_AddLogFilter(IP_MTYPE_APPLICATION);
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                                 // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                         // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB    , "IP_Task"  , IP_Task    , TASK_PRIO_IP_TASK   , _IPStack);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB  , "IP_RxTask", IP_RxTask  , TASK_PRIO_IP_RX_TASK, _IPRxStack);    // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                  // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                      // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                       // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  //
  // Create simple taskless accept telnet server.
  //
  hSockParent4 = _ListenAtTcpPort(PF_INET, SERVER_PORT);
  IP_TCP_Accept(&_AcceptHook, _cbAccept, hSockParent4, NULL);
  if (hSockParent4 == SOCKET_ERROR) {
    while (1) {
      BSP_ToggleLED(1);
      OS_Delay(2000);
    }
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
