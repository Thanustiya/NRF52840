/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2019  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       IoT Toolkit * HTTP Client and JSON Parser                    *
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
*       IoT Toolkit version: V2.32                                   *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : IOT_HTTP_SecureGet.c
Purpose     : REST API access using plain sockets and TLS.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT.h"
#include "SSL.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _PlainConnect    (void *pVoid, const char *sHost, unsigned Port);
static int _PlainDisconnect (void *pVoid);
static int _PlainSend       (void *pVoid, const void *pData, unsigned DataLen);
static int _PlainRecv       (void *pVoid,       void *pData, unsigned DataLen);
//
static int _SecureConnect   (void *pVoid, const char *sHost, unsigned Port);
static int _SecureDisconnect(void *pVoid);
static int _SecureSend      (void *pVoid, const void *pData, unsigned DataLen);
static int _SecureRecv      (void *pVoid,       void *pData, unsigned DataLen);

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  unsigned    Socket;
  SSL_SESSION Session;
} CONNECTION_CONTEXT;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const IOT_IO_API _PlainIO = {
  _PlainConnect,
  _PlainDisconnect,
  _PlainSend,
  _PlainRecv
};

static const IOT_IO_API _SecureIO = {
  _SecureConnect,
  _SecureDisconnect,
  _SecureSend,
  _SecureRecv
};

static const SSL_TRANSPORT_API _IP_Transport = {
  SEGGER_SYS_IP_Send,
  SEGGER_SYS_IP_Recv,
  NULL
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static char _aRedirectURL[128];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _PlainConnect()
*
*  Function description
*    Connect to host using secure sockets.
*
*  Parameters
*    pVoid - Pointer to connection context.
*    sHost - Name of server we wish to connect to.
*    Port  - Port number in host byte order.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _PlainConnect(void *pVoid, const char *sHost, unsigned Port) {
  int * pSocket;
  int   Status;
  //
  Status = SEGGER_SYS_IP_Open(sHost, Port);
  if (Status >= 0) {
    pSocket  = pVoid;
    *pSocket = Status;
  }
  return Status;
}

/*********************************************************************
*
*       _PlainDisconnect()
*
*  Function description
*    Disconnect from host.
*
*  Parameters
*    pVoid - Pointer to connection context.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _PlainDisconnect(void *pVoid) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  SEGGER_SYS_IP_Close(pConn->Socket);
  //
  return 0;
}

/*********************************************************************
*
*       _PlainSend()
*
*  Function description
*    Send data to host.
*
*  Parameters
*    pVoid   - Pointer to connection context.
*    pData   - Pointer to octet string to send.
*    DataLen - Octet length of the octet string to send.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _PlainSend(void *pVoid, const void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  return SEGGER_SYS_IP_Send(pConn->Socket, pData, DataLen, 0);
}

/*********************************************************************
*
*       _PlainRecv()
*
*  Function description
*    Receive data from host.
*
*  Parameters
*    pVoid   - Pointer to connection context.
*    pData   - Pointer to object that receives the data.
*    DataLen - Octet length of receiving object.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _PlainRecv(void *pVoid, void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  return SEGGER_SYS_IP_Recv(pConn->Socket, pData, DataLen, 0);
}

/*********************************************************************
*
*       _SecureConnect()
*
*  Function description
*    Connect to host, secure.
*
*  Parameters
*    pVoid - Pointer to connection context.
*    sHost - Name of server we wish to connect to.
*    Port  - Port number in host byte order.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _SecureConnect(void *pVoid, const char *sHost, unsigned Port) {
  CONNECTION_CONTEXT * pConn;
  int                  Status;
  //
  pConn = pVoid;
  Status = SEGGER_SYS_IP_Open(sHost, Port);
  if (Status >= 0) {
    pConn->Socket = Status;
    SSL_SESSION_Prepare(&pConn->Session, pConn->Socket, &_IP_Transport);
    Status = SSL_SESSION_Connect(&pConn->Session, sHost);
    if (Status < 0) {
      SEGGER_SYS_IP_Close(pConn->Socket);
    }
  }
  return Status;
}

/*********************************************************************
*
*       _SecureDisconnect()
*
*  Function description
*    Disconnect from host, secure.
*
*  Parameters
*    pVoid - Pointer to connection context.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _SecureDisconnect(void *pVoid) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  SSL_SESSION_Disconnect(&pConn->Session);
  SEGGER_SYS_IP_Close(pConn->Socket);
  //
  return 0;
}

/*********************************************************************
*
*       _SecureSend()
*
*  Function description
*    Send data to host, secure.
*
*  Parameters
*    pVoid   - Pointer to connection context.
*    pData   - Pointer to octet string to send over SSL.
*    DataLen - Octet length of the octet string to send.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _SecureSend(void *pVoid, const void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  return SSL_SESSION_Send(&pConn->Session, pData, DataLen);
}

/*********************************************************************
*
*       _SecureRecv()
*
*  Function description
*    Receive data from host, secure.
*
*  Parameters
*    pVoid   - Pointer to connection context.
*    pData   - Pointer to object that receives the data over SSL.
*    DataLen - Octet length of receiving object.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Processing error.
*/
static int _SecureRecv(void *pVoid, void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT * pConn;
  //
  pConn = pVoid;
  return SSL_SESSION_Receive(&pConn->Session, pData, DataLen);
}

/*********************************************************************
*
*       _HeaderCallback()
*
*  Function description
*    Process response header.
*
*  Parameters
*    pContext - Pointer to HTTP request context.
*    sKey     - Pointer to key string.
*    sValue   - Pointer to value string.
*/
static void _HeaderCallback(      IOT_HTTP_CONTEXT * pContext,
                            const char             * sKey,
                            const char             * sValue) {
  SEGGER_USE_PARA(pContext);
  if (IOT_STRCASECMP(sKey, "Location") == 0) {
    if (strlen(sValue)+1 < sizeof(_aRedirectURL)) {
      strcpy(_aRedirectURL, sValue);
    }
  }
}

/*********************************************************************
*
*       _ParseURL()
*
*  Function description
*    Parse a URL for the HTTP(S) scheme.
*
*  Parameters
*    pContext - Pointer to HTTP request context.
*    sText    - Pointer to zero-terminated URL.
*/
static void _ParseURL(IOT_HTTP_CONTEXT *pContext, char *sText) {
  char *pPos;
  char *sHost;
  char *sPath;
  //
  if (IOT_STRNCMP(sText, "https:", 6) == 0) {
    IOT_HTTP_AddScheme(pContext, "https");
    IOT_HTTP_SetPort  (pContext, 443);
    sText += 6;
  } else if (IOT_STRNCMP(sText, "http:", 5) == 0) {
    IOT_HTTP_AddScheme(pContext, "http");
    IOT_HTTP_SetPort  (pContext, 80);
    sText += 5;
  } else {
    IOT_HTTP_AddScheme(pContext, "http");
    IOT_HTTP_SetPort  (pContext, 80);
  }
  //
  if (IOT_STRNCMP(sText, "//", 2) == 0) {
    sText += 2;
  }
  sHost = sText;
  //
  pPos = IOT_STRCHR(sHost, '/');
  if (pPos) {
    *pPos = '\0';
    sPath = pPos + 1;
  } else {
    sPath = "";
  }
  //
  pPos = IOT_STRCHR(sHost, ':');
  if (pPos) {
    *pPos = '\0';
    IOT_HTTP_SetPort(pContext, (unsigned)strtoul(pPos+1, NULL, 0));
  }
  //
  IOT_HTTP_AddHost(pContext, sHost);
  IOT_HTTP_AddPath(pContext, "/");
  IOT_HTTP_AddPath(pContext, sPath);
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
  IOT_HTTP_CONTEXT   HTTP;
  IOT_HTTP_PARA      aPara[20];
  CONNECTION_CONTEXT Connection;
  char               aBuf[128];
  char               aPayload[128];
  unsigned           StatusCode;
  int                Status;
  //
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  SSL_Init();
  //
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_PlainIO, &Connection);
  IOT_HTTP_SetVersion(&HTTP, &IOT_HTTP_VERSION_HTTP_1v1);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_AddHost   (&HTTP, "www.segger.com");
  IOT_HTTP_SetPort   (&HTTP, 80);
  IOT_HTTP_AddPath   (&HTTP, "/");
  //
  for (;;) {
    Status = IOT_HTTP_Connect(&HTTP);
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("Cannot negotiate a connection to %s:%d!\n",
                           IOT_HTTP_GetHost(&HTTP),
                           IOT_HTTP_GetPort(&HTTP));
      SEGGER_SYS_OS_Halt(100);
    }
    //
    Status = IOT_HTTP_Exec(&HTTP);
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("Cannot execute GET request!\n",
                           IOT_HTTP_GetMethod(&HTTP));
      SEGGER_SYS_OS_Halt(100);
    }
    //
    Status = IOT_HTTP_ProcessStatusLine(&HTTP, &StatusCode);
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("Cannot process status line!\n");
      SEGGER_SYS_OS_Halt(100);
    }
    SEGGER_SYS_IO_Printf("Returned status code: %u\n\n", StatusCode);
    //
    Status = IOT_HTTP_ProcessHeaders(&HTTP, _HeaderCallback);
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("Cannot process headers!\n");
      SEGGER_SYS_OS_Halt(100);
    }
    //
    if (StatusCode == 301 || StatusCode == 302 ||
        StatusCode == 303 || StatusCode == 307 ) {
      //
      SEGGER_SYS_IO_Printf("Redirect to %s\n\n", _aRedirectURL);
      //
      IOT_HTTP_Disconnect(&HTTP);
      IOT_HTTP_Reset(&HTTP);
      IOT_HTTP_AddMethod(&HTTP, "GET");
      _ParseURL(&HTTP, _aRedirectURL);
      //
      if (IOT_STRCMP(IOT_HTTP_GetScheme(&HTTP), "http") == 0) {
        IOT_HTTP_SetIO(&HTTP, &_PlainIO, &Connection);
      } else if (IOT_STRCMP(IOT_HTTP_GetScheme(&HTTP), "https") == 0) {
        IOT_HTTP_SetIO(&HTTP, &_SecureIO, &Connection);
      } else {
        SEGGER_SYS_IO_Printf("Cannot handle scheme %s!\n",
                             IOT_HTTP_GetScheme(&HTTP));
        SEGGER_SYS_OS_Halt(100);
      }
    } else {
      break;
    }
  }
  //
  IOT_HTTP_GetBegin(&HTTP);
  do {
    Status = IOT_HTTP_GetPayload(&HTTP, &aPayload[0], sizeof(aPayload));
    SEGGER_SYS_IO_Printf("%.*s", Status, aPayload);
  } while (Status > 0);
  IOT_HTTP_GetEnd(&HTTP);
  //
  IOT_HTTP_Disconnect(&HTTP);
  //
  SSL_Exit();
  SEGGER_SYS_IP_Exit();
  SEGGER_SYS_Exit();
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
