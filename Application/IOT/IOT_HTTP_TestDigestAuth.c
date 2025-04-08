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

File        : IOT_HTTP_TestDigestAuth.c
Purpose     : Run digest athentication examples from RFC 7616.
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

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _Capture(void *pVoid, const void *pData, unsigned DataLen);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

const char _RFC2617_MD5_aAuthHeader[]      = "realm=\"testrealm@host.com\","
                                             "qop=\"auth,auth-int\","
                                             "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\","
                                             "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
const char _RFC2617_MD5_aCnonce[]          = "0a4f113b";
const char _RFC2617_MD5_aExpectedResp[]    = "6629fae49393a05397450978507c4ef1";


const char _RFC7616_MD5_aAuthHeader[]      = "realm=\"http-auth@example.org\","
                                             "qop=\"auth, auth-int\","
                                             "algorithm=SHA-256,"
                                             "nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\","
                                             "opaque=\"FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS\"";
const char _RFC7616_MD5_aCnonce[]          = "f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ";
const char _RFC7616_MD5_aExpectedResp[]    = "8ca523f5e9506fed4657c9700eebdbec";


const char _RFC7616_SHA256_aAuthHeader[]   = "realm=\"http-auth@example.org\","
                                             "qop=\"auth, auth-int\","
                                             "algorithm=SHA-256,"
                                             "nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\","
                                             "opaque=\"FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS\"";
const char _RFC7616_SHA256_aCnonce[]       = "f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ";
const char _RFC7616_SHA256_aExpectedResp[] = "753927fa0e85d155564e2e272a28d1802ca10daf4496794697cf8db5856cb6c1";

const char _RFC7616_SHA512_aAuthHeader[]   = "realm=\"api@example.org\","
                                             "qop=\"auth\","
                                             "algorithm=SHA-512-256,"
                                             "nonce=\"5TsQWLVdgBdmrQ0XsxbDODV+57QdFR34I9HAbC/RVvkK\","
                                             "opaque=\"HRPCssKJSGjCrkzDg8OhwpzCiGPChXYjwrI2QmXDnsOS\","
                                             "charset=UTF-8,"
                                             "userhash=true";
const char _RFC7616_SHA512_aCnonce[]       = "NTg6RKcb9boFIAS3KrFK9BGeh+iDa/sm6jUMp2wds69v";
const char _RFC7616_RFC7616_SHA512_aExpectedResp[] = "ae66e67d6b427bd3f120414a82e4acff38e8ecd9101d6c861229025f607a79dd";
const char _RFC7616_RFC7616_Errata_SHA512_aExpectedResp[] = "3798d4131c277846293534c3edc11bd8a5e4cdcbff78b05db9d95eeb1cec68a5";

static const IOT_IO_API _SimpleIO = {
  NULL,
  NULL,
  _Capture,
  NULL
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

char _aAuthLine[1024];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Capture()
*
*  Function description
*    Capture data sent for authentication.
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
static int _Capture(void *pVoid, const void *pData, unsigned DataLen) {
  unsigned i;
  //
  i = strlen(_aAuthLine);
  memcpy(&_aAuthLine[i], pData, DataLen);
  _aAuthLine[i + DataLen] = 0;
  return 0;
}

/*********************************************************************
*
*       _CheckResult()
*
*  Function description
*    Test response against known answer.
*
*  Parameters
*    sExpectedResponse - Pointer to zero-terminated expected
*                        response string.
*/
static void _CheckResult(const char *sExpectedResponse) {
  char *pResp;
  char *pEnd;
  //
  pResp = strstr(_aAuthLine, "response=");
  if (pResp == NULL) {
    printf("No response field!\n");
    exit(100);
  }
  pResp += 9;
  if (*pResp == '"') {
    ++pResp;
  }
  pEnd = pResp;
  while (*pEnd && isalnum(*pEnd)) {
    ++pEnd;
  }
  *pEnd = 0;
  if (strcmp(pResp, sExpectedResponse) != 0) {
    printf("Generated response '%s' does not match expected response '%s'!\n",
           pResp, sExpectedResponse);
    exit(100);
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
*    Application entry point.
*/
void MainTask(void);
void MainTask(void) {
  IOT_HTTP_AUTH_DIGEST_PARAS AuthParas;
  IOT_HTTP_CONTEXT           HTTP;
  IOT_HTTP_PARA              aPara[20];
  char                       aBuf[256];
  char                       aURL[100];
  //
  CRYPTO_Init();
  //
  // From RFC 2617 section 3.5
  //
  strcpy(aURL, "http://www.nowhere.org/dir/index.html");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_SimpleIO, NULL);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
  IOT_HTTP_AUTH_DIGEST_ParseParas(_RFC2617_MD5_aAuthHeader, &AuthParas);
  strcpy(AuthParas.aUserName, "Mufasa");
  strcpy(AuthParas.aPassword, "Circle Of Life");
  strcpy(AuthParas.aCnonce,   _RFC2617_MD5_aCnonce);
  strcpy(AuthParas.aQop,      "auth");  // Select standard authentication
  IOT_HTTP_SetAuth(&HTTP, IOT_HTTP_AUTH_DIGEST_MD5_WrHeader, &AuthParas);
  _aAuthLine[0] = 0;
  HTTP.Auth.pfWrHeader(&HTTP);
  _CheckResult(_RFC2617_MD5_aExpectedResp);
  //
  // From RFC 7616 section 3.9.1
  //
  strcpy(aURL, "http://www.example.org/dir/index.html");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_SimpleIO, NULL);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
  IOT_HTTP_AUTH_DIGEST_ParseParas(_RFC7616_MD5_aAuthHeader, &AuthParas);
  strcpy(AuthParas.aUserName, "Mufasa");
  strcpy(AuthParas.aPassword, "Circle of Life");
  strcpy(AuthParas.aCnonce,   _RFC7616_MD5_aCnonce);
  strcpy(AuthParas.aQop,      "auth");  // Select standard authentication
  IOT_HTTP_SetAuth(&HTTP, IOT_HTTP_AUTH_DIGEST_MD5_WrHeader, &AuthParas);
  _aAuthLine[0] = 0;
  HTTP.Auth.pfWrHeader(&HTTP);
  _CheckResult(_RFC7616_MD5_aExpectedResp);
  //
  // From RFC 7616 section 3.9.1
  //
  strcpy(aURL, "http://www.example.org/dir/index.html");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_SimpleIO, NULL);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
  IOT_HTTP_AUTH_DIGEST_ParseParas(_RFC7616_SHA256_aAuthHeader, &AuthParas);
  strcpy(AuthParas.aUserName, "Mufasa");
  strcpy(AuthParas.aPassword, "Circle of Life");
  strcpy(AuthParas.aCnonce,   _RFC7616_SHA256_aCnonce);
  strcpy(AuthParas.aQop,      "auth");  // Select standard authentication
  IOT_HTTP_SetAuth(&HTTP, IOT_HTTP_AUTH_DIGEST_SHA256_WrHeader, &AuthParas);
  _aAuthLine[0] = 0;
  HTTP.Auth.pfWrHeader(&HTTP);
  _CheckResult(_RFC7616_SHA256_aExpectedResp);
  //
  // From RFC 7616 section 3.9.1 -- with broken SHA-512/256.
  //
  strcpy(aURL, "http://api.example.org/doe.json");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_SimpleIO, NULL);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
  IOT_HTTP_AUTH_DIGEST_ParseParas(_RFC7616_SHA512_aAuthHeader, &AuthParas);
  strcpy(AuthParas.aUserName, "J\xC3\xA4s\xC3\xB8n Doe");
  strcpy(AuthParas.aPassword, "Secret, or not?");
  strcpy(AuthParas.aCnonce,   _RFC7616_SHA512_aCnonce);
  strcpy(AuthParas.aQop,      "auth");  // Select standard authentication
  IOT_HTTP_SetAuth(&HTTP, IOT_HTTP_AUTH_DIGEST_RFC7616_SHA512_256_WrHeader, &AuthParas);
  _aAuthLine[0] = 0;
  HTTP.Auth.pfWrHeader(&HTTP);
  _CheckResult(_RFC7616_RFC7616_SHA512_aExpectedResp);
  //
  // From RFC 7616 section 3.9.1 -- with FIPS-compliant SHA-512/256.
  // From errata https://www.rfc-editor.org/errata_search.php?rfc=7616
  //
  strcpy(aURL, "http://api.example.org/doe.json");
  IOT_HTTP_Init      (&HTTP, &aBuf[0], sizeof(aBuf), aPara, SEGGER_COUNTOF(aPara));
  IOT_HTTP_SetIO     (&HTTP, &_SimpleIO, NULL);
  IOT_HTTP_AddMethod (&HTTP, "GET");
  IOT_HTTP_ParseURL  (&HTTP, aURL);
  IOT_HTTP_AUTH_DIGEST_InitParas(&AuthParas);
  IOT_HTTP_AUTH_DIGEST_ParseParas(_RFC7616_SHA512_aAuthHeader, &AuthParas);
  strcpy(AuthParas.aUserName, "J\xC3\xA4s\xC3\xB8n Doe");
  strcpy(AuthParas.aPassword, "Secret, or not?");
  strcpy(AuthParas.aCnonce,   _RFC7616_SHA512_aCnonce);
  strcpy(AuthParas.aQop,      "auth");  // Select standard authentication
  IOT_HTTP_SetAuth(&HTTP, IOT_HTTP_AUTH_DIGEST_SHA512_256_WrHeader, &AuthParas);
  _aAuthLine[0] = 0;
  HTTP.Auth.pfWrHeader(&HTTP);
  _CheckResult(_RFC7616_RFC7616_Errata_SHA512_aExpectedResp);
  //
  printf("All tests passed!\n");
}

/*************************** End of file ****************************/
