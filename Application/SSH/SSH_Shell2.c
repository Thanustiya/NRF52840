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

File        : SSH_Shell2.c
Purpose     : SSH server that supports a command line.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "SEGGER_SYS.h"
#include <string.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define PROMPT                                                   \
  "emSSH> "

#define SIGNON                                                   \
  "\r\n"                                                         \
  "Welcome to the emSSH command line!  Type Ctrl+D to exit.\r\n" \
  "\r\n"                                                         \
  PROMPT

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

void MainTask(void);
static int _TerminalChannelData(      SSH_SESSION * pSession,
                                      unsigned      Channel,
                                const U8          * pData,
                                      unsigned      DataLen);

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

static const SSH_CHANNEL_API _TerminalAPI = {
  _TerminalChannelData,
  NULL,
  NULL,
  NULL
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U8       _aRxTxBuffer[8192];
static U8       _aCommandLine[70];
static unsigned _Cursor;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _AppExit()
*
*  Function description
*    Exit the application with an error.
*
*  Parameters
*    sReason - Reason for exit, displayed for the user.
*/
static void _AppExit(const char *sReason) {
  SEGGER_SYS_IO_Printf(sReason);
  SEGGER_SYS_OS_Halt(100);
}

/*********************************************************************
*
*       _ShellRequest()
*
*  Function description
*    Handle a shell channel request.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel receiving the data.
*    pParas   - Pointer to channel request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _ShellRequest(SSH_SESSION               * pSession,
                         unsigned                    Channel,
                         SSH_CHANNEL_REQUEST_PARAS * pParas) {
  int Status;
  //
  Status = SSH_CHANNEL_SendData(pSession, Channel,
                                SIGNON, (unsigned)strlen(SIGNON));
  if (Status < 0) {
    Status = SSH_CHANNEL_SendFailure(pSession, Channel);
  } else if (pParas->WantReply) {
    Status = SSH_CHANNEL_SendSuccess(pSession, Channel);
  }
  //
  return Status;
}

/*********************************************************************
*
*       _TerminalChannelData()
*
*  Function description
*    Handle data received from peer.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel receiving the data.
*    pData    - Pointer to object that contains the data.
*    DataLen  - Octet length of the object that contains the data.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*
*  Additional information
*    Provide in-callback handling of a command line processor.
*    This sample supports only one connection at a time.
*/
static int _TerminalChannelData(      SSH_SESSION * pSession,
                                      unsigned      Channel,
                                const U8          * pData,
                                      unsigned      DataLen) {
  unsigned i;
  U8       Ch;
  int      Status;
  //
  Status = 0;
  //
  for (i = 0; Status >= 0 && i < DataLen; ++i) {
    Ch = pData[i];
    if (0x20 <= Ch && Ch <= 0x7E) {
      if (_Cursor < sizeof(_aCommandLine)) {
        _aCommandLine[_Cursor++] = Ch;
        Status = SSH_CHANNEL_SendData(pSession, Channel, &Ch, 1);
      }
    } else if (Ch == 0x08 || Ch == 0x7F) {
      if (_Cursor > 0) {
        --_Cursor;
        Status = SSH_CHANNEL_SendData(pSession, Channel, "\b \b", 3);
      }
    } else if (Ch == '\r') {
      SSH_CHANNEL_SendData(pSession, Channel, "\r\n...", 5);
      SSH_CHANNEL_SendData(pSession, Channel, _aCommandLine, _Cursor);
      SSH_CHANNEL_SendData(pSession, Channel, "\r\n", 3);
      Status = SSH_CHANNEL_SendData(pSession, Channel, PROMPT, (unsigned)strlen(PROMPT));
      _Cursor = 0;
    } else if (Ch == 0x04) {
      SSH_CHANNEL_SendData(pSession, Channel, "\r\n\r\nBye!\r\n\r\n", 12);
      SSH_CHANNEL_Close(pSession, Channel);
      break;
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       _TerminalRequest()
*
*  Function description
*    Request a terminal.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel requesting the terminal.
*    pParas   - Pointer to channel request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _TerminalRequest(SSH_SESSION               * pSession,
                            unsigned                    Channel,
                            SSH_CHANNEL_REQUEST_PARAS * pParas) {
  int Status;
  //
  SSH_CHANNEL_Config(pSession, Channel, 128, &_TerminalAPI, 0);
  if (pParas->WantReply) {
    Status = SSH_CHANNEL_SendSuccess(pSession, Channel);
  } else {
    Status = 0;
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
static int _UserauthRequestNone(SSH_SESSION                * pSession,
                                SSH_USERAUTH_REQUEST_PARAS * pReqParas) {
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
*             Public code
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
void MainTask(void) {
  SSH_SESSION * pSession;
  int           BoundSocket;
  int           Socket;
  int           Status;
  //
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  SSH_Init();
  //
  SEGGER_SYS_IO_Printf("\nemSSH V%s - Shell2 compiled " __DATE__ " " __TIME__ "\n",
                       SSH_GetVersionText());
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n\n",
                       SSH_GetCopyrightText());
  //
  // Allow "none" user authentication.
  //
  SSH_SERVICE_Add(&SSH_SERVICE_USERAUTH, NULL);
  SSH_USERAUTH_METHOD_Add(&SSH_USERAUTH_METHOD_NONE, _UserauthRequestNone);
  //
  // Add support for interactive shells.
  //
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_SHELL,         _ShellRequest);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_ENV,           NULL);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_PTYREQ,        _TerminalRequest);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_WINDOW_CHANGE, NULL);
  //
  // Bind SSH port.
  //
  BoundSocket = SEGGER_SYS_IP_Bind(22);
  if (BoundSocket < 0) {
    _AppExit("Cannot bind port 22!");
  }
  //
  for (;;) {
    //
    // Wait for an incoming connection.
    //
    do {
      Socket = SEGGER_SYS_IP_Accept(BoundSocket);
    } while (Socket < 0);
    //
    SSH_SESSION_Alloc(&pSession);
    if (pSession == 0) {
      _AppExit("No available session!");
    }
    //
    SSH_SESSION_Init(pSession, Socket, &_IP_Transport);
    SSH_SESSION_ConfBuffers(pSession,
                            _aRxTxBuffer, sizeof(_aRxTxBuffer),
                            _aRxTxBuffer, sizeof(_aRxTxBuffer));
    do {
      Status = SSH_SESSION_Process(pSession);
    } while (Status >= 0);
  }
}

/*************************** End of file ****************************/
