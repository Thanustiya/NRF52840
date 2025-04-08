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

File        : SSL_ROT13Client.c
Purpose     : Simple client that uses a secure ROT13 service.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SSL.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define ROT13_SERVER  "127.0.0.1"
#define ROT13_PORT    19000

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
*       Static code
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
static int _RdLine(SSL_SESSION *pSession, U8 *pData, unsigned DataLen) {
  unsigned Len;
  int      Status;
  U8       Char;
  //
  Len = 0;
  for (;;) {
    Status = SSL_SESSION_Receive(pSession, &Char, 1);  /*emDoc #1*/
    if (Status == 0) {  /*emDoc #2*/
      return SSL_ERROR_EOF;
    } else if (Status < 0) {  /*emDoc #3*/
      return Status;
    }
    pData[Len] = Char;  /*emDoc #4*/
    if (Len+1 < DataLen) {
      ++Len;
    }
    if (Char == '\n') {  /*emDoc #5*/
      pData[Len] = 0;
      return Len;
    }
  }
}

/*********************************************************************
*
*       _RequestROT13()
*
*  Function description
*    Apply ROT13 transform using ROT13 server.
*
*  Parameters
*    pSession - Pointer to SSL session.
*    pData    - Pointer to text to transform.
*/
static void _RequestROT13(SSL_SESSION *pSession, const char *pData) {
  U8  aResponse[256];
  int Status;
  //
  // Send data to server.
  //
  Status = SSL_SESSION_SendStr(pSession, pData);
  if (Status >= 0) {
    SEGGER_SYS_IO_Printf("Sent: %s", pData);
    Status = _RdLine(pSession, aResponse, sizeof(aResponse));
    if (Status >= 0) {
      SEGGER_SYS_IO_Printf("Recv: %s", aResponse);
    } else {
      SEGGER_SYS_IO_Printf("Error receiving data: %s\n", SSL_ERROR_GetText(Status));
    }
  } else {
    SEGGER_SYS_IO_Printf("Error sending data: %s\n", SSL_ERROR_GetText(Status));
  }
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
*    Ask ROT13 service to transform something.
*/
void MainTask(void);
void MainTask(void) {
  SSL_SESSION Session;
  int         Socket;
  //
  // Kick off networking and start SSL.
  //
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  SSL_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", SSL_GetCopyrightText());
  SEGGER_SYS_IO_Printf("emSSL ROT13 Client ");
  SEGGER_SYS_IO_Printf("compiled " __DATE__ " " __TIME__ "\n\n");
  //
  // Open a plain socket to the server.
  //
  Socket = SEGGER_SYS_IP_Open(ROT13_SERVER, ROT13_PORT);
  if (Socket < 0) {
    SEGGER_SYS_IO_Printf("Cannot open %s:%d!\n", ROT13_SERVER, ROT13_PORT);
    SEGGER_SYS_OS_Halt(100);
  }
  //
  // Upgrade the connection to secure by negotiating a
  // session using SSL.
  //
  SSL_SESSION_Prepare(&Session, Socket, &_IP_Transport);
  if (SSL_SESSION_Connect(&Session, ROT13_SERVER) < 0) {
    SEGGER_SYS_IO_Printf("Cannot negotiate a secure connection to %s:%d!\n",
                         ROT13_SERVER, ROT13_PORT);
    SEGGER_SYS_OS_Halt(100);
  }
  //
  // We have established a secure connection, so send the server
  // some data.
  //
  _RequestROT13(&Session, "SEGGER - The Embedded Experts\n");
  _RequestROT13(&Session, "FRTTRE - Gur Rzorqqrq Rkcregf\n");
  _RequestROT13(&Session, "SEGGER - It simply works!\n");
  _RequestROT13(&Session, "FRTTRE - Vg fvzcyl jbexf!\n");
  //
  // Close the SSL connection.
  //
  SSL_SESSION_Disconnect(&Session);
  SEGGER_SYS_IP_Close(Socket);
  //
  // Finish up.
  //
  SSL_Exit();
  SEGGER_SYS_IP_Exit();
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
