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

File        : IOT_JSON_PrettyTrace.c
Purpose     : Simple parse of a JSON object.

Additional information:
  Preparations:
    None.

  Expected behavior:
    Parses a sample JSON object and prints it out on the console in a
    tree-like format.
    For more information see the IOTToolkit manual under the
    chapter "JSON parser".

  Sample output:
    Begin object
      Key = "name"
      String = "My TV"
      Key = "resolutions"
      Begin array
        Begin object
          Key = "width"
          Number = "1280"
          Key = "height"
          Number = "720"
        End object
        Begin object
          Key = "width"
          Number = "1920"
          Key = "height"
          Number = "1080"
        End object
        Begin object
          Key = "width"
          Number = "3840"
          Key = "height"
          Number = "2160"
        End object
      End array
    End object

    Parse OK

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT.h"

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  unsigned Indent;
} USER_CONTEXT;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static void _BeginObject(IOT_JSON_CONTEXT *pContext);
static void _EndObject  (IOT_JSON_CONTEXT *pContext);
static void _BeginArray (IOT_JSON_CONTEXT *pContext);
static void _EndArray   (IOT_JSON_CONTEXT *pContext);
static void _Key        (IOT_JSON_CONTEXT *pContext, const char *sText);
static void _String     (IOT_JSON_CONTEXT *pContext, const char *sText);
static void _Number     (IOT_JSON_CONTEXT *pContext, const char *sText);
static void _Literal    (IOT_JSON_CONTEXT *pContext, const char *sText);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const IOT_JSON_EVENTS _EventAPI = {
  _BeginObject,
  _EndObject,
  _BeginArray,
  _EndArray,
  _Key,
  _String,
  _Number,
  _Literal
};

static const char _sJSONValue[] = {
  "{                                                   "
  "    \"name\": \"My TV\",                            "
  "    \"resolutions\": [                              "
  "        { \"width\": 1280, \"height\": 720 },       "
  "        { \"width\": 1920, \"height\": 1080 },      "
  "        { \"width\": 3840, \"height\": 2160 }       "
  "    ]                                               "
  "}                                                   "
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _BeginObject()
*
*  Function description
*    Handle Begin Object event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*/
static void _BeginObject(IOT_JSON_CONTEXT *pContext) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sBegin object\n", pUserContext->Indent, "");
  pUserContext->Indent += 2;
}

/*********************************************************************
*
*       _EndObject()
*
*  Function description
*    Handle End Object event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*/
static void _EndObject(IOT_JSON_CONTEXT *pContext) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  pUserContext->Indent -= 2;
  printf("%-*sEnd object\n", pUserContext->Indent, "");
}

/*********************************************************************
*
*       _BeginArray()
*
*  Function description
*    Handle Begin Array event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*/
static void _BeginArray(IOT_JSON_CONTEXT *pContext) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sBegin array\n", pUserContext->Indent, "");
  pUserContext->Indent += 2;
}

/*********************************************************************
*
*       _EndArray()
*
*  Function description
*    Handle End Array event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*/
static void _EndArray(IOT_JSON_CONTEXT *pContext) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  pUserContext->Indent -= 2;
  printf("%-*sEnd array\n", pUserContext->Indent, "");
}

/*********************************************************************
*
*       _Key()
*
*  Function description
*    Handle Key event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*    sText    - Zero-terminated key ID.
*/
static void _Key(IOT_JSON_CONTEXT *pContext, const char *sText) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sKey = \"%s\"\n", pUserContext->Indent, "", sText);
}

/*********************************************************************
*
*       _String()
*
*  Function description
*    Handle Key event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*    sText    - Zero-terminated string value.
*/
static void _String(IOT_JSON_CONTEXT *pContext, const char *sText) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sString = \"%s\"\n", pUserContext->Indent, "", sText);
}

/*********************************************************************
*
*       _Number()
*
*  Function description
*    Handle Number event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*    sText    - Zero-terminated numeric value.
*/
static void _Number(IOT_JSON_CONTEXT *pContext, const char *sText) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sNumber = \"%s\"\n", pUserContext->Indent, "", sText);
}

/*********************************************************************
*
*       _Literal()
*
*  Function description
*    Handle Literal event.
*
*  Parameters
*    pContext - Pointer to JSON context.
*    sText    - Zero-terminated literal name.
*/
static void _Literal(IOT_JSON_CONTEXT *pContext, const char *sText) {
  USER_CONTEXT *pUserContext;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  printf("%-*sLiteral = \"%s\"\n", pUserContext->Indent, "", sText);
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
  IOT_JSON_CONTEXT JSON;
  USER_CONTEXT     User;
  char             acBuf[32];
  //
  User.Indent = 0;
  IOT_JSON_Init(&JSON, &_EventAPI, &acBuf[0], sizeof(acBuf));
  IOT_JSON_SetUserContext(&JSON, &User);
  if (IOT_JSON_Parse(&JSON, _sJSONValue, sizeof(_sJSONValue)-1) == 0) {
    printf("\nParse OK\n");
  } else {
    printf("\nParse FAIL\n");
  }
}

/*************************** End of file ****************************/
