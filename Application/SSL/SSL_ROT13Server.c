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
*       emSSL * Embedded Transport Layer Security                    *
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
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSL_ROT13Server.c
Purpose     : Simple server that provides a secure ROT13 service.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSL.h"
#include "SEGGER_SYS.h"
#include <stdio.h>
#include <stdlib.h>

/*********************************************************************
*
*             Static const data
*
**********************************************************************
*/

static const SSL_TRANSPORT_API _IP_Transport = {
  SEGGER_SYS_IP_Send,
  SEGGER_SYS_IP_Recv,
  NULL,
  NULL
};

/*********************************************************************
*
*             Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _RdLine()
*
*  Function description
*    Read text line terminated by newline.
*
*  Parameters
*    pSession - Pointer to SSL session.
*    pData    - Pointer to object that receives the data.
*    DataLen  - Octet length of the receiving object.
*
*  Return value
*    >= 0 - Success, number of octets received including newline.
*     < 0 - Failure.
*/
static int _RdLine(SSL_SESSION *pSession, char *pData, unsigned DataLen) {
  unsigned Len;
  int      Status;
  U8       Char;
  //
  Len = 0;
  for (;;) {
    Status = SSL_SESSION_Receive(pSession, &Char, 1);
    if (Status == 0) {
      return SSL_ERROR_EOF;
    } else if (Status < 0) {
      return Status;
    }
    pData[Len] = Char;
    if (Len+1 < DataLen) {
      ++Len;
    }
    if (Char == '\n') {
      pData[Len] = 0;
      return Len;
    }
  }
}

/*********************************************************************
*
*       _ApplyROT13()
*
*  Function description
*    Apply ROT13 transform.
*
*  Parameters
*    pData    - Pointer to object to transform.
*    DataLen  - Octet length of the object to transform.
*/
static void _ApplyROT13(char *pData, unsigned DataLen) {
  unsigned i;
  //
  for (i = 0; i < DataLen; ++i) {
    if ('a' <= pData[i] && pData[i] <= 'm') {
      pData[i] = pData[i] - 'a' + 'n';
    } else if ('n' <= pData[i] && pData[i] <= 'z') {
      pData[i] = pData[i] - 'n' + 'a';
    } else if ('A' <= pData[i] && pData[i] <= 'M') {
      pData[i] = pData[i] - 'A' + 'N';
    } else if ('N' <= pData[i] && pData[i] <= 'Z') {
      pData[i] = pData[i] - 'N' + 'A';
    } 
  }
}

/*********************************************************************
*
*       _Serve()
*
*  Function description
*    Process a single ROT13 line.
*
*  Parameters
*    pSession - Pointer to SSL session.
*
*  Return value
*    >= 0 - Success, number of characters read, session remains open.
*    <  0 - Session closed.
*/
static int _Serve(SSL_SESSION *pSession) {
  char aData[256];
  int  Status;
  //
  Status = _RdLine(pSession, aData, sizeof(aData));
  if (Status >= 0) {
    SEGGER_SYS_IO_Printf("Recv: %s", aData);
    _ApplyROT13(aData, Status);
    Status = SSL_SESSION_Send(pSession, &aData[0], Status);
    if (Status >= 0) {
      SEGGER_SYS_IO_Printf("Sent: %s", aData);
    } else {
      SEGGER_SYS_IO_Printf("Error sending data: %s\n", SSL_ERROR_GetText(Status));
    }
  } else {
    if (Status != SSL_ERROR_EOF) {
      SEGGER_SYS_IO_Printf("Error receiving data: %s\n", SSL_ERROR_GetText(Status));
    }
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
void MainTask(void);
void MainTask(void) {
  SSL_SESSION Session;
  int         BoundSocket;
  int         Socket;
  int         Status;
  //
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  SSL_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", SSL_GetCopyrightText());
  SEGGER_SYS_IO_Printf("emSSL ROT13 Server ");
  SEGGER_SYS_IO_Printf("compiled " __DATE__ " " __TIME__ "\n\n");
  //
  // Bind application's ROT13 port.
  //
  BoundSocket = SEGGER_SYS_IP_Bind(19000);
  if (BoundSocket < 0) {
    SEGGER_SYS_OS_Halt(100);
  }
  //
  for (;;) {
    //
    do {
      Socket = SEGGER_SYS_IP_Accept(BoundSocket);
    } while (Socket < 0);
    //
    SSL_SESSION_Prepare(&Session, Socket, &_IP_Transport);
    Status = SSL_SESSION_Accept(&Session);
    //
    if (Status < 0) {
      SEGGER_SYS_IO_Printf("Can't negotiate a secure connection.\n\n");
      SEGGER_SYS_IP_Close(Socket);
    } else {
      do {
        Status = _Serve(&Session);
      } while (Status >= 0);
      SSL_SESSION_Disconnect(&Session);
      SEGGER_SYS_IP_CloseWait(Socket);
    }
  }
}

/*************************** End of file ****************************/
