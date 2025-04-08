/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2013 - 2018  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       emModbus * Modbus stack for embedded applications            *
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
*       emModbus version: V1.02h                                     *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : MB_MASTER_TCPSample.c
Purpose : Sample program for Modbus master using embOS & embOS/IP.
          Demonstrates how to implement a Modbus master using the
          Modbus/TCP protocol.
          The sample connects to a Modbus/TCP slave via IP and toggles
          some LEDs on the slave.
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "MB.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK    0                                // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define TIMEOUT        3000                             // Timeout for Modbus response [ms].
#define SLAVE_IP_ADDR  IP_BYTES2ADDR(192, 168, 11, 63)  // IP addr. of slave to connect to.
#define SLAVE_IP_PORT  502
#define SLAVE_ADDR     1
#define BASE_ADDR      1000

//
// Task priorities
//
enum {
   TASK_PRIO_MB_MASTER_LP_TASK = 150  // Low priority Modbus task. Priority must be lower as interface management task used for Modbus.
  ,TASK_PRIO_MB_MASTER_HP_TASK        // High priority Modbus task. Priority must be lower as interface management task used for Modbus.
  ,TASK_PRIO_IP_TASK                  // Priority must be higher as all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK               // Must be the highest priority of all IP related tasks.
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
*       Defines
*
**********************************************************************
*/

#define LOCK()    OS_Use(&_Lock)
#define UNLOCK()  OS_Unuse(&_Lock)

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
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                   // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                          // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];              // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                        // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _MBMasterHPStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Define the stack of the Modbus High Priority master to 768 bytes.
static OS_TASK         _MBMasterHPTCB;

static OS_STACKPTR int _MBMasterLPStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Define the stack of the Modbus Low Priority master to 768 bytes.
static OS_TASK         _MBMasterLPTCB;

static OS_RSEMA        _Lock;                                                           // Resource semaphore for locking between tasks addressing same slave.

//
// Modbus configuration.
//
static MB_CHANNEL          _MBChannel;
static MB_IFACE_CONFIG_IP  _MBConfig;

static volatile int        _ResultHP;
static volatile int        _ResultLP;

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

static int  _Send      (MB_IFACE_CONFIG_IP *pConfig, const U8 *pData, U32 NumBytes);
static int  _Recv      (MB_IFACE_CONFIG_IP *pConfig,       U8 *pData, U32 NumBytes, U32 Timeout);
static int  _Connect   (MB_IFACE_CONFIG_IP *pConfig, U32 Timeout);
static void _Disconnect(MB_IFACE_CONFIG_IP *pConfig);

/*********************************************************************
*
*       Static const, Modbus API
*
**********************************************************************
*/

static const MB_IFACE_IP_API _IFaceAPI = {
  NULL,         // pfSendByte
  NULL,         // pfInit
  NULL,         // pfDeInit
  _Send,        // pfSend
  _Recv,        // pfRecv
  _Connect,     // pfConnect
  _Disconnect,  // pfDisconnect
  NULL,         // pfInitTimer
  NULL          // pfDeInitTimer
};

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
*       Local functions, Modbus network interface
*
**********************************************************************
*/

/*********************************************************************
*
*       _Connect()
*
* Function description
*   Create socket, connect to slave and store socket in configuration.
*
* Parameters
*   pConfig: Pointer to configuration.
*   Timeout: Timeout [ms] for the connect operation to return.
*
* Return value
*   0    : Connection to slave established.
*   Other: Error (or no new connection in case of non-blocking).
*/
static int _Connect(MB_IFACE_CONFIG_IP *pConfig, U32 Timeout) {
  struct sockaddr_in Addr;
         int         hSock;
         int         r;
         int         SoError;
         int         t0;
         int         t;

  //
  // Get socket for connecting.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if (hSock == SOCKET_ERROR) {
    while(1);  // This should never happen!
  }
  setsockopt(hSock, SOL_SOCKET, SO_NBIO, NULL, 0);     // Set socket non blocking.
  //
  // Configure socket and connect.
  //
  Addr.sin_family      = AF_INET;
  Addr.sin_port        = htons(pConfig->Port);
  Addr.sin_addr.s_addr = htonl(pConfig->IPAddr);
  t0                   = IP_OS_GetTime32();
  do {
    r = connect(hSock, (struct sockaddr*)&Addr, sizeof(struct sockaddr_in));
    if (r == 0) {
      setsockopt(hSock, SOL_SOCKET, SO_BIO, NULL, 0);  // Set socket blocking.
      pConfig->Sock = (MB_SOCKET)hSock;                // Store socket into configuration.
      return 0;                                        // Connected to slave.
    }
    getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    if (SoError != IP_ERR_IN_PROGRESS) {
      return MB_ERR_CONNECT;                           // Unable to connect to slave.
    }
    t = IP_OS_GetTime32() - t0;
    if (t >= (int)Timeout) {
      return MB_ERR_CONNECT_TIMEOUT;                   // Unable to connect to slave within timeout.
    }
    OS_Delay(1);                                       // Give lower prior tasks some time.
  } while (1);
}

/*********************************************************************
*
*       _Disconnect()
*
* Function description
*   Close connection.
*
* Parameters
*   pConfig: Pointer to configuration.
*/
static void _Disconnect(MB_IFACE_CONFIG_IP *pConfig) {
  closesocket((int)pConfig->Sock);
}

/*********************************************************************
*
*       _Send()
*
* Function description
*   Sends data on the interface.
*
* Parameters
*   pConfig : Pointer to configuration.
*   pData   : Pointer to data to send.
*   NumBytes: Number of bytes to send from pData.
*
* Return value
*   >= 0: NumBytes sent.
*   <  0: Error.
*/
static int _Send(MB_IFACE_CONFIG_IP *pConfig, const U8 *pData, U32 NumBytes) {
  int NumBytesSent;
  int r;

  NumBytesSent = send((int)pConfig->Sock, (const char*)pData, NumBytes, 0);
  if (NumBytesSent > 0) {
    r = NumBytesSent;
  } else {
    if (NumBytesSent == 0) {  // Connection closed ?
      r = MB_ERR_DISCONNECT;
    } else {
      r = MB_ERR_MISC;
    }
  }
  return r;
}

/*********************************************************************
*
*       _Recv()
*
* Function description
*   Receives data from the interface.
*
* Parameters
*   pConfig : Pointer to configuration.
*   Timeout : Timeout [ms] for the receive operation to return. Always 0 if slave channel.
*   pData   : Pointer to buffer to store data received.
*   NumBytes: Number of bytes received.
*
* Return value
*   >= 0: Number of bytes read.
*   <  0: Error.
*/
static int _Recv(MB_IFACE_CONFIG_IP *pConfig, U8 *pData, U32 NumBytes, U32 Timeout) {
  int NumBytesReceived;
  int SoError;
  int hSock;
  int r;

  hSock = (int)pConfig->Sock;
  //
  // Set current timeout. Might be less than the total timeout configured for the
  // channel if less than the requested number of bytes has been received before.
  //
  setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));
  NumBytesReceived = recv(hSock, (char*)pData, NumBytes, 0);
  if (NumBytesReceived > 0) {
    r = NumBytesReceived;
  } else {
    if (NumBytesReceived == 0) {  // Connection closed ?
      r = MB_ERR_DISCONNECT;
    } else {
      getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
      if (SoError == IP_ERR_TIMEDOUT) {
        r = MB_ERR_TIMEOUT;
      } else {
        r = MB_ERR_MISC;
      }
    }
  }
  return r;
}

/*********************************************************************
*
*       Local functions, Modbus master task
*
**********************************************************************
*/

/*********************************************************************
*
*       _ModbusHPMasterTask()
*
* Function description
*   Modbus High Priority master task executing request to a slave device.
*/
static void _ModbusHPMasterTask(void) {
  U8 State;

  State = 0;

  MB_MASTER_Init();
  MB_MASTER_AddIPChannel(&_MBChannel, &_MBConfig, &_IFaceAPI, TIMEOUT, SLAVE_ADDR, SLAVE_IP_ADDR, SLAVE_IP_PORT);  // Add a master channel.
  do {
    State ^= 1;
    LOCK();
    _ResultHP = MB_MASTER_WriteCoil(&_MBChannel, BASE_ADDR, State);
    UNLOCK();
    OS_Delay(50);
  } while (_ResultHP == 0);
  while (1) {
    BSP_ToggleLED(0);  // Error.
    OS_Delay(1000);
  }
}

/*********************************************************************
*
*       _ModbusLPMasterTask()
*
* Function description
*   Modbus Low Priority master task executing request to a slave device.
*/
static void _ModbusLPMasterTask(void) {
  U8 State;

  State = 0;

  do {
    State ^= 1;
    LOCK();
    _ResultLP = MB_MASTER_WriteCoil(&_MBChannel, BASE_ADDR + 1, State);
    UNLOCK();
    OS_Delay(200);
  } while (_ResultLP == 0);
  while (1) {
    BSP_ToggleLED(0);  // Error.
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  OS_CREATERSEMA(&_Lock);
  OS_CREATETASK(&_MBMasterHPTCB, "MBMasterHP", _ModbusHPMasterTask, TASK_PRIO_MB_MASTER_HP_TASK, _MBMasterHPStack);  // Start the Modbus HP task.
  OS_CREATETASK(&_MBMasterLPTCB, "MBMasterLP", _ModbusLPMasterTask, TASK_PRIO_MB_MASTER_LP_TASK, _MBMasterLPStack);  // Start the Modbus LP task.
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
