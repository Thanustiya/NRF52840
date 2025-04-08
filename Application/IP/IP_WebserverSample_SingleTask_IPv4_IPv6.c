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

Purpose : Sample program for emWeb.
          The sample demonstrates how the Web server can be used with
          one task. The sample can be used with IPv4 and IPv6.
          To enable IPv6 USE_V6 has to be set.
          All connection requests (sockets) are queued and processed
          sequentially. Every socket is closed after processing.
          This sample is suitable for targets with limited memory.

          Please note, in projects where multiple clients connect
          in parallel the standard Web server sample
          (IP_WebServerSample.c) should be used.

Additional Information:

          IP_WebserverSample_SingleTask_IPv4_IPv6.c has been tested
          with the following configuration:

          ===========
          - emNet:
          ===========
          #define ALLOC_SIZE 0x3000                         // 12KBytes RAM.
          mtu = 1280;                                       // 576 is minimum for IPv4, 1280 minimum for IPv6, 1500 is max. for Ethernet.
          IP_SetMTU(0, mtu);                                // Maximum Transmission Unit is 1500 for Ethernet by default.
          IP_AddBuffers(4, 256);                            // Small buffers.
          IP_AddBuffers(4, mtu + 16);                       // Big buffers. Size should be mtu + 16 byte for Ethernet header (2 bytes type, 2*6 bytes MAC, 2 bytes padding).
          IP_ConfTCPSpace(2 * (mtu - 40), 2 * (mtu - 40));  // Define the TCP Tx and Rx window size. At least Tx space for 2*(mtu-40/60) for two full TCP packets is needed.

          ======================
          - emWeb:
          ======================
          The used memory pool (WEBS_ALLOC_SIZE) of 1536 bytes
          consists of:

          WEBS_IN_BUFFER_SIZE              256              // Buffer size should be at least 256 bytes.
          WEBS_OUT_BUFFER_SIZE            1420              // The buffer size is derived from the used MTU.
                                                            // Please note, that the default is 1460 bytes and has to changed manually in WEBS_Conf.h.
                                                            // The size of the out buffer should aligned with the used MTU:
                                                            //       MTU - header size of the used protocols.
                                                            // E.g.: MTU = 576. IPv4 header = 20 bytes. TCP header size = 20 bytes.
                                                            //       MTU - 40 bytes.
                                                            // If IPv6 is the mostly used protocol, the buffer should be adapted.
                                                            // E.g.: MTU = 1280. IPv4 header = 40 bytes. TCP header size = 20 bytes.
                                                            //       MTU - 60 bytes.
                                                            // Please note, the size of the out buffer can be smaller as the MTU - protocol overhead,
                                                            // but a smaller out buffer size will limit the performance.
          WEBS_PARA_BUFFER_SIZE            256              // Required for dynamic content parameter handling.
          WEBS_FILENAME_BUFFER_SIZE         64
          WEBS_MAX_ROOT_PATH_LEN            12
          WEBS_SUPPORT_UPLOAD                0
          WEBS_USE_AUTH_DIGEST               0
          + Read-only file system

          The size of the memory pool can be fine tuned according
          to your configuration using IP_WEBS_CountRequiredMem().

          A typical webpage used for the tests consists of 3 elements:
          - 1 x html file
          - 1 x CSS file
          - 1 x gif

          Tested with the following browsers:
            - Mozilla Firefox 54.0.1
            - Google Chrome 60.0.3112.90
            - Microsoft Edge
            - Internet Explorer 11
            - Safari 9.1.2

          Features like Server-Sent-Events will work, since a reconnect
          in cases of connection loss is provided, but this erodes the
          idea of a persistent connection.
*/

#include <stdio.h>
#include <stdlib.h>
#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_Webserver.h"
#include "WEBS_Conf.h"        // Stack size depends on configuration


/*********************************************************************
*
*       Local Types
*
**********************************************************************
*/
typedef struct {
  int hSock;
} _CONN_QUEUE;


/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/
#define USE_RX_TASK   0                // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define USE_IPV6      IP_SUPPORT_IPV6  // Open an additional listening IPv6 port.

//
// Web server and IP stack
//
#define SERVER_PORT              80
#define WEBS_ALLOC_SIZE        2560   // NumBytes required from memory pool. Should be fine tuned according to your configuration using IP_WEBS_CountRequiredMem() .
#define MAX_CONNECTION_QUEUE     20   // Max. number of connections we accept and queue.

//
// Task priorities.
//
enum {
  TASK_PRIO_WEBS = 150
  ,TASK_PRIO_IP_TASK           // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK        // Must be the highest priority of all IP related tasks.
#endif
};

//
// UDP discover
//
#define ETH_UDP_DISCOVER_PORT    50020
#define PACKET_SIZE              0x80

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U32 _aPool[WEBS_ALLOC_SIZE / sizeof(int)];  // Memory pool for the Web server child tasks.

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

static long               _hSockListenV4;
static IP_TCP_ACCEPT_HOOK _HandleIPv4;

#if USE_IPV6
static long               _hSockListenV6;
static IP_TCP_ACCEPT_HOOK _HandleIPv6;
#endif

//
// Connection queue
//
static _CONN_QUEUE        _aQueueConn[MAX_CONNECTION_QUEUE];  // Like a ring buffer
static int                _iQueuePop;                         // Points to pos. in queue where to pop the next queued connection
static int                _iQueuePush;                        // Points to pos. in queue where to push the next connection
static unsigned           _QueueFillCnt;                      // Used to avoid the need of division for "free space" in queue, as an entry is not 1 byte in size but multiple ones, so pure pointer arithmetic does not do the trick
static WEBS_CONTEXT       _WebsContext;

//
// File system info
//
static const IP_FS_API*   _pFS_API;

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
*       _closesocket()
*
*  Function description
*    Wrapper for closesocket()
*/
static int _closesocket(long pConnectionInfo) {
  int r;
  struct linger Linger;
  long hSock;

  hSock = (long)pConnectionInfo;
  Linger.l_onoff  = 1;  // Enable linger for this socket to verify that all data is send.
  Linger.l_linger = 1;  // Linger timeout in seconds
  setsockopt(hSock, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));
  r = closesocket(hSock);
  IP_Logf_Application("WS: Socket no. %d closed.", hSock);
  return r;
}

/*********************************************************************
*
*       _Recv()
*
*  Function description
*    Wrapper for recv()
*/
static int _Recv(unsigned char *buf, int len, void *pConnectionInfo) {
  int r;

  r = recv((long)pConnectionInfo, (char *)buf, len, 0);
  return r;
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Wrapper for send()
*/
static int _Send(const unsigned char *buf, int len, void* pConnectionInfo) {
  int r;

  r = send((long)pConnectionInfo, (const char *)buf, len, 0);
  return r;
}

/*********************************************************************
*
*       WEBS_IP_API
*
*  Description
*   IP related function table
*/
static const WEBS_IP_API _Webs_IP_API = {
  _Send,
  _Recv
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
#if USE_IPV6
  //
  // Create IPv6 socket
  //
  if (IPProtVer == PF_INET6) {
    hSock = socket(AF_INET6, SOCK_STREAM, 0);
  }
#endif
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

  r = -1;

  //
  // Bind it to the port
  //
#if USE_IPV6
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
      r = listen(hSock, MAX_CONNECTION_QUEUE);
      if (r != 0) {
        hSock = SOCKET_ERROR;
      }
    }
  }
  return hSock;
}

/*********************************************************************
*
*       _ConnQueuePop()
*
*  Function description
*    Pops an entry from the connection queue, if there is anything to be popped
*
*  Return value
*    >= 0  O.K., entry popped
*     < 0  Error, no entry has been popped (most likely because there are no more entries)
*
*  Notes
*    (1) Called from multiple threads with interrupts and task changes enabled
*/
static int _ConnQueuePop(int* pSock) {
  int r;
  int i;

  r = -1;
  OS_EnterRegion();  // Make sure that queue is not touched by Webserver accept callback or any other child task, while we modify it
  if (_QueueFillCnt) {
    i = _iQueuePop;
    *pSock = _aQueueConn[i].hSock;
    i++;
    if (i >= MAX_CONNECTION_QUEUE) {  // Handle wrap-around
      i = 0;
    }
    _iQueuePop = i;
    _QueueFillCnt--;
    r = 0;
  }
  OS_LeaveRegion();
  return r;
}

/*********************************************************************
*
*       _ConnQueuePush()
*
*  Function description
*    Pushes an entry onto the connection queue, if there is any space left
*
*  Return value
*    >= 0  O.K., entry pushed
*     < 0  Error, entry not pushed (most likely because queue is full)
*
*  Notes
*    (1) Called from multiple threads with interrupts and task changes enabled
*/
static int _ConnQueuePush(int Sock) {
  int r;
  int i;

  r = -1;
  OS_EnterRegion();  // Make sure that queue is not touched by Webserver accept callback or any other child task, while we modify it
  if (_QueueFillCnt < MAX_CONNECTION_QUEUE) {
    i = _iQueuePush;
    _aQueueConn[i].hSock = Sock;
    i++;
    if (i >= MAX_CONNECTION_QUEUE) {  // Handle wrap-around
      i = 0;
    }
    _iQueuePush = i;
    _QueueFillCnt++;
    r = 0;
  }
  OS_LeaveRegion();
  return r;
}

/*********************************************************************
*
*       _cbAccept()
*
*  Function description
*    Called by IP stack each time a new connection from a client is opened.
*
*  Parameters
*    hSock   : Accepted child socket.
*    pInfo   : Connection information.
*    pContext: Custom context as registered with the accept hook.
*
*  Notes
*    (1) Must not block under any circumstances (e.g. only call send() for non-blocking sockets etc.)
*    (2)
*/
static void _cbAccept(int hSock, IP_TCP_ACCEPT_INFO* pInfo, void* pContext) {
  int Opt;
  int r;

  //
  // We queue the connections in a static list.
  //
  IP_USE_PARA(pContext);
  IP_USE_PARA(pInfo);
  IP_Logf_Application("WS: New sock %d.", hSock);
  //
  // Enable keep alive probes on new socket.
  //
  Opt   = 1;
  setsockopt(hSock, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
  //
  // Connection cannot be handled immediately? => Queue it, if possible
  //
  IP_Logf_Application("WS: Queue sock %d", hSock);
  r = _ConnQueuePush(hSock);
  if (r < 0) {
    IP_Logf_Application("WS: Cannot queue sock %d", hSock);
    _closesocket(hSock);
  }
}

/*********************************************************************
*
*       _HandleConnection
*/
static void _HandleConnection(WEBS_CONTEXT* pWebsContext) {
  int          hSock;
  int          NumBytes;
  int          r;
  int          v;

  do {
    //
    // Check if we have a queued socket.
    //
    v = _ConnQueuePop(&hSock);
    if (v >= 0) {
      //
      // Check if data is available in the socket buffer
      //
      getsockopt(hSock, SOL_SOCKET, SO_RXDATA, &NumBytes, sizeof(NumBytes));
      if (NumBytes > 0) {
        r = IP_WEBS_ProcessLastEx(pWebsContext, (void*)hSock, NULL);
        IP_Logf_Application("WS: Sock %d, Return value: %d", hSock, r);
      }
      _closesocket(hSock);
    }
  } while (v >= 0);
}

/*********************************************************************
*
*       _WebServerInit
*/
static void _WebServerInit(void) {
  WEBS_BUFFER_SIZES BufferSizes;
  U32               NumBytes;

  //
  // Assign file system
  //
  _pFS_API = &IP_FS_ReadOnly;  // To use a a real filesystem like emFile replace this line.
//  _pFS_API = &IP_FS_emFile;    // Use emFile
//  IP_WEBS_AddUpload();         // Enable upload
  //
  // Configure buffer size.
  //
  IP_MEMSET(&BufferSizes, 0, sizeof(BufferSizes));
  BufferSizes.NumBytesInBuf       = WEBS_IN_BUFFER_SIZE;
#if USE_IPV6
  BufferSizes.NumBytesOutBuf      = IP_TCP_GetMTU(_IFaceId) - 40 - 20;  // Use max. MTU configured for the last interface added minus IPv6(40 bytes)/TCP(20 bytes) headers.
                                                                        // Calculation for the memory pool is done under assumption of the best case headers with - 60 bytes for IPv6.
#else
  BufferSizes.NumBytesOutBuf      = IP_TCP_GetMTU(_IFaceId) - 20 - 20;  // Use max. MTU configured for the last interface added minus IPv4(20 bytes)/TCP(20 bytes) headers.
                                                                        // Calculation for the memory pool is done under assumption of the best case headers with - 40 bytes for IPv4.
#endif
  BufferSizes.NumBytesParaBuf     = WEBS_PARA_BUFFER_SIZE;
  BufferSizes.NumBytesFilenameBuf = WEBS_FILENAME_BUFFER_SIZE;
  BufferSizes.MaxRootPathLen      = WEBS_MAX_ROOT_PATH_LEN;
  //
  // Configure the size of the buffers used by the Webserver child tasks.
  //
  IP_WEBS_ConfigBufSizes(&BufferSizes);
  //
  // Check memory pool size.
  //
  NumBytes = IP_WEBS_CountRequiredMem(NULL);     // Get NumBytes for internals of one child thread.
  NumBytes = NumBytes + 64;                      // Total amount + some bytes for managing a memory pool.
  IP_Logf_Application("WEBS: Using a memory pool of %lu bytes. Min. required: %lu", sizeof(_aPool), NumBytes);
  if (NumBytes > sizeof(_aPool)) {
    IP_Warnf_Application("WEBS: Memory pool should be at least %lu bytes.", NumBytes);
  }
  //
  // Give the stack some more memory to enable the dynamical memory allocation for the Web server child tasks
  //
  IP_AddMemory(_aPool, sizeof(_aPool));
  //
  // Get a valid IPv4 socket listening on port 80
  //
  while (1) {
    _hSockListenV4 = _ListenAtTcpPort(PF_INET, SERVER_PORT);
    if (_hSockListenV4 == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
  IP_TCP_Accept(&_HandleIPv4, &_cbAccept, _hSockListenV4, NULL);
#if USE_IPV6
  //
  // Get a valid IPv6 socket listening on port 80
  //
  while (1) {
    _hSockListenV6 = _ListenAtTcpPort(PF_INET6, SERVER_PORT);
    if (_hSockListenV6 == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
  IP_TCP_Accept(&_HandleIPv6, &_cbAccept, _hSockListenV6, NULL);
#endif
  //
  // Initialize the context of the Web server task.
  //
  IP_WEBS_Init(&_WebsContext, &_Webs_IP_API, &_Webs_SYS_API, _pFS_API, &WebsSample_Application);
}

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
#if ETH_UDP_DISCOVER_PORT
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
  if (memcmp(pInData, "Discover", 8)) {
    goto Done;
  }
  //
  // Alloc packet
  //
  pOutPacket = IP_UDP_AllocEx(IFaceId, PACKET_SIZE);
  if (pOutPacket == NULL) {
    goto Done;
  }
  //
  // Fill packet with data
  //
  pOutData = (char*)IP_UDP_GetDataPtr(pOutPacket);
  IP_UDP_GetSrcAddr(pInPacket, &TargetAddr, sizeof(TargetAddr));    // Send back Unicast
  memset(pOutData, 0, PACKET_SIZE);
  strcpy(pOutData + 0x00, "Found");
  IPAddr = htonl(IP_GetIPAddr(IFaceId));
  memcpy(pOutData + 0x20, (void*)&IPAddr, 4);      // 0x20: IP address
  IP_GetHWAddr(IFaceId, (U8*)pOutData + 0x30, 6);  // 0x30: MAC address
  //
  // Send packet
  //
  IP_UDP_SendAndFree(IFaceId, TargetAddr, ETH_UDP_DISCOVER_PORT, ETH_UDP_DISCOVER_PORT, pOutPacket);
Done:
  return IP_OK;
}
#endif

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*/
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  //
  // Start TCP/IP task
  //
  OS_CREATETASK(&_IPTCB,   "IP_Task",   IP_Task,   TASK_PRIO_IP_TASK,    _IPStack);
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  //
  // IPv4 address configured ?
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(0);
    OS_Delay(200);
  }
  BSP_ClrLED(0);
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_WEBS);                                  // This task has highest prio!
  OS_SetTaskName(OS_GetTaskID(), "IP_WebServer");
#if ETH_UDP_DISCOVER_PORT
  //
  // Open UDP port ETH_UDP_DISCOVER_PORT for emNet discover
  //
  IP_UDP_Open(0L /* any foreign host */,  ETH_UDP_DISCOVER_PORT, ETH_UDP_DISCOVER_PORT,  _OnRx, 0L);
#endif
  IP_WEBS_X_SampleConfig();  // Load a web server sample config that might add other resources like REST.
  _WebServerInit();
  //
  // Handle connection requests.
  //
  while(1) {
    _HandleConnection(&_WebsContext);
    BSP_ToggleLED(0);
    OS_Delay(50);
  }
}

/*************************** End of file ****************************/
