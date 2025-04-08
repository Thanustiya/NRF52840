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

Purpose : Demonstrates use of the emFTP client in different configurations.

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

Additional information:
  The sample can easily be configured to start the emFTP Client
  with or without SSL support, and to upload a specific file
  when running a non-read-only filesystem on a storage medium.

  Preparations:
    Enable/disable the features that you want to use. The available
    binary configuration switches are:
      - APP_USE_SSL : Helper switch that enables/disables all SSL switches below.
      - USE_FS_RO   : Helper switch that enables/disables all read only filesystem switches below.

    Additionally the following configuration defines are available:
      - SERVER_ROOT_DIR       : Specify the user's root directory on the server (with trailing slash).
      - SERVER_TEST_DIR_NOSL  : Specify the test directory to create/delete on the server (without trailing slash).
      - FTP_HOST              : Specify the host the FTP server is running on.
      - FTP_PORT              : Specify the port and thus whether to use explicit or implicit mode.
      - FTP_USER              : Specify the username to use to log into the server.
      - FTP_PASS              : Specify the password to use to log into the server.

  Expected behavior:
    This sample starts an FTP client, which connects to a
    specified FTP Server and tries to perform various actions
    like creating a directory, uploading a file,
    and listing the current working directory.

  Example output:
    ...
    4:072 FTPClient - APP: Connect to server "ftptest@192.168.11.231:21".
    4:073 FTPClient - APP: Connected to 192.168.11.231, port 21.
    4:075 FTPClient - FTP command: FEAT
    4:077 FTPClient - FTP command: USER ftptest
    4:078 FTPClient - FTP command: PASS Secret123
    4:088 FTPClient - FTP command: SYST
    4:088 FTPClient - FTP command: PWD
    4:089 FTPClient - FTP command: TYPE A
    4:090 FTPClient - APP: Creating directory "Test".
    4:090 FTPClient - FTP command: MKD Test
    4:091 FTPClient - APP: --- Test with old API that needs to CWD into the directory to operate within.
    4:091 FTPClient - APP: Changing to directory "/home/ftptest/Test/".
    4:091 FTPClient - FTP command: CWD /home/ftptest/Test/
    4:092 FTPClient - APP: List directory (before upload):
    4:092 FTPClient - FTP command: LIST
    4:092 FTPClient - FTP command: PASV
    4:094 FTPClient - APP: Connected to 192.168.11.231, port 34763.
    4:095 FTPClient -
    4:095 FTPClient - APP: Uploading "index.htm".
    4:096 FTPClient - FTP command: TYPE I
    4:096 FTPClient - FTP command: PASV
    4:097 FTPClient - APP: Connected to 192.168.11.231, port 39997.
    4:097 FTPClient - FTP command: STOR index.htm
    4:099 FTPClient - APP: List directory (after upload):
    4:099 FTPClient - FTP command: LIST
    4:099 FTPClient - FTP command: PASV
    4:100 FTPClient - APP: Connected to 192.168.11.231, port 44117.
    4:102 FTPClient - -rw-r--r--   1 ftptest  ftptest       629 May 12 09:16 index.htm

    4:102 FTPClient - APP: Appending 45 bytes: "This has been added using the APPEnd command.".
    4:103 FTPClient - FTP command: TYPE I
    4:103 FTPClient - FTP command: PASV
    4:105 FTPClient - APP: Connected to 192.168.11.231, port 37429.
    4:105 FTPClient - FTP command: APPE index.htm
    4:106 FTPClient - APP: List directory (after append):
    4:106 FTPClient - FTP command: LIST
    4:107 FTPClient - FTP command: PASV
    4:108 FTPClient - APP: Connected to 192.168.11.231, port 40449.
    4:109 FTPClient - -rw-r--r--   1 ftptest  ftptest       674 May 12 09:16 index.htm

    4:109 FTPClient - APP: Deleting "index.htm".
    4:110 FTPClient - FTP command: DELE index.htm
    4:110 FTPClient - APP: List directory (after delete):
    4:110 FTPClient - FTP command: LIST
    4:111 FTPClient - FTP command: PASV
    4:112 FTPClient - APP: Connected to 192.168.11.231, port 41055.
    4:113 FTPClient -
    4:113 FTPClient - APP: Changing to parent directory.
    4:114 FTPClient - FTP command: CDUP
    4:114 FTPClient - APP: --- Test with new API using IP_FTPC_CMD_CONFIG that can operate regardless of current directory.
    4:114 FTPClient - APP: List directory (before upload, old API with path parameter):
    4:114 FTPClient - FTP command: LIST /home/ftptest/Test/
    4:115 FTPClient - FTP command: PASV
    4:116 FTPClient - APP: Connected to 192.168.11.231, port 34945.
    4:117 FTPClient -
    4:117 FTPClient - APP: Uploading "index.htm" to "/home/ftptest/Test/Test.txt" (new API).
    4:118 FTPClient - FTP command: TYPE I
    4:118 FTPClient - FTP command: PASV
    4:119 FTPClient - APP: Connected to 192.168.11.231, port 36859.
    4:120 FTPClient - FTP command: STOR /home/ftptest/Test/Test.txt
    4:121 FTPClient - APP: List directory (after upload, new API):
    4:121 FTPClient - FTP command: LIST /home/ftptest/Test/
    4:122 FTPClient - FTP command: PASV
    4:123 FTPClient - APP: Connected to 192.168.11.231, port 46065.
    4:125 FTPClient - -rw-r--r--   1 ftptest  ftptest       629 May 12 09:16 Test.txt

    4:125 FTPClient - APP: Deleting "/home/ftptest/Test/Test.txt" (new API).
    4:125 FTPClient - FTP command: DELE /home/ftptest/Test/Test.txt
    4:126 FTPClient - APP: List directory (after delete, new API):
    4:126 FTPClient - FTP command: LIST /home/ftptest/Test/
    4:126 FTPClient - FTP command: PASV
    4:127 FTPClient - APP: Connected to 192.168.11.231, port 44047.
    4:129 FTPClient -
    4:129 FTPClient - APP: Delete directory "/home/ftptest/Test/" (new API).
    4:129 FTPClient - FTP command: RMD /home/ftptest/Test/
    4:130 FTPClient - APP: Done.
*/

#include "RTOS.h"
#include "FS.h"
#include "BSP.h"
#include "IP.h"
#include "TaskPrio.h"
#include "IP_FTPC.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   FTPC_STRLEN
  #define FTPC_STRLEN          strlen
#endif

#ifndef   FTPC_MEMSET
  #define FTPC_MEMSET          memset
#endif

#ifndef   USE_RX_TASK
  #define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.
#endif

//
// Sample features enable/disable
//
#ifndef   APP_USE_SSL
  #define APP_USE_SSL                (0)                                         // Use SSL to enable support for secure connections.
#endif
#define   SERVER_ROOT_DIR            "/"                                         // Specify the user's root directory on the server (with trailing slash). For most users this will be "/".
#define   SERVER_TEST_DIR_NOSL       "Test"                                      // Specify the test directory to create/delete in the user's root directory on the server (without trailing slash!!!).
#define   SERVER_TEST_DIR            SERVER_TEST_DIR_NOSL "/"                    // The test directory name is used with the old API as the name alone (without trailing SLash).
#define   USE_FS_RO                  1                                           // Use a read-only filesystem. This runs only commands in this sample that do not require writing to a local FS.

#if (USE_FS_RO == 0)
  #define UPLOAD_FS                IP_FS_FS
  #define UPLOAD_FILENAME_OLD_API  "Readme.txt"                                // Filename present in the root folder on the (real)    FS, used with USE_FS_RO disabled.
  #define UPLOAD_PATH_LOCAL        "Readme.txt"                                // Local  filename/path                on the (real)    FS, used with USE_FS_RO disabled.
  #define UPLOAD_PATH_REMOTE       SERVER_ROOT_DIR SERVER_TEST_DIR "Test.txt"  // Remote filename/path to upload to                      , used with USE_FS_RO disabled.
#else
  #define UPLOAD_FS                IP_FS_ReadOnly
  #define UPLOAD_FILENAME_OLD_API  "index.htm"                                 // Filename present in the root folder on the read-only FS, used with USE_FS_RO enabled.
  #define UPLOAD_PATH_LOCAL        "index.htm"                                 // Local  filename/path                on the read-only FS, used with USE_FS_RO enabled.
  #define UPLOAD_PATH_REMOTE       SERVER_ROOT_DIR SERVER_TEST_DIR "Test.txt"  // Remote filename/path to upload to                      , used with USE_FS_RO enabled.
#endif

#if APP_USE_SSL
#include "SSL.h"
#endif

#define FTP_HOST  "192.168.11.122"

//
// Choose between FTPS implicit mode server using port 990
// or FTPS explicit mode and plain mode server using port 21.
//
#define FTP_PORT  21     // FTP plain / FTPS explicit.
//#define FTP_PORT  990    // FTP implicit

#define FTP_USER  "Admin"
#define FTP_PASS  "Secret"

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef       APP_MAIN_STACK_SIZE
  #if (APP_USE_SSL != 0)
    #define   APP_MAIN_STACK_SIZE      (6144)
  #else
    #define   APP_MAIN_STACK_SIZE      (4096)
  #endif
#endif
#ifndef       APP_TASK_STACK_OVERHEAD
  #define     APP_TASK_STACK_OVERHEAD  (0)
#endif
#ifndef       FTPC_STACK_SIZE
  #define     FTPC_STACK_SIZE          (APP_MAIN_STACK_SIZE + APP_TASK_STACK_OVERHEAD)
#endif

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef struct {
  unsigned                   PlainSocket;
#if APP_USE_SSL
  unsigned                   IsSecure;
  unsigned                   MustResume;
  SSL_SESSION                Session;
  SSL_SESSION_RESUME_PARAS   ResumeParas;
  const char*                sName;
#endif
} FTPS_APP_CONTEXT;

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
*       static data
*
**********************************************************************
*/

static const char* sAppendTest = "This has been added using the APPEnd command.";

static FTPS_APP_CONTEXT  _aContext[3];  // Connecting to one server requires two connections/sessions (command and data).
                                        // If active mode is used, an additional socket which waits for a connection is required.
                                        // If only passive mode is used, the number of available contexts can be set to 2.

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

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int APP_MainStack[FTPC_STACK_SIZE / sizeof(int)];        // Stack of the starting point of this sample.
static OS_TASK         APP_MainTCB;                                         // Task-Control-Block of the IP_Task.

static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

/*********************************************************************
*
*       static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnStateChange()
*
* Function description
*   Callback that will be notified once the state of an interface
*   changes.
*
* Parameters
*   IFaceId   : Zero-based interface index.
*   AdminState: Is this interface enabled ?
*   HWState   : Is this interface physically ready ?
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
*       _IsIPAddress()
*
*  Function description
*    Checks if string is a dot-decimal IP address, for example 192.168.1.1
*/
static unsigned _IsIPAddress(const char * sIPAddr) {
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
      if ((NumDots < 3) || (i > 15)) { // Error, every dot-decimal IP address includes 3 '.' and is not longer than 15 characters.
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
*    IP as 32-bit number in host endianess.
*/
static long _ParseIPAddr(const char * sIPAddr) {
  long     IPAddr;
  unsigned Value;
  unsigned NumDots;
  unsigned i;
  unsigned j;
  char     acDigits[4];
  char     c;

  IPAddr = 0;
  //
  // Check if string is a valid IP address
  //
  Value = _IsIPAddress(sIPAddr);
  if (Value) {
    //
    // Parse the IP address
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
*       _AllocContext()
*
*  Function description
*    Retrieves the next free context memory block to use.
*
*  Parameters
*    hSock: Socket handle.
*
*  Return value
*    != NULL: Free context found, pointer to context.
*    == NULL: Error, no more free entries.
*/
static FTPS_APP_CONTEXT* _AllocContext(long hSock) {
  FTPS_APP_CONTEXT* pContext;
  FTPS_APP_CONTEXT* pRunner;
  unsigned          i;

  pContext = NULL;
  i        = 0;
  OS_DI();
  pRunner = &_aContext[0];
  do {
    if (pRunner->PlainSocket == 0) {
#if APP_USE_SSL
      if (pRunner->Session.Socket == 0)
#endif
      {
        FTPC_MEMSET(pRunner, 0, sizeof(FTPS_APP_CONTEXT));
        pRunner->PlainSocket = hSock;  // Mark the entry to be in use.
        pContext             = pRunner;
        break;
      }
    }
    pRunner++;
    i++;
  } while (i < SEGGER_COUNTOF(_aContext));
  OS_EI();
  return pContext;
}

/*********************************************************************
*
*       _FreeContext()
*
*  Function description
*    Frees a context memory block.
*
*  Parameters
*    pContext: Connection context.
*/
static void _FreeContext(FTPS_APP_CONTEXT* pContext) {
  FTPC_MEMSET(pContext, 0, sizeof(*pContext));
}

#if APP_USE_SSL
/*********************************************************************
*
*       _UpgradeOnDemand()
*
*  Function description
*    Upgrades a connection to a secured one.
*
*  Parameters
*    pContext: Connection context.
*
*  Return value
*    == 0: Success.
*    <  0: Error.
*/
static int _UpgradeOnDemand(FTPS_APP_CONTEXT* pContext) {
  int Status;

  Status = 0;
  if ((pContext->IsSecure != 0) && (pContext->PlainSocket != 0) && (pContext->Session.Socket == 0)) {
    SSL_SESSION_Prepare(&pContext->Session, pContext->PlainSocket, &_IP_Transport);
    if (pContext->MustResume != 0) {
      SSL_SESSION_SetResumeParas(&pContext->Session, &pContext->ResumeParas);  // Bind data connection to control connection
    }
    Status = SSL_SESSION_Connect(&pContext->Session, pContext->sName);
    if (Status < 0) {
      closesocket(pContext->PlainSocket);
      FTPC_APP_LOG(("APP: Could not upgrade to secure."));
    } else {
      if (pContext->MustResume) {
        if (SSL_SESSION_QueryFlags(&pContext->Session) & SSL_SESSION_FLAG_RESUME_GRANTED) {
          FTPC_APP_LOG(("APP: Session was successfully resumed"));
        } else {
          FTPC_APP_LOG(("APP: Session was not resumed -- new session ID provided by server"));
        }
      }
      FTPC_APP_LOG(("APP: Secured using %s.", SSL_SUITE_GetIANASuiteName(SSL_SUITE_GetID(SSL_SESSION_GetSuite(&pContext->Session)))));
    }
    pContext->PlainSocket = 0;
  }
  return Status;
}
#endif


/*********************************************************************
*
*       _Connect
*
*  Function description
*    Creates a socket and opens a TCP connection to the FTP host.
*/
static FTPC_SOCKET _Connect(const char * sSrvAddr, unsigned SrvPort) {
  struct sockaddr_in sin;
  struct hostent*    pHostEntry;
  FTPS_APP_CONTEXT*  pContext;
  long               Ip;
  long               Sock;
  int                Error;
  int                r;

  if (_IsIPAddress(sSrvAddr)) {
    Ip = _ParseIPAddr(sSrvAddr);
    Ip = htonl(Ip);
  } else {
    //
    // Convert host into IP address
    //
    pHostEntry = gethostbyname((char*)sSrvAddr);
    if (pHostEntry == NULL) {
      FTPC_APP_LOG(("APP: gethostbyname failed: %s\r\n", sSrvAddr));
      return NULL;
    }
    Ip = *(unsigned*)(*pHostEntry->h_addr_list);
  }
  //
  // Create socket and connect to the FTP server
  //
  Sock = socket(AF_INET, SOCK_STREAM, 0);
  if(Sock  == -1) {
    FTPC_APP_LOG(("APP: Could not create socket!" ));
    return NULL;
  }
  FTPC_MEMSET(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(SrvPort);
  sin.sin_addr.s_addr = Ip;
  r = connect(Sock, (struct sockaddr*)&sin, sizeof(sin));
  if(r == SOCKET_ERROR) {
    getsockopt((long)Sock, AF_INET, SO_ERROR, &Error, sizeof(Error));
    closesocket(Sock);
    FTPC_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(Error)));
    return NULL;
  }
  //
  // Alloc context.
  //
  pContext = _AllocContext(Sock);
  if (pContext == NULL) {
    closesocket(Sock);
    FTPC_APP_LOG(("APP: Could not allocate context!" ));
    return NULL;
  }
#if APP_USE_SSL
  pContext->sName = sSrvAddr;
#endif
  FTPC_APP_LOG(("APP: Connected to %i, port %d.", Ip, SrvPort));
  return (FTPC_SOCKET)pContext;
}

/*********************************************************************
*
*       _Disconnect
*
*  Function description
*    Closes a socket.
*/
static void _Disconnect(FTPC_SOCKET Socket) {
  FTPS_APP_CONTEXT* pContext;

  pContext = (FTPS_APP_CONTEXT*)Socket;
  if (pContext != NULL) {
#if APP_USE_SSL
    //
    // Check if the connection is marked as a secured connection and check if it has been used so far.
    // If the connection was meant to be used secured but has not been used at all (no prepare and connect),
    // simply disconnect the plain socket.
    //
    if ((pContext->IsSecure != 0) && (SSL_SESSION_GetSuite(&pContext->Session) != NULL)) {
      SSL_SESSION_Disconnect(&pContext->Session);
      closesocket(pContext->Session.Socket);
    }
    else
#endif
    {
      if (pContext->PlainSocket != 0) {
        closesocket(pContext->PlainSocket);
      }
    }
    _FreeContext(pContext);
  }
}

/*********************************************************************
*
*       _Send
*
*  Function description
*    Sends data via socket interface.
*/
static int _Send(const char * buf, int len, void * pConnectionInfo) {
  FTPS_APP_CONTEXT* pContext;
  int               Status;

  pContext = (FTPS_APP_CONTEXT*)pConnectionInfo;
#if APP_USE_SSL
  _UpgradeOnDemand(pContext);
  if (pContext->IsSecure != 0) {
    Status = SSL_SESSION_Send(&pContext->Session, buf, len);
  }
  else
#endif
  {
    Status = send(pContext->PlainSocket, buf, len, 0);
  }
  //
  return Status;
}

/*********************************************************************
*
*       _Recv
*
*  Function description
*    Receives data via socket interface.
*/
static int _Recv(char * buf, int len, void * pConnectionInfo) {
  FTPS_APP_CONTEXT* pContext;
  int               Status;

  pContext = (FTPS_APP_CONTEXT*)pConnectionInfo;
#if APP_USE_SSL
  _UpgradeOnDemand(pContext);
  if (pContext->IsSecure != 0) {
    do {
      Status = SSL_SESSION_Receive(&pContext->Session, buf, len);
    } while (Status == 0);  // Receiving 0 bytes means something different on a plain socket.
    //
    // Translate EOF into "connection closed",
    // the FTP Server expects same return values as with recv().
    //
    if (Status == SSL_ERROR_EOF) {
      Status = 0;
    }
  }
  else
#endif
  {
    Status = recv(pContext->PlainSocket, buf, len, 0);
  }
  //
  return Status;
}

#if APP_USE_SSL
/*********************************************************************
*
*       _SetSecure
*
*  Function description
*    Configure the socket as secured.
*/
static int _SetSecure(FTPC_SOCKET Socket, FTPC_SOCKET Clone) {
  FTPS_APP_CONTEXT* pSocket;
  FTPS_APP_CONTEXT* pClone;
  //
  pSocket = (FTPS_APP_CONTEXT*)Socket;
  //
  // Check if we have a valid socket.
  // The FTP client prior V3.50.5 calls this routine even
  // if the connect to the server was not successful.
  //
  if (pSocket != NULL) {
    pClone = (FTPS_APP_CONTEXT*)Clone;
    //
    pSocket->IsSecure     = 1;
    if (pClone != NULL) {
      pSocket->MustResume = 1;
    } else {
      pSocket->MustResume = 0;
    }
    if (pClone != NULL) {
      SSL_SESSION_GetResumeParas(&pClone->Session, &pSocket->ResumeParas);
    }
  }
  //
  return 0;
}
#endif


/*********************************************************************
*
*       _Listen()
*
*  Function description
*    This function is called from the FTP client module if active
*    FTP should be used. It creates a socket and starts listening
*    on the port.
*
*    If active FTP should not be used, this function can be removed.
*
*  Parameter
*    CtrlSocket - Control socket
*    pPort      - [Out] Port number of the data connection.
*    pIPAddr    - [Out] IP address of the client.
*
*  Return value
*    > 0   Socket descriptor
*    NULL  Error
*/
static FTPC_SOCKET _Listen(FTPC_SOCKET CtrlSocket, U8* pIPAddr,  U16* pPort) {
  FTPS_APP_CONTEXT*  pCtrlContext;
  FTPS_APP_CONTEXT*  pDataContext;
  struct sockaddr_in CtrlSockAddrIn;
  struct sockaddr_in DataSockAddrIn;
  long               DataSock;
  long               CtrlSock;
  U32                IPAddr;
  int                AddrSize;
  int                r;

  pDataContext = NULL;
  pCtrlContext = (FTPS_APP_CONTEXT*)CtrlSocket;
  //
  // Create listening socket for the data channel.
  //
  if (pCtrlContext != NULL) {
#if APP_USE_SSL
    //
    // SSL
    //
    if (pCtrlContext->IsSecure != 0) {
      CtrlSock = pCtrlContext->Session.Socket;
    } else
#endif
    {
      CtrlSock = pCtrlContext->PlainSocket;
    }
    FTPC_MEMSET(&CtrlSockAddrIn, 0, sizeof(CtrlSockAddrIn));
    AddrSize = sizeof(CtrlSockAddrIn);
    r = getsockname(CtrlSock, (struct sockaddr*)&CtrlSockAddrIn, &AddrSize);
    if (r != SOCKET_ERROR) {
      DataSock     = socket(AF_INET, SOCK_STREAM, 0);  // Create a new socket for data connection to the client
      if(DataSock != SOCKET_ERROR) {                   // Socket created ?
        //
        // Bind a socket to the data port (data port: port number of the control socket+1)
        // and set into listening mode.
        //
        FTPC_MEMSET(&DataSockAddrIn, 0, sizeof(DataSockAddrIn));
        DataSockAddrIn.sin_family      = AF_INET;
        DataSockAddrIn.sin_port        = 0;                         // Let Stack find a free port.
        DataSockAddrIn.sin_addr.s_addr = INADDR_ANY;
        bind(DataSock, (struct sockaddr*)&DataSockAddrIn, sizeof(DataSockAddrIn));
        listen(DataSock, 1);
        //
        // Allocate a FTP context.
        //
        pDataContext = _AllocContext(DataSock);
        if (pDataContext == NULL) {
          closesocket(DataSock);
        } else {
          FTPC_MEMSET(&DataSockAddrIn, 0, sizeof(DataSockAddrIn));
          AddrSize = sizeof(DataSockAddrIn);
          getsockname(DataSock , (struct sockaddr*)&DataSockAddrIn, &AddrSize);
          *pPort = ntohs(DataSockAddrIn.sin_port);
          IPAddr = ntohl(CtrlSockAddrIn.sin_addr.s_addr);  // Load to host endianess.
          SEGGER_WrU32BE(pIPAddr, IPAddr);                 // Save from host endianess to network endiness.
        }
      }
    }
  }
  return (FTPC_SOCKET)pDataContext;
}


/*********************************************************************
*
*       _Accept()
*
*  Function description
*    This function is called from the FTP client module if active
*    FTP should be used. It waits for the connection of the server
*    to the data port.
*
*    If active FTP should not be used, this function can be removed.
*
*  Parameter
*    CtrlSocket - Control socket
*    ListenSock - Listen socket to use for the data channel operations.
*    pPort      - [Out] Port used by data connection.
*
*  Return value
*    > 0   Socket descriptor
*    NULL  Error
*/
static FTPC_SOCKET _Accept(FTPC_SOCKET CtrlSocket, FTPC_SOCKET ListenSock, U16* pPort) {
  FTPS_APP_CONTEXT*  pDataListenContext;
  FTPS_APP_CONTEXT*  pDataContext;
  struct sockaddr_in DataSockAddrIn;
  long               hSockListen;
  long               DataSock;
  int                SoError;
  int                t0;
  int                t;
  struct sockaddr    Addr;
  int                AddrSize;
  int                Opt;
  int                r;

  (void)CtrlSocket;
  r                  = 0;
  AddrSize           = sizeof(Addr);
  pDataListenContext = (FTPS_APP_CONTEXT*)ListenSock;
#if APP_USE_SSL
  if (pDataListenContext->PlainSocket == 0) {
    hSockListen = pDataListenContext->Session.Socket;
  }
  else
#endif
  {
    hSockListen = pDataListenContext->PlainSocket;
  }
  //
  // Set command socket non-blocking.
  //
  Opt = 1;
  setsockopt(hSockListen, SOL_SOCKET, SO_NONBLOCK, &Opt, sizeof(Opt));
  t0 = IP_OS_GET_TIME();
  do {
    DataSock = accept(hSockListen, &Addr, &AddrSize);
    if ((DataSock != SOCKET_ERROR) && (DataSock != 0)) {
      //
      // Connection accepted. Close listening socket.
      //
      closesocket(hSockListen);
      _FreeContext(pDataListenContext);
      //
      // Set data socket blocking. The data socket inherits the blocking
      // mode from the socket that was used as parameter for accept().
      // Therefore, we have to set it blocking after creation.
      //
      Opt = 0;
      setsockopt(DataSock, SOL_SOCKET, SO_NONBLOCK, &Opt, sizeof(Opt));
      //
      // SO_KEEPALIVE is required to guarantee that the socket will be
      // closed even if the client has lost the connection to the server
      // before it closes the connection.
      //
      Opt = 1;
      setsockopt(DataSock, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
      //
      // Allocate a FTP context.
      //
      pDataContext = _AllocContext(DataSock);
      if (pDataContext == NULL) {
        closesocket(DataSock);
      } else {
        //
        // Get connection information.
        //
        FTPC_MEMSET(&DataSockAddrIn, 0, sizeof(DataSockAddrIn));
        r = getsockname(DataSock , (struct sockaddr*)&DataSockAddrIn, &AddrSize);
        if (r != SOCKET_ERROR) {
          FTPC_APP_LOG(("APP: Data connection established on local port: %lu\r\n", ntohs(DataSockAddrIn.sin_port)));
        }
        *pPort = ntohs(DataSockAddrIn.sin_port);
      }
      return (FTPC_SOCKET)pDataContext;               // Successfully connected
    }
    //
    // Handle socket error.
    //
    getsockopt(hSockListen, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    if (SoError != IP_ERR_WOULD_BLOCK) {
      closesocket(hSockListen);
      _FreeContext(pDataListenContext);
      FTPC_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(SoError)));
      return NULL;       // Not in progress and not successful, error...
    }
    //
    // Wait for connection (max. 1 second)
    //
    t = IP_OS_GET_TIME() - t0;
    if (t >= 1000) {
      closesocket(hSockListen);
      _FreeContext(pDataListenContext);
      return NULL;
    }
    OS_Delay(1);                 // Give lower prior tasks some time
  } while (1);
}


//
// IP API.
//
static const IP_FTPC_API _IP_Api = {
  _Connect,
  _Disconnect,
  _Send,
  _Recv,
#if APP_USE_SSL
  _SetSecure,
#else
  NULL,
#endif
  _Listen,
  _Accept
};

#if (USE_FS_RO == 0)
/*********************************************************************
*
*       _FSTest
*
*  Function description
*    Initializes the file system and creates a test file on storage medium
*/
static void _FSTest(void) {
  FS_FILE*    pFile;
  unsigned    Len;
  const char* sInfo = "SEGGER emFTP client.\r\nFor further information please visit: www.segger.com\r\n";

  FS_Init();
  Len = FTPC_STRLEN(sInfo);
  if (FS_IsLLFormatted("") == 0) {
    FTPC_APP_LOG(("Low level formatting"));
    FS_FormatLow("");          // Erase & Low-level format the volume
  }
  if (FS_IsHLFormatted("") == 0) {
    FTPC_APP_LOG(("High level formatting\n"));
    FS_Format("", NULL);       // High-level format the volume
  }
  pFile = FS_FOpen(UPLOAD_FILENAME_OLD_API, "w");
  FS_Write(pFile, sInfo, Len);
  FS_FClose(pFile);
  FS_Unmount("");
}
#endif

/*********************************************************************
*
*       _FTPClientTask
*
*/
static void _FTPClientTask(void) {
  IP_FTPC_CMD_CONFIG Config;
  IP_FTPC_CONTEXT    FTPConnection;
  U32                acCtrlIn[FTPC_CTRL_BUFFER_SIZE / sizeof(U32)]; // U32 to make sure we have a word alignment.
  U32                acDataIn[FTPC_BUFFER_SIZE / sizeof(U32)];      // U32 to make sure we have a word alignment.
  U32                acDataOut[FTPC_BUFFER_SIZE / sizeof(U32)];     // U32 to make sure we have a word alignment.
  unsigned           Mode;
  int                r;

  FTPC_MEMSET(&Config, 0, sizeof(Config));
  //
  // Initialize FTP client context
  //
  FTPC_MEMSET(&FTPConnection, 0, sizeof(FTPConnection));
  //
  // Initialize the FTP client
  //
  IP_FTPC_Init(&FTPConnection, &_IP_Api, &UPLOAD_FS, (U8*)acCtrlIn, sizeof(acCtrlIn), (U8*)acDataIn, sizeof(acDataIn), (U8*)acDataOut, sizeof(acDataOut));
  //
  // Connect to the FTP server
  //
  Mode  = FTPC_MODE_PASSIVE;
//  Mode  = FTPC_MODE_ACTIVE;
#if APP_USE_SSL
  #if FTP_PORT == 990
    Mode |= FTPC_MODE_IMPLICIT_TLS_REQUIRED;
  #else
    Mode |= FTPC_MODE_EXPLICIT_TLS_REQUIRED;
  #endif
#endif
  FTPC_APP_LOG(("APP: Connect to server \"%s@%s:%u\".", FTP_USER, FTP_HOST, FTP_PORT));
  r = IP_FTPC_Connect(&FTPConnection, FTP_HOST, FTP_USER, FTP_PASS, FTP_PORT, Mode);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not connect to FTP server."));
    goto Disconnect;
  }
  //
  // Create the test directory
  //
  FTPC_APP_LOG(("APP: Creating directory \"%s\".", SERVER_TEST_DIR_NOSL));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_MKD, SERVER_TEST_DIR_NOSL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not create directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("APP: --- Test with old API that needs to CWD into the directory to operate within."));
  //
  // Change from root directory into test directory
  //
  FTPC_APP_LOG(("APP: Changing to directory \"%s\".", SERVER_ROOT_DIR SERVER_TEST_DIR));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_CWD, SERVER_ROOT_DIR SERVER_TEST_DIR);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not change working directory."));
    goto Disconnect;
  }
  //
  // List directory content (before upload)
  //
  FTPC_APP_LOG(("APP: List directory (before upload):"));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_LIST, NULL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Upload a file
  //
  FTPC_APP_LOG(("APP: Uploading \"%s\".", UPLOAD_FILENAME_OLD_API));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_STOR, UPLOAD_FILENAME_OLD_API);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not upload data file."));
    goto Disconnect;
  }
  //
  // List directory content (after upload)
  //
  FTPC_APP_LOG(("APP: List directory (after upload):"));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_LIST, NULL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Try to append something to the end of the file using
  // input from a buffer instead of a file. Can be used to
  // append log data to a remote file.
  //
  Config.sPara    = UPLOAD_FILENAME_OLD_API;
  Config.pData    = (U8*)sAppendTest;
  Config.NumBytes = FTPC_STRLEN(sAppendTest);
  FTPC_APP_LOG(("APP: Appending %u bytes: \"%s\".", Config.NumBytes, sAppendTest));
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_APPE, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not append to data file."));
    goto Disconnect;
  }
  Config.pData = NULL;  // No need to memset the whole structure but this would override further uploads from FS in this sample.
  //
  // List directory content (after append)
  //
  FTPC_APP_LOG(("APP: List directory (after append):"));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_LIST, NULL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Delete the file
  //
  FTPC_APP_LOG(("APP: Deleting \"%s\".", UPLOAD_FILENAME_OLD_API));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_DELE, UPLOAD_FILENAME_OLD_API);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not delete data file."));
    goto Disconnect;
  }
  //
  // List directory content
  //
  FTPC_APP_LOG(("APP: List directory (after delete):"));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_LIST, NULL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Change back to root directory.
  //
  FTPC_APP_LOG(("APP: Changing to parent directory."));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_CDUP, NULL);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Change to parent directory failed."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("APP: --- Test with new API using IP_FTPC_CMD_CONFIG that can operate regardless of current directory."));
  //
  // List directory content (before upload)
  //
  FTPC_APP_LOG(("APP: List directory (before upload, old API with path parameter):"));
  r = IP_FTPC_ExecCmd(&FTPConnection, FTPC_CMD_LIST, SERVER_ROOT_DIR SERVER_TEST_DIR);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Upload a file
  //
  FTPC_APP_LOG(("APP: Uploading \"%s\" to \"%s\" (new API).", UPLOAD_PATH_LOCAL, UPLOAD_PATH_REMOTE));
  Config.sLocalPath  = UPLOAD_PATH_LOCAL;
  Config.sRemotePath = UPLOAD_PATH_REMOTE;
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_STOR, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not upload data file."));
    goto Disconnect;
  }
  //
  // List directory content (after upload)
  //
  FTPC_APP_LOG(("APP: List directory (after upload, new API):"));
  Config.sRemotePath = SERVER_ROOT_DIR SERVER_TEST_DIR;
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_LIST, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Delete the file
  //
  FTPC_APP_LOG(("APP: Deleting \"%s\" (new API).", UPLOAD_PATH_REMOTE));
  Config.sRemotePath = UPLOAD_PATH_REMOTE;
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_DELE, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not delete data file."));
    goto Disconnect;
  }
  //
  // List directory content
  //
  FTPC_APP_LOG(("APP: List directory (after delete, new API):"));
  Config.sRemotePath = SERVER_ROOT_DIR SERVER_TEST_DIR;
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_LIST, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not list directory."));
    goto Disconnect;
  }
  FTPC_APP_LOG(("%s", acDataIn));
  //
  // Delete the test directory
  //
  FTPC_APP_LOG(("APP: Delete directory \"%s\" (new API).", SERVER_ROOT_DIR SERVER_TEST_DIR));
  Config.sRemotePath = SERVER_ROOT_DIR SERVER_TEST_DIR;
  r = IP_FTPC_ExecCmdEx(&FTPConnection, FTPC_CMD_RMD, &Config);
  if (r == FTPC_ERROR) {
    FTPC_APP_LOG(("APP: Could not delete directory."));
    goto Disconnect;
  }
  //
  // Disconnect.
  //
Disconnect:
  IP_FTPC_Disconnect(&FTPConnection);
  FTPC_APP_LOG(("APP: Done."));
  //
  while (1) {
    OS_Delay(500);
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
  OS_SetTaskName(OS_GetTaskID(), "FTP client");
#if (APP_USE_SSL != 0)
  //
  // Initialize SSL.
  //
  SSL_Init();
#endif
  _FTPClientTask();
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

#if (USE_FS_RO == 0)
  //
  // Create a test file on the storage medium
  //
  _FSTest();
#endif
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
