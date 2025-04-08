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

File        : IOT_JSON_MakeTree.c
Purpose     : Parse a JSON object and create an tree structure
              that represents the object.

Additional information:
  Preparations:
    None.

  Expected behavior:
    Parses a sample JSON object and prints it out on the console in a 
    tree format.
    For more information see the IOTToolkit manual under the
    chapter "JSON parser".

  Sample output:
    Parse OK, tree:

    {
      "name": "My TV", 
      "resolutions": 
        [
          {
            "width": 1280, 
            "height": 720
          }, 
          {
            "width": 1920, 
            "height": 1080
          }, 
          {
            "width": 3840, 
            "height": 2160
          }
        ]
    }

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT.h"
#include <stdlib.h>

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef enum {
  JSON_TYPE_OBJECT,
  JSON_TYPE_ARRAY,
  JSON_TYPE_PAIR,
  JSON_TYPE_STRING,
  JSON_TYPE_NUMBER,
  JSON_TYPE_TRUE,
  JSON_TYPE_FALSE,
  JSON_TYPE_NULL,
} JSON_TYPE;

typedef struct JSON_NODE JSON_NODE;
struct JSON_NODE {
  JSON_TYPE   Type;    // Type of this node
  JSON_NODE * pNext;   // Next node in list (linking arrays and object keys)
  JSON_NODE * pParent; // Pointer to node's parent
  JSON_NODE * pValue;  // Pointer to value (JSON_TYPE_KEY, JSON_TYPE_ARRAY, JSON_TYPE_OBJECT)
  char      * sStr;    // Only valid for JSON_TYPE_PAIR and JSON_TYPE_STRING
  double      Number;  // Only valid for JSON_TYPE_NUMBER
};

typedef struct {
  JSON_NODE *pContainer;
  JSON_NODE *pRoot;
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
*       _AppendNode()
*
*  Function description
*    Append node onto list.
*
*  Parameters
*    ppRoot - Pointer to list root.
*    pNode  - Pointer to node to append.
*/
static void _AppendNode(JSON_NODE **ppRoot, JSON_NODE *pNode) {
  if (*ppRoot == NULL) {
    *ppRoot = pNode;
  } else {
    while ((*ppRoot)->pNext != NULL) {
      ppRoot = &(*ppRoot)->pNext;
    }
    (*ppRoot)->pNext = pNode;
  }
}

/*********************************************************************
*
*       _MakeNode()
*
*  Function description
*    Create a new node, manage lists.
*
*  Parameters
*    Type     - Type of node to construct.
*    pContext - Pointer to use context.
*
*  Return value
*    Pointer to newly constructed node.
*/
static JSON_NODE *_MakeNode(JSON_TYPE Type, USER_CONTEXT *pContext) {
  JSON_NODE *pNode;
  //
  pNode = malloc(sizeof(JSON_NODE));
  memset(pNode, 0, sizeof(JSON_NODE));
  pNode->Type    = Type;
  pNode->pParent = pContext->pContainer;
  //
  if (pContext->pRoot == NULL) {
    pContext->pRoot = pNode;
  } else {
    _AppendNode(&pContext->pContainer->pValue, pNode);
  }
  //
  if (Type == JSON_TYPE_ARRAY || Type == JSON_TYPE_OBJECT || Type == JSON_TYPE_PAIR) {
    pContext->pContainer = pNode;
  } else if (pContext->pContainer->Type == JSON_TYPE_PAIR) {
    pContext->pContainer = pContext->pContainer->pParent;
  }
  //
  return pNode;
}

/*********************************************************************
*
*       _CloseNode()
*
*  Function description
*    Close a pair node or a container node.
*
*  Parameters
*    pUserContext - Pointer to user context.
*/
static void _CloseNode(USER_CONTEXT *pUserContext) {
  pUserContext->pContainer = pUserContext->pContainer->pParent;
  if (pUserContext->pContainer && pUserContext->pContainer->Type == JSON_TYPE_PAIR) {
    pUserContext->pContainer = pUserContext->pContainer->pParent;
  }
}

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
  _MakeNode(JSON_TYPE_OBJECT, pUserContext);
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
  _CloseNode(pUserContext);
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
  _MakeNode(JSON_TYPE_ARRAY, pUserContext);
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
  _CloseNode(pUserContext);
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
  JSON_NODE    *pNode;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  pNode = _MakeNode(JSON_TYPE_PAIR, pUserContext);
  pNode->sStr = malloc(strlen(sText)+1);
  strcpy(pNode->sStr, sText);
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
  JSON_NODE    *pNode;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  pNode = _MakeNode(JSON_TYPE_STRING, pUserContext);
  pNode->sStr = malloc(strlen(sText)+1);
  strcpy(pNode->sStr, sText);
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
  JSON_NODE    *pNode;
  //
  pUserContext = IOT_JSON_GetUserContext(pContext);
  pNode = _MakeNode(JSON_TYPE_NUMBER, pUserContext);
  sscanf(sText, "%lf", &pNode->Number);
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
  if (strcmp(sText, "true") == 0) {
    _MakeNode(JSON_TYPE_TRUE, pUserContext);
  } else if (strcmp(sText, "true") == 0) {
    _MakeNode(JSON_TYPE_FALSE, pUserContext);
  } else {
    _MakeNode(JSON_TYPE_NULL, pUserContext);
  }
}

/*********************************************************************
*
*       _PrintNode()
*
*  Function description
*    Print node.
*
*  Parameters
*    pNode  - Pointer to node to print.
*    Indent - Indentation level for node.
*/
static void _PrintNode(JSON_NODE *pNode, unsigned Indent) {
  for (; pNode; pNode = pNode->pNext) {
    switch (pNode->Type) {
    case JSON_TYPE_OBJECT:
      printf("\n%*s{", Indent, "");
      _PrintNode(pNode->pValue, Indent+2);
      printf("\n%*s}", Indent, "");
      break;
    case JSON_TYPE_ARRAY:
      printf("\n%*s[", Indent, "");
      _PrintNode(pNode->pValue, Indent+2);
      printf("\n%*s]", Indent, "");
      break;
    case JSON_TYPE_PAIR:
      printf("\n%*s\"%s\": ", Indent, "", pNode->sStr);
      _PrintNode(pNode->pValue, Indent+2);
      break;
    case JSON_TYPE_STRING:
      printf("\"%s\"", pNode->sStr);
      break;
    case JSON_TYPE_NUMBER:
      printf("%g", pNode->Number);
      break;
    case JSON_TYPE_TRUE:
      printf("true");
      break;
    case JSON_TYPE_FALSE:
      printf("false");
      break;
    case JSON_TYPE_NULL:
      printf("null");
      break;
    }
    if (pNode->pNext) {
      printf(", ");
    }
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
  IOT_JSON_CONTEXT JSON;
  USER_CONTEXT     User;
  char             acBuf[32];
  //
  User.pContainer = NULL;
  User.pRoot      = NULL;
  IOT_JSON_Init(&JSON, &_EventAPI, &acBuf[0], sizeof(acBuf));
  IOT_JSON_SetUserContext(&JSON, &User);
  if (IOT_JSON_Parse(&JSON, _sJSONValue, sizeof(_sJSONValue)-1) == 0) {
    printf("\nParse OK, tree:\n");
    _PrintNode(User.pRoot, 0);
    printf("\n");
  } else {
    printf("\nParse FAIL\n");
  }
}

/*************************** End of file ****************************/
