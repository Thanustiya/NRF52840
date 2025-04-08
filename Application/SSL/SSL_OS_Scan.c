/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSL * Embedded Transport Layer Security                    *
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
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSL_OS_Scan.c
Purpose     : Example to show common cipher suites between emSSL
              client and a remote server.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SSL.h"
#include "IP.h"
#include "RTOS.h"
#include <stdio.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define HOST         "www.segger.com"
#define PORT         443

#define USE_RX_TASK  0

//
// Task priorities
//
enum {
  TASK_PRIO_IP_APP = 150,
  TASK_PRIO_IP_TASK,         // Priority should be higher as all TCP/IP application tasks
  TASK_PRIO_IP_RX_TASK       // Must be the highest priority of all TCP/IP related tasks, comment out to read packets in ISR
};

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _Send(int Socket, const char *pData, int DataLen, int Flags);
static int _Recv(int Socket,       char *pData, int DataLen, int Flags);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const SSL_TRANSPORT_API _IP_Transport = {
  _Send,
  _Recv,
  0,
  0
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int _IPStack[2048/sizeof(int)];
static OS_TASK         _IPTCB;

static OS_STACKPTR int _SSLStack[8192/sizeof(int)];
static OS_TASK         _SSLTCB;

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[1024/sizeof(int)];
static OS_TASK         _IPRxTCB;
#endif

static unsigned        _TSocket;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Open()
*
*  Function description
*    Open a socket.
*
*  Parameters
*    sHost - Host name to be resolved.
*    Port  - Port to open.
*
*  Return value
*    >= 0 - Success, socket handle.
*    <  0 - Error.
*/
static int _Open(const char *sHost, int Port) {
  struct sockaddr_in sa;
  int    Socket;
  int    Status;
  U32    IPAddr;
  //
  // Decode host.
  //
  Status = IP_ResolveHost(sHost, &IPAddr, 1000);
  if (Status < 0) {
    return -1;
  }
  //
  Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (Socket < 0) {
    Status = Socket;
  } else {
    sa.sin_family = AF_INET;
    sa.sin_port = htons(Port);
    sa.sin_addr.s_addr = IPAddr;
    Status = connect(Socket, (struct sockaddr *)&sa, sizeof(sa));
    if (Status < 0) {
      closesocket(Socket);
    }
  }
  return Status < 0 ? -1 : Socket;
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Send data to socket.
*
*  Parameters
*    Socket  - Socket to write to.
*    pData   - Pointer to octet string to send.
*    DataLen - Octet length of the octet string to send.
*    Flags   - Socket send flags.
*
*  Return value
*    >= 0 - Number of bytes sent.
*    <  0 - Error.
*
*  Additional information
*    The number of bytes sent can be less than the number
*    of bytes that were requested to be written.
*/
static int _Send(int Socket, const char *pData, int DataLen, int Flags) {
  int Status;
  U32 T0;
  //
  T0 = OS_GetTime();
  Status = send(Socket, pData, DataLen, Flags);
  T0 = OS_GetTime() - T0;
  _TSocket += T0;
  //
  return Status < 0 ? -1 : Status;
}

/*********************************************************************
*
*       _Recv()
*
*  Function description
*    Receive data from socket.
*
*  Parameters
*    Socket  - Socket to read from.
*    pData   - Pointer to object that receives an octet string.
*    DataLen - Octet length of the octet string to receive.
*
*  Return value
*    >= 0 - Number of bytes received.
*    <  0 - Error.
*
*  Additional information
*    The number of bytes received can be less than the number
*    of bytes requested.
*/
static int _Recv(int Socket, char *pData, int DataLen, int Flags) {
  int Status;
  U32 T0;
  //
  T0 = OS_GetTime();
  Status = recv(Socket, pData, DataLen, Flags);
  T0 = OS_GetTime() - T0;
  _TSocket += T0;
  //
  return Status < 0 ? -1 : Status;
}

/*********************************************************************
*
*       _ScanTask()
*
*  Function description
*    Scans server for available, common cipher suites.
*/
static void _ScanTask(void) {
  static SSL_SESSION Session;
  unsigned           SupportedCount;
  unsigned           TestedCount;
  unsigned           i;
  int                Socket;
  int                Status;
  U32                T0;
  //
  // Don't warn that certificate validity dates are unchecked.
  //
  SSL_RemoveWarnFilter(SSL_WARN_X509);
  //
  SSL_Logf(SSL_LOG_APP, "Scanning cipher suites on %s:%d", HOST, PORT);
  //
  // Try cipher suites.
  //
  SupportedCount = 0;
  TestedCount = 0;
  for (i = 0; SSL_SUITE_FindByIndex(i); ++i) {
    const SSL_SUITE *pSuite;
    U16  aSuites[1];
    char acBuf[80];
    //
    pSuite = SSL_SUITE_FindByIndex(i);
    aSuites[0] = SSL_SUITE_GetID(pSuite);
    //
    // Skip preshared keys suites, we don't have any PSKs.
    //
    if (SSL_SUITE_QueryRequiresPSK(pSuite)) {
      continue;
    }
    //
    // Skip cipher suites that have null encryption.
    //
    if (SSL_SUITE_QueryNull(pSuite)) {
      continue;
    }
    //
    // We're going to test using this suite.
    //
    ++TestedCount;
    //
    do {
      T0 = OS_GetTime();
      Socket = _Open(HOST, PORT);
      if (Socket > 0) {
        break;
      }
    } while (Socket < 0);
    _TSocket = OS_GetTime() - T0;
    //
    // Set up socket transport.
    //
    SSL_SESSION_Prepare(&Session, Socket, &_IP_Transport);
    SSL_SESSION_SetAllowedSuites(&Session, aSuites, 1);
    SSL_SESSION_SetProtocolRange(&Session, SSL_PROTOCOL_ID_TLS_1v0, SSL_PROTOCOL_ID_TLS_1v2);
    //
    Status = SSL_SESSION_Connect(&Session, HOST);
    T0 = OS_GetTime() - T0;
    //
    SSL_SUITE_CopyName(acBuf, pSuite);
    if (Status < 0) {
      // Uncomment this to see failures
      SSL_Logf(SSL_LOG_APP, "%04X  %-45s %s", pSuite->ID, acBuf, SSL_ERROR_GetText(Status));
    } else {
      SSL_Logf(SSL_LOG_APP, "%04X  %-45s %s %5u ms processing, %5u ms socket, %5u ms total", pSuite->ID, acBuf, SSL_PROTOCOL_GetText(SSL_SESSION_GetProtocol(&Session)), T0-_TSocket, _TSocket, T0);
      ++SupportedCount;
    }
    SSL_SESSION_Disconnect(&Session);
    //
    closesocket(Socket);
  }
  //
  SSL_Logf(SSL_LOG_APP, "\n%d common cipher suites out of %d tested\n", SupportedCount, TestedCount);
  OS_Terminate(0);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Application entry point.
*/
void MainTask(void);
void MainTask(void) {
  IP_Init();
  SSL_Init();
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task   , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP task
#if USE_RX_TASK 
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask , TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  while (IP_IFaceIsReady() == 0) {
    OS_Delay(50);
  }
  //
  // SSL connections generally require more stack than MainTask()
  // is given by the canned startup code.  Therefore we start the
  // SSL-based task with more stack so that it runs correctly and
  // kill off MainTask.
  //
  OS_CREATETASK(&_SSLTCB, "ScanTask", _ScanTask, TASK_PRIO_IP_APP, _SSLStack);
  OS_TerminateTask(0);
}

/*************************** End of file ****************************/
