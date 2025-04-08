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

Purpose : Demonstrates how to start the emWeb server in different configurations.

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

        - APP_AFTER_CONFIG
          If defined, a call to "APP_AFTER_CONFIG(void* p);" is executed
          AFTER the generic configuration of the main sample.

        - APP_UDP_DISCOVER_PORT
          UDP port to start a DISCOVER listen server on. Use 0 to
          not start the DISCOVER server.

        - APP_ENABLE_IPV4_PLAIN
          Provide a plain/unsecured IPv4 webserver.

        - APP_ENABLE_IPV4_SSL
          Provide a secured IPv4 webserver.

        - APP_ENABLE_IPV6_PLAIN
          Provide a plain/unsecured IPv6 webserver.

        - APP_ENABLE_IPV6_SSL
          Provide a secured IPv6 webserver.

        - APP_USE_SSL
          Helper switch that can be used instead of disabling
          APP_ENABLE_IPV4_SSL and APP_ENABLE_IPV6_SSL separately.

        - APP_ENABLE_CAPTIVE_PORTAL
          Provide a captive portal (HTTP generate_204 request).

        - APP_FULL_URI_BUF_SIZE
          Optional buffer size to reserve for keeping the full URI
          including GET parameters available for later usage in the
          application callbacks.

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

Additional information:
  The sample can easily be configured to start the emWeb server with
  different configurations and features such as IPv6 only and/or
  with/without SSL support.

  Preparations:
    Enable/disable the features that you want to use. The available
    binary configuration switches are:
      - APP_ENABLE_IPV4_PLAIN    : Enable creation of a plain   IPv4 socket on port  80 (default).
      - APP_ENABLE_IPV4_SSL      : Enable creation of a secured IPv4 socket on port 443 (default). Requires emSSL.
      - APP_ENABLE_IPV6_PLAIN    : Enable creation of a plain   IPv6 socket on port  80 (default). Requires IPv6.
      - APP_ENABLE_IPV6_SSL      : Enable creation of a secured IPv6 socket on port 443 (default). Requires IPv6 and emSSL.
      - APP_ENABLE_CAPTIVE_PORTAL: Enable creation of a (WiFi) captive portal (HTTP generate_204 request).


  Expected behavior:
    This sample starts a webserver that listens on the enabled ports
    and protocols and can be accessed via a web browser using the
    IP(v4/v6) addresses that are printed in the terminal output when
    starting.

    Alternatively you can use the UDPDiscover PC executable,
    found at "/Windows/IP/UDPDiscoverGUI", to find
    the IPv4 address of the target on the network (please allow the
    tool to send broadcasts if necessary with your firewall).

  Sample output:
    ...
    6:039 IP_WebServer - WEBS: Using a memory pool of 5120 bytes for 2 connections.
    ...
    <User opens the site in a browser>
    14:985 IP_WebServer - New IPv4 client accepted.
    14:985 Webserver Child - WebS: Get /
    15:014 Webserver Child - WebS: Get /Styles.css
    15:016 Webserver Child - WebS: Get /Logo.gif
    15:016 IP_WebServer - New IPv4 client accepted.
    15:031 Webserver Child - WebS: Get /favicon.ico
    15:042 Webserver Child - WebS: Get /BGround.png
    ...
    <User visits a form sample page and interacts with it>
    64:586 IP_WebServer - New IPv4 client accepted.
    64:587 Webserver Child - WebS: Get /FormGET.htm
    76:650 IP_WebServer - New IPv4 client accepted.
    76:650 Webserver Child - WebS: Get /FormGET.htm
    ...
    <User visits an SSE example page>
    161:362 IP_WebServer - New IPv4 client accepted.
    161:362 Webserver Child - WebS: Get /SSE_Time.htm
    161:384 Webserver Child - WebS: Get /events.js
    161:389 Webserver Child - WebS: Get /SSETime.cgi
    ...
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "TaskPrio.h"
#include "IP_Webserver.h"
#include "WEBS_Conf.h"        // Stack size depends on configuration

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
  #define IP_SUPPORT_IPV4            (1)                              // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif
#ifndef   IP_SUPPORT_IPV6
  #define IP_SUPPORT_IPV6            (0)                              // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif

#ifndef   APP_USE_SSL
  #define APP_USE_SSL                (0)                              // If enabled, creates SSL sockets as well.
#endif

#ifndef   APP_ENABLE_IPV4_PLAIN
  #define APP_ENABLE_IPV4_PLAIN      (IP_SUPPORT_IPV4)                // Enables/disables the creation and usage of IPv4 sockets in this sample.
#endif
#ifndef   APP_ENABLE_IPV4_SSL
  #define APP_ENABLE_IPV4_SSL        (IP_SUPPORT_IPV4 & APP_USE_SSL)  // Enables/disables the creation and usage of IPv4 SSL connections in this sample.
#endif
#ifndef   APP_ENABLE_IPV6_PLAIN
  #define APP_ENABLE_IPV6_PLAIN      (IP_SUPPORT_IPV6)                // Enables/disables the creation and usage of IPv6 sockets in this sample.
#endif
#ifndef   APP_ENABLE_IPV6_SSL
  #define APP_ENABLE_IPV6_SSL        (IP_SUPPORT_IPV6 & APP_USE_SSL)  // Enables/disables the creation and usage of IPv6 SSL connections in this sample.
#endif
#ifndef   APP_ENABLE_CAPTIVE_PORTAL
  #define APP_ENABLE_CAPTIVE_PORTAL  (1)                              // Enables/disables a (WiFi) captive portal (HTTP generate_204 request) to fake internet connectivity (when being a WiFi AP).
#endif

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
#include "SSL.h"
#endif

//
// Web server and IP stack
//
#define MAX_CONNECTIONS           2
#define BACK_LOG                 20
//
// // Timeout [ms] after which the connection will be closed if no new data is received.
//
#define IDLE_TIMEOUT           1000
#define SERVER_PORT_PLAIN        80
#define SERVER_PORT_SSL         443
//
// NumBytes required from memory pool for one connection. Should be fine tuned according
// to your configuration using IP_WEBS_CountRequiredMem() .
//
#if WEBS_ENABLE_TEMPLATING
  #define CHILD_ALLOC_SIZE      (2560 + (WEBS_MAX_RECURSION_DEPTH * WEBS_TEMPLATE_TEMP_PATH_SIZE) + (WEBS_MAX_RECURSION_DEPTH * WEBS_TEMP_BUFFER_SIZE))
  #define TEMPLATING_OVERHEAD   512
#else
  #define CHILD_ALLOC_SIZE      (2560)
  #define TEMPLATING_OVERHEAD     0
#endif

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef     APP_MAIN_STACK_SIZE
  #if WEBS_ENABLE_TEMPLATING
    #define   APP_MAIN_STACK_SIZE      (3144)
  #else
    #define   APP_MAIN_STACK_SIZE      (3144+512)
  #endif
#endif
#ifndef     APP_TASK_STACK_OVERHEAD
  #define   APP_TASK_STACK_OVERHEAD  (0)
#endif
#ifndef     STACK_SIZE_SERVER
  #if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
    #define STACK_SIZE_SERVER        (4000 + APP_TASK_STACK_OVERHEAD + TEMPLATING_OVERHEAD)
  #else
    #define STACK_SIZE_SERVER        (3072 + APP_TASK_STACK_OVERHEAD + TEMPLATING_OVERHEAD)
  #endif
#endif

//
// UDP discover
//
#ifndef   APP_UDP_DISCOVER_PORT
  #define APP_UDP_DISCOVER_PORT         50020
#endif
#define   APP_UDP_DISCOVER_PACKET_SIZE  0x80

//
// DNS (UDP)
//
#define ETH_UDP_DNS_PORT  53

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef struct {
  long         hSock;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  SSL_SESSION* pSession;
#endif
} CONNECTION_CONTEXT;

#if APP_ENABLE_CAPTIVE_PORTAL
typedef struct {
  U16 Id;
  U16 Flag;
  U16 NbQuestions;
  U16 NbAnswers;
  U16 NbAuthorities;
  U16 NbAdditionals;
} DNS_HEADER;
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

static U32 _aPool[(CHILD_ALLOC_SIZE * MAX_CONNECTIONS) / sizeof(int)];  // Memory pool for the Web server child tasks.

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _ConnectCnt;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int APP_MainStack[APP_MAIN_STACK_SIZE / sizeof(int)];    // Stack of the starting point of this sample.
static OS_TASK         APP_MainTCB;                                         // Task-Control-Block of the IP_Task.

static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

//
// Webserver TCBs and stacks
//
static OS_TASK         _aWebTasks[MAX_CONNECTIONS];
static OS_STACKPTR int _aWebStacks[MAX_CONNECTIONS][STACK_SIZE_SERVER/sizeof(int)];

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
//
// SSL
//
static SSL_SESSION _aSSLSession[MAX_CONNECTIONS];
#endif

//
// File system info
//
static const IP_FS_API *_pFS_API;

//
// Webserver connection contexts
//
static CONNECTION_CONTEXT _aConContexts[MAX_CONNECTIONS];

#if APP_ENABLE_CAPTIVE_PORTAL
//
// Captive Portal VFile hook for serving "/generate_204" page.
//
static WEBS_VFILE_HOOK _CaptivePortalVFileHook;
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
*       _closesocket()
*
*  Function description
*    Wrapper for closesocket()
*/
static int _closesocket(long pConnectionInfo) {
  int r;
  struct linger Linger;

  Linger.l_onoff  = 1;  // Enable linger for this socket to verify that all data is send.
  Linger.l_linger = 1;  // Linger timeout in seconds
  setsockopt((long)pConnectionInfo, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));
  r = closesocket((long)pConnectionInfo);
  return r;
}

/*********************************************************************
*
*       _WEBS_Recv()
*
*  Function description
*    Web Server API wrapper for recv()
*/
static int _WEBS_Recv(unsigned char *buf, int len, void *pConnectionInfo) {
  int r;

  r = recv((long)pConnectionInfo, (char *)buf, len, 0);
  return r;
}

/*********************************************************************
*
*       _WEBS_Send()
*
*  Function description
*    Web Server API wrapper for send()
*/
static int _WEBS_Send(const unsigned char *buf, int len, void* pConnectionInfo) {
  int r;

  r = send((long)pConnectionInfo, (const char *)buf, len, 0);
  return r;
}

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))

/*********************************************************************
*
*       _WEBS_SSL_Recv()
*
*  Function description
*    Web Server API wrapper for SSL recv()
*/
static int _WEBS_SSL_Recv(unsigned char *buf, int len, void *pConnectionInfo) {
  SSL_SESSION *pSession;
  int r;

  pSession = (SSL_SESSION*)pConnectionInfo;
  do {
    r = SSL_SESSION_Receive(pSession, buf, len);
  } while (r == 0);  // Receiving 0 bytes means something different on a plain socket.
  //
  // Translate EOF into "connection closed",
  // the Webserver expects same return values as with recv().
  //
  if (r == SSL_ERROR_EOF) {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       _SSL_Recv()
*
*  Function description
*    SSL transport API wrapper for recv()
*/
static int _SSL_Recv(int Socket, char *pData, int Len, int Flags) {
  int r;

  r = recv(Socket, pData, Len, Flags);
  return r;
}

/*********************************************************************
*
*       _WEBS_SSL_Send()
*
*  Function description
*    Web Server API wrapper for SSL send()
*/
static int _WEBS_SSL_Send(const unsigned char *buf, int len, void* pConnectionInfo) {
  SSL_SESSION *pSession;
  int r;

  pSession = (SSL_SESSION*)pConnectionInfo;
  r = SSL_SESSION_Send(pSession, buf, len);
  return r;
}

/*********************************************************************
*
*       _SSL_Send()
*
*  Function description
*    SSL transport API wrapper for send()
*/
static int _SSL_Send(int Socket, const char *pData, int Len, int Flags) {
  int r;

  r = send(Socket, pData, Len, Flags);
  return r;
}

/*********************************************************************
*
*       WebS SSL transport layer
*
*  Description
*    Web server transport API for SSL connections.
*/
static const WEBS_IP_API _Webs_IP_API_SSL = {
  _WEBS_SSL_Send,
  _WEBS_SSL_Recv
};

/*********************************************************************
*
*       SSL transport layer
*
*  Description
*    SSL transport API.
*/
static const SSL_TRANSPORT_API _IP_Transport = {
  _SSL_Send,
  _SSL_Recv,
  IP_OS_GetTime32,
  NULL
};

#endif

/*********************************************************************
*
*       WebS transport layer
*
*  Description
*    Web server transport API for plain connections.
*/
static const WEBS_IP_API _Webs_IP_API = {
  _WEBS_Send,
  _WEBS_Recv
};

/*********************************************************************
*
*       _Alloc()
*
*  Function description
*    Wrapper for Alloc(). (emNet: IP_MEM_Alloc())
*/
static void * _Alloc(U32 NumBytesReq) {
  return IP_AllocEx(_aPool, NumBytesReq);
}

/*********************************************************************
*
*       _Free()
*
*  Function description
*    Wrapper for Alloc(). (emNet: IP_MEM_Alloc())
*/
static void _Free(void *p) {
  IP_Free(p);
}

/*********************************************************************
*
*       WEBS_SYS_API
*
*  Description
*   System related function table
*/
static const WEBS_SYS_API _Webs_SYS_API = {
  _Alloc,
  _Free
};

/*********************************************************************
*
*       _AddToConnectCnt
*/
static void _AddToConnectCnt(int Delta) {
  OS_IncDI();
  _ConnectCnt += Delta;
  OS_DecRI();
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

#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
  //
  // Create IPv4 socket
  //
  if (IPProtVer == AF_INET) {
    hSock = socket(AF_INET, SOCK_STREAM, 0);
  }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
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
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
  if (IPProtVer == AF_INET) {
    struct sockaddr_in Addr;

    WEBS_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(LPort);
    Addr.sin_addr.s_addr = INADDR_ANY;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (IPProtVer == PF_INET6) {
    struct sockaddr_in6 Addr;

    WEBS_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin6_family      = AF_INET6;
    Addr.sin6_port        = htons(LPort);
    Addr.sin6_flowinfo   = 0;
    WEBS_MEMSET(&Addr.sin6_addr, 0, 16);
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
      r = listen(hSock, BACK_LOG);
      if (r != 0) {
        hSock = SOCKET_ERROR;
      }
    }
  }
  return hSock;
}

/*********************************************************************
*
*       _WebServerChildTask
*/
static void _WebServerChildTask(void* pContext) {
  CONNECTION_CONTEXT* pCon;
  void*               pTransportContext;
  WEBS_CONTEXT        ChildContext;
  int  Opt;
  int  r;

  r    = 0;
  pCon = (CONNECTION_CONTEXT*)pContext;
  Opt  = 1;
  setsockopt(pCon->hSock, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pCon->pSession != NULL) {
    //
    // Init webserver context for SSL connection.
    //
    IP_WEBS_Init(&ChildContext, &_Webs_IP_API_SSL, &_Webs_SYS_API, _pFS_API, &WebsSample_Application);  // Initialize the context of the child task.
    pTransportContext = (void*)pCon->pSession;                                                          // SSL transport _Webs_IP_API_SSL operates on the SSL session.
  }
  else
#endif
  {
    //
    // Init webserver context for plain connection.
    //
    IP_WEBS_Init(&ChildContext, &_Webs_IP_API    , &_Webs_SYS_API, _pFS_API, &WebsSample_Application);  // Initialize the context of the child task.
    pTransportContext = (void*)pCon->hSock;                                                             // Plain transport _Webs_IP_API operates on the socket.
  }
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pCon->pSession != NULL) {
    //
    // Handle SSL connection.
    // Upgrade the connection to secure by negotiating a
    // session using SSL.
    //
    if (SSL_SESSION_Accept(pCon->pSession) >= 0) {
      if (_ConnectCnt < MAX_CONNECTIONS) {
        r = IP_WEBS_ProcessEx(&ChildContext, pTransportContext, NULL);
      } else {
        r = IP_WEBS_ProcessLastEx(&ChildContext, pTransportContext, NULL);
      }
      if (r != WEBS_CONNECTION_DETACHED) {
        //
        // Only close the session if it is still in web server context and has
        // not been detached to another handler like a WebSocket handler.
        //
        SSL_SESSION_Disconnect(pCon->pSession);
      }
    }
  }
  else
#endif
  {
    //
    // Handle plain connection.
    //
    if (_ConnectCnt < MAX_CONNECTIONS) {
      r = IP_WEBS_ProcessEx(&ChildContext, pTransportContext, NULL);
    } else {
      r = IP_WEBS_ProcessLastEx(&ChildContext, pTransportContext, NULL);
    }
  }
  if (r != WEBS_CONNECTION_DETACHED) {
    //
    // Only close the socket if it is still in web server context and has
    // not been detached to another handler like a WebSocket handler.
    //
    _closesocket(pCon->hSock);
  }
  OS_EnterRegion();
  _AddToConnectCnt(-1);
  OS_Terminate(0);
  OS_LeaveRegion();
}

/*********************************************************************
*
*       _WebServerParentTask
*/
static void _WebServerParentTask(void) {
  CONNECTION_CONTEXT* pCon;
  IP_fd_set ReadFds;
  U32  Timeout;
  U32  NumBytes;
#if APP_ENABLE_IPV4_PLAIN
  long hSockParent4_Plain;
#endif
#if APP_ENABLE_IPV4_SSL
  long hSockParent4_SSL;
#endif
#if APP_ENABLE_IPV6_PLAIN
  long hSockParent6_Plain;
#endif
#if APP_ENABLE_IPV6_SSL
  long hSockParent6_SSL;
#endif
  long hSock;
  int  i;
  int  t;
  int  t0;
  int  r;
  int  IFaceId;
  WEBS_BUFFER_SIZES BufferSizes;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  char IsSSL;
#endif

  hSock   = 0;  // Avoid uninitialized warning.
  Timeout = IDLE_TIMEOUT;
  //
  // Assign file system
  //
  _pFS_API = &IP_FS_ReadOnly;  // To use a a real filesystem like emFile replace this line.
//  _pFS_API = &IP_FS_emFile;    // Use emFile
//  IP_WEBS_AddUpload();         // Enable upload
  //
  // Configure buffer size.
  //
  IFaceId = IP_INFO_GetNumInterfaces() - 1;                            // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  WEBS_MEMSET(&BufferSizes, 0, sizeof(BufferSizes));
  BufferSizes.NumBytesInBuf       = WEBS_IN_BUFFER_SIZE;
  BufferSizes.NumBytesOutBuf      = IP_TCP_GetMTU(IFaceId) - 40 - 20;  // Use max. MTU configured for the last interface added minus IPv6(40 bytes)/TCP(20 bytes) headers.
                                                                       // Calculation for the memory pool is done under assumption of the best case headers with - 40 bytes for IPv4.
  BufferSizes.NumBytesParaBuf     = WEBS_PARA_BUFFER_SIZE;
  BufferSizes.NumBytesFilenameBuf = WEBS_FILENAME_BUFFER_SIZE;
  BufferSizes.MaxRootPathLen      = WEBS_MAX_ROOT_PATH_LEN;
#ifdef APP_FULL_URI_BUF_SIZE
  BufferSizes.NumBytesFullUriBuf  = APP_FULL_URI_BUF_SIZE;
#endif
  //
  // Configure the size of the buffers used by the Webserver child tasks.
  //
  IP_WEBS_ConfigBufSizes(&BufferSizes);
  //
  // Check memory pool size.
  //
  NumBytes = IP_WEBS_CountRequiredMem(NULL);     // Get NumBytes for internals of one child thread.
  NumBytes = (NumBytes + 64) * MAX_CONNECTIONS;  // Calc. the total amount for x connections (+ some bytes for managing a memory pool).
  IP_Logf_Application("WEBS: Using a memory pool of %lu bytes for %lu connections.", sizeof(_aPool), MAX_CONNECTIONS);
  if (NumBytes > sizeof(_aPool)) {
    IP_Warnf_Application("WEBS: Memory pool should be at least %lu bytes.", NumBytes);
  }
  //
  // Give the stack some more memory to enable the dynamical memory allocation for the Web server child tasks
  //
  IP_AddMemory(_aPool, sizeof(_aPool));
  //
  // Try until we get valid parent sockets.
  //
#if APP_ENABLE_IPV4_PLAIN
  while (1) {
    hSockParent4_Plain = _ListenAtTcpPort(PF_INET, SERVER_PORT_PLAIN);
    if (hSockParent4_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV4_SSL
  while (1) {
    hSockParent4_SSL = _ListenAtTcpPort(PF_INET, SERVER_PORT_SSL);
    if (hSockParent4_SSL == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV6_PLAIN
  while (1) {
    hSockParent6_Plain = _ListenAtTcpPort(PF_INET6, SERVER_PORT_PLAIN);
    if (hSockParent6_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV6_SSL
  while (1) {
    hSockParent6_SSL = _ListenAtTcpPort(PF_INET6, SERVER_PORT_SSL);
    if (hSockParent6_SSL == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
  //
  // Loop once per client and create a thread for the actual server
  //
  do {
Next:
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
    IsSSL = 0;
#endif
    IP_FD_ZERO(&ReadFds);                     // Clear the set
#if APP_ENABLE_IPV4_PLAIN
    IP_FD_SET(hSockParent4_Plain, &ReadFds);  // Add IPv4 plain socket to the set
#endif
#if APP_ENABLE_IPV4_SSL
    IP_FD_SET(hSockParent4_SSL, &ReadFds);    // Add IPv4 SSL socket to the set
#endif
#if APP_ENABLE_IPV6_PLAIN
    IP_FD_SET(hSockParent6_Plain, &ReadFds);  // Add IPv6 plain socket to the set
#endif
#if APP_ENABLE_IPV6_SSL
    IP_FD_SET(hSockParent6_SSL, &ReadFds);    // Add IPv6 SSL socket to the set
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
      goto QueueConnection;
    }
#endif
#if APP_ENABLE_IPV4_SSL
    //
    // Check if the IPv4 SSL socket is ready
    //
    if (IP_FD_ISSET(hSockParent4_SSL, &ReadFds)) {
      hSock = accept(hSockParent4_SSL, NULL, NULL);
      if (hSock == SOCKET_ERROR) {
        continue;               // Error, try again.
      }
      IP_Logf_Application("New IPv4 SSL client accepted.");
      IsSSL = 1;
      goto QueueConnection;
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
      goto QueueConnection;
    }
#endif
#if APP_ENABLE_IPV6_SSL
    //
    // Check if the IPv6 SSL socket is ready
    //
    if (IP_FD_ISSET(hSockParent6_SSL, &ReadFds)) {
      hSock = accept(hSockParent6_SSL, NULL, NULL);
      if (hSock == SOCKET_ERROR) {
        continue;               // Error, try again.
      }
      IP_Logf_Application("New IPv6 SSL client accepted.");
      IsSSL = 1;
      goto QueueConnection;
    }
#endif
QueueConnection:
    //
    // Create server thread to handle connection.
    // If connection limit is reached, keep trying for some time before giving up and outputting an error message
    //
    t0 = OS_GetTime32() + 1000;
    do {
      if (_ConnectCnt < MAX_CONNECTIONS) {
        for (i = 0; i < MAX_CONNECTIONS; i++) {
          r = OS_IsTask(&_aWebTasks[i]);
          if (r == 0) {
            //
            // Prepare context that is given to child task.
            //
            pCon = &_aConContexts[i];
            WEBS_MEMSET(pCon, 0, sizeof(*pCon));
            pCon->hSock = hSock;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
            if (IsSSL != 0) {
              pCon->pSession = &_aSSLSession[i];
              //
              // Prepare for upgrading the connection to secure using SSL.
              //
              SSL_SESSION_Prepare(&_aSSLSession[i], hSock, &_IP_Transport);
            }
#endif
            setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));  // Set receive timeout for the client.
            OS_CREATETASK_EX(&_aWebTasks[i], "Webserver - Child", _WebServerChildTask, APP_TASK_PRIO_WEBSERVER_CHILD, _aWebStacks[i], (void*)pCon);
            _AddToConnectCnt(1);
            goto Next;
          }
        }
      }
      //
      // Check time out
      //
      t = OS_GetTime32();
      if ((t - t0) > 0) {
        IP_WEBS_OnConnectionLimit(_WEBS_Send, _WEBS_Recv, (void*)hSock);
        _closesocket(hSock);
        break;
      }
      OS_Delay(10);
    } while(1);
  }  while(1);
}

#if APP_ENABLE_CAPTIVE_PORTAL
/*********************************************************************
*
*       _DNS_SwapHeader()
*
*  Function description
*    Swaps the DNS header between host/network endianess.
*/
static void _DNS_SwapHeader(DNS_HEADER* pDNS) {
  pDNS->Flag          = htons(pDNS->Flag);
  pDNS->NbQuestions   = htons(pDNS->NbQuestions);
  pDNS->NbAnswers     = htons(pDNS->NbAnswers);
  pDNS->NbAuthorities = htons(pDNS->NbAuthorities);
  pDNS->NbAdditionals = htons(pDNS->NbAdditionals);
}

/*********************************************************************
*
*       _cbOnRxAnyDomainDNS()
*
*  Function description
*    Callback that answers any DNS request sent to the target
*    with our own IP address, regardless of the domain requested.
*    Only IPv4 DNS requests will be handled but can return IPv6
*    addresses in a response.
*
*  Return value
*    IP_RX_ERROR: Packet is invalid for some reason and not handled.
*    IP_OK      : Packet handled.
*
*  Additional information
*    A "captive portal" means that a browser sends a request to a
*    internet resource to check if it is reachable. If the "portal"
*    can not be reached we are a "captive" in a network without
*    internet access. WiFi clients might instantly switch to any
*    other known WiFi connection that is known to have internet
*    access if this check results in a negative outcome.
*
*    As different browsers and different URLs are used for testing,
*    faking/rerouting only a portion of some domains is not enough.
*    What all have in common is the /generate_204 URL requested.
*    Therefore this callback will try to fake answer any request
*    for "<domain.tld>/generate_204" with a DNS resolve to its own
*    IP address for "<domain.tld>".
*
*    A web server then needs to serve the "/generate_204" page
*    to let the client know that "internet access is available".
*/
static int _cbOnRxAnyDomainDNS(IP_PACKET* pInPacket, void* pContext) {
  DNS_HEADER* pInHeader;
  DNS_HEADER* pOutHeader;
  IP_PACKET*  pOutPacket;
  U16*        pType;
  U16*        pClass;
  U16*        p16;
  U8*         p;
  U8*         pInData;
  char*       sHostName;
  U32         TargetAddr;
  U32         IPAddr;
  unsigned    RequestLength;
  unsigned    IFaceId;
  unsigned    Size;
#if IP_SUPPORT_IPV6
  U8          NumAddr;
#endif

  IP_USE_PARA(pContext);

  //
  // Get our own interface the apcket came in and IP address.
  //
  IFaceId = IP_UDP_GetIFIndex(pInPacket);  // Find out the interface that the packet came in.
  IPAddr  = htonl(IP_GetIPAddr(IFaceId));  // Get the IP address
  if (IPAddr == 0) {                       // As a WiFi AP we should have a static IPv4 address at this point. If not we can not answer the DNS request.
    goto Done;
  }
  pInData   = (U8*)IP_UDP_GetDataPtr(pInPacket);
  pInHeader = (DNS_HEADER*)pInData;
  _DNS_SwapHeader(pInHeader);
  //
  // Ignore DNS packet if not a request.
  //
  if ((pInHeader->Flag & (1u << 15)) != 0u) {  // Is this a response ?
    goto Done;                                 // ==> Discard.
  }
  RequestLength = 0u;
  //
  // Check the length of the request/hostname.
  // We accept any hostname so we do not need to compare it.
  //
  sHostName     = (char*)(pInHeader + 1u);  // The hostname typically comes after the DNS header.
  RequestLength = WEBS_STRLEN(sHostName);
  //
  // Skip to next fields after the hostname + string terminate(1): Type (U16) and Class (U16).
  //
  p              = (U8*)(sHostName + RequestLength + 1);
  pType          = (U16*)p;  // Remember Type field location.
  p             += 2;        // Skip to Class field.
  pClass         = (U16*)p;  // Remember Class field location.
  RequestLength += 5;        // +1 (hostname string terminate) + Type(2) + Class(2).
  //
  // Allocate packet for answer.
  //
  Size = sizeof(DNS_HEADER) + RequestLength + 16;  // 16 bytes is the length for an answer with a 4-bytes IPv4 address and its fields.
  if (*pType == 0x1Cu) {                           // Is this an IPv6 DNS request ?
#if IP_SUPPORT_IPV6
    Size = Size - 4 + 16;                          // ==> Remove space for 4-bytes IPv4 address and add space for 16-bytes IPv6 address.
#else
    goto Done;                                     // IPv6 not supported.
#endif
  }
  pOutPacket = IP_UDP_AllocEx(IFaceId, Size);
  if (pOutPacket == NULL) {
    goto Done;
  }
  //
  // Fill packet with response DNS header.
  //
  IP_UDP_GetSrcAddr(pInPacket, &TargetAddr, sizeof(TargetAddr));  // Get IPv4 address of requester to send back unicast.
  pOutHeader = (DNS_HEADER*)IP_UDP_GetDataPtr(pOutPacket);        // Get the pointer to our packet payload space and DNS header.
  WEBS_MEMSET(pOutHeader, 0, Size);                                 // Zero the packet payload to be on the safe side.
  pOutHeader->Id            = pInHeader->Id;
  pOutHeader->Flag          = 0x8000;  // Reply
  pOutHeader->NbQuestions   = 1;
  pOutHeader->NbAnswers     = 1;
  pOutHeader->NbAuthorities = 0;
  pOutHeader->NbAdditionals = 0;
  _DNS_SwapHeader(pOutHeader);
  p = (U8*)(pOutHeader + 1);  // Skip to after the response DNS header.
  //
  // Copy the requested domain name (after the DNS header) including Type and Class
  // fields into our answer to fake all DNS requests, regardless of the domain name (catch all).
  //
  pInData += sizeof(DNS_HEADER);
  WEBS_MEMCPY(p, pInData, RequestLength);
  p += RequestLength;
  //
  // Fill in the answer: compressed address, type, class, time to leave, data length, IP address.
  //
  *p   = 0xC0u;
  p++;
  *p   = 0x0Cu;
  p++;
  p16  = (U16*)p;
  *p16 = *pType;
  p16++;
  *p16 = *pClass;
  p16++;
  *p16 = 0u;
  p16++;
  *p16 = 0x2B01u;
  p16++;
  *p16 = htons(0x0004u);
  p16++;
#if IP_SUPPORT_IPV6
  if (*pType == 0x1Cu) {  // IPv6 request ?
    IP_IPV6_GetIPv6Addr(IFaceId, 0, (IPV6_ADDR*)p16, &NumAddr);  // Fill in primary IPv6 address of the interface.
  }
  else
#endif
  {
    IPAddr = htonl(IP_GetIPAddr(IFaceId));                       // Get IPv4 address of the interface.
    WEBS_MEMCPY(p16, &IPAddr, sizeof(IPAddr));                     // Fill in the IP address.
  }
  //
  // Send packet.
  //
  IP_UDP_SendAndFree(IFaceId, TargetAddr, IP_UDP_GetFPort(pInPacket), ETH_UDP_DNS_PORT, pOutPacket);
Done:
  return IP_OK;
}

/*********************************************************************
*
*       _CP_CheckVFile()
*
*  Function description
*    Checks if we have content that we can deliver for the requested
*    file using the VFile hook system.
*
*  Parameters
*   sFileName: Name of the file that is requested.
*   pIndex   : Pointer to a variable that has to be filled with
*              the index of the entry found in case of using a
*              filename<=>content list.
*              Alternative all comparisons can be done using the
*              filename. In this case the index is meaningless
*              and does not need to be returned by this routine.
*
*  Return value
*    0: We do not have content to send for this filename,
*       fall back to the typical methods for retrieving
*       a file from the web server.
*    1: We have content that can be sent using the VFile
*       hook system.
*/
static int _CP_CheckVFile(const char* sFileName, unsigned* pIndex) {
  int r;

  WEBS_USE_PARA(pIndex);

  //
  // Generated VFiles
  //
  if ((WEBS_STRCMP(sFileName, "/generate_204") == 0) || (WEBS_STRCMP(sFileName, "/gen_204") == 0)) {
    r = 1;
  } else {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       _CP_SendVFile()
*
*  Function description
*   Sends the captive portal response page .
*
*  Parameters
*    pContextIn: Send context of the connection processed for
*                forwarding it to the callback used for output.
*    Index     : Index of the entry of a filename<=>content list
*                if used. Alternative all comparisons can be done
*                using the filename. In this case the index is
*                meaningless. If using a filename<=>content list
*                this is faster than searching again.
*    sFileName : Name of the file that is requested. In case of
*                working with the Index this is meaningless.
*    pf        : Function pointer to the callback that has to be
*                for sending the content of the VFile.
*      pContextOut: Out context of the connection processed.
*      pData      : Pointer to the data that will be sent
*      NumBytes   : Number of bytes to send from pData. If NumBytes
*                   is passed as 0 the send function will run a strlen()
*                   on pData expecting a string.
*/
static void _CP_SendVFile(void* pContextIn, unsigned Index, const char* sFileName, void (*pf)(void* pContextOut, const char* pData, unsigned NumBytes)) {
  WEBS_USE_PARA(Index);
  WEBS_USE_PARA(sFileName);
  WEBS_USE_PARA(pf);

  IP_WEBS_Send204NoContent((WEBS_OUTPUT*)pContextIn);
}

/*********************************************************************
*
*       _CP_VFileAPI
*
*  Function description
*    Application data table, defines all Captive portal application
*    specifics used by the web server.
*/
static const WEBS_VFILE_APPLICATION _CP_VFileAPI = {
  _CP_CheckVFile,
  _CP_SendVFile
};

#endif  // APP_ENABLE_CAPTIVE_PORTAL

/*********************************************************************
*
*       _OnRx
*
*  Function description
*    Discover client UDP callback. Called from stack
*    whenever we get a discover request.
*
*  Return value
*    IP_RX_ERROR  if packet is invalid for some reason
*    IP_OK        if packet is valid
*/
#if APP_UDP_DISCOVER_PORT
static int _OnRx(IP_PACKET *pInPacket, void *pContext) {
  char *      pInData;
  IP_PACKET * pOutPacket;
  char *      pOutData;
  U32         TargetAddr;
  U32         IPAddr;
  unsigned    IFaceId;

  (void)pContext;

  IFaceId = IP_UDP_GetIFIndex(pInPacket);  // Find out the interface that the packet came in.
  IPAddr  = htonl(IP_GetIPAddr(IFaceId));
  if (IPAddr == 0) {
    goto Done;
  }
  pInData = (char*)IP_UDP_GetDataPtr(pInPacket);
  if (WEBS_MEMCMP(pInData, "Discover", 8)) {
    goto Done;
  }
  //
  // Alloc packet
  //
  pOutPacket = IP_UDP_AllocEx(IFaceId, APP_UDP_DISCOVER_PACKET_SIZE);
  if (pOutPacket == NULL) {
    goto Done;
  }
  //
  // Fill packet with data
  //
  pOutData = (char*)IP_UDP_GetDataPtr(pOutPacket);
  IP_UDP_GetSrcAddr(pInPacket, &TargetAddr, sizeof(TargetAddr));    // Send back Unicast
  WEBS_MEMSET(pOutData, 0, APP_UDP_DISCOVER_PACKET_SIZE);
  WEBS_STRCPY(pOutData + 0x00, "Found");
  IPAddr = htonl(IP_GetIPAddr(IFaceId));
  WEBS_MEMCPY(pOutData + 0x20, (void*)&IPAddr, 4);      // 0x20: IP address
  IP_GetHWAddr(IFaceId, (U8*)pOutData + 0x30, 6);  // 0x30: MAC address
  //
  // Send packet
  //
  IP_UDP_SendAndFree(IFaceId, TargetAddr, APP_UDP_DISCOVER_PORT, APP_UDP_DISCOVER_PORT, pOutPacket);
Done:
  return IP_OK;
}
#endif

/*********************************************************************
*
*       APP_MainTask()
*
*  Function description
*    Sample starting point.
*/
static void APP_MainTask(void) {
#ifdef APP_AFTER_CONFIG
  int IFaceId;
#endif

  OS_SetTaskName(OS_GetTaskID(), "Webserver - Parent");
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  //
  // Initialize SSL.
  //
  SSL_Init();
#endif
#if APP_UDP_DISCOVER_PORT
  //
  // Open UDP port APP_UDP_DISCOVER_PORT for emNet discover.
  //
  IP_UDP_Open(0L /* any foreign host */, APP_UDP_DISCOVER_PORT, APP_UDP_DISCOVER_PORT, _OnRx, 0L);
#endif
#if APP_ENABLE_CAPTIVE_PORTAL
  //
  // Answer all DNS requests and fake hostname regardless of the request to
  // succeed in a captive portal test when we are running as WiFi AP.
  //
  IP_UDP_Open(0L /* any foreign host */,  0, ETH_UDP_DNS_PORT,  _cbOnRxAnyDomainDNS, 0L);
  IP_WEBS_AddVFileHook(&_CaptivePortalVFileHook, (WEBS_VFILE_APPLICATION*)&_CP_VFileAPI, HTTP_ENCODING_FROM_CONTEXT);
#endif
  IP_WEBS_X_SampleConfig();  // Load a web server sample config that might add other resources like REST.
#ifdef APP_AFTER_CONFIG
  //
  // Add additional configuration when being included
  // into another sample via preprocessor.
  //
  IFaceId = IP_INFO_GetNumInterfaces() - 1;  // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  APP_AFTER_CONFIG(SEGGER_ADDR2PTR(void, IFaceId));
#endif
  _WebServerParentTask();
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
