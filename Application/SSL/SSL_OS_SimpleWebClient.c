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

File        : SSL_OS_SimpleWebClient.c
Purpose     : Example to show secure connection to www.segger.com
              to retrieve the default web page.

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

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define HOST         "www.segger.com"
#define PAGE         "/emssl-testpage.php"

#define USE_RX_TASK 0

//
// Task priorities
//
enum {
  TASK_PRIO_IP_APP = 150,
  TASK_PRIO_IP_TASK,         // Priority should be higher than all IP application tasks.
  TASK_PRIO_IP_RX_TASK       // Must be the highest priority of all IP related tasks, comment out to read packets in ISR
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
  NULL,
  NULL
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int _IPStack[2048/sizeof(int)];
static OS_TASK         _IPTCB;

static OS_STACKPTR int _SSLStack[4096/sizeof(int)];
static OS_TASK         _SSLTCB;

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[1024/sizeof(int)];
static OS_TASK         _IPRxTCB;
#endif

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
  //
  Status = send(Socket, pData, DataLen, Flags);
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
*    Flags   - Optional flags.
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
  //
  Status = recv(Socket, pData, DataLen, Flags);
  //
  return Status < 0 ? -1 : Status;
}

/*********************************************************************
*
*       _ClientTask()
*
*  Function description
*    Read data from web server.
*/
static void _ClientTask(void) {
  static SSL_SESSION Session;
  static char        acBuf[256];
  int                Socket;
  int                Status;
  //
  // Don't warn that certificate validity dates are unchecked.
  //
  SSL_RemoveWarnFilter(SSL_WARN_X509);
  //
  // Open a plain socket to www.segger.com on the default
  // HTTPS port, 443.
  //
  Socket = _Open(HOST, 443);
  if (Socket < 0) {
    SSL_Logf(SSL_LOG_APP, "Cannot open " HOST ":443!");
    SSL_Panic(-100);
  }
  //
  // Upgrade the connection to secure by negotiating a
  // session using SSL.
  //
  SSL_SESSION_Prepare(&Session, Socket, &_IP_Transport);
  if (SSL_SESSION_Connect(&Session, HOST) < 0) {
    SSL_Logf(SSL_LOG_APP, "Cannot negotiate a secure connection to " HOST ":443!");
    SSL_Panic(-100);
  }
  //
  // We have established a secure connection, so ask the server
  // for some data.  This sends an HTTP GET request to retrieve
  // the default index page.
  //
  SSL_SESSION_SendStr(&Session, "GET " PAGE " HTTP/1.0\r\n");
  SSL_SESSION_SendStr(&Session, "Host: " HOST "\r\n");
  SSL_SESSION_SendStr(&Session, "\r\n");
  //
  // Now read the response.  We requested HTTP 1.0 which causes
  // the underlying socket to close once the reply is complete,
  // so we have no need to decode the headers.
  //
  for (;;) {
    Status = SSL_SESSION_Receive(&Session, acBuf, sizeof(acBuf)-1);
    if (Status < 0) {
      break;
    }
    acBuf[Status] = 0;
    //
    // Replace any control characters with '.' so it doesn't upset output.
    //
    while (--Status >= 0) {
      if (acBuf[Status] < 0x20 || 0x7F <= acBuf[Status]) {
        acBuf[Status] = '.';
      }
    }
    SSL_Logf(SSL_LOG_APP, "Data: %s", acBuf);
  }
  //
  // Close the SSL connection.
  //
  SSL_SESSION_Disconnect(&Session);
  closesocket(Socket);
  SSL_Logf(SSL_LOG_APP, "Data received, socket closed: done");
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
  OS_CREATETASK(&_SSLTCB, "ClientTask", _ClientTask, TASK_PRIO_IP_APP, _SSLStack);
  OS_TerminateTask(0);
}

/*************************** End of file ****************************/
