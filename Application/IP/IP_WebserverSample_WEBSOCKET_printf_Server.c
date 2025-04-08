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

Purpose : Demonstrates how to implement a WebSocket server behind an
          emWeb server processing all other page requests.

Additional information:
  For further details about the sample itself and its configuration
  parameters, please refer to the main sample included by this wrapper.
*/

#include "IP.h"
#include "IP_Webserver.h"
#include "IP_WEBSOCKET.h"
#include "RTOS.h"
#include "TaskPrio.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int _WebSocketStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];  // Stack of the WebSocket task
static OS_TASK         _WebSocketTCB;                                         // Task-Control-Block of the WebSocket task.

static WEBS_WEBSOCKET_HOOK _WSHook;
static void*               _pConnection;  // NULL if disconnected. Set (if NULL) when accepting a connection to tell the server task that a new connection has been opened.
static const char*         _sURI   = "/printf";
static const char*         _sProto = "debug";

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbWebSocket_GenerateAcceptKey()
*
*  Function description
*    Generates the value to send back for the Sec-WebSocket-Accept
*    field when accepting the connection.
*
*  Parameters
*    pOutput           : Web server connection context.
*    pSecWebSocketKey  : Pointer to a buffer containing the string of
*                        the Sec-WebSocket-Key from the HTTP request.
*    SecWebSocketKeyLen: Number of characters of the Sec-WebSocket-Key
*                        (without string termination).
*    pBuffer           : Buffer where to store the accept key.
*    BufferSize        : Size of buffer where to store the accept key.
*
*  Return value
*    Length of accept key        : >  0
*    Error, buffer not big enough: == 0
*/
static int _cbWebSocket_GenerateAcceptKey(WEBS_OUTPUT* pOutput, void* pSecWebSocketKey, int SecWebSocketKeyLen, void* pBuffer, int BufferSize) {
  WEBS_USE_PARA(pOutput);

  return IP_WEBSOCKET_GenerateAcceptKey(pSecWebSocketKey, SecWebSocketKeyLen, pBuffer, BufferSize);
}

/*********************************************************************
*
*       _cbWebSocket_DispatchConnection()
*
*  Function description
*    Dispatches the web server connection to the WebSocket
*    application for further handling and signals the application
*    task.
*
*  Parameters
*    pOutput    : Web server connection context.
*    pConnection: Network connection handle.
*/
static void _cbWebSocket_DispatchConnection(WEBS_OUTPUT* pOutput, void* pConnection) {
  U32 Timeout;

  WEBS_USE_PARA(pOutput);
  OS_EnterRegion();  // Make sure that _pConnection does not get set from another task after being read by us.
  //
  // Check if no other client is connected right now.
  //
  if (_pConnection == NULL) {
    //
    // Set timeout to zero.
    // This is necessary since the Web server child task uses a socket with a configured timeout (IDLE_TIMEOUT).
    // By default, the WebSocket add-on uses blocking sockets.
    //
    Timeout = 0;  // Disable timeout
    setsockopt((long)pConnection, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));  // Set receive timeout for the client.
    _pConnection = pConnection;
    OS_LeaveRegion();
  } else {
    OS_LeaveRegion();
    //
    // Another client is still connected.
    // Simply close the new socket to close the WebSocket connection as well.
    // Instead of closing the new connection the old connection could be closed for continuation with the new client.
    //
    closesocket((long)pConnection);
  }
}

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
*       WebSocket API structures
*
**********************************************************************
*/

static const IP_WEBS_WEBSOCKET_API _WebSocketAPI = {
  _cbWebSocket_GenerateAcceptKey,  // pfGenerateAcceptKey
  _cbWebSocket_DispatchConnection  // pfDispatchConnection
};

static const IP_WEBSOCKET_TRANSPORT_API _WebSocketTransportAPI = {
  _cbWebSocket_Recv,  // pfReceive
  _cbWebSocket_Send,  // pfSend
  NULL                // pfGenMaskKey, client only.
};

/*********************************************************************
*
*       _WebSocketTask()
*/
static void _WebSocketTask(void) {
  IP_WEBSOCKET_CONTEXT WebSocketContext;
  int                  r;
  int                  NumBytesRead;
  U8                   MessageType;
  char                 ac[64];
  char*                s;

  do {
    if (_pConnection == NULL) {
      OS_Delay(50);
    } else {
      //
      // Initialize the WebSocket context for the server.
      //
      IP_WEBSOCKET_InitServer(&WebSocketContext, &_WebSocketTransportAPI, (IP_WEBSOCKET_CONNECTION*)_pConnection);
      //
      // Process WebSocket messages.
      //
      while (1) {
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
          if (MessageType == IP_WEBSOCKET_FRAME_TYPE_TEXT) {
            //
            // Receive a text message and printf it.
            //
            NumBytesRead = 0;
            s            = &ac[0];
            for (;;) {
              r = IP_WEBSOCKET_Recv(&WebSocketContext, (void*)s, (sizeof(ac) - 1 - NumBytesRead));
              if        (r > 0) {
                s            += r;
                NumBytesRead += r;
              } else if (r == IP_WEBSOCKET_ERR_ALL_DATA_READ) {
                *s = '\0';
                IP_Logf_Application("Client: %s\n", ac);
                break;
              } else if (r == 0) {
                goto OnDisconnect;
              } else {  // r < 0 ?
                if (r != IP_WEBSOCKET_ERR_AGAIN) {
                  _Panic();  // Error, any other unhandled return value that should not occur.
                }
              }
            }
          } else {
            //
            // Discard other (binary) messages.
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
      }
OnDisconnect:
    IP_Logf_Application("WebSocket client disconnected.\n");
OnError:
    closesocket((int)_pConnection);
    _pConnection = NULL;
  };
  } while (1);
}

/*********************************************************************
*
*       _APP_AfterConfig()
*
*  Function description
*    This routine gets executed after the generic configuration of
*    the main sample has been applied. Additional configurations
*    can be applied from here.
*
*  Parameters
*    p: Opaque parameter that can be used to pass sample specific
*       information to this routine.
*/
static void _APP_AfterConfig(void* p) {
  IP_USE_PARA(p);

  //
  // Start the WebSocket handling task and hook the WebSocket URI
  // endpoint into the webserver.
  //
  OS_CREATETASK(&_WebSocketTCB, "WebSocket_Task", _WebSocketTask, APP_TASK_PRIO_WEBSERVER_CHILD, _WebSocketStack);
  IP_WEBS_WEBSOCKET_AddHook(&_WSHook, &_WebSocketAPI, _sURI, _sProto);
}

/*********************************************************************
*
*       Main sample configuration and include
*
**********************************************************************
*/

//
// Include and configure the webserver main sample.
//
#define APP_AFTER_CONFIG  _APP_AfterConfig
#include "IP_WebserverSample.c"
#undef  APP_AFTER_CONFIG

/*************************** End of file ****************************/
