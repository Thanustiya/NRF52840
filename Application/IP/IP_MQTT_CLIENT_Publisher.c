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

Purpose : Sample program for embOS & emNet demonstrating an MQTT publisher.

          The client connects to the free broker test.mosquitto.org
          and periodically "publishes" a Message to a specific topic.
          All QoS levels are supported.

Preparations:
  Works out-of-the-box.
  Change the define USE_MQTT_5 to 0 if you want to test MQTT 3.1.1.
  Change the define TOPIC_PUBLISH_QOS to one of the following defines
  to set a specific "quality of service" level for publishing:
    IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS0:  QOS = 0, Fire and forget    (At most once)
    IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS1:  QOS = 1, Simple acknowledge (At least once)
    IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS2:  QOS = 2, Full handshake     (Exactly once)

  If you want to connect to your own broker you can simply alter
  the defines MQTT_BROKER_ADDR and MQTT_BROKER_PORT
  with the address and port of your broker.
  Additionally you can change the name of the client and the topic
  to publish to via the defines MQTT_CLIENT_NAME
  and TOPIC_FILTER_TO_PUBLISH respectively.

Sample output:
  ...
  4:303 MainTask - APP: Connected to 5.196.95.208, port 1883.
  4:317 MainTask - _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM" received
  4:317 MainTask - _HandleProperties: IN: Maximum number of topic aliases: 0 (Server reports 10)
  4:317 MainTask - _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_RECEIVE_MAXIMUM" received
  4:317 MainTask - _HandleProperties: IN: Maximum number of concurrent QoS 1 and QoS 2: 10 (Server reports 10)
  4:317 MainTask - MQTT: Session established.
  4:345 MainTask - APP: --------
  4:345 MainTask - APP: Message No. 1:
  4:345 MainTask - APP:   Topic  : "eMQTT"
  4:345 MainTask - APP:   Payload: "www.SEGGER.com MQTT_HelloWorld_Sample"
  5:372 MainTask - APP: --------
  5:372 MainTask - APP: Message No. 2:
  5:372 MainTask - APP:   Topic  : "eMQTT"
  5:372 MainTask - APP:   Payload: "www.SEGGER.com MQTT_HelloWorld_Sample"
  6:399 MainTask - APP: --------
  6:399 MainTask - APP: Message No. 3:
  6:399 MainTask - APP:   Topic  : "eMQTT"
  6:399 MainTask - APP:   Payload: "www.SEGGER.com MQTT_HelloWorld_Sample"
  7:426 MainTask - APP: --------
  7:426 MainTask - APP: Message No. 4:
  7:426 MainTask - APP:   Topic  : "eMQTT"
  7:426 MainTask - APP:   Payload: "www.SEGGER.com MQTT_HelloWorld_Sample"
  ...
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_MQTT_CLIENT.h"
#include "SEGGER.h"
#include "SEGGER_UTIL.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#ifndef   MQTT_STRLEN
  #define MQTT_STRLEN          strlen
#endif

#ifndef   MQTT_MEMSET
  #define MQTT_MEMSET          memset
#endif

#ifndef   MQTT_MEMCPY
  #define MQTT_MEMCPY          memcpy
#endif

#ifndef USE_MQTT_5
  //
  // This sample is able to communicate using MQTT 5 and MQTT 3.1.1.
  // The default is to use MQTT 5, if you want to use MQTT 3.1.1 set the following define to 0.
  // IP_MQTT_CLIENT_SUPPORT_V5 must be enabled to use MQTT 5.
  //
  #if IP_MQTT_CLIENT_SUPPORT_V5
    #define USE_MQTT_5 1
  #else
    #define USE_MQTT_5 0
  #endif
#endif

#define USE_RX_TASK              0                                  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define MQTT_CLIENT_BUFFER_SIZE  256
#define RCV_TIMEOUT              5000

#define MQTT_BROKER_ADDR         "test.mosquitto.org"               // Alternate test broker: broker.emqx.io
#define MQTT_BROKER_PORT         1883

#define TOPIC_PUBLISH_QOS        IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS2  // Quality of service level to use when publishing

#define MQTT_CLIENT_NAME         "eMQTT_Pub"
#define TOPIC_FILTER_TO_PUBLISH  "eMQTT"
#define PAYLOAD                  "www.SEGGER.com MQTT_HelloWorld_Sample"
//
// Set keep-alive timeout to 60 seconds.
// For "test.mosquitto.org" this must not be 0, otherwise the server will refuse the connection.
//
#define PING_TIMEOUT             60                                 // Configure MQTT ping to x seconds.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

//
// Functions in this sample which are not part of the MQTT add-on.
// If you are not using emNet you should change these macros.
//
#ifndef GET_TIME_32
  #define GET_TIME_32 IP_OS_GetTime32()
#endif

#ifndef CHECK_TIME_EXPIRED
  #define CHECK_TIME_EXPIRED(x) IP_IsExpired(x)
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
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

static IP_MQTT_CLIENT_CONTEXT _MQTTClient;

static char            _acBuffer[MQTT_CLIENT_BUFFER_SIZE];                  // Memory block used by the MQTT client.

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
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnStateChange()
*
*  Function description
*    Callback that will be notified once the state of an interface
*    changes.
*
*  Parameters
*    IFaceId   : Zero-based interface index.
*    AdminState: Is this interface enabled ?
*    HWState   : Is this interface physically ready ?
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
*       _Connect()
*
*  Function description
*    Creates a socket and opens a TCP connection to the MQTT broker.
*/
static IP_MQTT_CLIENT_SOCKET _Connect(const char* sBrokerAddr, unsigned BrokerPort) {
  struct hostent*         pHostEntry;
  struct sockaddr_in      sin;
  SEGGER_PARSE_IP_STATUS  Status;
  SEGGER_PARSE_IP_TYPE    Type;
  long                    Ip;
  long                    hSock;
  int                     SoError;
  int                     r;
  U32                     Timeout;
  U8                      acBuff[16];

  Status = SEGGER_ParseIP(sBrokerAddr, (unsigned char*)&acBuff, sizeof(acBuff), &Type);
  if (Status == SEGGER_PARSE_IP_STATUS_OK) {
    MQTT_MEMCPY(&Ip, &acBuff[0], sizeof(Ip));
    Ip = htonl(Ip);
  } else {
    //
    // Convert host into IP address.
    //
    pHostEntry = gethostbyname((char*)sBrokerAddr);
    if (pHostEntry == NULL) {
      IP_MQTT_CLIENT_APP_LOG(("APP: gethostbyname failed: %s", sBrokerAddr));
      return NULL;
    }
    Ip = *(unsigned*)(*pHostEntry->h_addr_list);
  }
  //
  // Create socket and connect to the MQTT broker.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if(hSock  == -1) {
    IP_MQTT_CLIENT_APP_WARN(("APP: Could not create socket!"));
    return NULL;
  }
  //
  // Set receive timeout.
  //
  Timeout = RCV_TIMEOUT;
  setsockopt(hSock , SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(Timeout));
  //
  // Connect.
  //
  MQTT_MEMSET(&sin, 0, sizeof(sin));
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons((U16)BrokerPort);
  sin.sin_addr.s_addr = Ip;
  r = connect(hSock, (struct sockaddr*)&sin, sizeof(sin));
  if(r == SOCKET_ERROR) {
    getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    closesocket(hSock);
    IP_MQTT_CLIENT_APP_LOG(("APP: Connect error: %s", IP_Err2Str(SoError)));
    return NULL;
  }
  IP_MQTT_CLIENT_APP_LOG(("APP: Connected to %i, port %d.", Ip, BrokerPort));
  return (IP_MQTT_CLIENT_SOCKET)hSock;
}

/*********************************************************************
*
*       _Disconnect()
*
*  Function description
*    Closes a socket.
*/
static void _Disconnect(void* pSocket) {
  if (pSocket) {
    closesocket((long)pSocket);
  }
}

/*********************************************************************
*
*       _Recv()
*
*  Function description
*    Receives data via socket interface.
*
*  Return value
*    >   0: O.K., number of bytes received.
*    ==  0: Connection has been gracefully closed by the broker.
*    == -1: Error.
*/
static int _Recv(void* pSocket, char* pBuffer, int NumBytes) {
  return recv((long)pSocket, pBuffer, NumBytes, 0);
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Sends data via socket interface.
*/
static int _Send(void* pSocket, const char* pBuffer, int NumBytes) {
  return send((long)pSocket, pBuffer, NumBytes, 0);
}

static const IP_MQTT_CLIENT_TRANSPORT_API _IP_Api = {
  _Connect,
  _Disconnect,
  _Recv,
  _Send
};

/*********************************************************************
*
*       _GenRandom()
*
*  Function description
*    Generates a random number.
*/
static U16 _GenRandom(void) {
  U32 TimeStamp;

  TimeStamp = OS_GetTime32();
  return (U16)TimeStamp;  // Return a random value, which can be used for packet Id, ...
}

static const IP_MQTT_CLIENT_APP_API _APP_Api = {
  _GenRandom,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

/*********************************************************************
*
*       _MQTT_Client()
*
*  Function description
*    MQTT client application.
*/
static void _MQTT_Client(void) {
  IP_MQTT_CLIENT_MESSAGE Publish;
  IP_MQTT_CONNECT_PARAM  ConnectPara;
  U32                    TimeExpirePing;
  U8                     ReasonCode;
  int                    Cnt;
  int                    r;
  int                    Error;

  //
  // Initialize MQTT client context.
  //
  IP_MQTT_CLIENT_Init(&_MQTTClient, _acBuffer, MQTT_CLIENT_BUFFER_SIZE, &_IP_Api, &_APP_Api, MQTT_CLIENT_NAME);
  //
  // Main application loop.
  //
  do {
    //
    // Configure MQTT keep alive time (for sending PINGREQ messages)
    //
    if (PING_TIMEOUT != 0) {
      r = IP_MQTT_CLIENT_SetKeepAlive(&_MQTTClient, PING_TIMEOUT);
      if (r != 0) {
        IP_MQTT_CLIENT_APP_LOG(("APP: Could not set timeout (currently connected ?)"));
      }
    }
    TimeExpirePing = 0;
    //
    // Connect to the MQTT broker.
    //
    MQTT_MEMSET(&ConnectPara, 0, sizeof(IP_MQTT_CONNECT_PARAM));
    ConnectPara.CleanSession = 1;
    ConnectPara.Port  = MQTT_BROKER_PORT;
    ConnectPara.sAddr = MQTT_BROKER_ADDR;
#if USE_MQTT_5
    ConnectPara.Version = 5;
#else
    ConnectPara.Version = 4; // MQTT 3.1.1
#endif
    r = IP_MQTT_CLIENT_ConnectEx(&_MQTTClient, &ConnectPara, &ReasonCode);
    if (r != 0) {
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: MQTT connect error: %d, ReasonCode %s.", r, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: MQTT connect error: %d", r));
#endif
      goto Disconnect;
    }
    BSP_SetLED(0);
    //
    // Publish message loop.
    //
    Cnt = 0;
    while (1) {
      //
      // Initially set timeouts. This is done after the connect check because
      // the KeepAlive timeout value can change depending on the CONNACK from the server.
      // The server's KeepAlive timeout takes precedence.
      // KeepAlive time is in seconds.
      // As per MQTT 3.1.1 or 5 spec the server allows for one and a half times the
      // KeepAlive time period before disconnecting the client.
      //
      if (TimeExpirePing == 0) {
        TimeExpirePing    = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000);
      }
      //
      // Send KeepAlive (PINGREQ) if configured.
      //
      if (_MQTTClient.KeepAlive != 0 && CHECK_TIME_EXPIRED(TimeExpirePing)) {
        TimeExpirePing = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000); // Set new expire time.
        IP_MQTT_CLIENT_Timer(); // Send PINGREQ.
        IP_MQTT_CLIENT_APP_LOG(("APP: PINGREQ sent."));
      }
      //
      // Initialize the MQTT message.
      //
      MQTT_MEMSET(&Publish, 0, sizeof(Publish));
      Publish.sTopic  = TOPIC_FILTER_TO_PUBLISH;
      Publish.pData   = PAYLOAD;
      Publish.DataLen = MQTT_STRLEN(PAYLOAD);
      Publish.QoS     = TOPIC_PUBLISH_QOS;
      //
      // Publish a message.
      //
      r = IP_MQTT_CLIENT_Publish(&_MQTTClient, &Publish);
      if (r < 0) {
        Error = 0;
        getsockopt((long)_MQTTClient.Connection.Socket, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
        if (Error == IP_ERR_TIMEDOUT) {
          //
          // Retry.
          //
          OS_Delay(100);
          continue;
        }
        break;
      }
      Cnt++;
      IP_MQTT_CLIENT_APP_LOG(("APP: --------"));
      IP_MQTT_CLIENT_APP_LOG(("APP: Message No. %d:", Cnt));
      IP_MQTT_CLIENT_APP_LOG(("APP:   Topic  : \"%s\"", Publish.sTopic));
      IP_MQTT_CLIENT_APP_LOG(("APP:   Payload: \"%s\"", Publish.pData));
      OS_Delay(1000);
    }
    //
    // Disconnect.
    //
Disconnect:
    IP_MQTT_CLIENT_Disconnect(&_MQTTClient);
    IP_MQTT_CLIENT_APP_LOG(("APP: Done."));
    BSP_ClrLED(0);
    OS_Delay(10000);
  } while (1);
}

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Main task executed by the RTOS to create further resources and
*    running the main application.
*/
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(1);
    OS_Delay(50);
  }
  BSP_ClrLED(0);
  _MQTT_Client();
}

/*************************** End of file ****************************/
