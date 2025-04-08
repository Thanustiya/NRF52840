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

Purpose : Speed client for TCP/IP stack using zero copy interface.

Additional information:
  This application can be used to connect to a server
  running the SpeedTestServer pc application
  and run a speed test using a zero copy interface.

  Preparations:
    The speed client can be configured in a number of ways via
    configuration defines that can freely be overridden by the user.
    These defines include:
      SERVER_IP_ADDR:         IP address of the server
      SERVER_PORT:            Port the server is listening on
      NUMBER_OF_BYTES:        Number of bytes you want to transmit

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
    6:694 Client - 4194304 Bytes received (without headers) in 604 ms.
    6:695 Client - 4349446 Bytes received (with headers) in 604 ms.
    6:695 Client - Average transfer speed (without headers): 6944000 Bytes/s
    6:695 Client - Average transfer speed (with headers): 7201000 Bytes/s

    7:431 Client - 4194304 Bytes sent (without headers) in 736 ms.
    7:432 Client - 4403464 Bytes sent (with headers) in 736 ms.
    7:432 Client - Average transfer speed (without headers): 5698000 Bytes/s
    7:432 Client - Average transfer speed (with headers): 5982000 Bytes/s

    8:073 Client - 4194304 Bytes received (without headers) in 591 ms.
    8:073 Client - 4349446 Bytes received (with headers) in 591 ms.
    8:073 Client - Average transfer speed (without headers): 7096000 Bytes/s
    8:073 Client - Average transfer speed (with headers): 7359000 Bytes/s

    8:803 Client - 4194304 Bytes sent (without headers) in 729 ms.
    8:803 Client - 4403464 Bytes sent (with headers) in 729 ms.
    8:803 Client - Average transfer speed (without headers): 5753000 Bytes/s
    8:803 Client - Average transfer speed (with headers): 6040000 Bytes/s

    9:445 Client - 4194304 Bytes received (without headers) in 592 ms.
    9:445 Client - 4349446 Bytes received (with headers) in 592 ms.
    9:445 Client - Average transfer speed (without headers): 7084000 Bytes/s
    9:446 Client - Average transfer speed (with headers): 7347000 Bytes/s

    10:179 Client - 4194304 Bytes sent (without headers) in 732 ms.
    10:179 Client - 4403464 Bytes sent (with headers) in 732 ms.
    10:179 Client - Average transfer speed (without headers): 5729000 Bytes/s
    10:179 Client - Average transfer speed (with headers): 6015000 Bytes/s
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

#define SERVER_IP_ADDR   "192.168.88.12"                  // IP address of server, for example 192.168.88.12 .
#define SERVER_PORT      1234                             // Remote destination port.
#define NUMBER_OF_BYTES  (4uL * 1024uL * 1024uL)          // Number of bytes to transmit.
#define DIRECTION        3                                // 1 for receive, 2 for send, 3 for Rx & Tx .
#define USE_RX_TASK      0                                // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

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

static OS_STACKPTR int _StackIP[TASK_STACK_SIZE_IP_TASK];      // Task stacks
static OS_STACKPTR int _StackClient[768 + APP_TASK_STACK_OVERHEAD];
static OS_TASK         _TCBIP;                                 // Task-control-blocks
static OS_TASK         _TCBClient;
#if USE_RX_TASK
static OS_TASK         _TCBIPRx;
static OS_STACKPTR int _StackIPRx[TASK_STACK_SIZE_IP_RX_TASK];
#endif
static volatile U32    _ReceiveCnt;
static volatile U32    _ReceiveCntOverhead;  // The real number of bytes received including headers.
static int             _TimeStart;
static int             _TimeEnd;
static OS_EVENT        _RxEvent;

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
*       _rx_callback
*
*  Function description
*    Sample implementation of a receive callback required by the TCP zero-copy
*    interface to process received packets.
*/
static int _rx_callback(long so, IP_PACKET * pPacket, int code) {
  IP_USE_PARA(so);
  IP_USE_PARA(code);
  _ReceiveCnt         += pPacket->NumBytes;
  _ReceiveCntOverhead += pPacket->NumBytes + (U32)(pPacket->pData - pPacket->pBuffer - 2);
  //
  // Signal receiver task if all bytes expected have been received.
  //
  if (_ReceiveCnt == NUMBER_OF_BYTES) {
    OS_EVENT_Set(&_RxEvent);
  }
  return IP_OK;
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
  int r;
  int Error;

  _ReceiveCnt         = 0;
  _ReceiveCntOverhead = 0;
  _TimeStart          = 0;
  _TimeEnd            = 0;
  acBuffer[0]         = 'S';                                         // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                      // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                  // [5:8]: MTU
  r = send(TCPSockID, (const char*) &acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error sending command: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  setsockopt(TCPSockID, SOL_SOCKET, SO_CALLBACK, (void*)_rx_callback, 0);  // Register Rx callback to process data.
  _TimeStart = OS_GetTime();
  while (_ReceiveCnt < NUMBER_OF_BYTES) {
    r = (int)OS_EVENT_WaitTimed(&_RxEvent, 10000);  // Make sure we do not get stuck here in case the server disconnects.
    if (r != 0) {
      IP_Warnf_Application("Error receiving data.");
      return SOCKET_ERROR;
    }
  }
  _TimeEnd  = OS_GetTime();
  Flag      = 'X';            // Confirmation
  r         = send(TCPSockID, (const char*) &Flag, 1, 0);
  if (r == SOCKET_ERROR) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error sending confirmation: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  IP_Logf_Application("%lu Bytes received (without headers) in %d ms.", _ReceiveCnt, (_TimeEnd - _TimeStart));
  IP_Logf_Application("%lu Bytes received (with headers) in %d ms.", _ReceiveCntOverhead, (_TimeEnd - _TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", (_ReceiveCnt / (_TimeEnd - _TimeStart) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", (_ReceiveCntOverhead / (_TimeEnd - _TimeStart) * 1000));
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
  U8          acBuffer[20];
  U32         SendCnt;
  U32         SendCntOverhead;  // The real number of bytes sent including headers (overhead is roughly calculated).
  U8          Flag;
  IP_PACKET * pPacket;
  unsigned    AllocFailCnt;
  unsigned    FreePacketCnt;
  int         r, e;
  int         Error;
  int         SizeToSend;

  setsockopt(TCPSockID, SOL_SOCKET, SO_CALLBACK, (void*)NULL, 0);    // De-register Rx callback to receive real data with recv().
  //
  // Send command and data
  //
  acBuffer[0] = 'R';                                                 // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                      // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                  // [5:8]: MTU
  r = send(TCPSockID, (const char*) &acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    return SOCKET_ERROR;
  }
  SendCnt         = 0;
  SendCntOverhead = 0;
  AllocFailCnt    = 0;
  _TimeStart      = OS_GetTime();
  do {
    if ((NUMBER_OF_BYTES - SendCnt) < Mtu) {
      SizeToSend = NUMBER_OF_BYTES - SendCnt;
      if (SizeToSend < 4) {
        SizeToSend = 4; // At least for the Time;
      }
    } else {
      SizeToSend = Mtu;
    }
    FreePacketCnt = IP_GetFreePacketCnt(SizeToSend);
    if (FreePacketCnt != 0) {
      pPacket = IP_TCP_Alloc(SizeToSend);
    } else {
      pPacket = NULL;
    }
    if (pPacket != NULL) {  // If we fail to get a big packet buffer this is due to a limited configuration or high traffic on the network.
      AllocFailCnt = 0;
      //
      // Put timestamp into packet buffer
      //
      *(U32*)pPacket->pData = OS_GetTime();
      //
      // Send packet
      //
      do {
        e = IP_TCP_Send(TCPSockID, pPacket);
      } while (e < 0);
      SendCnt         += SizeToSend;
      SendCntOverhead += IPV4_TCP_HEADER_LEN + SizeToSend;
    } else {
      if (++AllocFailCnt == 100) {  // Try for 100 times to get a big packet buffer.
        IP_Warnf_Application("Error getting big packet buffer.");
        return SOCKET_ERROR;
      }
    }
  } while (SendCnt < NUMBER_OF_BYTES);
  _TimeEnd = OS_GetTime();
  Flag = 0;
  //
  // Wait for response to make sure data has been sent completly
  //
  r = recv(TCPSockID, (char*)&Flag, 1, 0);
  if (r == SOCKET_ERROR) {
    getsockopt(TCPSockID, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
    IP_Warnf_Application("Error getting response: %s", IP_Err2Str(Error));
    return SOCKET_ERROR;
  }
  IP_Logf_Application("%lu Bytes sent (without headers) in %d ms.", SendCnt, (_TimeEnd - _TimeStart));
  IP_Logf_Application("%lu Bytes sent (with headers) in %d ms.", SendCntOverhead, (_TimeEnd - _TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", ((SendCnt / (_TimeEnd - _TimeStart)) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", ((SendCntOverhead / (_TimeEnd - _TimeStart)) * 1000));
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
  Mtu = IP_TCP_GetMTU(_IFaceId) + 16 - IPV4_TCP_HEADER_LEN;        // MTU - TCP/IP Header - RTTM
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
  OS_EVENT_Create(&_RxEvent);
  OS_CREATETASK(&_TCBClient, "Client"   , _Client  , 100, _StackClient);  // Start the speed client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);               // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                   // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                    // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (1) {
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
