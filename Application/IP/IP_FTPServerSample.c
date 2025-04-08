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

Purpose : Demonstrates how to start the emFTP server in different configurations.

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
  The sample can easily be configured to start the emFTP server in
  different configurations and features such as IPv6 only and/or
  with/without SSL support.

  Preparations:
    Enable/disable the features that you want to use. The available
    binary configuration switches are:
      - APP_USE_SSL          : Helper switch that enables/disables all SSL switches below.
      - APP_ENABLE_IPV4_PLAIN: Enable creation of a plain   IPv4 socket on port  21 (default).
      - APP_ENABLE_IPV4_SSL  : Enable creation of a secured IPv4 socket on port 990 (default) for FTPS and explicitly secured FTPES on port 21 (default). Requires emSSL.
      - APP_ENABLE_IPV6_PLAIN: Enable creation of a plain   IPv6 socket on port  21 (default). Requires IPv6.
      - APP_ENABLE_IPV6_SSL  : Enable creation of a secured IPv6 socket on port 990 (default) for FTPS and explicitly secured FTPES on port 21 (default). Requires emSSL.

      ALLOW_SECURE_ONLY is set by default when SSL is used to allow secured connections only.

  Expected behavior:
    This sample starts an FTP server that listens on the enabled ports
    and protocols and can be accessed via a standard FTP client such as
    Filezilla by connecting to the IP(v4/v6) addresses that are printed
    in the terminal output when starting.

    The FTP server can be accessed with different protocols, typically by
    preceding the address given to the FTP client with the protocol scheme as follows:
      - IPv4 plain                                         : <IPv4 address or hostname>
      - IPv6 plain                                         : [<IPv6 address or hostname>]
      - IPv4 FTPS  (implicit SSL, typically on port 990)   : ftps://<IPv4 address or hostname>
      - IPv6 FTPS  (implicit SSL, typically on port 990)   : ftps://[<IPv6 address or hostname>]
      - IPv4 FTPES (explicit SSL, starting on plain socket): ftpes://<IPv4 address or hostname>
      - IPv6 FTPES (explicit SSL, starting on plain socket): ftpes://[<IPv6 address or hostname>]
      - IPvX SFTP  (SSH File Transfer Protocol)            : Not supported by RFC 959 as it is a different protocol!

    Two default user accounts are installed for this sample:
      - User:      anonymous
        - Pass:    <anything>
        - Rights:  Read-Only

      - User:      Admin
        - Pass:    Secret
        - Rights:  Read-Write

  Sample output (with APP_USE_SSL enabled):
    0:000 MainTask - INIT: emNet init started. Version 3.40
    0:000 MainTask - *********************************************************************
    0:000 MainTask - *                        emNet Configuration                        *
    0:000 MainTask - *********************************************************************
    0:000 MainTask - * IP_DEBUG: 2
    0:000 MainTask - * Memory added: 24576 bytes
    0:000 MainTask - * Buffer configuration:
    0:000 MainTask - *   12 buffers of 256 bytes
    0:000 MainTask - *   6 buffers of 1516 bytes
    0:000 MainTask - * TCP Tx/Rx window size per socket: 4380/4380 bytes
    0:000 MainTask - * Number of interfaces added: 1
    0:000 MainTask - * Interface #0 configuration:
    0:000 MainTask - *   Type: ETH
    0:000 MainTask - *   MTU: 1500
    0:000 MainTask - *   HW addr.: 00:22:C7:FF:FF:FF
    0:000 MainTask - *********************************************************************
    0:001 MainTask - INIT: Link is down
    0:001 MainTask - DRIVER: Found PHY with Id 0x7 at addr 0x0
    0:002 MainTask - INIT: Init completed
    0:005 MainTask - *****************************************************************
    0:005 MainTask - *                    emSSL Configuration                        *
    0:005 MainTask - *****************************************************************
    0:005 MainTask - *
    0:005 MainTask - * Environment:
    0:005 MainTask - *   SSL_VERSION: 25801 [2.58a]
    0:005 MainTask - *   SSL_DEBUG:   2
    0:005 MainTask - *
    0:005 MainTask - * Configuration:
    0:005 MainTask - *   SSL_MAX_SESSION_TICKET_LEN:     256
    0:006 MainTask - *   SSL_MAX_APP_DATA_FRAGMENT_LEN:  1024
    0:006 MainTask - *   SSL_SESSION_CACHE_SIZE:         0
    0:006 MainTask - *
    0:006 MainTask - * Protocols:
    0:006 MainTask - *   TLS 1.0: Enabled
    0:006 MainTask - *   TLS 1.1: Enabled
    0:006 MainTask - *   TLS 1.2: Enabled
    0:006 MainTask - *
    0:006 MainTask - * Cipher suites:
    0:006 MainTask - *   TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA
    0:006 MainTask - *   TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
    ...
    0:019 MainTask - *
    0:019 MainTask - * Ciphers:
    0:019 MainTask - *   NULL
    0:019 MainTask - *   3DES-EDE-CBC
    0:019 MainTask - *   AES-128-CBC
    ...
    0:026 IP_Task - INIT: IP_Task started
    0:027 FTPServer_Parent - FTPS: Using a memory pool of 8192 bytes for 2 connections.
    2:000 IP_Task - LINK: Link state changed: Full duplex, 100MHz
    2:400 IP_Task - NDP: Link-local IPv6 addr.: FE80:0000:0000:0000:0222:C7FF:FEFF:FFFF added to IFace: 0
    3:000 IP_Task - DHCPc: Sending discover!
    3:021 IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.189, Mask: 255.255.0.0, GW: 192.168.13.1.
    4:000 IP_Task - DHCPc: IP addr. checked, no conflicts
    4:000 IP_Task - DHCPc: Sending Request.
    4:022 IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.189, Mask: 255.255.0.0, GW: 192.168.13.1.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "TaskPrio.h"
#include "IP_FTP_SERVER.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   FTPS_STRCMP
  #define FTPS_STRCMP          strcmp
#endif

#ifndef   FTPS_MEMSET
  #define FTPS_MEMSET          memset
#endif

#ifndef   FTPS_MEMCPY
  #define FTPS_MEMCPY          memcpy
#endif

#ifndef   USE_RX_TASK
  #define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.
#endif

//
// Sample features enable/disable
//
#ifndef   IP_SUPPORT_IPV4
  #define IP_SUPPORT_IPV4        (1)                              // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif
#ifndef   IP_SUPPORT_IPV6
  #define IP_SUPPORT_IPV6        (0)                              // Set a default if not available through IP_ConfDefaults.h or project settings.
#endif

#ifndef   APP_USE_SSL
  #define APP_USE_SSL            (0)                              // If enabled, creates SSL sockets as well.
#endif

#ifndef   APP_ENABLE_IPV4_PLAIN
  #define APP_ENABLE_IPV4_PLAIN  (IP_SUPPORT_IPV4)                // Enables/disables the creation and usage of IPv4 sockets in this sample.
#endif
#ifndef   APP_ENABLE_IPV4_SSL
  #define APP_ENABLE_IPV4_SSL    (IP_SUPPORT_IPV4 & APP_USE_SSL)  // Enables/disables the creation and usage of IPv4 SSL connections in this sample.
#endif
#ifndef   APP_ENABLE_IPV6_PLAIN
  #define APP_ENABLE_IPV6_PLAIN  (IP_SUPPORT_IPV6)                // Enables/disables the creation and usage of IPv6 sockets in this sample.
#endif
#ifndef   APP_ENABLE_IPV6_SSL
  #define APP_ENABLE_IPV6_SSL    (IP_SUPPORT_IPV6 & APP_USE_SSL)  // Enables/disables the creation and usage of IPv6 SSL connections in this sample.
#endif

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
#include "SSL.h"

#define ALLOW_SECURE_ONLY        (1)                              // Allow only secured connections. No fallback to insecure.
#else
#define ALLOW_SECURE_ONLY        (0)
#endif

//
// FTP server sample configuration.
//
#define MAX_CONNECTIONS     2               // Number of connections to handle at the same time
#define BACK_LOG            5               // Number of incoming connections to hold in case one connection gets freed
#define SERVER_PORT_PLAIN   21              // Server port to use for plain FTP and explicit TLS FTPES mode.
#define SERVER_PORT_SSL     990             // Server port to use for implicit TLS FTPS mode.
#define CHILD_ALLOC_SIZE    4096            // NumBytes required from memory pool for one connection. Should be fine tuned according
                                            // to your configuration using IP_FTPS_CountRequiredMem() .
#define FS_SECTOR_SIZE      2048            // Size of one or multiple FS sector(s). For the best write performance on
                                            // uploading a file the InBuffer should have the size of one or a multiple
                                            // of a sector size. This avoids read-modify-write operations in the FS.
#define FS_COPY_BUFFER_SIZE 512             // FS__CopyFile needs 512 Bytes on the stack.
#define IDLE_TIMEOUT        (2 * 60 * 1000) // 2 minutes idle timeout before an open FTP is automatically closed by the server.

enum {
  USER_ID_ANONYMOUS = 1,
  USER_ID_ADMIN
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef     APP_MAIN_STACK_SIZE
  #define   APP_MAIN_STACK_SIZE      (3144)
#endif
#ifndef     APP_TASK_STACK_OVERHEAD
  #define   APP_TASK_STACK_OVERHEAD  (0)
#endif
#ifndef     STACK_SIZE_SERVER
  #if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
    #define STACK_SIZE_SERVER        (4096 + APP_TASK_STACK_OVERHEAD)
  #else
    #define STACK_SIZE_SERVER        (2304 + APP_TASK_STACK_OVERHEAD)
  #endif
#endif

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef struct {
  FTPS_CONTEXT* pContext;
  long          PlainSocket;
  unsigned      IPProtVer;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  unsigned      IsSecure;
  char          IsImplicit;
  SSL_SESSION   Session;
#endif
} CONNECTION_CONTEXT;

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

static U32 _aPool[(CHILD_ALLOC_SIZE * MAX_CONNECTIONS) / sizeof(int)];  // Memory pool for the FTP server child tasks.

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _ConnectCnt;

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
//
// SSL transport API.
//
static const SSL_TRANSPORT_API _IP_Transport = {
  send,
  recv,
  NULL, // Don't verify the time. Otherwise a function that returns the unix time is needed.
  NULL
};
#endif

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
// FTP server TCBs and stacks
//
static OS_TASK         _aFTPsTasks[MAX_CONNECTIONS];
static OS_STACKPTR int _aFTPsStacks[MAX_CONNECTIONS][(STACK_SIZE_SERVER + FS_COPY_BUFFER_SIZE)/sizeof(int)];

//
// File system info
//
static const IP_FS_API* _pFS_API;

//
// FTP server connection contexts
//
static CONNECTION_CONTEXT _aConContexts[2 * MAX_CONNECTIONS];  // One client requires two connections/sessions.

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
*       _Alloc()
*
*  Function description
*    Wrapper for memory allocations. (emNet: IP_MEM_AllocEx())
*
*  Parameters
*    NumBytesReq: Number of bytes to allocate from our memory pool.
*
*  Return value
*    != NULL: O.K., pointer to allocated memory block.
*    == NULL: Error.
*/
static void* _Alloc(U32 NumBytesReq) {
  return IP_AllocEx(_aPool, NumBytesReq);
}

/*********************************************************************
*
*       _AllocContext()
*
*  Function description
*    Retrieves the next free context memory block to use.
*
*  Parameters
*    hSock    : Socket handle.
*    IPProtVer: Protocol family which should be used (PF_INET or PF_INET6).
*
*  Return value
*    != NULL: Free context found, pointer to context.
*    == NULL: Error, no more free entries.
*/
static CONNECTION_CONTEXT* _AllocContext(long hSock, unsigned IPProtVer) {
  CONNECTION_CONTEXT* pContext;
  CONNECTION_CONTEXT* pRunner;
  unsigned            i;

  pContext = NULL;
  i        = 0u;
  OS_DI();
  pRunner = &_aConContexts[0];
  do {
    if (pRunner->PlainSocket == 0) {
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
      if (pRunner->Session.Socket == 0)
#endif
      {
        pRunner->PlainSocket = hSock;  // Mark the entry to be in use.
        pContext             = pRunner;
        pContext->pContext   = NULL;
        pContext->IPProtVer  = IPProtVer;
        break;
      }
    }
    pRunner++;
    i++;
  } while (i < SEGGER_COUNTOF(_aConContexts));
  OS_EI();
  return pContext;
}

/*********************************************************************
*
*       _FreeContext()
*
*  Function description
*    Frees a context memory block.
*
*  Parameters
*    pContext: Connection context.
*/
static void _FreeContext(CONNECTION_CONTEXT* pContext) {
  FTPS_MEMSET(pContext, 0, sizeof(*pContext));
}

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
/*********************************************************************
*
*       _UpgradeOnDemand()
*
*  Function description
*    Upgrades a connection to a secured one.
*
*  Parameters
*    pContext: Connection context.
*
*  Return value
*    == 0: Success.
*    <  0: Error.
*/
static int _UpgradeOnDemand(CONNECTION_CONTEXT* pContext) {
  int Status;

  Status = 0;
  if ((pContext->IsSecure != 0) && (pContext->PlainSocket != 0) && (pContext->Session.Socket == 0)) {
    SSL_SESSION_Prepare(&pContext->Session, pContext->PlainSocket, &_IP_Transport);
    Status = SSL_SESSION_Accept(&pContext->Session);
    if (Status < 0) {
      closesocket(pContext->PlainSocket);
    }
    pContext->PlainSocket = 0;
  }
  return Status;
}
#endif

/*********************************************************************
*
*       _Recv()
*/
static int _Recv(unsigned char* buf, int len, void* pConnectionInfo) {
  CONNECTION_CONTEXT* pContext;
  int                 Status;

  pContext = (CONNECTION_CONTEXT*)pConnectionInfo;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  _UpgradeOnDemand(pContext);
  if (pContext->PlainSocket == 0) {
    do {
      Status = SSL_SESSION_Receive(&pContext->Session, buf, len);
    } while (Status == 0);  // Receiving 0 bytes means something different on a plain socket.
    //
    // Translate EOF into "connection closed",
    // the FTP Server expects same return values as with recv().
    //
    if (Status == SSL_ERROR_EOF) {
      Status = 0;
    }
  }
  else
#endif
  {
    Status = recv(pContext->PlainSocket, (char*)buf, len, 0);
  }
  return Status;
}

/*********************************************************************
*
*       _Send()
*/
static int _Send(const unsigned char* buf, int len, void* pConnectionInfo) {
  CONNECTION_CONTEXT* pContext;
  int                 Status;

  pContext = (CONNECTION_CONTEXT*)pConnectionInfo;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  _UpgradeOnDemand(pContext);
  if (pContext->PlainSocket == 0) {
    Status = SSL_SESSION_Send(&pContext->Session, buf, len);
  }
  else
#endif
  {
    Status = send(pContext->PlainSocket, (const char*)buf, len, 0);
  }
  return Status;
}

/*********************************************************************
*
*       _SendPlainSock()
*/
static int _SendPlainSock(const unsigned char* buf, int len, void* pConnectionInfo) {
  long hSock;
  int  Status;

  hSock  = (long)pConnectionInfo;
  Status = send(hSock, (const char*)buf, len, 0);
  return Status;
}

/*********************************************************************
*
*       _PrintSocketError()
*
*  Function description
*    Fetches the socket error state and prints it to the console.
*
*  Parameters:
*    hSock: Socket that previously returned SOCKET_ERROR .
*/
static void _PrintSocketError(long hSock) {
  int Error;

  getsockopt(hSock, AF_INET, SO_ERROR, &Error, sizeof(Error));
  FTPS_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(Error)));
}

/*********************************************************************
*
*       _Connect()
*
*  Function description
*    This function is called from the FTP server module if the client
*    uses active FTP to establish the data connection.
*
*  Parameters
*    CtrlSocket: Socket of the FTP control connection to the client.
*    Port      : ACTIVE port as requested by the client to which we
*                shall actively connect to.
*
*  Return value
*    == NULL: Error, unable to open the data connection.
*    != NULL: Data connection context.
*/
static FTPS_SOCKET _Connect(FTPS_SOCKET CtrlSocket, U16 Port) {
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
  struct sockaddr_in*  pAddr4;
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
  struct sockaddr_in6* pAddr6;
  struct sockaddr_in6  Addr;
#else
  struct sockaddr_in   Addr;
#endif
  CONNECTION_CONTEXT*  pCtrlContext;
  CONNECTION_CONTEXT*  pDataContext;
  long                 hSock;
  long                 DataSock;
  int                  AddrSize;
  int                  Error;

  (void)Port;

  pCtrlContext = (CONNECTION_CONTEXT*)CtrlSocket;
  DataSock     = SOCKET_ERROR;  // Avoid may be uninitialized warning.
  pDataContext = NULL;

  //
  // Get the parameters of the control connection.
  //
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pCtrlContext->PlainSocket == 0) {
    hSock = pCtrlContext->Session.Socket;
  }
  else
#endif
  {
    hSock = pCtrlContext->PlainSocket;
  }
  AddrSize = sizeof(Addr);
  FTPS_MEMSET(&Addr, 0, AddrSize);
  Error    = getsockname(hSock, (struct sockaddr*)&Addr, &AddrSize);
  if (Error == SOCKET_ERROR) {
    _PrintSocketError(DataSock);
  } else {
    //
    // Get a data socket and bind to local port 20 as this
    // might give an advantage with firewalls letting this
    // default port pass.
    //
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
    pAddr4 = NULL;  // Avoid not set warning.
    if (pCtrlContext->IPProtVer == PF_INET) {
      DataSock = socket(AF_INET, SOCK_STREAM, 0);   // Create a new socket for data connection to the client.
      if (DataSock != SOCKET_ERROR) {               // Socket created ?
        pAddr4                  = (struct sockaddr_in*)&Addr;
        pAddr4->sin_port        = htons(20);
      }
    }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
    pAddr6 = NULL;  // Avoid not set warning.
    if (pCtrlContext->IPProtVer == PF_INET6) {
      DataSock = socket(AF_INET6, SOCK_STREAM, 0);  // Create a new IPv6 socket for data connection to the client.
      if (DataSock != SOCKET_ERROR) {               // Socket created ?
        pAddr6                  = (struct sockaddr_in6*)&Addr;
        pAddr6->sin6_port       = htons(20);
      }
    }
#endif
    if (DataSock == SOCKET_ERROR) {
      _PrintSocketError(DataSock);
    } else {
      //
      // Bind to local port 20 and the IP address (and by this the interface)
      // of the control connection (if multiple interfaces exist).
      //
      Error = bind(DataSock, (struct sockaddr*)&Addr, sizeof(Addr));
      if (Error == SOCKET_ERROR) {
        _PrintSocketError(DataSock);
      } else {
        //
        // Allocate a context.
        //
        pDataContext = _AllocContext(DataSock, pCtrlContext->IPProtVer);
        if (pDataContext == NULL) {
          FTPS_APP_LOG(("APP: \nUnable to allocate context for data connection."));
        } else {
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
          if (pCtrlContext->PlainSocket == 0) {
            //
            // Check if data connection shall be secured too.
            //
            if (IP_FTPS_IsDataSecured(pCtrlContext->pContext) != 0) {
              pDataContext->IsSecure = 1;
            }
          }
#endif
          //
          // Get the IP address and other parameters of the control connection to the client
          // and change the port to the listening port of the client and connect to it.
          //
          AddrSize = sizeof(Addr);
          FTPS_MEMSET(&Addr, 0, AddrSize);
          getpeername(hSock, (struct sockaddr*)&Addr, &AddrSize);
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
          if (pCtrlContext->IPProtVer == PF_INET) {
            pAddr4->sin_port  = htons(Port);
          }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
          if (pCtrlContext->IPProtVer == PF_INET6) {
            pAddr6->sin6_port = htons(Port);
          }
#endif
          Error = connect(DataSock, (struct sockaddr*)&Addr, sizeof(Addr));
          if (Error == SOCKET_ERROR) {
            _PrintSocketError(DataSock);
            _FreeContext(pDataContext);
            pDataContext = NULL;
          }
        }
      }
      //
      // Cleanup in error cases.
      // Error cases have/set "pDataContext = NULL".
      //
      if (pDataContext == NULL) {
        if (DataSock != SOCKET_ERROR) {
          closesocket(DataSock);
        }
      }
    }
  }
  return (void*)pDataContext;
}

/*********************************************************************
*
*       _Disconnect()
*
*  Function description
*    This function is called from the FTP server module to close the
*    data connection.
*/
static void _Disconnect(FTPS_SOCKET hSock) {
  CONNECTION_CONTEXT* pContext;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  U32                 Timeout;
  char                c;
#endif

  if (hSock != NULL) {
    pContext = (CONNECTION_CONTEXT*)hSock;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
    if (pContext->IsSecure != 0) {
      if (pContext->Session.Socket != 0) {  // Only close if we ever had a connection.
        //
        // Some FTP clients like Filezilla expect a secure connection
        // to be established, whether payload gets sent or not.
        // Upgrading the connection is typically done when calling
        // send/recv for the first time on this connection. If this
        // is not done (no data) we need to do it here, right before
        // disconnecting/downgrading it again.
        //
        _UpgradeOnDemand(pContext);
        SSL_SESSION_Disconnect(&pContext->Session);
        //
        // FTP with SSL/TLS is a strange thing. The FTP protocol
        // expects the server to close the connection once all
        // data has been sent. When SSL/TLS is in use this means
        // the server closes/disconnects SSL/TLS. Once this is done
        // roles switch and the client seems to expect to close the
        // TCP connection first!
        // As a workaround we call recv() with timeout which will
        // either return 0 if the connection gets closed or error
        // on timeout (just used to avoid infinite wait).
        //
        Timeout = 1000;
        setsockopt(pContext->Session.Socket, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));
        recv(pContext->Session.Socket, &c, 1, 0);
        closesocket(pContext->Session.Socket);
      }
    }
    else
#endif
    {
      if (pContext->PlainSocket != 0) {  // Only close if we ever had a connection.
        closesocket(pContext->PlainSocket);
      }
    }
    _FreeContext(pContext);
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

    FTPS_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(LPort);
    Addr.sin_addr.s_addr = INADDR_ANY;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (IPProtVer == PF_INET6) {
    struct sockaddr_in6 Addr;

    FTPS_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin6_family      = AF_INET6;
    Addr.sin6_port        = htons(LPort);
    Addr.sin6_flowinfo   = 0;
    FTPS_MEMSET(&Addr.sin6_addr, 0, 16);
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
*   Backlog   - Number of connections held until accept() gets called.
*
* Return value
*   O.K. : Socket handle.
*   Error: SOCKET_ERROR .
*/
static int _ListenAtTcpPort(unsigned IPProtVer, U16 Port, int Backlog) {
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
      r = listen(hSock, Backlog);
      if (r != 0) {
        hSock = SOCKET_ERROR;
      }
    }
  }
  return hSock;
}

/*********************************************************************
*
*       _Listen()
*
*  Function description
*    This function is called from the FTP server module if the client
*    uses passive FTP. It creates a socket and searches for a free port
*    which can be used for the data connection.
*
*  Return value
*    > 0   Socket descriptor
*    NULL  Error
*/
static FTPS_SOCKET _Listen(FTPS_SOCKET CtrlSocket, U16* pPort, U8* pIPAddr) {
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
  struct sockaddr_in*  pAddr4;
  U32  IPAddr;
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
  struct sockaddr_in6* pAddr6;
  struct sockaddr_in6  Addr;
#else
  struct sockaddr_in   Addr;
#endif
  CONNECTION_CONTEXT*  pCtrlContext;
  CONNECTION_CONTEXT*  pDataContext;
  long DataSock;
  long hSock;
  int  AddrSize;

  (void)pPort;
  (void)pIPAddr;

  pCtrlContext = (CONNECTION_CONTEXT*)CtrlSocket;

  DataSock = _ListenAtTcpPort(pCtrlContext->IPProtVer, 0u, 1);  // Create a new socket for data connection to the client.
  if(DataSock == SOCKET_ERROR) {                                // Socket created ?
    return NULL;
  }
  //
  // Allocate an FTP context.
  //
  pDataContext = _AllocContext(DataSock, pCtrlContext->IPProtVer);
  if (pDataContext == NULL) {
    closesocket(DataSock);
    return NULL;
  }
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pCtrlContext->PlainSocket == 0) {
    hSock                 = pCtrlContext->Session.Socket;
    //
    // Check if data connection shall be secured too.
    //
    if (IP_FTPS_IsDataSecured(pCtrlContext->pContext) != 0) {
      pDataContext->IsSecure = 1;
    }
  }
  else
#endif
  {
    hSock                 = pCtrlContext->PlainSocket;
  }
  //
  //  Get port number and IP address assigned by the stack.
  //
#if ((APP_ENABLE_IPV4_PLAIN != 0) || (APP_ENABLE_IPV4_SSL != 0))
  if (pCtrlContext->IPProtVer == PF_INET) {
    pAddr4   = (struct sockaddr_in*)&Addr;
    AddrSize = sizeof(struct sockaddr_in);
    getsockname(DataSock, (struct sockaddr*)pAddr4, &AddrSize);
    *pPort   = htons(pAddr4->sin_port);
    getsockname(hSock, (struct sockaddr*)pAddr4, &AddrSize);
    IPAddr   = ntohl(pAddr4->sin_addr.s_addr);  // Load to host endianness.
    SEGGER_WrU32BE(pIPAddr, IPAddr);            // Save from host endianness to network endianness.
  }
#endif
#if ((APP_ENABLE_IPV6_PLAIN != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pCtrlContext->IPProtVer == PF_INET6) {
    pAddr6   = (struct sockaddr_in6*)&Addr;
    AddrSize = sizeof(struct sockaddr_in6);
    getsockname((long)DataSock, (struct sockaddr*)&Addr, &AddrSize);
    *pPort   = htons(pAddr6->sin6_port);
    getsockname(hSock, (struct sockaddr*)pAddr6, &AddrSize);
    FTPS_MEMCPY(pIPAddr, &pAddr6->sin6_addr.Union.aU8[0], IPV6_ADDR_LEN);
  }
#endif
  return (FTPS_SOCKET)pDataContext;
}

/*********************************************************************
*
*       _Accept()
*
*  Function description
*    This function is called from the FTP server module if the client
*    uses passive FTP. It sets the command socket to non-blocking before
*    accept() will be called. This guarantees that the FTP server always
*    returns even if the connection to the client gets lost while
*    accept() waits for a connection. The timeout is set to 10 seconds.
*
*  Return value
*    0    O.K.
*   -1    Error
*/
static int _Accept(FTPS_SOCKET CtrlSocket, FTPS_SOCKET* pSock) {
  CONNECTION_CONTEXT* pDataContext;
  long hSock;
  long DataSock;
  int  SoError;
  int  t0;
  int  t;
  int  Opt;

  (void)CtrlSocket;

  pDataContext = *(CONNECTION_CONTEXT**)pSock;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pDataContext->PlainSocket == 0) {
    hSock = pDataContext->Session.Socket;
  }
  else
#endif
  {
    hSock = pDataContext->PlainSocket;
  }
  //
  // Set command socket non-blocking
  //
  Opt = 1;
  setsockopt(hSock, SOL_SOCKET, SO_NONBLOCK, &Opt, sizeof(Opt));
  t0 = IP_OS_GET_TIME();
  do {
    DataSock = accept(hSock, NULL, NULL);
    if ((DataSock != SOCKET_ERROR) && (DataSock != 0)) {
      //
      // Set data socket blocking. The data socket inherits the blocking
      // mode from the socket that was used as parameter for accept().
      // Therefore, we have to set it blocking after creation.
      //
      Opt = 0;
      setsockopt(DataSock, SOL_SOCKET, SO_NONBLOCK, &Opt, sizeof(Opt));
      //
      // SO_KEEPALIVE is required to guarantee that the socket will be
      // closed even if the client has lost the connection to the server
      // before it closes the connection.
      //
      Opt = 1;
      setsockopt(DataSock, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
      if (pDataContext->PlainSocket == 0) {
        pDataContext->Session.Socket = DataSock;
      }
      else
#endif
      {
        pDataContext->PlainSocket    = DataSock;
      }
      closesocket(hSock);
      return 0;                  // Successfully connected
    }
    getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    if (SoError != IP_ERR_WOULD_BLOCK) {
      closesocket(hSock);
      _FreeContext(pDataContext);
      FTPS_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(SoError)));
      return SOCKET_ERROR;       // Not in progress and not successful, error...
    }
    t = IP_OS_GET_TIME() - t0;
    if (t >= 10000) {
      closesocket(hSock);
      _FreeContext(pDataContext);
      return SOCKET_ERROR;
    }
    OS_Delay(1);                 // Give lower prior tasks some time
  } while (1);
}

#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
/*********************************************************************
*
*       _SetSecure()
*
*  Function description
*    Update the connection to a secured one.
*/
static int _SetSecure(FTPS_SOCKET hSock, FTPS_SOCKET hSockClone) {
  CONNECTION_CONTEXT* pSock;
  CONNECTION_CONTEXT* pSockClone;

  pSock                = (CONNECTION_CONTEXT*)hSock;
  pSockClone           = (CONNECTION_CONTEXT*)hSockClone;
  pSock->IsSecure      = 1;
  pSockClone->IsSecure = 1;
  return 0;
}
#endif

/*********************************************************************
*
*       IP_FTPS_API
*
*  Description
*    IP stack function table
*/
static const IP_FTPS_API _IP_API = {
  _Send,
  _Recv,
  _Connect,
  _Disconnect,
  _Listen,
  _Accept,
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  _SetSecure
#else
  NULL
#endif
};

//
// To send a simple connection limit reached we use an API table with only a plain send callback.
//
static const IP_FTPS_API _IP_API_SendPlainSock = {
  _SendPlainSock,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

/*********************************************************************
*
*       FTPS_SYS_API
*
*  Description
*    Memory handling function table
*/
static const FTPS_SYS_API _Sys_API = {
  _Alloc,
  IP_Free
};

/**************************************************************************************************************************************************************
*
*       User management.
*/

/*********************************************************************
*
*       _FindUser()
*
*  Function description
*    Callback function for user management.
*    Checks if user name is valid.
*
*  Return value
*    0    UserID invalid or unknown
*  > 0    UserID, no password required
*  < 0    - UserID, password required
*/
static int _FindUser (const char * sUser) {
  if (FTPS_STRCMP(sUser, "Admin") == 0) {
    return (0 - USER_ID_ADMIN);
  }
  if (FTPS_STRCMP(sUser, "anonymous") == 0) {
    return USER_ID_ANONYMOUS;
  }
  return 0;
}

/*********************************************************************
*
*       _CheckPass()
*
*  Function description
*    Callback function for user management.
*    Checks user password.
*
*  Return value
*    0    UserID known, password valid
*    1    UserID unknown or password invalid
*/
static int _CheckPass (int UserId, const char * sPass) {
  if ((UserId == USER_ID_ADMIN) && (FTPS_STRCMP(sPass, "Secret") == 0)) {
    return 0;
  } else {
    return 1;
  }
}

/*********************************************************************
*
*       _GetDirInfo()
*
*  Function description
*    Callback function for permission management.
*    Checks directory permissions.
*
*  Return value
*    Returns a combination of the following:
*    IP_FTPS_PERM_VISIBLE    - Directory is visible as a directory entry
*    IP_FTPS_PERM_READ       - Directory can be read/entered
*    IP_FTPS_PERM_WRITE      - Directory can be written to
*
*  Parameters
*    UserId        - User ID returned by _FindUser()
*    sDirIn        - Full directory path and with trailing slash
*    sDirOut       - Reserved for future use
*    DirOutSize    - Reserved for future use
*
*  Notes
*    In this sample configuration anonymous user is allowed to do anything.
*    Samples for folder permissions show how to set permissions for different
*    folders and users. The sample configures permissions for the following
*    directories:
*      - /READONLY/: This directory is read only and can not be written to.
*      - /VISIBLE/ : This directory is visible from the folder it is located
*                    in but can not be accessed.
*      - /ADMIN/   : This directory can only be accessed by the user "Admin".
*/
static int _GetDirInfo(int UserId, const char * sDirIn, char * sDirOut, int DirOutSize) {
  int Perm;

  (void)sDirOut;
  (void)DirOutSize;

  //
  // Generic permissions.
  //  Anonymous : IP_FTPS_PERM_VISIBLE |
  //              IP_FTPS_PERM_READ
  //  Valid user: IP_FTPS_PERM_VISIBLE |
  //              IP_FTPS_PERM_READ    |
  //              IP_FTPS_PERM_WRITE
  //
  if (UserId == USER_ID_ANONYMOUS) {
    Perm = IP_FTPS_PERM_VISIBLE | IP_FTPS_PERM_READ;
  } else {
    Perm = IP_FTPS_PERM_VISIBLE | IP_FTPS_PERM_READ | IP_FTPS_PERM_WRITE;
  }

  if (FTPS_STRCMP(sDirIn, "/READONLY/") == 0) {
    Perm = IP_FTPS_PERM_VISIBLE | IP_FTPS_PERM_READ;
  }
  if (FTPS_STRCMP(sDirIn, "/VISIBLE/") == 0) {
    Perm = IP_FTPS_PERM_VISIBLE;
  }
  if (FTPS_STRCMP(sDirIn, "/ADMIN/") == 0) {
    if (UserId != USER_ID_ADMIN) {
      return 0;  // Only Admin is allowed for this directory
    }
  }
  return Perm;
}

/*********************************************************************
*
*       _GetFileInfo()
*
*  Function description
*    Callback function for permission management.
*    Checks file permissions.
*
*  Return value
*    Returns a combination of the following:
*    IP_FTPS_PERM_VISIBLE    - File is visible as a file entry
*    IP_FTPS_PERM_READ       - File can be read
*    IP_FTPS_PERM_WRITE      - File can be written to
*
*  Parameters
*    UserId        - User ID returned by _FindUser()
*    sFileIn       - Full path to the file
*    sFileOut      - Reserved for future use
*    FileOutSize   - Reserved for future use
*
*  Notes
*    In this sample configuration all file accesses are allowed. File
*    permissions are checked against directory permissions. Therefore it
*    is not necessary to limit permissions on files that reside in a
*    directory that already limits access.
*    Setting permissions works the same as for _GetDirInfo() .
*/
static int _GetFileInfo(int UserId, const char * sFileIn, char * sFileOut, int FileOutSize) {
  int Perm;

  (void)UserId;
  (void)sFileIn;
  (void)sFileOut;
  (void)FileOutSize;

  Perm = IP_FTPS_PERM_VISIBLE | IP_FTPS_PERM_READ | IP_FTPS_PERM_WRITE;

  return Perm;
}

/*********************************************************************
*
*       FTPS_ACCESS_CONTROL
*
*  Description
*   Access control function table
*/
static FTPS_ACCESS_CONTROL _Access_Control = {
  _FindUser,
  _CheckPass,
  _GetDirInfo,
  _GetFileInfo  // Optional, only required if permissions for individual files shall be used
};

/*********************************************************************
*
*       _GetTimeDate()
*
*  Description:
*    Current time and date in a format suitable for the FTP server.
*
*    Bit 0-4:   2-second count (0-29)
*    Bit 5-10:  Minutes (0-59)
*    Bit 11-15: Hours (0-23)
*    Bit 16-20: Day of month (1-31)
*    Bit 21-24: Month of year (1-12)
*    Bit 25-31: Count of years from 1980 (0-127)
*
*  Notes
*    FTP server requires a real time clock for to transmit the
*    correct timestamp of files. Lists transmits either the
*    year or the HH:MM. For example:
*    -rw-r--r--   1 root 1000 Jan  1  1980 DEFAULT.TXT
*    or
*    -rw-r--r--   1 root 1000 Jul 29 11:40 PAKET01.TXT
*    The decision which of both infos the server transmits
*    depends on the system time. If the year of the system time
*    is identical to the year stored in the timestamp of the file,
*    the time will be transmitted, if not the year.
*/
static U32 _GetTimeDate(void) {
  U32 r;
  U16 Sec, Min, Hour;
  U16 Day, Month, Year;

  Sec   = 0;        // 0 based.  Valid range: 0..59
  Min   = 0;        // 0 based.  Valid range: 0..59
  Hour  = 0;        // 0 based.  Valid range: 0..23
  Day   = 1;        // 1 based.    Means that 1 is 1. Valid range is 1..31 (depending on month)
  Month = 1;        // 1 based.    Means that January is 1. Valid range is 1..12.
  Year  = 0;        // 1980 based. Means that 2008 would be 28.
  r   = Sec / 2 + (Min << 5) + (Hour  << 11);
  r  |= (U32)(Day + (Month << 5) + (Year  << 9)) << 16;
  return r;
}

/*********************************************************************
*
*       FTPS_APPLICATION
*
*  Description
*   Application data table, defines all application specifics used by the FTP server
*/
static const FTPS_APPLICATION _Application = {
  &_Access_Control,
  _GetTimeDate
};

/*********************************************************************
*
*       _AddToConnectCnt()
*
*/
static void _AddToConnectCnt(int Delta) {
  OS_EnterRegion();
  _ConnectCnt += Delta;
  OS_LeaveRegion();
}

/*********************************************************************
*
*       _FTPServerChildTask()
*/
static void _FTPServerChildTask(void* pContext) {
  CONNECTION_CONTEXT* pAppContext;
  FTPS_CONTEXT        FTPSContext;
  long                hSock;
  int                 Opt;
  U32                 Timeout;

  _pFS_API    = &IP_FS_emFile;
  pAppContext = (CONNECTION_CONTEXT*)pContext;
  hSock       = pAppContext->PlainSocket;
  Opt         = 1;
  //
  pAppContext->pContext = (FTPS_CONTEXT*)&FTPSContext;
  setsockopt(hSock, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
  Timeout = IDLE_TIMEOUT;
  setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(Timeout));
  IP_FTPS_Init(&FTPSContext, &_IP_API, _pFS_API, &_Application, &_Sys_API);
#if ALLOW_SECURE_ONLY
  //
  // Requires the user to use only secured connection (for data too).
  //
  IP_FTPS_AllowOnlySecured(&FTPSContext, 1);
#endif
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  if (pAppContext->IsImplicit != 0) {  // Is this an FTPS (implicit TLS) connection ?
    IP_FTPS_SetImplicitMode(&FTPSContext);
    pAppContext->IsSecure = 1u;
  }
#endif
#if 0
  //
  // If using FTP implicit mode (port 990).
  //
  IP_FTPS_SetImplicitMode(&FTPSContext);
#endif
  //
  IP_FTPS_ProcessEx(&FTPSContext, pContext);
  closesocket(hSock);
  FTPS_APP_LOG(("Closed FTP connection (socket 0x%x)", pAppContext->PlainSocket));
  _FreeContext(pAppContext);
  _AddToConnectCnt(-1);
  OS_Terminate(0);
}

/*********************************************************************
*
*       _FTPServerParentTask()
*/
static void _FTPServerParentTask(void) {
  CONNECTION_CONTEXT* pContext;
  IP_fd_set           ReadFds;
  U32                 NumBytes;
  unsigned            IPProtVer;
#if APP_ENABLE_IPV4_PLAIN
  long                hSockParent4_Plain;
#endif
#if APP_ENABLE_IPV4_SSL
  long                hSockParent4_SSL;
#endif
#if APP_ENABLE_IPV6_PLAIN
  long                hSockParent6_Plain;
#endif
#if APP_ENABLE_IPV6_SSL
  long                hSockParent6_SSL;
#endif
  long                hSock;
  int                 i;
  int                 r;
  int                 IFaceId;
  FTPS_BUFFER_SIZES   BufferSizes;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  char                IsSSL;  // Connection started directly as implicit FTPS connection.
#endif

  hSock = 0;  // Avoid warning about uninitialized variable.

  //
  // Configure buffer size.
  //
  IFaceId = IP_INFO_GetNumInterfaces() - 1;                            // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  FTPS_MEMSET(&BufferSizes, 0, sizeof(BufferSizes));
  BufferSizes.NumBytesInBuf            = FS_SECTOR_SIZE;
  BufferSizes.NumBytesInBufBeforeFlush = FS_SECTOR_SIZE;               // Fill the InBuffer completely before writing the data to the filesystem.
  BufferSizes.NumBytesOutBuf           = IP_TCP_GetMTU(IFaceId) - 72;  // Use max. MTU configured for the last interface added minus worst case IPv4/TCP/VLAN headers.
                                                                       // Calculation for the memory pool is done under assumption of the best case headers with - 40 bytes.
  BufferSizes.NumBytesCwdNameBuf       = FTPS_MAX_PATH_DIR;
  BufferSizes.NumBytesPathNameBuf      = FTPS_MAX_PATH;
  BufferSizes.NumBytesDirNameBuf       = FTPS_MAX_PATH;
  BufferSizes.NumBytesFileNameBuf      = FTPS_MAX_FILE_NAME;
  //
  // Configure the size of the buffers used by the FTP server child tasks.
  //
  IP_FTPS_ConfigBufSizes(&BufferSizes);
  //
  // Check memory pool size.
  //
  NumBytes = IP_FTPS_CountRequiredMem(NULL);     // Get NumBytes for internals of one child thread.
  NumBytes = (NumBytes + 64) * MAX_CONNECTIONS;  // Calc. the total amount for x connections (+ some bytes for managing a memory pool).
  FTPS_APP_LOG(("FTPS: Using a memory pool of %lu bytes for %lu connections.", sizeof(_aPool), MAX_CONNECTIONS));
  if (NumBytes > sizeof(_aPool)) {
    FTPS_APP_WARN(("FTPS: Memory pool should be at least %lu bytes.", NumBytes));
  }
  //
  // Give the stack some more memory to enable the dynamical memory allocation for the FTP server child tasks
  //
  IP_AddMemory(_aPool, sizeof(_aPool));
  //
  // Try until we get valid parent sockets.
  //
#if APP_ENABLE_IPV4_PLAIN
  while (1) {
    hSockParent4_Plain = _ListenAtTcpPort(PF_INET, SERVER_PORT_PLAIN, BACK_LOG);
    if (hSockParent4_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV4_SSL
  while (1) {
    hSockParent4_SSL = _ListenAtTcpPort(PF_INET, SERVER_PORT_SSL, BACK_LOG);
    if (hSockParent4_SSL == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV6_PLAIN
  while (1) {
    hSockParent6_Plain = _ListenAtTcpPort(PF_INET6, SERVER_PORT_PLAIN, BACK_LOG);
    if (hSockParent6_Plain == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
#endif
#if APP_ENABLE_IPV6_SSL
  while (1) {
    hSockParent6_SSL = _ListenAtTcpPort(PF_INET6, SERVER_PORT_SSL, BACK_LOG);
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
  for (;;) {
    IPProtVer = PF_INET;
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
    IsSSL     = 0;
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
      FTPS_APP_LOG(("New IPv4 client accepted."));
      IPProtVer = PF_INET;
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
      FTPS_APP_LOG(("New IPv4 SSL client accepted."));
      IPProtVer = PF_INET;
      IsSSL     = 1;
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
      FTPS_APP_LOG(("New IPv6 client accepted."));
      IPProtVer = PF_INET6;
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
      FTPS_APP_LOG(("New IPv6 SSL client accepted."));
      IPProtVer = PF_INET6;
      IsSSL     = 1;
      goto QueueConnection;
    }
#endif
QueueConnection:
    if (_ConnectCnt < MAX_CONNECTIONS) {
      for (i = 0; i < MAX_CONNECTIONS; i++) {
        r = OS_IsTask(&_aFTPsTasks[i]);
        if (r == 0) {
          //
          // Alloc an FTP context.
          //
          pContext = _AllocContext(hSock, IPProtVer);
          if (pContext == NULL) {
            break;
          }
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
          pContext->IsImplicit = IsSSL;
#endif
          _AddToConnectCnt(1);
          OS_CREATETASK_EX(&_aFTPsTasks[i], "FTP server - Child", _FTPServerChildTask, APP_TASK_PRIO_FTP_SERVER_CHILD, _aFTPsStacks[i], (void*)pContext);
          break;
        }
      }
    } else {
      IP_FTPS_OnConnectionLimit(&_IP_API_SendPlainSock, (void*)hSock);
      OS_Delay(2000);          // Give connection some time to complete
      closesocket(hSock);
    }
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
  OS_SetTaskName(OS_GetTaskID(), "FTP server - Parent");
#if ((APP_ENABLE_IPV4_SSL != 0) || (APP_ENABLE_IPV6_SSL != 0))
  //
  // Initialize SSL.
  //
  SSL_Init();
#endif
  _FTPServerParentTask();
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
  OS_CREATETASK(&APP_MainTCB, "APP_MainTask", APP_MainTask, APP_TASK_PRIO_FTP_SERVER_PARENT, APP_MainStack);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
