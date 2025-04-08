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
          Demonstrates a simple Telnet server on a port
          (by default: 23) that outputs
          the current system time for each character received.
          To connect to the target, use the command line:
          > telnet <target-ip>
          Where <target-ip> represents the IP address of the target,
          which depends on the configuration used in IP_X_Config() .
          The recv() function waits for a given timeout for further
          characters to receive before it disconnects the client due
          to idle timeout.

          The following is a sample of the output to the terminal window:

            MainTask - INIT: Init started.
            MainTask - DRIVER: Found PHY with Id 0x2000 at addr 0x1
            MainTask - INIT: Link is down
            MainTask - INIT: Init completed
            IP_Task - INIT: IP_Task started
            IP_Task - LINK: Link state changed: Full duplex, 100MHz
            IP_Task - NDP: Link-local IPv6 addr.: FE80:0000:0000:0000:0222:C7FF:FEFF:FF88
            IP_Task - DHCPc: Sending discover!
            IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.231, Mask: 255.255.0.0, GW: 192.168.13.1.
            IP_Task - DHCPc: IP addr. checked, no conflicts
            IP_Task - DHCPc: Sending Request.
            IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.231, Mask: 255.255.0.0, GW: 192.168.13.1.
            Telnet - New IPv4 client accepted.
            Telnet - recv() timeout after 5 seconds of inactivity!
            Telnet - Disconnecting client.
            Telnet - New IPv6 client accepted.
            Telnet - recv() timeout after 5 seconds of inactivity!
            Telnet - Disconnecting client.


Notes:
  (1) The following requirements need to be met when including this
      sample into another C-module:

        - BSP_Init() for LED routines and IP_Init() for emNet are
          called before starting APP_MainTask() .

        - The optional IP_Connect() is called and a link state hook
          is installed if necessary for the emNet interface used
          before starting APP_MainTask() .

  (2) The following application defines can be overwritten when
      including this sample into another C-module:

        - APP_MAIN_STACK_SIZE
          Stack size in bytes to use for APP_MainTask() .

        - APP_TASK_STACK_OVERHEAD
          Additional task stack size [multiples of sizeof(int)] if some
          underlying processes require more task stack than default.

        - APP_ENABLE_IPV4_PLAIN
          Provide a plain/unsecured IPv4 Telnet like server.

        - APP_ENABLE_IPV6_PLAIN
          Provide a plain/unsecured IPv6 Telnet like server.

  (3) The following symbols can be used and renamed via preprocessor
      if necessary when including this sample into another C-module:

        - MainTask
          Main starting point of the sample when used as a
          standalone sample. Can be renamed to anything else to
          avoid multiple MainTask symbols and to skip common
          initializing steps done in every sample.

        - APP_MainTask
          Functional starting point of the sample itself. Typically
          called by the MainTask in this sample. Should be renamed
          via preprocessor to a more application specific name to
          avoid having multiple APP_MainTask symbols in linker map
          files for a better overview when including multiple samples
          this way.

        - APP_MainTCB
          Task-Control-Block used for the APP_MainTask when started
          as a task from the MainTask in this sample. Can/should be
          renamed via preprocessor to a more application specific name.

        - APP_MainStack
          Task stack used for the APP_MainTask when started as a task
          from the MainTask in this sample. Can/should be renamed via
          preprocessor to a more application specific name.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "TaskPrio.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   USE_RX_TASK
  #define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.
#endif

//
// Sample features enable/disable
//
#ifndef   IP_SUPPORT_IPV4
  #define IP_SUPPORT_IPV4            (1)                // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif
#ifndef   IP_SUPPORT_IPV6
  #define IP_SUPPORT_IPV6            (0)                // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif

#ifndef   APP_ENABLE_IPV4_PLAIN
  #define APP_ENABLE_IPV4_PLAIN      (IP_SUPPORT_IPV4)  // Enables/disables the creation and usage of IPv4 sockets in this sample.
#endif
#ifndef   APP_ENABLE_IPV6_PLAIN
  #define APP_ENABLE_IPV6_PLAIN      (IP_SUPPORT_IPV6)  // Enables/disables the creation and usage of IPv6 sockets in this sample.
#endif

//
// Telnet server sample.
//
#define TIMEOUT      5000  // Timeout for recv() [ms].
#define SERVER_PORT  23

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_MAIN_STACK_SIZE
  #define APP_MAIN_STACK_SIZE      (1024)
#endif
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD  (0)
#endif
#ifndef   STACK_SIZE_SERVER
  #define STACK_SIZE_SERVER        (APP_MAIN_STACK_SIZE + APP_TASK_STACK_OVERHEAD)
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
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static char                    _acOut[512];

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int APP_MainStack[STACK_SIZE_SERVER / sizeof(int)];      // Stack of the starting point of this sample.
static OS_TASK         APP_MainTCB;                                         // Task-Control-Block of the IP_Task.

static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
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

  (void)IPProtVer;

  hSock = SOCKET_ERROR;

#if (APP_ENABLE_IPV4_PLAIN != 0)
  //
  // Create IPv4 socket
  //
  if (IPProtVer == AF_INET) {
    hSock = socket(AF_INET, SOCK_STREAM, 0);
  }
#endif
#if (APP_ENABLE_IPV6_PLAIN != 0)
  //
  // Create IPv6 socket
  //
  if (IPProtVer == PF_INET6) {
    hSock = socket(AF_INET6, SOCK_STREAM, 0);
  }
#endif
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
*   IPProtVer - Protocol family which should be used (PF_INET or PF_INET6).
*   hSock     - Socket handle
*   Port      - Port which should be to wait for connections.
*
* Return value
*   O.K. : == 0
*   Error: != 0
*/
static int _BindAtTcpPort(unsigned IPProtVer, int hSock, U16 LPort) {
  int r;

  (void)IPProtVer;
  (void)hSock;
  (void)LPort;

  r = -1;

  //
  // Bind it to the port
  //
#if (APP_ENABLE_IPV4_PLAIN != 0)
  if (IPProtVer == AF_INET) {
    struct sockaddr_in Addr;

    IP_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(LPort);
    Addr.sin_addr.s_addr = INADDR_ANY;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
#endif
#if (APP_ENABLE_IPV6_PLAIN != 0)
  if (IPProtVer == PF_INET6) {
    struct sockaddr_in6 Addr;

    IP_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin6_family      = AF_INET6;
    Addr.sin6_port        = htons(LPort);
    Addr.sin6_flowinfo   = 0;
    IP_MEMSET(&Addr.sin6_addr, 0, 16);
    Addr.sin6_scope_id   = 0;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
#endif
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
*       _Process()
*
* Function description
*   This is the actual (very simple) Telnet server. This function
*   processes one client connection.
*
* Parameters
*   hSock: Client socket to serve.
*/
static void _Process(int hSock) {
  const char *sHello = "Hello ... Press any key.\n\r";
        U32   Timeout;
        int   Error;
        int   r;
        char  Dummy;  // Used to receive one character, key press.


  r = send(hSock, (char*)sHello, IP_STRLEN(sHello), 0);
  if (r == SOCKET_ERROR) {
    IP_Logf_Application("send() failed with socket error.");
    return;
  }
  //
  // recv() by default is a blocking function. this means it blocks
  // until it receives data. We set a timeout to guarantee that the
  // function returns after the given delay.
  //
  Timeout = TIMEOUT;
  setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));
  do {
    r = recv(hSock, &Dummy, 1, 0);
    if (r <= 0) {
      getsockopt(hSock, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
      if (Error == IP_ERR_TIMEDOUT) {
        IP_Logf_Application("recv() timeout after %lu seconds of inactivity!", (TIMEOUT / 1000));
      } else {
        IP_Logf_Application("recv() failed with error: %s", IP_Err2Str(Error));
      }
      IP_Logf_Application("Disconnecting client.");
      return;  // Error, receive timeout => disconnect client.
    }
    SEGGER_snprintf(_acOut, sizeof(_acOut), "OS_Time = %lu\n\r", OS_Time);
    r = send(hSock, (char*)_acOut, IP_STRLEN(_acOut), 0);
    if (r == SOCKET_ERROR) {
      IP_Logf_Application("send() failed with socket error.");
      return;
    }
  } while (1);
}

/*********************************************************************
*
*       _TelnetTask()
*
* Function description
*   Creates a parent socket and handles clients that connect to the
*   server. This sample can handle one client at the same time. Each
*   client that connects creates a child socket that is then passed
*   to the process routine to detach client handling from the server
*   functionality. This could also be used for a multi-task setup.
*/
static void _TelnetTask(void) {
  IP_fd_set ReadFds;
#if APP_ENABLE_IPV4_PLAIN
  int       hSockParent4_Plain;
#endif
#if APP_ENABLE_IPV6_PLAIN
  int       hSockParent6_Plain;
#endif
  int       hSock;
  int       r;

  hSock = 0;  // Avoid uninitialized warning.
  //
  // Try until we get valid parent sockets.
  //
#if APP_ENABLE_IPV4_PLAIN
  while (1) {
    hSockParent4_Plain = _ListenAtTcpPort(PF_INET, SERVER_PORT);
    if (hSockParent4_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV6_PLAIN
  while (1) {
    hSockParent6_Plain = _ListenAtTcpPort(PF_INET6, SERVER_PORT);
    if (hSockParent6_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
  //
  // Wait for a connection and process the data request after accepting the connection.
  //
  for (;;) {
    IP_FD_ZERO(&ReadFds);                     // Clear the set
#if APP_ENABLE_IPV4_PLAIN
    IP_FD_SET(hSockParent4_Plain, &ReadFds);  // Add IPv4 plain socket to the set
#endif
#if APP_ENABLE_IPV6_PLAIN
    IP_FD_SET(hSockParent6_Plain, &ReadFds);  // Add IPv6 plain socket to the set
#endif
    r = select(&ReadFds, NULL, NULL, 5000);   // Check for activity. Wait 5 seconds
    if (r <= 0) {
      continue;
    }
#if APP_ENABLE_IPV4_PLAIN
    //
    // Check if the IPv4 plain socket is ready
    //
    if (IP_FD_ISSET(hSockParent4_Plain, &ReadFds)) {
      hSock = accept(hSockParent4_Plain, NULL, NULL);
      if (hSock == SOCKET_ERROR) {
        continue;               // Error, try again.
      }
      IP_Logf_Application("New IPv4 client accepted.");
      goto HandleConnection;
    }
#endif
#if APP_ENABLE_IPV6_PLAIN
    //
    // Check if the IPv6 plain socket is ready
    //
    if (IP_FD_ISSET(hSockParent6_Plain, &ReadFds)) {
      hSock = accept(hSockParent6_Plain, NULL, NULL);
      if (hSock == SOCKET_ERROR) {
        continue;               // Error, try again.
      }
      IP_Logf_Application("New IPv6 client accepted.");
      goto HandleConnection;
    }
#endif
HandleConnection:
    _Process(hSock);     // Process the client.
    closesocket(hSock);  // Close connection to client from our side (too).
  }
}

/*********************************************************************
*
*       APP_MainTask()
*
*  Function description
*    Sample starting point.
*/
static void APP_MainTask(void) {
  OS_SetTaskName(OS_GetTaskID(), "Telnet server");
  _TelnetTask();
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
*  Function description
*    Main sample starting point when running one individual sample.
*
*  Additional information
*    Runs the sample in a task of its own to decouple it from not
*    knowing the required task stack size of this sample when
*    starting from main() . This task typically is terminated once
*    it has fulfilled its purpose.
*
*    This routine initializes everything that might be common with
*    other samples of the same kind. These initializations can then
*    be skipped by not starting from MainTask() but APP_MainTask()
*    instead.
*/
void MainTask(void) {
  int IFaceId;

  //
  // Initialize the IP stack
  //
  IP_Init();
  //
  // Start TCP/IP task
  //
  OS_CREATETASK(&_IPTCB,   "IP_Task",   IP_Task,   APP_TASK_PRIO_IP_TASK  , _IPStack);
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, APP_TASK_PRIO_IP_RXTASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                               // Register hook to be notified on disconnects.
  IFaceId = IP_INFO_GetNumInterfaces() - 1;                                               // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  IP_Connect(IFaceId);                                                                    // Connect the interface if necessary.
  //
  // IPv4 address configured ?
  //
  while (IP_IFaceIsReadyEx(IFaceId) == 0) {
    BSP_ToggleLED(0);
    OS_Delay(200);
  }
  BSP_ClrLED(0);
  //
  // Start the sample itself.
  //
  OS_CREATETASK(&APP_MainTCB, "APP_MainTask", APP_MainTask, APP_TASK_PRIO_WEBSERVER_PARENT, APP_MainStack);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
