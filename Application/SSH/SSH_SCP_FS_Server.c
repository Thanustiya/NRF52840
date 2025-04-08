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
*       emSSH * Embedded Secure Shell                                *
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
*       emSSH version: V2.56.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSH_SCP_FS_Server.c
Purpose     : Simple SCP server that targets the embedded or null file
              system.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "SEGGER_SYS.h"
#include "SEGGER.h"
#ifndef WIN32
#include "FS.h"
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define BANNER \
  "\r\n"                                                              \
  "*************************************************************\r\n" \
  "* This server is powered by SEGGER emSSH.  It simply works! *\r\n" \
  "*************************************************************\r\n" \
  "\r\n"

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  int           Socket;
  SSH_SESSION * pSession;
} SSH_CONTEXT;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const SSH_TRANSPORT_API _IP_Transport = {
  SEGGER_SYS_IP_Send,
  SEGGER_SYS_IP_Recv,
  SEGGER_SYS_IP_Close,
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U32          _aRxBuffer[SSH_SCP_INITIAL_WINDOW_SIZE/4];
static U32          _aTxBuffer[SSH_SCP_INITIAL_WINDOW_SIZE/4];
static SSH_CONTEXT _aTask    [SSH_SCP_CONFIG_MAX_SESSIONS];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _PrintAppSignOn()
*
*  Function description
*    Displays the application's help information on stderr.
*/
static void _PrintAppSignOn(void) {
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("emSSH SCP Server V%s ", SSH_GetVersionText());
  SEGGER_SYS_IO_Printf("compiled " __DATE__ " " __TIME__ "\n");
  SEGGER_SYS_IO_Printf("(c) 2015-2019 SEGGER Microcontroller GmbH    www.segger.com\n\n");
}

/*********************************************************************
*
*       _UserauthServiceRequest()
*
*  Function description
*    Request the user authentication service.
*
*  Parameters
*    pSelf        - Pointer to session.
*    sServiceName - Service being requested.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*
*  Additional information
*    Displays a banner before user authentication commences.
*/
static int _UserauthServiceRequest(SSH_SESSION *pSelf, const char *sServiceName) {
  int Status;
  //
  Status = SSH_SESSION_SendServiceAccept(pSelf, sServiceName);
   if (Status >= 0) {
     Status = SSH_SESSION_SendUserauthBanner(pSelf, BANNER, "en");
   }
  //
  return Status;
}

/*********************************************************************
*
*       _UserauthRequestNone()
*
*  Function description
*    Request authentication of user with method "none".
*
*  Parameters
*    pSession  - Pointer to session.
*    pReqParas - Pointer to user authentication request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _UserauthRequestNone(SSH_SESSION                *pSession,
                                SSH_USERAUTH_REQUEST_PARAS *pReqParas) {
  SSH_USERAUTH_NONE_PARAS NoneParas;
  int                     Status;
  //
  SSH_USE_PARA(pSession);
  //
  Status = SSH_USERAUTH_NONE_ParseParas(pReqParas, &NoneParas);
  if (Status < 0) {
    Status = SSH_ERROR_USERAUTH_FAIL;
  } else if (pReqParas->UserNameLen == 4 &&
             SSH_MEMCMP(pReqParas->pUserName, "anon", 4) == 0) {
    Status = 0;
  } else {
    Status = SSH_ERROR_USERAUTH_FAIL;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _UserauthRequestPassword()
*
*  Function description
*    Request authentication of user with method "password".
*
*  Parameters
*    pSession  - Pointer to session.
*    pReqParas - Pointer to user authentication request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _UserauthRequestPassword(SSH_SESSION                *pSession,
                                    SSH_USERAUTH_REQUEST_PARAS *pReqParas) {
  SSH_USERAUTH_PASSWORD_PARAS PasswordParas;
  int                         Status;
  //
  SSH_USE_PARA(pSession);
  //
  Status = SSH_USERAUTH_PASSWORD_ParseParas(pReqParas, &PasswordParas);
  if (Status < 0) {
    Status = SSH_ERROR_USERAUTH_FAIL;
  } else if (pReqParas->UserNameLen == 5 && SSH_STRNCMP(pReqParas->pUserName, "admin", 5) == 0) {
    if (PasswordParas.PasswordLen == 6 && SSH_MEMCMP(PasswordParas.pPassword, "secret", 6) == 0) {
      Status = 0;
    } else {
      Status = SSH_ERROR_USERAUTH_FAIL;
    }
  } else {
    Status = SSH_ERROR_USERAUTH_FAIL;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _ExecRequest()
*
*  Function description
*    Request execution of an application.
*
*  Parameters
*    pSelf   - Pointer to session.
*    Channel - Zero-based channel index of request.
*    pParas  - Pointer to channel request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _ExecRequest(SSH_SESSION               * pSelf,
                        unsigned                    Channel,
                        SSH_CHANNEL_REQUEST_PARAS * pParas) {
  int Status;
  //
  Status = SSH_SCP_SINK_Accept(pSelf, Channel, pParas);
  if (Status >= 0) {
    if (pParas->WantReply) {
      Status = SSH_CHANNEL_SendSuccess(pSelf, Channel);
    }
    Status = SSH_SCP_SINK_Start(pSelf, Status);
  } else {
    Status = SSH_SCP_SOURCE_Accept(pSelf, Channel, pParas);
    if (pParas->WantReply) {
      Status = SSH_CHANNEL_SendCompletion(pSelf, Channel, Status);
    }
    if (Status >= 0) {
      Status = SSH_SCP_SOURCE_Start(pSelf, Status);
    }
  }
  //
  if (Status < 0) {
    SSH_CHANNEL_Close(pSelf, Channel);
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SSH_Buffering()
*
*  Function description
*    Handles SSH connection.
*
*  Parameters
*    pVoid - Pointer to user-provided context.
*/
static void _SSH_Buffering(SSH_CONTEXT * pContext) {
  int Status;
  //
  SSH_SESSION_Alloc(&pContext->pSession);
  if (pContext->pSession == NULL) {
    return;
  }
  //
  SSH_SESSION_Init(pContext->pSession, pContext->Socket, &_IP_Transport);
  SSH_SESSION_ConfBuffers(pContext->pSession,
                          _aRxBuffer, sizeof(_aRxBuffer),
                          _aTxBuffer, sizeof(_aTxBuffer));
  //
  do {
    Status = SSH_SESSION_Process(pContext->pSession);
  } while (Status >= 0);
  //
  SEGGER_SYS_IP_Close(pContext->Socket);
  pContext->Socket = 0;
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
  int BoundSocket;
  int Socket;
  int i;
  const char * sVolumeName = "";
  //
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  SSH_Init();
  //
  // Initialize SCP in sink and source mode.
  //
#ifdef WIN32
  SSH_SCP_SINK_Init  (&SSH_SCP_SINK_FS_Win32,   NULL);
  SSH_SCP_SOURCE_Init(&SSH_SCP_SOURCE_FS_Win32, NULL);
#else
  FS_Init();
  FS_FAT_SupportLFN();
  FS_FormatLLIfRequired(sVolumeName);
  if (FS_IsHLFormatted(sVolumeName) == 0) {
    FS_X_Log("High-level format\n");
    (void)FS_Format(sVolumeName, NULL);
  }
  FS_MkDir("\\SEGGER");
  //
  SSH_SCP_SINK_Init  (&SSH_SCP_SINK_FS_FS,   NULL);
  SSH_SCP_SOURCE_Init(&SSH_SCP_SOURCE_FS_FS, NULL);
#endif
  //
  // Hook the userauth service to show a banner.
  //
  SSH_SERVICE_Add(&SSH_SERVICE_USERAUTH, &_UserauthServiceRequest);
  //
  // Support None and Password user authentication methods.
  //
  SSH_USERAUTH_METHOD_Add(&SSH_USERAUTH_METHOD_NONE,     &_UserauthRequestNone);
  SSH_USERAUTH_METHOD_Add(&SSH_USERAUTH_METHOD_PASSWORD, &_UserauthRequestPassword);
  //
  // Add support for scp execution.
  //
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_EXEC, _ExecRequest);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_ENV,  NULL);
  //
  // Bind SSH port.
  //
  BoundSocket = SEGGER_SYS_IP_Bind(22);
  if (BoundSocket < 0) {
    SEGGER_SYS_OS_Halt(100);
  }
  _PrintAppSignOn();
  //
  for (;;) {
    //
    // Wait for an incoming connection.
    //
    SEGGER_SYS_IO_Printf("Awaiting connection...\n");
    Socket = SEGGER_SYS_IP_Accept(BoundSocket);
    if (Socket < 0) {
      continue;
    }
    //
    // Open an SSH connection.
    //
    SEGGER_SYS_IO_Printf("Connection made...\n");
    //
    // Create a task to handle the socket connections.
    //
    for (i = 0; i < (int)SEGGER_COUNTOF(_aTask); ++i) {
      if (_aTask[i].Socket == 0) {
        break;
      }
    }
    if (i < (int)SEGGER_COUNTOF(_aTask)) {
      _aTask[i].Socket = Socket;
      _SSH_Buffering(&_aTask[i]);
    } else {
      SEGGER_SYS_IP_Close(Socket);
    }
  }
}

/*************************** End of file ****************************/
