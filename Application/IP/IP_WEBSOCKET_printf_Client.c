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

Purpose : Simple WebSocket terminal that periodically sends a timestamp
          text message to a server.
          The WebSocket protocol used in this sample is "debug".

Example output:
          ...
          3:025 WebSocketClient - APP: Connected to 192.168.101.202:8181.
          3:027 WebSocketClient - WebSocket successfully opened.
          ...

Additional information:
  The sample can easily be configured to start the emWeb server with different configurations and features
  such as SSL support.

  Preparations:
    When using SSL: To use this sample together with the WEBSOCKET_printf_Server sample, it is necessary to
    generate a self-signed SSL certificate for authentication.

    First, use openSSL to generate a RSA private key, and then generate a new certificate using this key.
    The 'Common Name'for the certificate must be the URL of the server, or in this case the IP address of
    the server.

    The certificate and key now need to be installed into emSSL. Convert both to a C array, using the Bin2C
    tool supplied with emSSL. The .c and .h files then need to be imported into the Shared\SSL\Sample\Certificates
    folder.

    In SSL_X_Config.c, add the certificate in _GetCertificate() and the private key in _GetPrivateKey().
    Please refer to the SSL documentation (UM15001_emSSL) chapter 3.61 'Creating certificates using OpenSSL'
    and 3.7.1 'Installing a single RSA certificate and key' for more information, including the exact commands
    necessary to generate the keys and certificate.

    Use the PrintCert tool, also supplied with emSSL, to add the certificate to SSL_X_TrustedCerts.c, as
    described in chapter 3.7.3 'Installing root certificates' in UM15001_emSSL. Finally, add this root
    certificate in SSL_X_Config() in SSL_X_Config.c.


    Enable/disable the features that you want to use. The available
    binary configuration switches are:
      - APP_USE_SSL : Enable creation of a secured IPv4 socket on port 443 (default). Requires emSSL.

*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_WEBSOCKET.h"

#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   WEBS_STRCMP
  #define WEBS_STRCMP          strcmp
#endif

#ifndef   WEBS_STRLEN
  #define WEBS_STRLEN          strlen
#endif

#ifndef   WEBS_MEMSET
  #define WEBS_MEMSET          memset
#endif

#define USE_RX_TASK      0                           // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#ifndef   APP_USE_SSL
  #define APP_USE_SSL   (0)                          // If enabled, creates SSL sockets as well.
#endif

#if (APP_USE_SSL != 0)
  #include "SSL.h"
#endif

//
// WebSocket sample configuration.
//
#define SERVER_HOST      "192.168.88.12"             // Host to connect to.
#if (APP_USE_SSL != 0)
#define SERVER_PORT      443                         // Use a port other than 80 as on Windows 10 this port is used by default.
#else
#define SERVER_PORT      8181                        // Use a port other than 80 as on Windows 10 this port is used by default.
#endif
#define WEBSOCKET_PROTO  "debug"                     // WebSocket subprotocol sent back.
#define WEBSOCKET_KEY    "vqbvaT9fjaLvKG4UM6RuvQ=="  // Connect key, can be randomized but does not add to security.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_WEBSOCKET = 150
  ,TASK_PRIO_IP_TASK         // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK      // Must be the highest priority of all IP related tasks.
#endif
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD     0
#endif

#ifndef     APP_MAIN_STACK_SIZE
  #if (APP_USE_SSL != 0)
    #define   APP_MAIN_STACK_SIZE      (5120)
  #else
    #define   APP_MAIN_STACK_SIZE      (1280)
  #endif
#endif
/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define OPEN_REQUEST  "GET /printf HTTP/1.1\r\n"       \
                      "Host: %s:%d\r\n"                \
                      "Connection: Upgrade\r\n"        \
                      "Upgrade: websocket\r\n"         \
                      "Sec-WebSocket-Version: 13\r\n"  \
                      "Sec-WebSocket-Key: %s\r\n"      \
                      "Sec-WebSocket-Protocol: %s\r\n" \
                      "\r\n"

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

static int _cbWebSocket_Recv      (IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection,       void* pData, unsigned NumBytes);
static int _cbWebSocket_Send      (IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes);
static U32 _cbWebSocket_GenMaskKey(void);

#if (APP_USE_SSL != 0)
static int _cbWebSocket_SSL_Recv  (IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection,       void* pData, unsigned NumBytes);
static int _cbWebSocket_SSL_Send  (IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes);
#endif

#if (APP_USE_SSL != 0)
static const SSL_TRANSPORT_API _IP_Transport;
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static int _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                 // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                        // Task-Control-Block of the IP_Task.

static OS_STACKPTR int APP_MainStack[APP_MAIN_STACK_SIZE / sizeof(int)];    // Stack of the starting point of this sample.
static OS_TASK         APP_MainTCB;                                         // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];            // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                      // Task-Control-Block of the IP_RxTask.
#endif

//
// WebSocket
//
static char _acBuffer[1024];

#if (APP_USE_SSL != 0)
//
// SSL
//
static SSL_SESSION _SSLSession;
#endif

static const IP_WEBSOCKET_TRANSPORT_API _WebSocketTransportAPI = {
#if (APP_USE_SSL != 0)
  _cbWebSocket_SSL_Recv,  // pfReceive
  _cbWebSocket_SSL_Send,  // pfSend
  _cbWebSocket_GenMaskKey // pfGenMaskKey, client only.
#else
  _cbWebSocket_Recv,      // pfReceive
  _cbWebSocket_Send,      // pfSend
  _cbWebSocket_GenMaskKey // pfGenMaskKey, client only.
#endif

};

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbWebSocket_Send()
*
*  Function description
*    WebSocket callback that for sending data using the underlying
*    network communication API (typically BSD socket API).
*
*  Parameters
*    pContext   : WebSocket context.
*    pConnection: Network connection handle.
*    pData      : Data to send.
*    NumBytes   : Amount of data to send.
*
*  Return value
*    Amount of data sent: >  0
*    Connection closed  : == 0
*    Error              : <  0
*/
static int _cbWebSocket_Send(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  return send((long)pConnection, (const char*)pData, NumBytes, 0);
}

/*********************************************************************
*
*       _cbWebSocket_Recv()
*
*  Function description
*    WebSocket callback that for sending data using the underlying
*    network communication API (typically BSD socket API).
*
*  Parameters
*    pContext   : WebSocket context.
*    pConnection: Network connection handle.
*    pData      : Where to store the received data.
*    NumBytes   : Maximum amount of data to receive.
*
*  Return value
*    Amount of data received: >  0
*    Connection closed      : == 0
*    Error                  : <  0
*/
static int _cbWebSocket_Recv(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  return recv((long)pConnection, (char*)pData, NumBytes, 0);
}

/*********************************************************************
*
*       _cbWebSocket_GenMaskKey()
*
*  Function description
*    Generates a 4 byte mask key for encoding data sent by a
*    WebSocket client.
*
*  Additional information
*    The mask key does not increase the security of the connection
*    and is more like a checksum for the data. Therefore a simple
*    implementation like the current system time is sufficient.
*
*  Return value
*    U32 mask key to use for the next frame sent.
*/
static U32 _cbWebSocket_GenMaskKey(void) {
  return OS_GetTime32();
}

#if (APP_USE_SSL != 0)

/*********************************************************************
*
*       _cbWebSocket_Send_SSL()
*/
static int _cbWebSocket_SSL_Send(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  SSL_SESSION* pSession;
  int          r;

  pSession = (SSL_SESSION*)pConnection;
  r = SSL_SESSION_Send(pSession, pData, NumBytes);
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
  return send(Socket, pData, Len, Flags);
}

/*********************************************************************
*
*       _cbWebSocket_Recv_SSL()
*/
static int _cbWebSocket_SSL_Recv(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  SSL_SESSION* pSession;
  int          r;

  pSession = (SSL_SESSION*)pConnection;
    do {
      r = SSL_SESSION_Receive(pSession, pData, NumBytes);
    } while (r == 0);
    //
    // Translate EOF into "connection closed",
    // the Websocket expects same return values as with recv().
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
  return recv(Socket, pData, Len, Flags);
}

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
  NULL,
  NULL
};

#endif

/*********************************************************************
*
*       _IsIPAddress()
*
*  Function description
*    Checks if string is a dot-decimal IP address, for example 192.168.1.1
*/
static unsigned _IsIPAddress(const char* sIPAddr) {
  unsigned NumDots;
  unsigned i;
  char c;

  NumDots = 0;
  i       = 0;
  while (1) {
    c = *(sIPAddr + i);
    if ((c >= '0') && (c <= '9')) {
      goto Loop;
    }
    if (c == '.') {
      NumDots++;
      goto Loop;
    }
    if (c == '\0') {
      if ((NumDots < 3) || (i > 15)) { // Error, every dot-decimal IP address includes 3 '.' and is not longer as 15 characters.
Error:
        return 0;
      }
      return 1;
    } else {
      goto Error;
    }
Loop:
    i++;
  }
}

/*********************************************************************
*
*       _ParseIPAddr()
*
*  Function description
*    Parses a string for a dot-decimal IP address and returns the
*    IP as 32-bit number in host endianness.
*/
static long _ParseIPAddr(const char* sIPAddr) {
  long     IPAddr;
  unsigned Value;
  unsigned NumDots;
  unsigned i;
  unsigned j;
  char     acDigits[4];
  char     c;

  IPAddr = 0;
  //
  // Check if string is a valid IP address.
  //
  Value = _IsIPAddress(sIPAddr);
  if (Value) {
    //
    // Parse the IP address.
    //
    NumDots = 3;
    i       = 0;
    j       = 0;
    while (1) {
      c = *(sIPAddr + i);
      if (c == '\0') {
        //
        // Add the last byte of the IP address.
        //
        acDigits[j] = '\0';
        Value = SEGGER_atoi(acDigits);
        if (Value < 255) {
          IPAddr |= Value;
        }
        return IPAddr; // O.K., string completely parsed. Returning IP address.
      }
      //
      // Parse the first three byte of the IP address.
      //
      if (c != '.') {
        acDigits[j] = c;
        j++;
      } else {
        acDigits[j] = '\0';
        Value = SEGGER_atoi(acDigits);
        if (Value <= 255) {
          IPAddr |= (Value << (NumDots * 8));
          NumDots--;
          j = 0;
        } else {
          return -1;  // Error, illegal number in IP address.
        }
      }
      i++;
    }
  }
  return -1;
}

/*********************************************************************
*
*       _Panic()
*
*  Function description
*    Error/panic collection routine.
*
*  Additional information
*    This is an error routine that collects all cases that are dead
*    ends in the program flow. With blocking sockets we should never
*    end in here as the sample will handle the return values that are
*    returned by a blocking socket I/O.
*
*    When using this sample with non-blocking socket I/O and ending up
*    in here, this is an indicator that you are not handling all
*    non-blocking return values correctly. Typically the case missing
*    is to check for a WOULDBLOCK socket error when an API has
*    returned with SOCKET_ERROR (-1).
*/
static void _Panic(void) {
  IP_Logf_Application("Error, program halted!\n");
  for (;;);
}

/*********************************************************************
*
*       _WebSocketTask()
*/
static void _WebSocketTask(void) {
  char*                sAccept;
  char*                s;
  IP_fd_set            ReadFds;
  IP_WEBSOCKET_CONTEXT WebSocketContext;
  struct sockaddr_in   Addr;
  struct hostent*      pHostEntry;
  U32                  IPAddr;
  long                 hSock;
  int                  r;
  int                  Len;
  U8                   MessageType;
  char                 ac[64];

  if (_IsIPAddress(SERVER_HOST)) {
    IPAddr = _ParseIPAddr(SERVER_HOST);
    IPAddr = htonl(IPAddr);
  } else {
    //
    // Convert host into IP address.
    //
    pHostEntry = gethostbyname((char*)SERVER_HOST);
    if (pHostEntry == NULL) {
      IP_Logf_Application("APP: gethostbyname failed: %s\n", SERVER_HOST);
      return;  // Error.
    }
    IPAddr = *(U32*)(*pHostEntry->h_addr_list);
  }
  for (;;) {
    //
    // Create socket and connect to the server.
    //
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if(hSock == SOCKET_ERROR) {
      while(1); // This should never happen!
    }
    //
    // Connect.
    //
    WEBS_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(SERVER_PORT);
    Addr.sin_addr.s_addr = IPAddr;
    r = connect(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
    if(r == SOCKET_ERROR) {
      IP_Logf_Application("APP: Connect error.\n");
      goto OnError;
    }
    IP_Logf_Application("APP: Connected to %s:%d.\n", SERVER_HOST, SERVER_PORT);

#if (APP_USE_SSL != 0)
    //
    // Activate Security
    //
    SSL_SESSION_Prepare(&_SSLSession, hSock, &_IP_Transport);
    //
    // Connect.
    //
    r = SSL_SESSION_Connect(&_SSLSession, SERVER_HOST);
    if(r < 0) {
      IP_Logf_Application("APP: SSL upgrade error : %d", r);
      closesocket(hSock);
      return;
    }
    //
    if (_SSLSession.State == SSL_CONNECTED) {
      IP_Logf_Application("APP: Secured using %s.", SSL_SUITE_GetIANASuiteName(SSL_SUITE_GetID(SSL_SESSION_GetSuite(&_SSLSession))));
    }
    //
    IP_Logf_Application("APP: Connected to %i, port %d.", IPAddr, SERVER_PORT);
#endif

    //
    // Generate the accept response key that is expected to be returned by the server.
    //
    r = IP_WEBSOCKET_GenerateAcceptKey((void*)WEBSOCKET_KEY, sizeof(WEBSOCKET_KEY) - 1, (void*)&ac[0], sizeof(ac) - 1);
    if (r == 0) {
      IP_Logf_Application("Buffer for accept key is not big enough.\n");
      goto OnError;  // Error.
    }
    ac[r] = '\0';
    //
    // Create the request message and send it to the server.
    //
    r = SEGGER_snprintf(_acBuffer, sizeof(_acBuffer), OPEN_REQUEST, SERVER_HOST, SERVER_PORT, WEBSOCKET_KEY, WEBSOCKET_PROTO);
    if (r == sizeof(_acBuffer)) {
      IP_Logf_Application("_acBuffer is too small for the response.\n");
      goto OnError;  // Error.
    }
#if (APP_USE_SSL != 0)
    r = SSL_SESSION_Send(&_SSLSession, (const char*)_acBuffer, WEBS_STRLEN(_acBuffer));
#else
    r = send(hSock, (const char*)_acBuffer, WEBS_STRLEN(_acBuffer), 0);
#endif
    if (r == SOCKET_ERROR) {
        IP_Logf_Application("send(): socket error.\n");
        goto OnError;  // Error.
    }
    //
    // Read response HTTP header.
    //
#if (APP_USE_SSL != 0)
    r = SSL_SESSION_Receive(&_SSLSession, _acBuffer, WEBS_STRLEN(_acBuffer));
    //
    // Translate EOF into "connection closed",
    // the Websocket expects same return values as with recv().
    //
    if (r == SSL_ERROR_EOF) {
      r = 0;
    }
#else
    r = recv(hSock, _acBuffer, sizeof(_acBuffer), 0);
#endif
    if (r == sizeof(_acBuffer)) {
      IP_Logf_Application("_acBuffer is too small to read the complete HTTP header.\n");
      goto OnError;  // Error.
    }
    if (r <= 0) {
      goto OnError;  // Error.
    }
    _acBuffer[r] = '\0';  // Treat the HTTP header in buffer as one big string.
    //
    // Find Sec-WebSocket-Accept field value in HTTP header.
    //
    sAccept = strstr(&_acBuffer[0], "Sec-WebSocket-Accept: ");
    if (sAccept == NULL) {
      IP_Logf_Application("Sec-WebSocket-Accept field not found.\n");
      goto OnError;  // Error.
    }
    sAccept += sizeof("Sec-WebSocket-Accept: ") - 1;
    s = strstr(sAccept, "\r\n");
    if (s == NULL) {
      IP_Logf_Application("End of Sec-WebSocket-Accept field not found.\n");
      goto OnError;  // Error.
    }
    *s = '\0';       // Terminate as string for easier usage.
    //
    // Check if the expected accept key has been received.
    //
    if (WEBS_STRCMP(&ac[0], sAccept) != 0) {
      IP_Logf_Application("Wrong accept key received:\n");
      IP_Logf_Application("Expected: %s\n", &ac[0]);
      IP_Logf_Application("Received: %s\n", sAccept);
      goto OnError;  // Error.
    }
    IP_Logf_Application("WebSocket successfully opened.\n");
    //
    // Initialize the WebSocket context for the client.
    //
    IP_WEBSOCKET_InitClient(&WebSocketContext, &_WebSocketTransportAPI, (IP_WEBSOCKET_CONNECTION*)hSock);
#if (APP_USE_SSL != 0)
    //
    // add SSL Session to Connection Context
    //
    WebSocketContext.pConnection = (IP_WEBSOCKET_CONNECTION*)&_SSLSession;
#endif
    //
    // Process WebSocket messages (if any) and periodically send the time.
    //
    for (;;) {
      IP_FD_ZERO(&ReadFds);                    // Clear the set.

#if (APP_USE_SSL != 0)
      IP_FD_SET(_SSLSession.Socket, &ReadFds);  // Add socket to the set
#else
      IP_FD_SET(hSock, &ReadFds);              // Add the socket to the set.
#endif
      r = select(&ReadFds, NULL, NULL, 1000);  // Wait 1 second for receiving a WebSocket message. This is also our period for sending the time as
                                               // it is unlikely to receive something here as our sample does not expect it. However a PING might occur.
      if (r > 0) {
        //
        // Wait/read the next message type.
        //
        do {
          r = IP_WEBSOCKET_WaitForNextMessage(&WebSocketContext, &MessageType);
          if (r == 0) {  // Connection closed.
            goto OnDisconnect;
          }
          if ((r < 0) && (r != IP_WEBSOCKET_ERR_AGAIN)) {
            _Panic();
          }
        } while (r == IP_WEBSOCKET_ERR_AGAIN);
        //
        // Evaluate the message type.
        //
        if (MessageType == IP_WEBSOCKET_FRAME_TYPE_CLOSE) {
          //
          // Send a close frame with a goodbye message.
          //
          r = SEGGER_snprintf(ac, sizeof(ac), "Bye, bye");
          if (r == sizeof(ac)) {
            IP_Logf_Application("ac buffer is too small for close payload.\n");
            goto OnError;  // Error.
          }
          do {
            //
            // In this special case do not check if this is an error
            // other than AGAIN as WebSockets are explicitly allowed
            // to be closed on socket level. The next call might return
            // with a real error as the other side has already closed
            // the socket, this is fine for this single case. Simply
            // close the socket in this case.
            //
            r = IP_WEBSOCKET_Close(&WebSocketContext, &ac[0], IP_WEBSOCKET_CLOSE_CODE_NORMAL_CLOSURE);
          } while (r == IP_WEBSOCKET_ERR_AGAIN);
          break;
        } else {
          //
          // Discard other messages.
          //
          do {
            r = IP_WEBSOCKET_DiscardMessage(&WebSocketContext);
            if (r == 0) {
              goto OnDisconnect;
            }
            if ((r < 0) && (r != IP_WEBSOCKET_ERR_AGAIN)) {
              _Panic();
            }
          } while (r == IP_WEBSOCKET_ERR_AGAIN);
        }
      }
      //
      // Send our time to the server.
      //
      Len = SEGGER_snprintf(&ac[0], sizeof(ac), "OS time: %lu", OS_GetTime32());
      if (Len == sizeof(ac)) {
        IP_Logf_Application("ac buffer is too small for message.\n");
        goto OnError;  // Error.
      }
      do {
        r = IP_WEBSOCKET_Send(&WebSocketContext, &ac[0], Len, IP_WEBSOCKET_FRAME_TYPE_TEXT, 0);
        if (r == IP_WEBSOCKET_ERR_AGAIN) {
          continue;
        } else if (r <= 0) {
          IP_Logf_Application("Error sending timestamp.\n");
          _Panic();
        }
      } while (r <= 0);
    }
OnDisconnect:
    IP_Logf_Application("WebSocket closed.\n");
OnError:
    closesocket(hSock);
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
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_WEBSOCKET);
  OS_SetTaskName(OS_GetTaskID(), "WebSocketClient");
#if ((APP_USE_SSL != 0))
  //
  // Initialize SSL.
  //
  SSL_Init();
#endif
  _WebSocketTask();
  OS_TASK_Terminate(NULL);
}

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
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  //
  // IPv4 address configured ?
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(0);
    OS_Delay(200);
  }
  BSP_ClrLED(0);
  //
  // Start the sample itself.
  //
  OS_CREATETASK(&APP_MainTCB, "APP_MainTask", APP_MainTask, TASK_PRIO_IP_TASK - 1, APP_MainStack);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
