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

File        : IOT_HTTP_AuthGetRequest.c
Purpose     : Issue a GET request, to include authentication,
              over a plain socket.

Additional information:
  Preparations:
    Set up a server with a protected URI that requires Digest
    Authentication (RFC 7616).  A standard GET request is issued and,
    if authentication is required, the application follows up by
    answering the challenge with an authenticated GET request that
    encodes the response using the registered user credentials.
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT.h"
#include "CRYPTO.h"
#include "SEGGER.h"
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
  printf("%.*s", DataLen, (const char *)pData);
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
  CONNECTION_CONTEXT * pConn;
  int                  Status;
  //
  pConn = pVoid;
  Status = SEGGER_SYS_IP_Recv(pConn->Socket, pData, DataLen, 0);
  if (Status > 0) {
    printf("%.*s", Status, (const char *)pData);
  }
  return Status;
}

/*********************************************************************
*
*       _RememberAuthenticate()
*
*  Function description
*    Note any received authentication header.
*
*  Parameters
*    pContext - Pointer to HTTP context.
*    sKey     - Pointer to header key.
*    sValue   - Pointer to header data.
*    pUserCtx - Pointer to user-provided context.
*/
static void _RememberAuthenticate(      IOT_HTTP_CONTEXT * pContext,
                                  const char             * sKey,
                                  const char             * sValue,
                                        void             * pUserCtx) {
  if (SEGGER_strcasecmp(sKey, "WWW-Authenticate") == 0) {
    strcpy(SEGGER_PTR2PTR(char, pUserCtx), sValue);
  }
}

/*********************************************************************
*
*       _ParseQop()
*
*  Function description
*    Parse and set up quality of protection.
*
*  Parameters
*    pHTTP      - Pointer to HTTP context.
*    pAuthParas - Pointer to authentication parameters.
*
*  Return value
*    >= 0 - Quality of protection supported and set up.
*    <  0 - Quality of protection not supported.
*/
static int _ParseQop(IOT_HTTP_CONTEXT           * pHTTP,
                     IOT_HTTP_AUTH_DIGEST_PARAS * pAuthParas) {
  char * pQopAtom;
  int    CanRetry;
  //
  CanRetry = 0;
  for (pQopAtom = strtok(pAuthParas->aQop, ", ");
       pQopAtom != NULL;
       pQopAtom = strtok(pAuthParas->aQop, ", ")) {
    if (strcmp(pQopAtom, "auth") == 0) {
      if (strcmp(pAuthParas->aAlgorithm, "MD5") == 0) {
        CanRetry = 1;
        strcpy(pAuthParas->aQop, "auth");
        IOT_HTTP_SetAuth(pHTTP, IOT_HTTP_AUTH_DIGEST_MD5_WrHeader, pAuthParas);
        break;
      } else if (strcmp(pAuthParas->aAlgorithm, "SHA-256") == 0) {
        CanRetry = 1;
        strcpy(pAuthParas->aQop, "auth");
        IOT_HTTP_SetAuth(pHTTP, IOT_HTTP_AUTH_DIGEST_SHA256_WrHeader, pAuthParas);
        break;
      } else if (strcmp(pAuthParas->aAlgorithm, "SHA-512-256") == 0) {
        CanRetry = 1;
        strcpy(pAuthParas->aQop, "auth");
        IOT_HTTP_SetAuth(pHTTP, IOT_HTTP_AUTH_DIGEST_SHA512_256_WrHeader, pAuthParas);
        break;
      }
    }
  }
  //
  return CanRetry ? 0 : -1;
}

/*********************************************************************
*
*       _ChooseNewNonce()
*
*  Function description
*    Create a new client nonce.
*
*  Parameters
*    pAuthParas - Pointer to authentication parameters.
*/
static void _ChooseNewNonce(IOT_HTTP_AUTH_DIGEST_PARAS *pAuthParas) {
  unsigned i;
  //
  memset(pAuthParas->aCnonce, 0, sizeof(pAuthParas->aCnonce));
  CRYPTO_RNG_Get(pAuthParas->aCnonce, sizeof(pAuthParas->aCnonce)-1);
  for (i = 0; i < sizeof(pAuthParas->aCnonce) - 1; ++i) {
    pAuthParas->aCnonce[i] &= 0x07;
    pAuthParas->aCnonce[i] |= 0x30;
  }
}

/*********************************************************************
*
*       _Exec()
*
*  Function description
*    Execute HTTP request.
*
*  Parameters
*    pHTTP       - Pointer to HTTP context.
*    pAuthHeader - Pointer to object that receives any autorization
*                  header.
*
*  Return value
*    >= 0 - HTTP response code.
*    <  0 - Error executing request.
*/
static int _Exec(IOT_HTTP_CONTEXT *pHTTP, char *pAuthHeader) {
  unsigned StatusCode;
  int      Status;
  char     aPayload[128];
  //
  Status = IOT_HTTP_ExecWithAuth(pHTTP);
  if (Status < 0) {
    return Status;
  }
  //
  Status = IOT_HTTP_ProcessStatusLine(pHTTP, &StatusCode);
  if (Status < 0) {
    return Status;
  }
  //
  pAuthHeader[0] = 0;
  Status = IOT_HTTP_ProcessHeadersEx(pHTTP, _RememberAuthenticate, pAuthHeader);
  if (Status < 0) {
    return Status;
  }
  //
  IOT_HTTP_GetBegin(pHTTP);
  do {
    Status = IOT_HTTP_GetPayload(pHTTP, &aPayload[0], sizeof(aPayload));
  } while (Status > 0);
  IOT_HTTP_GetEnd(pHTTP);
  //
  return Status >= 0 ? StatusCode : Status;
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
  IOT_HTTP_AUTH_DIGEST_PARAS AuthParas;
  IOT_HTTP_CONTEXT           HTTP;
  IOT_HTTP_PARA              aPara[20];
  CONNECTION_CONTEXT         Connection;
  char                       aAuthHeader[256];
  char                       aBuf[256];
  char                       aURL[100];
  int                        Status;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  SEGGER_SYS_IP_Init();
  //
  strcpy(aURL, "http://apidemo.segger.com/digesttest/conf/index.htm");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_PlainAPI, &Connection);
  IOT_HTTP_SetVersion(&HTTP, &IOT_HTTP_VERSION_HTTP_1v1);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  //
  Status = IOT_HTTP_Connect(&HTTP);
  if (Status < 0) {
    SEGGER_SYS_IO_Printf("Cannot negotiate a connection to %s:%d!\n",
                         IOT_HTTP_GetHost(&HTTP),
                         IOT_HTTP_GetPort(&HTTP));
    SEGGER_SYS_OS_Halt(100);
  }
  //
  printf("\n\n*** INITIAL GET WITHOUT AUTHORIZATION ***\n\n");
  //
  memset(aAuthHeader, 0, sizeof(aAuthHeader));
  Status = _Exec(&HTTP, aAuthHeader);
  //
  // Authentication required?
  //
  if (Status == 401) {
    Status = -401;  // Say "Authentication not possible"
    if (SEGGER_strncasecmp(aAuthHeader, "Digest ", 7) == 0 && aAuthHeader[0] != 0) {
      IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
      Status = IOT_HTTP_AUTH_DIGEST_ParseParas(aAuthHeader+7, &AuthParas);
      if (Status >= 0) {
        Status = _ParseQop(&HTTP, &AuthParas);
      }
    }
  }
  //
  // Have we accepted the digest authentication scheme with supported parameters?
  //
  if (Status >= 0) {
    //
    // At this point, display realm and request user name, password.
    // This user name and password is hard coded for the SEGGER server.
    // We get this wrong.
    //
    printf("\n\n*** SECOND GET WITH INCORRECT AUTHORIZATION ***\n\n");
    //
    memset(aAuthHeader, 0, sizeof(aAuthHeader));
    _ChooseNewNonce(&AuthParas);
    strcpy(AuthParas.aUserName, "some-bad-dude");
    strcpy(AuthParas.aPassword, "open sesame");
    //
    Status = IOT_HTTP_AUTH_DIGEST_ParseParas(aAuthHeader+7, &AuthParas);
    if (Status >= 0) {
      Status = _ParseQop(&HTTP, &AuthParas);
    }
    Status = _Exec(&HTTP, aAuthHeader);
    //
    if (Status == 401 && 
        SEGGER_strncasecmp(aAuthHeader, "Digest ", 7) == 0 &&
        aAuthHeader[0] != 0) {
      //
      // OK, we expected this.  Try with correct parameters.
      //
      printf("\n\n*** THIRD GET WITH CORRECT AUTHORIZATION ***\n\n");
      //
      memset(aAuthHeader, 0, sizeof(aAuthHeader));
      _ChooseNewNonce(&AuthParas);
      strcpy(AuthParas.aUserName, "anybody");
      strcpy(AuthParas.aPassword, "segger");
      //
      Status = IOT_HTTP_AUTH_DIGEST_ParseParas(aAuthHeader+7, &AuthParas);
      if (Status >= 0) {
        Status = _ParseQop(&HTTP, &AuthParas);
      }
      Status = _Exec(&HTTP, aAuthHeader);
      //
      if (Status == 200) {
        printf("\nAuthenticated and received!\n");
      } else {
        printf("\nAuthentication failed!\n");
      }
    }
  }
  //
  SEGGER_SYS_IP_Exit();
  SEGGER_SYS_Exit();
  SEGGER_SYS_OS_Halt(Status);
}

/*************************** End of file ****************************/
