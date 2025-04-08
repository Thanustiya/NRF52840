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

Purpose : Sample program for embOS & emNet demonstrating an MQTT subscriber.

          The client connects to the free broker test.mosquitto.org
          and subscribes to a specific topic.
          All QoS levels are supported.

Additional information:
  Preparations:
    Works out-of-the-box.
    Change the define USE_MQTT_5 to 0 if you want to test MQTT 3.1.1.
    Change the defines TOPIC_SUBSCRIBE_QOS to one of the following defines
    to set a specific "quality of service" level for subscribing:
      IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS0:  QOS = 0, Fire and forget    (At most once)
      IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS1:  QOS = 1, Simple acknowledge (At least once)
      IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS2:  QOS = 2, Full handshake     (Exactly once)

    If you want to connect to your own broker you can simply alter
    the defines MQTT_BROKER_ADDR and MQTT_BROKER_PORT
    with the address and port of your broker.
    Additionally you can change the name of the client and the topic
    to subscribe to via the defines MQTT_CLIENT_NAME
    and TOPIC_FILTER_TO_SUBSCRIBE respectively.

  Sample output:
    ...
    44:253 MainTask - APP: Connected to 5.196.95.208, port 1883.
    44:267 MainTask - _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM" received
    44:267 MainTask - _HandleProperties: IN: Maximum number of topic aliases: 0 (Server reports 10)
    44:268 MainTask - _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_RECEIVE_MAXIMUM" received
    44:268 MainTask - _HandleProperties: IN: Maximum number of concurrent QoS 1 and QoS 2: 10 (Server reports 10)
    44:268 MainTask - MQTT: Session established.
    44:268 MainTask - MQTT: SUBSCRIBE message (Id: 44268) sent.
    44:282 MainTask - _WaitForNextMessage: PUBLISH received.
    44:282 MainTask - APP: --------
    44:282 MainTask - APP: Message No. 1:
    44:282 MainTask - APP:   Topic  : eMQTT
    44:282 MainTask - APP:   Payload: Hello World
    77:253 MainTask - _WaitForNextMessage: PUBLISH received.
    77:253 MainTask - APP: --------
    77:253 MainTask - APP: Message No. 2:
    77:253 MainTask - APP:   Topic  : eMQTT
    77:254 MainTask - APP:   Payload: MQTT Example publish sent from a client.
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

#define NUM_PROPERTIES_CONNECT           2

#define USE_RX_TASK                      0                                  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define MQTT_CLIENT_BUFFER_SIZE          256
#define MQTT_CLIENT_MESSAGE_BUFFER_SIZE  512
#define RCV_TIMEOUT                      5000
#define MQTT_CLIENT_MEMORY_POOL_SIZE     1024                               // Pool size should be at least 512 bytes.

#define MQTT_BROKER_ADDR                 "test.mosquitto.org"               // Alternate test broker: broker.emqx.io
#define MQTT_BROKER_PORT                 1883

#define TOPIC_SUBSCRIBE_QOS              IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS1  // Quality of service level to use when subscribing

#define MQTT_CLIENT_NAME                 "eMQTT_Sub"
#define TOPIC_FILTER_TO_SUBSCRIBE        "eMQTT"

//
// Set keep-alive timeout to 60 seconds.
// For "test.mosquitto.org" this must not be 0, otherwise the server will refuse the connection.
//
#define PING_TIMEOUT                     60                                 // Configure MQTT ping to x seconds.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150, // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

//
// Functions in this sample which are not part of the MQTT add-on.
// If you are not using emNet you can change these macros.
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

static char            _acBuffer[MQTT_CLIENT_BUFFER_SIZE];                  // Memory block used by the MQTT client.
static char            _aPayload[200];                                      // Memory used to store the received payload for printf().

static IP_MQTT_CLIENT_CONTEXT _MQTTClient;
static U32             _aMQTTPool[MQTT_CLIENT_MEMORY_POOL_SIZE / sizeof(int)]; // Memory pool for the MQTT client. Required for the maintenance structures


//
// Packet types.
//
static const char* MQTT_PACKET_Types[] = {
  "Unknown",
  "CONNECT",
  "CONNACK",
  "PUBLISH",
  "PUBACK",
  "PUBREC",
  "PUBREL",
  "PUBCOMP",
  "SUBSCRIBE",
  "SUBACK",
  "UNSUBSCRIBE",
  "UNSUBACK",
  "PINGREQ",
  "PINGRESP",
  "DISCONNECT"
};

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
*
*  Return value
*    != NULL: O.K.
*    == NULL: Error.
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
  if(hSock == -1) {
    IP_MQTT_CLIENT_APP_WARN(("APP: Could not create socket!"));
    return NULL;
  }
  //
  // Set receive timeout.
  //
  Timeout = RCV_TIMEOUT;
  setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(Timeout));
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
    IP_MQTT_CLIENT_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(SoError)));
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
    IP_MQTT_CLIENT_APP_LOG(("_Disconnect: closing socket %d", (long)pSocket));
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
*
*  Return value
*    >   0: O.K., number of bytes sent.
*    ==  0: Connection has been gracefully closed by the broker.
*    == -1: Error.
*/
static int _Send(void* pSocket, const char* pBuffer, int NumBytes) {
  return send((long)pSocket, pBuffer, NumBytes, 0);
}

/*********************************************************************
*
*       IP_MQTT_CLIENT_TRANSPORT_API
*
*  Description
*    IP stack related function table
*/
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

/*********************************************************************
*
*       _Alloc()
*
*  Function description
*    Wrapper for Alloc(). (emNet: IP_MEM_Alloc())
*/
static void* _Alloc(U32 NumBytesReq) {
  return IP_AllocEx(_aMQTTPool, NumBytesReq);
}

/*********************************************************************
*
*       _Free()
*
*  Function description
*    Wrapper for Free(). (emNet: IP_Free())
*/
static void _Free(void *p) {
  IP_Free(p);
}

/*********************************************************************
*
*       _RecvMessageEx()
*
*  Function description
*    Process received PUBLISH messages.
*
*  Return value
*    >  0: O.K.
*    == 0: Connection has been gracefully closed by the broker.
*    <  0: Error
*/
static int _RecvMessageEx (void* pContext, void* pHandle, int NumBytesRem, U8 * pReasonCode) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;
  IP_MQTT_CLIENT_MESSAGE* pPublish;
  int                     NumBytes;
  int                     r;
  int                     NumBytesTopic;
  int                     NumBytesPayload;
  int                     NumBytesReceived;
  int                     NumBytesProperties;
  char*                   pTopic;
  char*                   pPayload;
  char*                   pProperties;
#if USE_MQTT_5
  U8                      PropType;
  U16                     Data16;
  U32                     Data32;
#endif

  pPublish            = (IP_MQTT_CLIENT_MESSAGE*)pHandle;
  pMQTTClient         = (IP_MQTT_CLIENT_CONTEXT*)pContext;
  pTopic              = NULL;
  pPayload            = NULL;
  pProperties         = NULL;
  NumBytesTopic       = 0;
  NumBytesReceived    = 0;
  NumBytesPayload     = 0;
  NumBytesProperties  = 0;
  //
  // Check if we can store the complete MQTT message in the application buffer.
  //
  if (NumBytesRem > MQTT_CLIENT_MESSAGE_BUFFER_SIZE) {
    NumBytes = MQTT_CLIENT_MESSAGE_BUFFER_SIZE;   // Get as much data as fits into the buffer.
  } else {
    NumBytes = NumBytesRem;                       // Get the complete message.
  }
  //
  // Read and process the MQTT message
  //
  r = IP_MQTT_CLIENT_Recv(pMQTTClient, _aPayload, NumBytes);
  if (r > 0) {
    NumBytesRem      -= r;
    NumBytesReceived += r;
    //
    // Get the information in MQTT message header.
    //
    IP_MQTT_CLIENT_ParsePublishEx(pMQTTClient, pPublish, _aPayload, NumBytes, &pTopic, &NumBytesTopic, &pPayload, &NumBytesPayload, &pProperties, &NumBytesProperties);
    //
    // Process the data
    // To visualize the data transmission, we output the message if it fits into the supplied buffer.
    //
    // Attention: The topic is not zero-terminated (strings in MQTT are generally not zero terminated because they always have a length header).
    //
    IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with (Id: %d | QoS: %d | Retain: %d | Duplicate: %d) received for topic \"%.*s\"", pPublish->PacketId, pPublish->QoS, pPublish->Retain, pPublish->Duplicate, NumBytesTopic, pTopic));
    if ((r < MQTT_CLIENT_MESSAGE_BUFFER_SIZE) && (NumBytesPayload < MQTT_CLIENT_MESSAGE_BUFFER_SIZE)) {  // If possible finalize the string to output it.
#if USE_MQTT_5
      if (pProperties != NULL) {
        do {
          PropType = *pProperties;
          pProperties++; // First data byte.
          NumBytesProperties--;
          IP_MQTT_CLIENT_APP_LOG(("APP: IN:   Property \"%s\" received", IP_MQTT_Property2String(PropType)));
          //
          // PUBLISH properties only.
          //
          switch (PropType) {
            case IP_MQTT_PROP_TYPE_USER_PROPERTY:
              //
              // A User property is a key-value UTF-8 string pair with the format
              // [U16 Len1][String1][U16 Len2][String2]
              // The strings are normally _not_ zero-terminated.
              //
              Data16 = SEGGER_RdU16BE((const U8*)pProperties); // Len1
              pProperties += 2;
              if (Data16 > 0) {
                IP_MQTT_CLIENT_APP_LOG(("APP: IN:     Key:   \"%.*s\" ", Data16, pProperties));
              }
              pProperties += Data16;
              NumBytesProperties -= 2 + Data16;
              Data16 = SEGGER_RdU16BE((const U8*)pProperties); // Len2
              pProperties += 2;
              if (Data16 > 0) {
                IP_MQTT_CLIENT_APP_LOG(("APP: IN:     Value: \"%.*s\" ", Data16, pProperties));
              }
              pProperties += Data16;
              NumBytesProperties -= 2 + Data16;
              break;
            case IP_MQTT_PROP_TYPE_MESSAGE_EXPIRY_INTERVAL:
              //
              // The Expiry Interval property is a 4 byte integer containing the expiry time in seconds.
              //
              Data32 = SEGGER_RdU32BE((const U8*)pProperties);
              NumBytesProperties -= 4;
              IP_MQTT_CLIENT_APP_LOG(("APP: IN:   Message expire interval set to: \"%d\" seconds ", Data32));
              break;
            case IP_MQTT_PROP_TYPE_PAYLOAD_FORMAT_INDICATOR:
            case IP_MQTT_PROP_TYPE_CONTENT_TYPE:
            case IP_MQTT_PROP_TYPE_RESPONSE_TOPIC:
            case IP_MQTT_PROP_TYPE_CORRELATION_DATA:
            case IP_MQTT_PROP_TYPE_SUBSCRIPTION_ID:
            case IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM:
            case IP_MQTT_PROP_TYPE_RETAIN_AVAILABLE:
            default:
              //
              // Property not decoded by this sample application.
              // NumBytesProperties is set to zero to exit the loop.
              //
              NumBytesProperties = 0;
              break;
          }
        } while (NumBytesProperties != 0);
      }
#endif
      if (pPayload != NULL) {
        *(pPayload + NumBytesPayload) = '\0';
        IP_MQTT_CLIENT_APP_LOG(("APP: IN:   Payload: \"%s\"", pPayload));
      } else {
        IP_MQTT_CLIENT_APP_LOG(("APP: IN:   No payload."));
      }
    } else {
      IP_MQTT_CLIENT_APP_LOG(("APP: IN:   Payload: Too large to output."));
    }
    //
    // If message is larger then the available buffer, read and discard it to clean the buffer.
    //
    while (NumBytesRem > 0) {
      if (NumBytesRem < NumBytes) {
        NumBytes = NumBytesRem;
      }
      r = IP_MQTT_CLIENT_Recv(pMQTTClient, _aPayload, NumBytes);
      if (r < 0) {
        NumBytesReceived = r; // Receive failed. Connection closed ?
        break;
      }
      NumBytesRem      -= r;
      NumBytesReceived += r;
    };
  }
  //
  // Set a Reason Code to send back to the server.
  // This is only relevant if QoS of the received message is 1 or 2.
  //
  if (pPublish->QoS > 0) {
    *pReasonCode = IP_MQTT_REASON_SUCCESS;
    //*pReasonCode = IP_MQTT_REASON_IMPL_SPECIFIC_ERR; // Error code for testing.
  }
  IP_MQTT_CLIENT_APP_LOG(("APP: ----"));
  OS_Delay(10);
  return NumBytesReceived;
}

/*********************************************************************
*
*       _MessageHandledEx()
*
*  Function description
*    Called if all QoS related messages are processed and
*      - Sent messages were freed or can be freed.
*      - Received messages can be processed by the application.
*
*  Note
*   The application has to store the message with QoS > 0 until it gets/has sent an acknowledgement.
*   Target sends PUBLISH with QoS 0 to broker -> Message can be directly discarded after message has been sent.
*   Target sends PUBLISH with QoS 1 to broker -> Message can be discarded after PUBACK has been received from the broker.
*   Target sends PUBLISH with QoS 2 to broker -> Message can be discarded after PUBCOMP has been received.
*   Broker sends PUBLISH with QoS 0 to target -> Message can be directly processed.
*   Broker sends PUBLISH with QoS 1 to target -> Message can be processed after PUBACK has been sent to the broker.
*   Broker sends PUBLISH with QoS 2 to target -> Message can be processed after PUBCOMP has been received from the broker.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
static int _MessageHandledEx(void* pContext, U8 Type, U16 PacketId, U8 ReasonCode) {
  (void)pContext;
#if USE_MQTT_5 == 0
  (void)ReasonCode;
#endif
  //
  switch (Type) {
    //
    // Tx related packet types
    //
    case IP_MQTT_CLIENT_TYPES_PUBACK:  // PUBACK received. QoS level 1 successfully done. -> Message was freed.
    case IP_MQTT_CLIENT_TYPES_PUBCOMP: // PUCOMP received. QoS level 2 successfully done. -> Message was freed.
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message with Id: %d has been freed. Reason Code received from server: %s.", PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message with Id: %d has been freed.", PacketId));
#endif
      break;
    //
    // Rx related packet types
    //
    case IP_MQTT_CLIENT_TYPES_PUBLISH: // PUBACK sent. -> Received packet can be processed.
    case IP_MQTT_CLIENT_TYPES_PUBREL:  // PUBREC sent. PUBREL received. PUBCOMP sent. -> Received packet can be processed.
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with Id: %d can be processed. Reason Code sent to server: %s.", PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with Id: %d can be processed. ", PacketId));
#endif
      break;
    case IP_MQTT_CLIENT_TYPES_PINGRESP:
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Received PING Response from server."));
      break;
    //
    // Subscription packet types
    //
    case IP_MQTT_CLIENT_TYPES_SUBACK:
    case IP_MQTT_CLIENT_TYPES_UNSUBACK: // UNSUBACK received. The subscribe structure can be freed.
    default:
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: Message (Type: %s, Id: %d) received. Reason Code: %s.", MQTT_PACKET_Types[Type], PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: Message (Type: %s, Id: %d) received.", MQTT_PACKET_Types[Type], PacketId));
#endif
      break;
  }
  IP_MQTT_CLIENT_APP_LOG(("APP: ----"));
  return 1;
}

/*********************************************************************
*
*       _OnProperty()
*
*  Function description
*    Called when a non-PUBLISH (CONNACK, PUBACK, PUBREC, PUBCOMP, SUBACK, UNSUBACK, DISCONNECT, AUTH) message is received with a Property.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
#if USE_MQTT_5
static void _OnProperty (void* pMQTTClient, U16 PacketId, U8 PacketType, IP_MQTT_PROPERTY * pProp) {
  (void)pMQTTClient;
  IP_MQTT_CLIENT_APP_LOG(("APP: Received Property %s for %s with PacketID %d.", IP_MQTT_Property2String(pProp->PropType), MQTT_PACKET_Types[PacketType], PacketId));
  return;
}
#endif

/*********************************************************************
*
*       _HandleError()
*
*  Function description
*    Called in case of an error.
*    Can be used to analyze/handle a connection problem.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
static int _HandleError (void* pContext) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;
  int Error;

  Error       = 1;
  pMQTTClient = (IP_MQTT_CLIENT_CONTEXT*)pContext;
  getsockopt((long)pMQTTClient->Connection.Socket, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
  if (Error < 0) {
    IP_MQTT_CLIENT_APP_LOG(("APP: Socket error %d. Disconnect MQTT client.", Error));
    IP_MQTT_CLIENT_Disconnect(pMQTTClient);
  }
  return Error;
}

/*********************************************************************
*
*       _HandleDisconnect()
*
*  Function description
*    Called in case of a disconnect from the server.
*    Only available if MQTT 5 is used.
*    With MQTT 3.1.1 there is no option for the server to send a disconnect.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
#if USE_MQTT_5
static void _HandleDisconnect (void* pContext, U8 ReasonCode) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;

  pMQTTClient = (IP_MQTT_CLIENT_CONTEXT*)pContext;

  IP_MQTT_CLIENT_APP_LOG(("APP: _HandleDisconnect: %d %s", ReasonCode, IP_MQTT_ReasonCode2String(ReasonCode)));
  IP_MQTT_CLIENT_Disconnect(pMQTTClient);
  return;
}
#endif

/*********************************************************************
*
*       IP_MQTT_CLIENT_APP_API
*
*  Description
*    Application related function table
*/
static const IP_MQTT_CLIENT_APP_API _APP_Api = {
  _GenRandom,
  _Alloc,
  _Free,
  NULL,
  NULL,
  NULL,
  NULL,
  _HandleError,     // (*pfHandleError)
#if USE_MQTT_5
  _HandleDisconnect,// (*pfHandleDisconnect)    // Used with MQTT v5 only
#else
  NULL,
#endif
  _MessageHandledEx,// (*pfOnMessageConfirmEx)  // Used with MQTT v5 or 3.1.1 if "pfOnMessageConfirm" is not set
  _RecvMessageEx,   // (*pfRecvMessageEx)
#if USE_MQTT_5
  _OnProperty       // (*pfOnProperty)          // Used with MQTT v5 only
#else
  NULL
#endif
};

/*********************************************************************
*
*       _MQTT_Client()
*
*  Function description
*    MQTT client application.
*/
static void _MQTT_Client(void) {
  IP_MQTT_CLIENT_TOPIC_FILTER TopicFilter;
  IP_MQTT_CLIENT_SUBSCRIBE    Subscribe;
  IP_MQTT_CONNECT_PARAM       ConnectPara;
  U8                          ReasonCode;
  int                         r;
  IP_fd_set                   ReadFds;

  IP_AddMemory(_aMQTTPool, sizeof(_aMQTTPool));    // Memory pool used for allocation of maintenance structures.
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
    //
    // Connect to the MQTT broker.
    //
    MQTT_MEMSET(&ConnectPara, 0, sizeof(IP_MQTT_CONNECT_PARAM));
    ConnectPara.CleanSession  = 1;
    ConnectPara.Port          = MQTT_BROKER_PORT;
    ConnectPara.sAddr         = MQTT_BROKER_ADDR;
#if USE_MQTT_5
    ConnectPara.Version       = 5;
#else
    ConnectPara.Version       = 4; // MQTT 3.1.1
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
    // Initialize the topic filter and subscribe structures.
    //
    MQTT_MEMSET(&TopicFilter, 0, sizeof(TopicFilter));
    TopicFilter.sTopicFilter = TOPIC_FILTER_TO_SUBSCRIBE;
    TopicFilter.QoS          = TOPIC_SUBSCRIBE_QOS;
    MQTT_MEMSET(&Subscribe, 0, sizeof(Subscribe));
    Subscribe.pTopicFilter   = &TopicFilter;
    Subscribe.TopicCnt       = 1;
    //
    // Subscribe to topic.
    //
    r = IP_MQTT_CLIENT_Subscribe(&_MQTTClient, &Subscribe);
    if (r > 0) {
      //
      // Receive a message.
      //
      do {
        //
        // Process incoming messages and send messages
        //
        r = IP_MQTT_CLIENT_IsClientConnected(&_MQTTClient);
        if (r > 0) {
          IP_FD_ZERO(&ReadFds);                                       // Clear the set
          IP_FD_SET((long)_MQTTClient.Connection.Socket, &ReadFds);   // Add socket to the set
          r = select(&ReadFds, NULL, NULL, 500);                      // Check for activity. Wait 500ms.
          if (r > 0) {
            r = IP_MQTT_CLIENT_Exec(&_MQTTClient);                    // Get messages for subscribed topics, process QoS messages, ...
            if (r == 0) {
              IP_MQTT_CLIENT_APP_LOG(("IP_MQTT_CLIENT_Exec: Connection gracefully closed by peer."));
              break; // Exit loop and try to re-connect.
            } else if (r < 0) {
              IP_MQTT_CLIENT_APP_LOG(("IP_MQTT_CLIENT_Exec: Error: %d.", r));
              break; // Exit loop and try to re-connect.
            }
          }
        } else {
          //
          // If we lost our connection - exit the message processing loop.
          //
          break;
        }
      } while (r != -1);
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
