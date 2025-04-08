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
File    : MB_SLAVE_TCPSample.c
Purpose : Sample program for Modbus slave using embOS & embOS/IP.
          Demonstrates how to implement a Modbus slave using the
          Modbus/TCP protocol.
          The sample setups a Modbus/TCP slave via IP and provides
          access to some LEDs and other resources.
          The slave provides the following resources:
          - LEDs on coils BASE_ADDR & BASE_ADDR + 1.
          - Two Discrete Inputs on BASE_ADDR & BASE_ADDR + 1 that
            are toggled separately on each access.
          - Two 16-bit registers on BASE_ADDR & BASE_ADDR + 1.
          - Two 16-bit Input Registers on BASE_ADDR & BASE_ADDR + 1
            that count up on each access.
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

#define USE_RX_TASK      0           // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define IP_ADDR_ALLOWED  INADDR_ANY  // IP addr. to accept a connect from.
#define IP_PORT          502
#define SLAVE_ADDR       1
#define BASE_ADDR        1000

//
// Task priorities
//
enum {
   TASK_PRIO_MB_POLL_SLAVE_TASK = 150  // Priority must be lower as Modbus slave task.
  ,TASK_PRIO_MB_SLAVE_TASK             // Priority must be higher as all Modbus related tasks but lower as IP task.
  ,TASK_PRIO_IP_TASK                   // Priority must be higher as all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK                // Must be the highest priority of all IP related tasks.
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
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                    // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                           // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];               // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                         // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _MBSlaveStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];      // Define the stack of the MB_SLAVE_Task to 1024 bytes.
static OS_TASK         _MBSlaveTCB;

static OS_STACKPTR int _MBPollSlaveStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Define the stack of the channel polling task to 1024 bytes.
static OS_TASK         _MBPollSlaveTCB;

//
// Modbus configuration.
//
static MB_CHANNEL          _MBChannel;
static MB_IFACE_CONFIG_IP  _MBConfig;

static char                _MBCoil[2];    // Simulate module with 2 coils.
static char                _MBToggle[2];  // Simulate module with 2 Discrete Input bits, each toggling with each access.
static U16                 _MBReg[2];     // Simulate module with 2 registers.
static U16                 _MBCnt[2];     // Simulate module with 2 Input Registers used as counter, each counting up with each access.

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

static int  _Init      (MB_IFACE_CONFIG_IP *pConfig);
static void _DeInit    (MB_IFACE_CONFIG_IP *pConfig);
static int  _Send      (MB_IFACE_CONFIG_IP *pConfig, const U8 *pData, U32 NumBytes);
static int  _Recv      (MB_IFACE_CONFIG_IP *pConfig,       U8 *pData, U32 NumBytes, U32 Timeout);
static int  _Connect   (MB_IFACE_CONFIG_IP *pConfig, U32 Timeout);
static void _Disconnect(MB_IFACE_CONFIG_IP *pConfig);

static int  _WriteCoil (U16 Addr, char OnOff);
static int  _ReadCoil  (U16 Addr);
static int  _ReadDI    (U16 Addr);
static int  _WriteReg  (U16 Addr, U16   Val);
static int  _ReadHR    (U16 Addr, U16 *pVal);
static int  _ReadIR    (U16 Addr, U16 *pVal);

/*********************************************************************
*
*       Static const, Modbus API
*
**********************************************************************
*/

static const MB_IFACE_IP_API _IFaceAPI = {
  NULL,         // pfSendByte
  _Init,        // pfInit
  _DeInit,      // pfDeInit
  _Send,        // pfSend
  _Recv,        // pfRecv
  _Connect,     // pfConnect
  _Disconnect,  // pfDisconnect
  NULL,         // pfInitTimer
  NULL          // pfDeInitTimer
};

static const MB_SLAVE_API _SlaveAPI = {
  _WriteCoil,  // pfWriteCoil
  _ReadCoil,   // pfReadCoil
  _ReadDI,     // pfReadDI
  _WriteReg,   // pfWriteReg
  _ReadHR,     // pfReadHR
  _ReadIR      // pfReadIR
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
*       _Init()
*
* Function description
*   Gets listen socket and sets it into listen state.
*
* Parameters
*   pConfig: Pointer to configuration.
*
* Return value
*   O.K. : 0
*   Error: Other
*/
static int _Init(MB_IFACE_CONFIG_IP *pConfig) {
  struct sockaddr_in Addr;
         int         hSock;
         int         r;

  //
  // Get listen socket.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if (hSock == SOCKET_ERROR) {
    while(1);  // This should never happen!
  }
  Addr.sin_family      = AF_INET;
  Addr.sin_port        = htons(pConfig->Port);
  Addr.sin_addr.s_addr = htonl(pConfig->IPAddr);
  r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  if (r) {
    while(1);  // This should never happen!
  }
  r = listen(hSock, 1);
  if (r) {
    while(1);  // This should never happen!
  }
  //
  // Store socket into configuration.
  //
  pConfig->ListenSock = (MB_SOCKET)hSock;
  return 0;
}

/*********************************************************************
*
*       _DeInit()
*
* Function description
*   Close listen socket.
*
* Parameters
*   pConfig: Pointer to configuration.
*/
static void _DeInit(MB_IFACE_CONFIG_IP *pConfig) {
  closesocket((int)pConfig->ListenSock);
}

/*********************************************************************
*
*       _Connect()
*
* Function description
*   Accept a new connection.
*
* Parameters
*   pConfig: Pointer to configuration.
*   Timeout: Not used for slave.
*
* Return value
*   0    : New connection accepted.
*   Other: Error (or no new connection in case of non-blocking).
*/
static int _Connect(MB_IFACE_CONFIG_IP *pConfig, U32 Timeout) {
  struct sockaddr Addr;
         int      AddrSize;
         int      hSock;

  IP_USE_PARA(Timeout);

  AddrSize = sizeof(Addr);
  hSock    = accept((int)pConfig->ListenSock, &Addr, &AddrSize);
  if (hSock != SOCKET_ERROR) {
    pConfig->Sock = (MB_SOCKET)hSock;  // Store socket into configuration.
    return 0;                          // O.K., new connection accepted.
  }
  return MB_ERR_CONNECT;               // Error (or no new connection in case of non-blocking).
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
*       Local functions, Modbus slave API
*
**********************************************************************
*/

/*********************************************************************
*
*       _WriteCoil()
*
* Function description
*   Writes a single coil.
*
* Parameters
*   Addr: Addr. of coil to write.
*
* Return value
*     0: O.K.
*   < 0: Error
*/
static int _WriteCoil(U16 Addr, char OnOff) {
  int r;

  r = 0;
  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    _MBCoil[Addr] = OnOff;
    if (OnOff) {
      BSP_SetLED(Addr);
    } else {
      BSP_ClrLED(Addr);
    }
    r = 0;
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _ReadCoil()
*
* Function description
*   Reads status of a single coil.
*
* Parameters
*   Addr: Addr. of coil to read.
*
* Return value
*     1: On
*     0: Off
*   < 0: Error
*/
static int _ReadCoil(U16 Addr) {
  int r;

  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    r = _MBCoil[Addr];
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _ReadDI()
*
* Function description
*   Reads single Discrete Input status.
*
* Parameters
*   Addr: Addr. of input to read.
*
* Return value
*     1: On
*     0: Off
*   < 0: Error
*/
static int _ReadDI(U16 Addr) {
  int r;

  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    r = _MBToggle[Addr];
    _MBToggle[Addr] ^= 1;
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _WriteReg()
*
* Function description
*   Writes a single register.
*
* Parameters
*   Addr: Addr. of register to write.
*
* Return value
*     0: O.K.
*   < 0: Error
*/
static int _WriteReg(U16 Addr, U16 Val) {
  int r;

  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    _MBReg[Addr] = Val;
    r = 0;
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _ReadHR()
*
* Function description
*   Reads a single Holding Register.
*
* Parameters
*   Addr: Addr. of Holding Register to read.
*
* Return value
*     0: O.K.
*   < 0: Error
*/
static int _ReadHR(U16 Addr, U16 *pVal) {
  int r;

  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    *pVal = _MBReg[Addr];
    r = 0;
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _ReadIR()
*
* Function description
*   Reads a single Input Register.
*
* Parameters
*   Addr: Addr. of Holding Register to read.
*
* Return value
*     0: O.K.
*   < 0: Error
*/
static int _ReadIR(U16 Addr, U16 *pVal) {
  int r;

  if ((Addr >= BASE_ADDR) && (Addr <= (BASE_ADDR + 1))) {
    Addr -= BASE_ADDR;
    *pVal = _MBCnt[Addr];
    _MBCnt[Addr]++;
    r = 0;
  } else {
    r = MB_ERR_ILLEGAL_DATA_ADDR;
  }
  return r;
}

/*********************************************************************
*
*       _HandleCustomFunctionCode()
*
* Function description
*   Handles custom function codes or overwrites stack internal handling.
*
* Parameters
*   pChannel: Pointer to element of type MB_CHANNEL.
*   pPara   : Input/output parameter structure.
*
* Return value
*               >= 0: O.K., function code handled. Number of bytes to send back.
*               <  0: Error, use official Modbus error codes like MB_ERR_ILLEGAL_DATA_VAL .
*   MB_ERR_FUNC_CODE: Function code not handled. Try stack internal handling.
*
* Additional information
*   Function code 0x08 (Diagnostic), subfunction code 0x00 0x00 (Return Query Data) :
*     This very basic diagnostic subfunction echoes back a two byte
*     value that has just been received.
*   Function code 0x30 :
*     The payload of the message is expected to be a printable
*     string with termination. As the string itself is properly
*     terminated no length field is necessary.
*     One U8 is expected as return code that lets the master
*     know if the string has been printed or not. In this sample
*     this is decided by checking if MB_DEBUG is active or not.
*/
static int _HandleCustomFunctionCode(MB_CHANNEL *pChannel, MB_CUSTOM_FUNC_CODE_PARA *pPara) {
  U32 SubCode;
  int r;

  MB_USE_PARA(pChannel);

  r = MB_ERR_FUNC_CODE;  // Assume that we can not handle this function code.

  //
  // Handle custom function codes.
  //
  switch (pPara->Function) {
  case 0x08:
    SubCode = MB_LoadU16BE((const U8*)pPara->pData);
    switch (SubCode) {
    case 0x0000:
      r = 4;  // Send back Subfunction Hi/Lo & Data Hi/Lo fields. Data is echoed back as it is in the input/output buffer.
      break;
    }
    break;
  case 0x30:
    //
    // Output the string that has been sent.
    //
    MB_Log((const char*)pPara->pData);
    //
    // Store MB_DEBUG level as 1 byte answer.
    // Up to pPara->BufferSize bytes might be stored.
    //
    *pPara->pData  = MB_DEBUG;
    r              = 1;  // Tell the stack that we are sending back 1 byte data.
    break;
  }
  return r;
}

/*********************************************************************
*
*       Local functions, Modbus slave channel polling tasks
*
* All slave channels that are not able to pass their received data to
* the Modbus stack from an interrupt directly need to be polled.
*
**********************************************************************
*/

/*********************************************************************
*
*       _PollSlaveChannelTask()
*
* Function description
*   Polls a slave channel in a task, allowing the task to sleep when
*   nothing to do.
*
* Parameters
*   pChannel: Pointer to element of type MB_CHANNEL.
*/
static void _PollSlaveChannelTask(void *pChannel) {
  while (1) {
    MB_SLAVE_PollChannel((MB_CHANNEL*)pChannel);
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
  //
  // Initially clear all LEDs that are used in Modbus sample.
  //
  BSP_ClrLED(0);
  BSP_ClrLED(1);
  //
  // Start Modbus slave using Modbus/TCP protocol.
  //
  MB_SLAVE_Init();
  OS_CREATETASK(&_MBSlaveTCB, "MB_SLAVE_Task", MB_SLAVE_Task, TASK_PRIO_MB_SLAVE_TASK, _MBSlaveStack);                                                     // Start the MB_SLAVE_Task.
  MB_SLAVE_AddIPChannel(&_MBChannel, &_MBConfig, &_SlaveAPI, &_IFaceAPI, SLAVE_ADDR, 0, IP_ADDR_ALLOWED, IP_PORT);                                         // Add a slave channel.
  MB_SLAVE_SetCustomFunctionCodeHandler(&_MBChannel, _HandleCustomFunctionCode);                                                                           // Add a custom function code handler for this channel.
  OS_CREATETASK_EX(&_MBPollSlaveTCB, "MB_SLAVE_PollChannel", _PollSlaveChannelTask, TASK_PRIO_MB_POLL_SLAVE_TASK, _MBPollSlaveStack, (void*)&_MBChannel);  // Start channel polling task for the slave channel.
  while (1) {
    OS_Delay(1000);
  }
}

/*************************** End of file ****************************/
