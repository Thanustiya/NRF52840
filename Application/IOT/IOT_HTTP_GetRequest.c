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

File        : IOT_HTTP_GetRequest.c
Purpose     : Issue a GET request over a plain socket.

Additional information:
  Preparations:
    Set up the HTTP GET request in the MainTask.
    (Default is to get the index page of www.segger.com)
    For more information see the IoTToolkit manual under the chapter
    "2.2  A simple GET request".

  Expected behavior:
    Sends a custom HTTP request to a custom host and prints out the
    status code and the payload on the console.

  Sample output:
    Returned status code: 301

    <!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
    <html><head>
    <title>301 Moved Permanently</title>
    </head><body>
    <h1>Moved Permanently</h1>
    <p>The document has moved <a href="https://www.segger.com/">here</a>.</p>
    <hr>
    <address>Apache/2.4.18 (Ubuntu) Server at www.segger.com Port 80</address>
    </body></html>

    STOP.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  unsigned Socket;
} CONNECTION_CONTEXT;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _Connect   (void *pVoid, const char *sHost, unsigned Port);
static int _Disconnect(void *pVoid);
static int _Send      (void *pVoid, const void *pData, unsigned DataLen);
static int _Recv      (void *pVoid,       void *pData, unsigned DataLen);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const IOT_IO_API _PlainAPI = {
  _Connect,
  _Disconnect,
  _Send,
  _Recv
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Connect()
*
*  Function description
*    Connect to host.
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
static int _Connect(void *pVoid, const char *sHost, unsigned Port) {
  CONNECTION_CONTEXT * pConn;
  int                  Status;
  //
  pConn = pVoid;
  //
  Status = SEGGER_SYS_IP_Open(sHost, Port);
  if (Status >= 0) {
    pConn->Socket = Status;
  }
  return Status;
}

/*********************************************************************
*
*       _Disconnect()
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
static int _Disconnect(void *pVoid) {
  CONNECTION_CONTEXT *pConn;
  //
  pConn = pVoid;
  SEGGER_SYS_IP_Close(pConn->Socket);
  //
  return 0;
}

/*********************************************************************
*
*       _Send()
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
static int _Send(void *pVoid, const void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT *pConn;
  //
  pConn = pVoid;
  return SEGGER_SYS_IP_Send(pConn->Socket, pData, DataLen, 0);
}

/*********************************************************************
*
*       _Recv()
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
static int _Recv(void *pVoid, void *pData, unsigned DataLen) {
  CONNECTION_CONTEXT *pConn;
  //
  pConn = pVoid;
  return SEGGER_SYS_IP_Recv(pConn->Socket, pData, DataLen, 0);
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
  //
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_PlainAPI, &Connection);
  IOT_HTTP_SetVersion(&HTTP, &IOT_HTTP_VERSION_HTTP_1v1);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_AddHost   (&HTTP, "www.segger.com");
  IOT_HTTP_SetPort   (&HTTP, 80);
  IOT_HTTP_AddPath   (&HTTP, "/");
  //
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
  Status = IOT_HTTP_ProcessHeaders(&HTTP, NULL);
  if (Status < 0) {
    SEGGER_SYS_IO_Printf("Cannot process headers!\n");
    SEGGER_SYS_OS_Halt(100);
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
  SEGGER_SYS_IP_Exit();
  SEGGER_SYS_Exit();
  SEGGER_SYS_OS_Halt(Status);
}

/*************************** End of file ****************************/
