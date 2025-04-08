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

Purpose : Speed client for TCP/IP stack using socket interface.

Additional information:
  This application can be used to connect to a server
  running the SpeedTestServer pc application and run a speed test.

  Preparations:
    The speed client can be configured in a number of ways via
    configuration defines that can freely be overridden by the user.
    These defines include:
      SERVER_IP_ADDR:         IP address of the server
      SERVER_PORT:            Port the server is listening on
      NUMBER_OF_BYTES:        Number of bytes you want to transmit
      SPEEDCLIENT_NUM_CHUNKS: Number of full packets to try and transfer at once
      BUFFER_SIZE:            Maximum amount of bytes we can transfer at once

    Additionally you can configure the client behavior
    via the DIRECTION define:
      1:  Only receive from server
      2:  Only send to server
      3:  Take turns sending and receiving from and to the server

  Expected behavior:
    The TCP speed client will connect to a server application counterpart
    on a pc to run a transmission speed test. The client and server will
    (by default) take turns sending packets of a predefined size to each other
    while measuring the time per packet and calculating
    an average speed of the transmission.

  Sample output:
    ...
    4:973 Client - 4194304 Bytes received (without headers) in 901 ms.
    4:973 Client - 4349446 Bytes received (with headers) in 901 ms.
    4:973 Client - Average transfer speed (without headers): 4655000 Bytes/s
    4:973 Client - Average transfer speed (with headers): 4827000 Bytes/s

    5:678 Client - 4194304 Bytes sent (without headers) in 704 ms.
    5:679 Client - 4349446 Bytes sent (with headers) in 704 ms.
    5:679 Client - Average transfer speed (without headers): 5957000 Bytes/s
    5:679 Client - Average transfer speed (with headers): 6178000 Bytes/s

    6:635 Client - 4194304 Bytes received (without headers) in 906 ms.
    6:635 Client - 4349446 Bytes received (with headers) in 906 ms.
    6:636 Client - Average transfer speed (without headers): 4629000 Bytes/s
    6:636 Client - Average transfer speed (with headers): 4800000 Bytes/s

    7:343 Client - 4194304 Bytes sent (without headers) in 707 ms.
    7:344 Client - 4349446 Bytes sent (with headers) in 707 ms.
    7:344 Client - Average transfer speed (without headers): 5932000 Bytes/s
    7:344 Client - Average transfer speed (with headers): 6151000 Bytes/s

    8:291 Client - 4194304 Bytes received (without headers) in 896 ms.
    8:291 Client - 4349446 Bytes received (with headers) in 896 ms.
    8:291 Client - Average transfer speed (without headers): 4681000 Bytes/s
    8:291 Client - Average transfer speed (with headers): 4854000 Bytes/s

    8:999 Client - 4194304 Bytes sent (without headers) in 707 ms.
    8:999 Client - 4349446 Bytes sent (with headers) in 707 ms.
    8:999 Client - Average transfer speed (without headers): 5932000 Bytes/s
    8:999 Client - Average transfer speed (with headers): 6151000 Bytes/s
    ...
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/

#ifndef   SPEEDCLIENT_NUM_CHUNKS
  #define SPEEDCLIENT_NUM_CHUNKS  1                                       // Number of chunks (full packets) to try to transfer at once. More could increase speed.
#endif
#define   SERVER_IP_ADDR          "192.168.88.12"                         // IP address of server, for example 192.168.88.12 .
#define   SERVER_PORT             1234                                    // Remote destination port.
#define   NUMBER_OF_BYTES         (4uL * 1024uL * 1024uL)                 // Number of bytes to transmit.
#define   BUFFER_SIZE             (SPEEDCLIENT_NUM_CHUNKS * (1500 - 40))  // Maximum number of bytes we can transfer at once; MTU - TCP/IP header.
#define   DIRECTION               3                                       // 1 for receive, 2 for send, 3 for Rx & Tx .
#define   USE_RX_TASK             0                                       // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

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

static U32             _aRxTxBuffer[BUFFER_SIZE / sizeof(U32)];// U32 to make sure we have a word alignment.
static OS_STACKPTR int _StackIP[TASK_STACK_SIZE_IP_TASK];      // Task stacks
static OS_STACKPTR int _StackClient[768 + APP_TASK_STACK_OVERHEAD];
static OS_TASK         _TCBIP;                                 // Task-control-blocks
static OS_TASK         _TCBClient;
#if USE_RX_TASK
static OS_TASK         _TCBIPRx;
static OS_STACKPTR int _StackIPRx[TASK_STACK_SIZE_IP_RX_TASK];
#endif

//
// Statistics to count all successful transmissions of NUMBER_OF_BYTES
//
static struct {
  U32 RxCnt;
  U32 TxCnt;
  U32 ErrCnt;
} _Statistics;

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
*       _Receive
*
*  Function description
*    Sends a command to server and receives data from server.
*/
static int _Receive(long TCPSockID, unsigned Mtu) {
  U8  acBuffer[20];
  U8  Flag;
  int NumBytesAtOnce;
  U32 ReceiveCnt;
  U32 ReceiveCntOverhead;  // The real number of bytes received including headers (overhead is roughly calculated).
  int r;
  int Error;
  int TimeStart;
  int TimeEnd;

  //
  // Send command and receive data
  //
  acBuffer[0] = 'S';                                                // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                     // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                 // [5:8]: MTU
  r = send(TCPSockID, (const char*)&acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error sending command: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  ReceiveCnt         = 0;
  ReceiveCntOverhead = 0;
  TimeStart          = OS_GetTime();
  do {
    NumBytesAtOnce = recv(TCPSockID, (char*)_aRxTxBuffer, sizeof(_aRxTxBuffer), 0);
    if (NumBytesAtOnce <= 0) {
      getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
      IP_Warnf_Application("Error receiving data: %s", IP_Err2Str(Error));
      return SOCKET_ERROR;
    } else {
      ReceiveCnt         += NumBytesAtOnce;
      ReceiveCntOverhead += (SPEEDCLIENT_NUM_CHUNKS * 54) + NumBytesAtOnce;
    }
  } while (ReceiveCnt < NUMBER_OF_BYTES);
  TimeEnd  = OS_GetTime();
  Flag     = 'X';            // Confirmation
  r        = send(TCPSockID, (const char*)&Flag, 1, 0);
  if (r == SOCKET_ERROR) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error sending confirmation: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  //
  // Output performance values
  //
  IP_Logf_Application("%lu Bytes received (without headers) in %d ms.", ReceiveCnt, (TimeEnd - TimeStart));
  IP_Logf_Application("%lu Bytes received (with headers) in %d ms.", ReceiveCntOverhead, (TimeEnd - TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", (ReceiveCnt / (TimeEnd - TimeStart) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", (ReceiveCntOverhead / (TimeEnd - TimeStart) * 1000));
  BSP_ToggleLED(1);
  return 0;
}

/*********************************************************************
*
*       _Send
*
*  Function description
*    Sends a command to server and sends data to server.
*/
static int _Send(long TCPSockID, unsigned Mtu) {
  U8  acBuffer[20];
  int NumBytesAtOnce;
  U32 SendCnt;
  U32 SendCntOverhead;  // The real number of bytes sent including headers (overhead is roughly calculated).
  U8  Flag;
  int r;
  int Error;
  int TimeStart;
  int TimeEnd;
  int SizeToSend;

  //
  // Send command
  //
  acBuffer[0] = 'R';                                                 // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                      // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                  // [5:8]: MTU
  r = send(TCPSockID, (const char*) &acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    return SOCKET_ERROR;
  }
  //
  // Send data
  //
  SendCnt         = 0;
  SendCntOverhead = 0;
  TimeStart       = OS_GetTime();
  do {
    if ((NUMBER_OF_BYTES - SendCnt) < Mtu) {
      SizeToSend = NUMBER_OF_BYTES - SendCnt;
    } else {
      SizeToSend = Mtu;
    }
    NumBytesAtOnce = send(TCPSockID, (const char*)&_aRxTxBuffer[0], SizeToSend, 0);
    if (NumBytesAtOnce == SOCKET_ERROR) {
      return NumBytesAtOnce;
    } else {
      SendCnt         += NumBytesAtOnce;
      SendCntOverhead += (SPEEDCLIENT_NUM_CHUNKS * 54) + NumBytesAtOnce;
    }
  } while (SendCnt < NUMBER_OF_BYTES);
  TimeEnd = OS_GetTime();
  Flag    = 0;
  //
  // Wait for response to make sure data has been sent completely
  //
  r = recv(TCPSockID, (char*)&Flag, 1, 0);
  if (r <= 0) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error getting response: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  //
  // Output performance values
  //
  IP_Logf_Application("%lu Bytes sent (without headers) in %d ms.", SendCnt, (TimeEnd - TimeStart));
  IP_Logf_Application("%lu Bytes sent (with headers) in %d ms.", SendCntOverhead, (TimeEnd - TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", ((SendCnt / (TimeEnd - TimeStart)) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", ((SendCntOverhead / (TimeEnd - TimeStart)) * 1000));
  BSP_ToggleLED(1);
  return 0;
}

/*********************************************************************
*
*       _Client
*/
static void _Client(void) {
  long               TCPSockID;
  struct sockaddr_in ServerAddr;
  int                ConnectStatus;
  int                Error;
  int                r;
  int                Opt;
  int                Mtu;
  IP_ADDR            TargetAddr;

  //
  // Wait until link is up and network interface is configured.
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  Mtu = IP_TCP_GetMTU(_IFaceId) - 40;             // MTU - TCP/IP header
  while(1) {
    TCPSockID = socket(AF_INET, SOCK_STREAM, 0);  // Open socket
    if (TCPSockID == 0) {                         // Error, Could not get socket
      while (1) {
        BSP_ToggleLED(0);
        OS_Delay(20);
      }
    } else {
      //
      // Set keep alive option
      //
      Opt = 1;
      setsockopt(TCPSockID, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
      //
      // Connect to server
      //
      BSP_SetLED(0);
      ServerAddr.sin_family      = AF_INET;
      ServerAddr.sin_port        = htons(SERVER_PORT);
      r = IP_IPV4_ParseIPv4Addr(SERVER_IP_ADDR, &TargetAddr);
      if (r < 0) {
        IP_PANIC("Illegal IP Address.");
      }
      ServerAddr.sin_addr.s_addr = TargetAddr;
      ConnectStatus              = connect(TCPSockID, (struct sockaddr *)&ServerAddr, sizeof(struct sockaddr_in));
      if (ConnectStatus != SOCKET_ERROR) {
        while(1) {
          if (DIRECTION & 1) {
            r = _Receive(TCPSockID, Mtu);
            if (r == SOCKET_ERROR) {
              break;
            }
            _Statistics.RxCnt++;
          }
          if (DIRECTION & 2) {
            r = _Send(TCPSockID, Mtu);
            if (r == SOCKET_ERROR) {
              break;
            }
            _Statistics.TxCnt++;
          }
          OS_Delay(50);
        }
      } else {
        getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
        IP_Warnf_Application("connect() to %s:%d failed: %s", SERVER_IP_ADDR, SERVER_PORT, IP_Err2Str(Error));
      }
    }
    _Statistics.ErrCnt++;
    closesocket(TCPSockID);
    OS_Delay(1000);
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                              // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), 150);                                    // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_TCBIP    , "IP_Task"  , IP_Task  , 150, _StackIP);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_TCBIPRx  , "IP_RxTask", IP_RxTask, 250, _StackIPRx);    // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_TCBClient, "Client"   , _Client  , 100, _StackClient);  // Start the speed client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);               // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                   // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                    // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (1) {
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
