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

Purpose : Sample program for embOS & emNet.
          Demonstrates sending a mail via a SMTP server
          and resolving the servers host name via DNS.
          For full functionality the sample needs to be configured to
          the configuration of your mail server and your sender and
          recipient addresses.

Notes:
  (1) The following requirements need to be met when including this
      sample into another C-module:

        - BSP_Init() for LED routines and IP_Init() for emNet are
          called before starting APP_MainTask() .

        - The optional IP_Connect() is called and a link state hook
          is installed if necessary for the emNet interface used
          before starting APP_MainTask() .

  (2) The following application defines can be overwritten when
      including this sample into another C-module:

        - APP_MAIN_STACK_SIZE
          Stack size in bytes to use for APP_MainTask() .

        - APP_TASK_STACK_OVERHEAD
          Additional task stack size [multiples of sizeof(int)] if some
          underlying processes require more task stack than default.

        - APP_USE_SSL
          Enables/disables secure connection support.

  (3) The following symbols can be used and renamed via preprocessor
      if necessary when including this sample into another C-module:

        - MainTask
          Main starting point of the sample when used as a
          standalone sample. Can be renamed to anything else to
          avoid multiple MainTask symbols and to skip common
          initializing steps done in every sample.

        - APP_MainTask
          Functional starting point of the sample itself. Typically
          called by the MainTask in this sample. Should be renamed
          via preprocessor to a more application specific name to
          avoid having multiple APP_MainTask symbols in linker map
          files for a better overview when including multiple samples
          this way.

        - APP_MainTCB
          Task-Control-Block used for the APP_MainTask when started
          as a task from the MainTask in this sample. Can/should be
          renamed via preprocessor to a more application specific name.

        - APP_MainStack
          Task stack used for the APP_MainTask when started as a task
          from the MainTask in this sample. Can/should be renamed via
          preprocessor to a more application specific name.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "TaskPrio.h"
#include "IP_SMTPC.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   SMTP_MEMSET
  #define SMTP_MEMSET          memset
#endif

#ifndef   SMTP_STRLEN
  #define SMTP_STRLEN          strlen
#endif

#ifndef   USE_RX_TASK
  #define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.
#endif

//
// Sample features enable/disable
//
#ifndef   APP_USE_SSL
  #define APP_USE_SSL      0                                // Use SSL to secure the connection if supported by the server.
#endif
#define   SECURITY_POLICY  SMTPC_SEC_POLICY_ALLOW_INSECURE  // Default: Try to secure the connection but allow fallback to insecure.

#if APP_USE_SSL
#include "SSL.h"
#endif

//
// SMTP client sample: Sender configuration.
//
#define SENDER_NAME  ""  // Your name that will be displayed as senders name in a mail client. Example "John Doe".
#define SENDER_ADDR  ""  // Your mail address. Example "john.doe@foobar.com".
#define SERVER_ADDR  ""  // Host name or IP addr. of your mail server. Example "mail.foobar.com".
#define SERVER_USER  ""  // Your user name for your mail account. Example "john.doe@foobar.com".
#define SERVER_PASS  ""  // Your password for your mail account. Example "password".

//
// SMTP client sample: Recipient configuration.
//
#define RECIPIENT_0_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_0_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_0_TYPE  SMTPC_REC_TYPE_TO  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC .

#define RECIPIENT_1_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_1_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_1_TYPE  0                  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC . 0 disables this recipient for the sample.

#define RECIPIENT_2_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_2_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_2_TYPE  0                  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC . 0 disables this recipient for the sample.

//
// SMTP client sample: Message configuration.
//
#define MESSAGE_SUBJECT  "SMTP message sent via emNet SMTP client"
#define MESSAGE_BODY     "emNet SMTP client - www.segger.com"

//
// SMTP client sample: Generic configuration.
//
#define OWN_DOMAIN  "sample.com"  // Your domain. According to RFC 821 the maximum total length of a domain name or number is 64 characters. Can be NULL.
#define UTC_OFFSET  "+0100"       // The offset from Coordinated Universal Time (UTC). Can be NULL.
#define TIMEOUT     10000         // Timeout for DNS request [ms].

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef       APP_MAIN_STACK_SIZE
  #if (APP_USE_SSL != 0)
    #define   APP_MAIN_STACK_SIZE      (3584)
  #else
    #define   APP_MAIN_STACK_SIZE      (2048)
  #endif
#endif
#ifndef       APP_TASK_STACK_OVERHEAD
  #define     APP_TASK_STACK_OVERHEAD  (0)
#endif
#ifndef       SMTPC_STACK_SIZE
  #define     SMTPC_STACK_SIZE         (APP_MAIN_STACK_SIZE + APP_TASK_STACK_OVERHEAD)
#endif

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef struct {
  int hSock;            // Server socket.
#if APP_USE_SSL
  int IsSecure;         // Nonzero indicates secure state.
  SSL_SESSION Session;  // emSSL TLS session.
#endif
} SMTPC_APP_CONTEXT;

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

static int _cbSendAttachment(IP_SMTPC_CONTEXT* pContext, const IP_SMTPC_MULTIPART_ITEM* pItem, const IP_SMTPC_MULTIPART_API* pAPI);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SMTPC_APP_CONTEXT _Context;

#if APP_USE_SSL
//
// SSL transport API.
//
static const SSL_TRANSPORT_API _IP_Transport = {
  send,
  recv,
  NULL, // Don't verify the time. Otherwise a function that returns the unix time is needed.
  NULL
};
#endif

//
// SMTP client configuration data.
//
static const IP_SMTPC_MAIL_ADDR _aMailAddr[] = {
   { SENDER_NAME     , SENDER_ADDR     , SMTPC_REC_TYPE_FROM }
  ,{ RECIPIENT_0_NAME, RECIPIENT_0_ADDR, RECIPIENT_0_TYPE }
#if (RECIPIENT_1_TYPE != 0)
  ,{ RECIPIENT_1_NAME, RECIPIENT_1_ADDR, RECIPIENT_1_TYPE }
#endif
#if (RECIPIENT_2_TYPE != 0)
  ,{ RECIPIENT_2_NAME, RECIPIENT_2_ADDR, RECIPIENT_2_TYPE }
#endif
};

static const IP_SMTPC_MTA _MTAConfig = {
  SERVER_ADDR,
  SERVER_USER,
  SERVER_PASS,
  SECURITY_POLICY
};

//
// Table of attachments (multipart items) to add
// to the message. A custom context (pContext) can
// be passed to the callback sending the content of
// the item. For this sample it is the index to an
// array of strings. For a real application pContext
// could be used to pass the local file system path
// of a file to attach to the callback.
//

static const char* _aAttachmentContent[] = {
  "Hello world!",
  "Another text file attachment."
};

static const IP_SMTPC_MULTIPART_ITEM _aAttachment[] = {
  // sFilename  , pUserContext  , sContentType, pfSend
  // ---------------------------------------------------------------
  {  "Test0.txt", (const void*)0, "text/plain", _cbSendAttachment },
  {  "Test1.txt", (const void*)1, "text/plain", _cbSendAttachment }
};

static const IP_SMTPC_MESSAGE _Message = {
  MESSAGE_SUBJECT,
  MESSAGE_BODY,
  "------------030007030004040809060800",
  &_aAttachment[0],
  SEGGER_COUNTOF(_aAttachment)
};

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int APP_MainStack[SMTPC_STACK_SIZE / sizeof(int)];       // Stack of the starting point of this sample.
static OS_TASK         APP_MainTCB;                                         // Task-Control-Block of the IP_Task.

static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
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

#if APP_USE_SSL
/*********************************************************************
*
*       _cbUpgrade()
*
*  Function description
*    Upgrades a connection from insecure to secure.
*
*  Parameters
*    hSock  : SMTPC insecure socket (or context).
*    sServer: Server we are connecting to for authentication.
*
*  Return value
*    O.K. : >   0
*    Error: <=  0
*/
static int _cbUpgrade(SMTPC_SOCKET hSock, const char* sServer) {
  SMTPC_APP_CONTEXT* pContext;
  int                Status;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  SSL_SESSION_Prepare(&pContext->Session, pContext->hSock, &_IP_Transport);
  Status = SSL_SESSION_Connect(&pContext->Session, sServer);
  if (Status < 0) {
    return Status;
  } else {
    pContext->IsSecure = 1;
    return 0;
  }
}

/*********************************************************************
*
*       _cbDowngrade()
*
*  Function description
*    Downgrades a connection from insecure to secure.
*
*  Parameters
*    hSock: SMTPC secure socket (or context).
*/
static void _cbDowngrade(SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  SSL_SESSION_Disconnect(&pContext->Session);
  pContext->IsSecure = 0;
}
#endif

/*********************************************************************
*
*       _cbConnect()
*
*  Function description
*    Creates a socket and opens a TCP connection to the mail server.
*
*  Parameters
*    sServer: Host name of the mail server.
*
*  Return value
*    O.K. : Socket handle (or context).
*    Error: NULL.
*/
static SMTPC_SOCKET _cbConnect(const char* sServer) {
  SMTPC_APP_CONTEXT* pContext;
  struct sockaddr_in Addr;
  U32 IPAddr;
  int ErrCode;
  int hSock;
  int r;

  //
  // Resolve server host name to IP addr.
  //
  r = IP_ResolveHost(sServer, &IPAddr, TIMEOUT);  // Resolve host name.
  if (r) {
    SMTPC_APP_LOG(("DNS resolve for \"%s\" failed.\n", sServer));
    return NULL;
  }
  //
  // Create socket and connect to mail server.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if (hSock == SOCKET_ERROR) {
    SMTPC_APP_LOG(("Could not create socket!\n"));
    getsockopt((long)hSock, AF_INET, SO_ERROR, &ErrCode, sizeof(ErrCode));
    SMTPC_APP_LOG(("Socket error: %s", IP_Err2Str(ErrCode)));
    return NULL;
  }
  SMTP_MEMSET(&Addr, 0, sizeof(Addr));
  Addr.sin_family = AF_INET;
  Addr.sin_port = htons(SMTPC_SERVER_PORT);
  Addr.sin_addr.s_addr = IPAddr;
  r = connect(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  if (r == SOCKET_ERROR) {
    getsockopt((long)hSock, AF_INET, SO_ERROR, &ErrCode, sizeof(ErrCode));
    SMTPC_APP_LOG(("Socket error: %s", IP_Err2Str(ErrCode)));
    closesocket(hSock);
    return NULL;
  }
  //
  // Alloc memory for context. In this sample the context
  // is used as static data which is fine if only a single
  // task is sending mails. For multiple tasks being able
  // to send mails independent the memory for the context
  // should be allocated.
  // If memory gets allocated this might fail and in this
  // case should be taken into account for the return value.
  //
  pContext = &_Context;
  SMTP_MEMSET(pContext, 0, sizeof(*pContext));
  pContext->hSock = hSock;
  SMTPC_APP_LOG(("APP: Connected.\n"));
  return (SMTPC_SOCKET)pContext;
}

/*********************************************************************
*
*       _cbDisconnect()
*
*  Function description
*    Closes a socket (or context).
*
*  Parameters
*    hSock: Socket handle (or context) to close.
*/
static void _cbDisconnect(SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  closesocket(pContext->hSock);
}

/*********************************************************************
*
*       _cbSend()
*
*  Function description
*    Sends data via socket interface.
*
*  Parameters
*    pData   : Pointer to data to send.
*    NumBytes: Number of bytes to send.
*    hSock   : Socket handle (or context).
*
*  Return value
*    O.K. : >= 0
*    Error: <  0
*/
static int _cbSend(const char* pData, int NumBytes, SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
#if APP_USE_SSL
  if (pContext->IsSecure != 0) {
    return SSL_SESSION_Send(&pContext->Session, pData, NumBytes);
  }
  else
#endif
  {
    return send(pContext->hSock, pData, NumBytes, 0);
  }
}

/*********************************************************************
*
*       _cbRecv()
*
*  Function description
*    Receives data via socket interface.
*
*  Parameters
*    pData   : Pointer to data buffer to store received data.
*    NumBytes: Max. number of bytes to receive.
*    hSock   : Socket handle (or context).
*
*  Return value
*    O.K. : >   0
*    Error: <=  0
*/
static int _cbRecv(char* pData, int NumBytes, SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;
  int                Status;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
#if APP_USE_SSL
  if (pContext->IsSecure != 0) {
    do {
      Status = SSL_SESSION_Receive(&pContext->Session, pData, NumBytes);
    } while (Status == 0);  // Receiving 0 bytes means something different on a plain socket.
    //
    // Translate EOF into "connection closed",
    // the Server expects same return values as with recv().
    //
    if (Status == SSL_ERROR_EOF) {
      Status = 0;
    }
  }
  else
#endif
  {
    Status = recv(pContext->hSock, pData, NumBytes, 0);
  }
  return Status;
}

/*********************************************************************
*
*       _cbGetTimeDate()
*
*  Function description
*    Returns time and date of the system.
*
*  Return value
*    Date and time of the system as U32.
*/
static U32 _cbGetTimeDate(void) {
  U32 r;
  U16 Sec;
  U16 Min;
  U16 Hour;
  U16 Day;
  U16 Month;
  U16 Year;

  Sec    = 0;   //    0 based. Valid range: 0..59 .
  Min    = 0;   //    0 based. Valid range: 0..59 .
  Hour   = 9;   //    0 based. Valid range: 0..23 .
  Day    = 9;   //    1 based. Means that 1 is 1. Valid range is 1..31 (depending on month).
  Month  = 1;   //    1 based. Means that January is 1. Valid range is 1..12 .
  Year   = 29;  // 1980 based. Means that 2008 would be 28.
  r      = (Sec / 2) + (Min << 5) + (Hour  << 11);
  r     |= ((U32)(Day + (Month << 5) + (Year  << 9))) << 16;
  return r;
}

/*********************************************************************
*
*       _cbSendAttachment()
*
*  Function description
*    Sends the content of an attachment.
*
*  Parameters
*    pContext: SMTPc context.
*    pItem   : Item to send.
*    pAPI    : API used to send the content.
*
*  Return value
*    == 0: O.K., attachment sent.
*    <  0: Error.
*/
static int _cbSendAttachment(IP_SMTPC_CONTEXT* pContext, const IP_SMTPC_MULTIPART_ITEM* pItem, const IP_SMTPC_MULTIPART_API* pAPI) {
  unsigned Index;

  //
  // Send the content of the attachment. Sending can be done in chunks
  // for generated content or e.g. when sending content of a file that
  // is read in chunks.
  //
  Index = (unsigned)pItem->pUserContext;
  pAPI->pfSend(pContext, _aAttachmentContent[Index], SMTP_STRLEN(_aAttachmentContent[Index]));
  return 0;
}

/*********************************************************************
*
*       SMTP client callback_API
*
*  Description
*    Callback API function tables
*/
static const IP_SMTPC_API _IP_Api = {
   _cbConnect
  ,_cbDisconnect
  ,_cbSend
  ,_cbRecv
#if APP_USE_SSL
  ,_cbUpgrade
  ,_cbDowngrade
#else
  ,NULL
  ,NULL
#endif
};

static const IP_SMTPC_APPLICATION _SMTPC_App = {
  _cbGetTimeDate,  // Time and Date routine, adapt _GetTimeDate() .
  NULL,          // Pointer to status callback function. Can be NULL.
  OWN_DOMAIN,    // Your domain. According to RFC 821 the maximum total length of a domain name or number is 64 characters. Can be NULL.
  UTC_OFFSET     // The offset from Coordinated Universal Time (UTC). Can be NULL.
};

/*********************************************************************
*
*       _SMTPcTask()
*
*  Function description
*    SMTP client task that tries to resolve the server name via DNS
*    to send a message as configured.
*/
static void _SMTPcTask(void) {
  int  r;
  int  i;
  int  NumIFaces;
  U8   IFaceReady;

  //
  // Check if sample is correctly configured!
  //
  if(*_MTAConfig.sServer == 0) {
    SMTPC_APP_WARN(("Configuration not valid. Enter valid SMTP server, sender and recipient(s).\n"));
    while (1) {
      OS_Delay(100);
    }
  }
  //
  // Make sure that at least one interface is ready before starting the sample.
  //
  IFaceReady = 0;
  NumIFaces  = IP_INFO_GetNumInterfaces();
  do {
    for (i = 0; i < NumIFaces; i++) {
      if (IP_IFaceIsReadyEx(i) != 0) {
        IFaceReady = 1;
        break;
      }
      OS_Delay(100);
    }
  } while (IFaceReady == 0);
  //
  // Send mail.
  //
  r = IP_SMTPC_Send(&_IP_Api, &_aMailAddr[0], SEGGER_COUNTOF(_aMailAddr), &_Message, &_MTAConfig, &_SMTPC_App);
  if (r != 0) {
    SMTPC_APP_WARN(("Mail could not be sent.\n"));
  } else {
    SMTPC_APP_LOG(("Mail sent.\n"));
  }
  while (1) {
    OS_Delay(100);
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
  OS_SetTaskName(OS_GetTaskID(), "SMTP client");
#if (APP_USE_SSL != 0)
  //
  // Initialize SSL.
  //
  SSL_Init();
#endif
  _SMTPcTask();
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
*  Function description
*    Main sample starting point when running one individual sample.
*
*  Additional information
*    Runs the sample in a task of its own to decouple it from not
*    knowing the required task stack size of this sample when
*    starting from main() . This task typically is terminated once
*    it has fulfilled its purpose.
*
*    This routine initializes everything that might be common with
*    other samples of the same kind. These initializations can then
*    be skipped by not starting from MainTask() but APP_MainTask()
*    instead.
*/
void MainTask(void) {
  int IFaceId;

  //
  // Initialize the IP stack
  //
  IP_Init();
  //
  // Start TCP/IP task
  //
  OS_CREATETASK(&_IPTCB,   "IP_Task",   IP_Task,   APP_TASK_PRIO_IP_TASK  , _IPStack);
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, APP_TASK_PRIO_IP_RXTASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                               // Register hook to be notified on disconnects.
  IFaceId = IP_INFO_GetNumInterfaces() - 1;                                               // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  IP_Connect(IFaceId);                                                                    // Connect the interface if necessary.
  //
  // IPv4 address configured ?
  //
  while (IP_IFaceIsReadyEx(IFaceId) == 0) {
    BSP_ToggleLED(0);
    OS_Delay(200);
  }
  BSP_ClrLED(0);
  //
  // Start the sample itself.
  //
  OS_CREATETASK(&APP_MainTCB, "APP_MainTask", APP_MainTask, APP_TASK_PRIO_MAINTASK, APP_MainStack);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
