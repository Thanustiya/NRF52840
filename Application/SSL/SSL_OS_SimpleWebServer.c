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

File        : SSL_OS_SimpleWebServer.c
Purpose     : Simple web server that accepts incoming connections
              and serves a static welcome page under emNet.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSL.h"
#include "IP.h"
#include "RTOS.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

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

static OS_STACKPTR int _ServerStack[8192/sizeof(int)];
static OS_TASK         _ServerTCB;

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
*       _Bind()
*
*  Function description
*    Bind a TCP port.
*
*  Parameters
*    Port - Bound port.
*
*  Return value
*    >= 0 - Bound socket handle.
*    <  0 - Error.
*/
static int _Bind(int Port) {
  int Socket;
  int Status;
  int Enable;
  struct sockaddr_in local_addr;

  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(Port);
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  Socket = Status = socket(PF_INET, SOCK_STREAM, 0);
  if (Status >= 0) {
    Enable = 1;
    Status = setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&Enable, sizeof(Enable));
  }
  if (Status >= 0) {
    Status = bind(Socket, (struct sockaddr *)&local_addr, sizeof(local_addr));
  }
  if (Status >= 0) {
    Status = listen(Socket, 10);
  }
  return Status >= 0 ? Socket : Status;
}

/*********************************************************************
*
*       _Accept()
*
*  Function description
*    Accept an incoming connection.
*
*  Parameters
*    Port - Bound port.
*
*  Return value
*    >= 0 - Socket handle.
*    <  0 - Error.
*/
static int _Accept(int Socket) {
  struct sockaddr_in client_addr;
  int                client_addr_len;
  //
  client_addr_len = sizeof(client_addr);
  Socket = accept(Socket, (struct sockaddr *) &client_addr, &client_addr_len);
  if (Socket < 0) {
    return -1;
  } else {
    IP_SOCKET_SetLinger(Socket, 1);  // Server sockets are a scarce resource, so do not stay in TIME_WAIT long.
    return Socket;
  }
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
*       ServerTask()
*
*  Function description
*    Application entry point.
*/
static void _ServerTask(void) {
  static char        aData[1024];
  static SSL_SESSION Session;
  static char        acBuf[128];
  unsigned           DataLen;
  int                BoundSocket;
  int                Socket;
  int                Status;
  int                Get;
  char             * pNL;
  const SSL_SUITE  * pSuite;
  //
  BoundSocket = _Bind(443);
  if (BoundSocket < 0) {
    SSL_Panic(-100);
  }
  //
  for (;;) {
    //
    // Wait for an incoming connection.
    //
    SSL_Log("Awaiting connection");
    Socket = _Accept(BoundSocket);
    if (Socket < 0) {
      continue;
    }
    //
    // Upgrade the insecure socket to secure.
    //
    SSL_Log("Connection made, attempting to upgrade to secure");
    SSL_SESSION_Prepare(&Session, Socket, &_IP_Transport);
#ifdef REQUEST_CERTIFICATE
    SSL_SESSION_SetFlags(&Session, SSL_SESSION_FLAG_REQUEST_CERTIFICATE);
#endif
    Status = SSL_SESSION_Accept(&Session);
    //
    // Wait for requests and serve web page.
    //
    if (Status < 0) {
      SSL_Log("Can't negotiate a secure connection -- session closed");
      closesocket(Socket);
    } else {
      DataLen = 0;
      Get = 0;
      pSuite = SSL_SESSION_GetSuite(&Session);
      SSL_SUITE_CopyName(acBuf, pSuite);
      SSL_Log("Session is now secured, cipher suite follows");
      SSL_Log(acBuf);
      //
      // Process headers.
      //
      for (;;) {
        Status = SSL_SESSION_Receive(&Session, &aData[DataLen], sizeof(aData)-DataLen-1);
        if (Status < 0) {
          break;
        } else {
          DataLen += Status;
          aData[DataLen] = 0;
          pNL = strchr(aData, '\n');
          while (pNL) {
            unsigned N;
            //
            // Terminate incoming text that may have a CRLF.
            //
            *pNL = 0;
            N = strlen(aData);
            if (N >= 1 && aData[N-1] == '\r') {
              aData[N-1] = 0;
            }
            //
            // Remember if we see a GET request for the index.
            //
            if (strcmp(aData, "GET / HTTP/1.0") == 0 ||
                strcmp(aData, "GET /index.html HTTP/1.0") == 0) {
              Get = 10;
            } else if (strcmp(aData, "GET / HTTP/1.1") == 0 ||
                       strcmp(aData, "GET /index.html HTTP/1.1") == 0) {
              Get = 11;
            }
            //
            // Blank line indicating end of headers?
            //
            if (aData[0] == 0) {
              break;
            }
            //
            // Slide window along.
            //
            ++N;
            memmove(&aData[0], pNL+1, DataLen-N);
            DataLen -= N;
            aData[DataLen] = 0;
            pNL = strchr(aData, '\n');
          }
        }
        if (aData[0] == 0) {
          break;
        }
      }
      if (Status >= 0) {
        //
        // Serve a small web page, whatever the method...
        //
        if (Get) {
          //
          // GET of the index document.
          //
          SSL_SUITE_CopyName(acBuf, SSL_SESSION_GetSuite(&Session));
          SSL_SESSION_SendStr(&Session, "HTTP/1.0 200 OK\r\n");
          SSL_SESSION_SendStr(&Session, "Content-Type: text/html\r\n");
          SSL_SESSION_SendStr(&Session, "Connection: close\r\n");
          SSL_SESSION_SendStr(&Session, "\r\n");
          SSL_SESSION_SendStr(&Session, "<html><head><title>SEGGER - The Embedded Experts</title></head><body><h1>Welcome to SEGGER's emSSL Server!</h1>");
          SSL_SESSION_SendStr(&Session, "<p><b>It Simply Works!</b></p>\r\n");
          SSL_SESSION_SendStr(&Session, "<p>The agreed cipher suite is ");
          SSL_SESSION_SendStr(&Session, acBuf);
          SSL_SESSION_SendStr(&Session, ".</p><p>Have a great day!  :-)</p></body></html>\r\n");
        } else {
          //
          // GET of something else that isn't the index, or some other method.
          //
          SSL_SESSION_SendStr(&Session, "HTTP/1.1 404 Not Found\r\n\r\n");
          SSL_SESSION_SendStr(&Session, "<html><head></head><body><h1>I can't find that for you</h1></body></html>\r\n");
        }
      }
      //
      SSL_SESSION_Disconnect(&Session);
      closesocket(Socket);
      SSL_Log("Session closed");
    }
  }
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
  int IFaceId;
  IP_Init();
  IFaceId = IP_INFO_GetNumInterfaces() - 1;  // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  SSL_Init();
  //
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task   , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP task
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask , TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_Connect(IFaceId);
  while (IP_IFaceIsReady() == 0) {
    OS_Delay(50);
  }
  //
  // SSL connections generally require more stack than MainTask()
  // is given by the canned startup code.  Therefore we start the
  // SSL-based task with more stack so that it runs correctly and
  // kill off MainTask.
  //
  OS_CREATETASK(&_ServerTCB, "WebServerTask", _ServerTask, TASK_PRIO_IP_APP, _ServerStack);
  OS_TerminateTask(0);
}

/*************************** End of file ****************************/
