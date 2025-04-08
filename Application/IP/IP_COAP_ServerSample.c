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

Purpose : Sample program for embOS & emNet demonstrating a simple
          CoAP server.

Additional information:
  This sample comes preconfigured to handle 2 clients
  and 5 observers at once and works out-of-the-box.

  Preparation:
    To test the server you can use the emNet CoAP client sample
    or the "coap-client" application on Linux.
    The following defines can be changed to configure the server:
      IP_COAP_MAX_NUM_CLIENT:  Maximum number of clients
      IP_COAP_MAX_NUM_OBS:     Maximum number of observers

  Example output:
    ...
    <using emNet CoAP client sample>
    8:261 MainTask - Getting with LATE reply.
    8:262 MainTask - Getting with LATE reply.
    8:263 MainTask - Getting Test.
    8:264 MainTask - Putting Test.
    8:264 MainTask - Putting Test.
    8:266 MainTask - Deleting Test.
    8:267 MainTask - Putting Test.
    8:268 MainTask - Putting Test.
    8:268 MainTask - Getting Observer.
    18:269 MainTask - Getting Observer.
    28:269 MainTask - Getting Observer.
    38:269 MainTask - Getting Observer.
    48:269 MainTask - Getting Observer.
    48:270 MainTask - Getting Observer.
    ...
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_COAP.h"
#include "SEGGER.h"
#include "SEGGER_UTIL.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK             0   // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150          // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK             // Must be the highest priority of all IP related tasks.
#endif
};

#define NUM_SERVER_DATA         11

#define POST_DATA_LENGTH        128

#define IP_COAP_MAX_NUM_CLIENT  2   // Maximum number of clients the server can handle at the same time.

#define IP_COAP_MAX_NUM_OBS     5   // Maximum number of observers the server can handle.

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

//
// CoAP contexts
//
static IP_COAP_SERVER_CONTEXT      _COAPServer;
static IP_COAP_CONN_INFO           _ConnInfo;
//
// Two times the MTU. The buffer is automatically split for receive and send data.
// U32 to make sure we have a word alignment.
//
static U32                         _MsgBuffer[(2 * 1500) / sizeof(U32)];
static IP_COAP_SERVER_DATA         _COAPServerData[NUM_SERVER_DATA];
static IP_COAP_SERVER_CLIENT_INFO  _COAPClientInfo[IP_COAP_MAX_NUM_CLIENT];
static IP_COAP_OBSERVER            _COAPObservers[IP_COAP_MAX_NUM_OBS];

//
// Application's data
//

//
// Meas 1 variable for GET (piggybacked) and PUT.
//
static I16 _Meas1_Temp    = 21;
//
// Average variable for GET (late reply).
//
static int _AverageTemp;
static int _AverageReady;
//
// Block transfer data. GET (piggybacked) and PUT.
//
static U8  _Data128[128 + 1]   = "<-----------Block 0----------->\n<-----------Block 1----------->\n<-----------Block 2----------->\n<-----------Block 3----------->\n";
static U8  _Data128Length  = 128;
//
// Block transfer data. GET (late reply).
//
static int _DelayedBlockReady;
static U8  _DelayedBlock[] = {"=======0========-------1--------_______2________########3#######::::::::4:::::::"};
//
// Observable data block. GET (piggybacked).
//
static U8  _Data64[64 + 1]     = "<-----------Block 0----------->\n<-----------Block 1----------->\n";
//
// Observable sensor. GET (piggybacked) + periodic update.
//
static U16 _ObsValue       = 17;
//
// POST example: 1 creation of maximum POST_DATA_LENGTH bytes.
//
static IP_COAP_SERVER_DATA _POSTServerData;
static char                _POSTDataPath[64];
static int                 _POSTDataValid;
static U8                  _POSTData[POST_DATA_LENGTH];
static U8                  _POSTDataLength;
//
// Separate.
//
static int                 _SeparateReady;

//
// Some shortcuts for ease of maintainability.
//
static IP_COAP_SERVER_DATA* _pSDTempAverage;
static IP_COAP_SERVER_DATA* _pSDDelayedBlock;
static IP_COAP_SERVER_DATA* _pSDObsData;
static IP_COAP_SERVER_DATA* _pSDSeparate;

static U32                  _RefTime;

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

static U32 _APP_GetTimeMs(void);

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


/*********************************************************************
*
*       Local functions - CoAP application example
*
**********************************************************************
*/

/*********************************************************************
*
*       _Temperature_Meas1_GetPayload
*
*  Immediate reply (GET piggybacked) no block handling.
*  Text or binary format.
*/
static int _Temperature_Meas1_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  int r;
  U16 Format;

  (void)pContext;

  pBuffer = *ppBuffer;
  Length  = *pLength;

  IP_COAP_APP_LOG(("Getting Temperature Measurement 1."));
  //
  //
  // Check Accept option.
  //
  if (IP_COAP_GetAcceptFormat(pParam, &Format) != IP_COAP_RETURN_OK) {
    //
    // Accept is not defined, use plain/text as default.
    //
    Format = IP_COAP_CT_TXT;
  }
  //
  if (Format == IP_COAP_CT_TXT) {
    r          = SEGGER_snprintf((char*)pBuffer, Length, "%d C", _Meas1_Temp);
    pBuffer   += r;
    Length    -= r;
  } else if (Format == IP_COAP_CT_OCTET_STREAM) {
    SEGGER_WrU16BE(pBuffer, (U16)_Meas1_Temp);
    *pBuffer += 2;
    Length   -= 2;
  } else {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Accept either text/plain or octect-stream");
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _Temperature_Meas1_PutPayload
*
*  Handles a PUT. Text or binary format.
*/
static int _Temperature_Meas1_PutPayload(IP_COAP_SERVER_CONTEXT* pContext, U8* pPayload, U16 Length, IP_COAP_CALLBACK_PARAM* pParam) {
  int r;
  int i;
  U8  c;
  int Sign;
  U16 Format;

  IP_COAP_APP_LOG(("Putting Temperature Measurement 1."));
  if (Length == 0) {
    return IP_COAP_RETURN_ERR;
  }
  if (pParam->pBlock != NULL) {
    if (pParam->pBlock->Index > 0) {
      IP_COAP_SERVER_SetErrorDescription(pContext, "Block transfer not supported");
      return IP_COAP_RETURN_ERR;
    }
  }
  //
  // Check if there is a content format specified.
  //
  if (IP_COAP_GetContentFormat(pParam, &Format) != IP_COAP_RETURN_OK) {
    //
    // Assume plain/text.
    //
    Format = IP_COAP_CT_TXT;
  }
  //
  // Format plain/text.
  //
  if (Format == IP_COAP_CT_TXT) {
    Sign = 1;
    if (*pPayload == '-') {
      Sign = -1;
      pPayload++;
      Length--;
    }
    while (*pPayload == ' ') {
      pPayload++;
      Length--;
      if (Length == 0) {
        break;
      }
    }
    r = 0;
    for (i = 0; i < Length; i++) {
      c = *pPayload++;
      if ((c < '0') && (c > '9')) {
        break;
      }
      r = (r * 10) + c - '0';
    }
    r *= Sign;
    if ((r < -65536) || (r > 65535)) {
      IP_COAP_SERVER_SetErrorDescription(pContext, "Takes a I16 value");
      return IP_COAP_RETURN_ERR;
    }
    _Meas1_Temp = (I16)r;
  //
  // Format binary.
  //
  } else if (Format == IP_COAP_CT_OCTET_STREAM) {
    //
    // Binary data, read the first 2 bytes.
    //
    _Meas1_Temp = 0;
    if (Length > 2) {
      IP_COAP_SERVER_SetErrorDescription(pContext, "Takes a I16 value");
      return IP_COAP_RETURN_ERR;
    }
    if (Length == 2) {
      _Meas1_Temp = SEGGER_RdU16BE(pPayload);
    } else {
      _Meas1_Temp = *pPayload;
    }
  } else {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Accept either text/plain or octect-stream");
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }

  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _Temperature_Meas2_GetPayload
*
*  Immediate reply no block handling. Octet stream.
*/
static int _Temperature_Meas2_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  U32 Temp;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Temperature Measurement 2."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_OCTET_STREAM) != IP_COAP_RETURN_OK) {
    IP_COAP_SERVER_SetErrorDescription(pContext, "octet-stream");
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }

  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  Temp = 19;
  SEGGER_WrU32BE(pBuffer, Temp);
  pBuffer += 4;
  Length  -= 4;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _Temperature_Average_GetPayload
*
*  Delayed reply no block handling. Text format.
*/
static int _Temperature_Average_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  int r;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Temperature Average."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_TXT) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }

  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  if (_AverageReady == 0) {
    //
    // Do the processing to get the data.
    // Ask to send an ACK in the meantime.
    //
    _AverageTemp = (19 + _Meas1_Temp) / 2;
    _AverageReady = 1;
    return IP_COAP_RETURN_SEND_SEPARATE;
  }
  //
  // Result is ready.
  //
  r = SEGGER_snprintf((char*)pBuffer, Length, "Average: %d", _AverageTemp);
  pBuffer += r;
  Length  -= r;
  //
  // Set it back to 0 for next attempt.
  //
  _AverageReady = 0;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _DelayedBlock_GetPayload
*
*  Late reply with block handling. Text format.
*/
static int _DelayedBlock_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8*                 pBuffer;
  U16                 Length;
  int                 Offset;
  int                 Len;
  int                 r;
  IP_COAP_BLOCK_INFO* pBlock;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Delayed Block."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_TXT) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  pBuffer = *ppBuffer;
  Length  = *pLength;
  pBlock  = pParam->pBlock;
  //
  if (_DelayedBlockReady == 0) {
    _DelayedBlockReady = 1;  // Simulation of processing time.
    return IP_COAP_RETURN_SEND_SEPARATE;
  }
  //
  Offset = pBlock->Index * pBlock->Size;
  Len    = IP_COAP_STRLEN((char*)_DelayedBlock);
  if (Len - Offset > pBlock->Size) {
    Len  = pBlock->Size;
    r    = IP_COAP_RETURN_SEND_BLOCK;
  } else {
    Len -= Offset;
    r    = IP_COAP_RETURN_SEND_END;
    //
    // Re-arm the delayed ack simulation on the last block.
    //
    _DelayedBlockReady = 0;
  }
  IP_COAP_MEMCPY(pBuffer, _DelayedBlock + Offset, Len);
  pBuffer += Len;
  Length  -= Len;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;
  //

  return r;
}

/*********************************************************************
*
*       _DataSupporting_GetPayload
*
*  Immediate reply no block handling. Octet stream.
*/
static int _DataSupporting_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  U32 Temp;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Data Supporting."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_OCTET_STREAM) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  Temp = 77;
  IP_COAP_MEMCPY(pBuffer, &Temp, 4);
  pBuffer += 4;
  Length  -= 4;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _DataSupporting_DelHandler
*
*  Indicate DELETE is authorized.
*/
static int _DataSupporting_DelHandler(IP_COAP_SERVER_CONTEXT* pContext, IP_COAP_CALLBACK_PARAM* pParam) {

  (void)pContext;
  (void)pParam;
  //
  // Always authorized.
  //
  return IP_COAP_RETURN_OK;
}


/*********************************************************************
*
*       _Data128bytes_GetPayload
*
*  Block handling of textual data.
*/
static int _Data128bytes_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  int Offset;
  int Len;
  int r;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Data."));
  //
  // If the content format option is present, check this is a plain/text.
  //
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_TXT) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  pBuffer = *ppBuffer;
  Length  = *pLength;
  r       = IP_COAP_RETURN_SEND_BLOCK;
  Len     = Length;
  //
  if (pParam->pBlock != NULL) {
    Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  } else {
    Offset = 0;
  }
  //
  if (Offset >= _Data128Length) {
    return IP_COAP_RETURN_NO_PAYLOAD;
  }
  if ((Offset + Len) >= _Data128Length) {
    Len = _Data128Length - Offset;
    r   = IP_COAP_RETURN_SEND_END;
  }
  IP_COAP_MEMCPY(pBuffer, _Data128 + Offset, Len);
  pBuffer += Len;
  Length  -= Len;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return r;
}

/*********************************************************************
*
*       _Data128bytes_PutPayload
*
*  Handles a PUT. Text format.
*/
static int _Data128bytes_PutPayload(IP_COAP_SERVER_CONTEXT* pContext, U8* pPayload, U16 Length, IP_COAP_CALLBACK_PARAM* pParam) {
  int Offset;

  IP_COAP_APP_LOG(("Putting Data."));
  //
  // Request that content format is present and of type plain/text.
  //
  if (IP_COAP_CheckContentFormat(pParam, IP_COAP_CT_TXT, 1) != IP_COAP_RETURN_OK) {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Content Format plain/text requested");
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  if (pParam->pBlock != NULL) {
    Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  } else {
    Offset = 0;
  }
  //
  if (Offset >= 128) {
    return IP_COAP_RETURN_BUFFER_TOO_SMALL;
  }
  if ((Offset + Length) > 128) {
    return IP_COAP_RETURN_BUFFER_TOO_SMALL;
  }
  IP_COAP_MEMCPY(_Data128 + Offset, pPayload, Length);
  if (IP_COAP_IsLastBlock(pParam, IP_COAP_CODE_REQ_PUT) != 0) {
    _Data128Length = Offset + Length;
  }

  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _POSTCreateEntry_DelHandler
*
*  DELETE function for the created POST entry.
*/
static int _POSTCreateEntry_DelHandler(IP_COAP_SERVER_CONTEXT* pContext, IP_COAP_CALLBACK_PARAM* pParam) {

  (void)pContext;
  (void)pParam;
  //
  // Always authorized.
  //
  _POSTDataValid  = 0;
  _POSTDataLength = 0;
  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _POSTCreateEntry_GetPayload
*
*  GET function for the created POST entry.
*/
static int _POSTCreateEntry_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  int Offset;
  int Len;
  int r;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting POST entry."));
  pBuffer = *ppBuffer;
  Length  = *pLength;
  r       = IP_COAP_RETURN_SEND_BLOCK;
  Len     = Length;
  //
  if (pParam->pBlock != NULL) {
    Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  } else {
    Offset = 0;
  }
  //
  if (Offset >= _POSTDataLength) {
    return IP_COAP_RETURN_NO_PAYLOAD;
  }
  if ((Offset + Len) >= _POSTDataLength) {
    Len = _POSTDataLength - Offset;
    r   = IP_COAP_RETURN_SEND_END;
  }
  IP_COAP_MEMCPY(pBuffer, _POSTData + Offset, Len);
  pBuffer += Len;
  Length  -= Len;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return r;
}

/*********************************************************************
*
*       _POSTCreateEntry_PutPayload
*
*  PUT function for the created POST entry.
*/
static int _POSTCreateEntry_PutPayload(IP_COAP_SERVER_CONTEXT* pContext, U8* pPayload, U16 Length, IP_COAP_CALLBACK_PARAM* pParam) {
  int Offset;
  int Len;

  (void)pContext;
  IP_COAP_APP_LOG(("Putting POST entry."));
  //
  Len = Length;
  if (pParam->pBlock != NULL) {
    Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  } else {
    Offset = 0;
  }
  //
  if ((Offset >= POST_DATA_LENGTH) ||
      ((Offset + Len) > POST_DATA_LENGTH)) {
    //
    // If this is an error on the POST for creation (_POSTDataLength == 0)
    // the ServerData will be removed.
    //
    if (_POSTDataLength == 0) {
      _POSTDataValid = 0;
    }
    return IP_COAP_RETURN_BUFFER_TOO_SMALL;
  }
  //
  IP_COAP_MEMCPY(_POSTData + Offset, pPayload, Len);
  if (IP_COAP_IsLastBlock(pParam, IP_COAP_CODE_REQ_PUT) != 0) {
    _POSTDataLength = Offset + Len;
  }

  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _POSTCreateEntry
*
*  Create new entry when called with POST.
*/
static int _POSTCreateEntry(IP_COAP_SERVER_CONTEXT* pContext, IP_COAP_CALLBACK_PARAM* pParam, U32 PayloadLength, IP_COAP_SERVER_DATA** ppServerData) {
  U8* pURI;
  U8  Length;

  //
  // Only one POST data could be reated.
  //
  if (_POSTDataValid != 0) {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Maximum 1 POST");
    return IP_COAP_RETURN_NOT_ALLOWED;
  }
  //
  if (PayloadLength > POST_DATA_LENGTH) {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Maximum size is 128 bytes");
    return IP_COAP_RETURN_BUFFER_TOO_SMALL;
  }
  //
  if (pParam->pOptDesc->URILength >= POST_DATA_LENGTH) {
    IP_COAP_SERVER_SetErrorDescription(pContext, "Maximum URI path is 64 long");
    return IP_COAP_RETURN_BUFFER_TOO_SMALL;
  }
  if (IP_COAP_GetURIPath(pParam, &pURI, &Length) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_ERR;
  }
  //
  // Create the new data.
  //
  _POSTDataValid = 1;
  IP_COAP_MEMCPY(&_POSTDataPath[0], pURI, Length);
  _POSTDataPath[Length]        = 0;
  IP_COAP_MEMSET(&_POSTServerData, 0, sizeof(_POSTServerData));
  _POSTServerData.sURI         = &_POSTDataPath[0];
  _POSTServerData.sDescription = "title=\"User created entry\"";
  _POSTServerData.pfGETPayload = _POSTCreateEntry_GetPayload;
  _POSTServerData.pfPUTPayload = _POSTCreateEntry_PutPayload;
  _POSTServerData.pfDELHandler = _POSTCreateEntry_DelHandler;
  //
  *ppServerData = &_POSTServerData;

  return IP_COAP_RETURN_OK;
}


/*********************************************************************
*
*       _ObsSensor_GetPayload
*
*  Immediate reply no block handling. Octect stream.
*/
static int _ObsSensor_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Observer Sensor."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_OCTET_STREAM) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }

  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  SEGGER_WrU16BE(pBuffer, _ObsValue);
  pBuffer += 2;
  Length  -= 2;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}


/*********************************************************************
*
*       _ObsData_GetPayload
*
*  Block handling of textual data.
*/
static int _ObsData_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8* pBuffer;
  U16 Length;
  int Offset;
  int Len;
  int r;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting Observer Data."));
  //
  // If the content format option is present, check this is a plain/text.
  //
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_TXT) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }
  //
  pBuffer = *ppBuffer;
  Length  = *pLength;
  r       = IP_COAP_RETURN_SEND_BLOCK;
  Len     = Length;
  //
  if (pParam->pBlock != NULL) {
    Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  } else {
    Offset = 0;
  }
  //
  if (Offset >= 64) {
    return IP_COAP_RETURN_NO_PAYLOAD;
  }
  if ((Offset + Len) >= 64) {
    Len = 64 - Offset;
    r   = IP_COAP_RETURN_SEND_END;
  }
  IP_COAP_MEMCPY(pBuffer, _Data64 + Offset, Len);
  pBuffer += Len;
  Length  -= Len;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return r;
}

/*********************************************************************
*
*       _Separate_GetPayload
*
*  Delayed reply no block handling. Text format.
*/
static int _Separate_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8*  pBuffer;
  U16  Length;
  int  r;

  (void)pContext;

  IP_COAP_APP_LOG(("Getting with LATE reply."));
  if (IP_COAP_CheckAcceptFormat(pParam, IP_COAP_CT_TXT) != IP_COAP_RETURN_OK) {
    return IP_COAP_RETURN_CT_FORMAT_ERROR;
  }

  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  if (_SeparateReady == 0) {
    //
    // Ask to send an ACK in the meantime.
    //
    _SeparateReady = 1;
    return IP_COAP_RETURN_SEND_SEPARATE;
  }
  //
  // Result is ready.
  //
  r = SEGGER_snprintf((char*)pBuffer, Length, "00:00:00");
  pBuffer += r;
  Length  -= r;
  //
  // Set it back to 0 for next attempt.
  //
  _SeparateReady = 0;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _Test_GetPayload
*
*  immediate reply.
*/
static int _Test_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8*  pBuffer;
  U16  Length;
  int  r;

  (void)pContext;
  (void)pParam;

  IP_COAP_APP_LOG(("Getting Test."));
  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  r = SEGGER_snprintf((char*)pBuffer, Length, "Test data");
  pBuffer += r;
  Length  -= r;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _Test_PutPayload
*
*  PUT function for the test entry.
*/
static int _Test_PutPayload(IP_COAP_SERVER_CONTEXT* pContext, U8* pPayload, U16 Length, IP_COAP_CALLBACK_PARAM* pParam) {

  (void)pContext;
  (void)pPayload;
  (void)Length;
  (void)pParam;
  IP_COAP_APP_LOG(("Putting Test."));

  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _Test_DelHandler
*
*  DELETE function for the test entry.
*/
static int _Test_DelHandler(IP_COAP_SERVER_CONTEXT* pContext, IP_COAP_CALLBACK_PARAM* pParam) {

  (void)pContext;
  (void)pParam;
  IP_COAP_APP_LOG(("Deleting Test."));
  //
  // Always authorized but don't actually delete the entry.
  //
  return IP_COAP_RETURN_OK_NO_DELETE;
}

/*********************************************************************
*
*       _Obs_GetPayload
*
*  immediate reply.
*/
static int _Obs_GetPayload(IP_COAP_SERVER_CONTEXT* pContext, U8** ppBuffer, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  U8*  pBuffer;
  U16  Length;
  int  r;

  (void)pContext;
  (void)pParam;

  IP_COAP_APP_LOG(("Getting Observer."));
  pBuffer = *ppBuffer;
  Length  = *pLength;
  //
  r = SEGGER_snprintf((char*)pBuffer, Length, "Obs data");
  pBuffer += r;
  Length  -= r;
  //
  // Set it back to 0 for next attempt.
  //
  _SeparateReady = 0;
  //
  *ppBuffer = pBuffer;
  *pLength  = Length;

  return IP_COAP_RETURN_SEND_END;
}

/*********************************************************************
*
*       _ConfigureServer()
*
*  Function description
*    Configuration of the CoAP server.
*/
static void _ConfigureServer(void) {
  IP_COAP_SERVER_DATA* pServerData;

  //
  // Add the clients and observers buffers.
  //
  IP_COAP_SERVER_AddClientBuffer(&_COAPServer, &_COAPClientInfo[0], IP_COAP_MAX_NUM_CLIENT);
  IP_COAP_SERVER_AddObserverBuffer(&_COAPServer, &_COAPObservers[0], IP_COAP_MAX_NUM_OBS);
  //
  // Add a handler to perform POST command.
  //
  IP_COAP_SERVER_SetPOSTHandler(&_COAPServer, _POSTCreateEntry);
  //
  // Configure a default block size.
  //
  IP_COAP_SERVER_SetDefaultBlockSize(&_COAPServer, 32);
  //
  // Disable some features (block / observe).
  //
//  IP_COAP_SERVER_ConfigSet(&_COAPServer, IP_COAP_CONFIG_DISABLE_BLOCKS);
//  IP_COAP_SERVER_ConfigSet(&_COAPServer, IP_COAP_CONFIG_DISABLE_OBSERVE);
  //
  // Configure the server data.
  //
  IP_COAP_MEMSET(&_COAPServerData[0], 0, sizeof(IP_COAP_SERVER_DATA) * NUM_SERVER_DATA);
  //
  // temperature/meas1:
  //   Example of a simple value supporting GET and PUT methods.
  //
  pServerData                    = &_COAPServerData[0];
  pServerData->sURI              = "temperature/meas1";
  pServerData->sDescription      = "ct=0;title=\"Textual or binary value. GET piggybacked, PUT\"";
  pServerData->pfGETPayload      = _Temperature_Meas1_GetPayload;
  pServerData->pfPUTPayload      = _Temperature_Meas1_PutPayload;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  IP_COAP_MEMCPY(&pServerData->ETag[0], "\x01\x02\x03\x04\x05\x06\x07\x08", 8);
  pServerData->ETagLength        = 8;
  pServerData->MaxAge            = 120;
  pServerData->Size2             = 8;
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_ETAG | IP_COAP_OPTMASK_CONTENT_FORMAT | IP_COAP_OPTMASK_MAX_AGE | IP_COAP_OPTMASK_SIZE2;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // temperature/meas2:
  //   Example of a simple value supporting only GET.
  //
  pServerData                    = &_COAPServerData[1];
  pServerData->sURI              = "temperature/meas2";
  pServerData->sDescription      = "title=\"Binary value (int). GET piggybacked\"";
  pServerData->pfGETPayload      = _Temperature_Meas2_GetPayload;
  pServerData->ContentFormat     = IP_COAP_CT_OCTET_STREAM;
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_CONTENT_FORMAT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // temperature/average:
  //   Example of a simple value with a late GET support (reply in a second message).
  //
  pServerData                    = &_COAPServerData[2];
  _pSDTempAverage                = pServerData;
  pServerData->sURI              = "temperature/average";
  pServerData->sDescription      = "title=\"Textual value. GET late reply.\"";
  pServerData->pfGETPayload      = _Temperature_Average_GetPayload;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // DataSupportingDelete:
  //   Example of the DELETE command. As ETag is used, possible to
  //   test the If-Match option.
  //
  pServerData                    = &_COAPServerData[3];
  pServerData->sURI              = "DataSupportingDelete";
  pServerData->sDescription      = "title=\"Binary value (int). GET piggybacked, Support DELETE\"";
  pServerData->pfGETPayload      = _DataSupporting_GetPayload;
  pServerData->pfDELHandler      = _DataSupporting_DelHandler;
  pServerData->ContentFormat     = IP_COAP_CT_OCTET_STREAM;
  pServerData->ETagLength        = 4;
  IP_COAP_MEMCPY(&pServerData->ETag[0], "\x45\x02\x21\x08", 4);
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // BlockTransfer/DelayedBlock:
  //   Example of a "big" data that requires block transfer.
  //   GET is handled with a late reply (reply in a second message).
  //
  pServerData                    = &_COAPServerData[4];
  _pSDDelayedBlock               = pServerData;
  pServerData->sURI              = "BlockTransfer/DelayedBlock";
  pServerData->sDescription      = "title=\"Textual value. GET late reply with block support.\"";
  pServerData->pfGETPayload      = _DelayedBlock_GetPayload;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // BlockTransfer/Data128bytes:
  //   Example of a "big" data that requires block transfer.
  //   GET and PUT are supported.
  //
  pServerData                    = &_COAPServerData[5];
  pServerData->sURI              = "BlockTransfer/Data128bytes";
  pServerData->sDescription      = "title=\"Textual value. GET and PUT, block support. Size 128 Bytes.\"";
  pServerData->pfGETPayload      = _Data128bytes_GetPayload;
  pServerData->pfPUTPayload      = _Data128bytes_PutPayload;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // ObsSensor:
  //   Example of an observable simple data.
  //   Generates a NON message at MaxAge expiry.
  //
  pServerData                    = &_COAPServerData[6];
  _pSDObsData                    = pServerData;
  pServerData->sURI              = "ObsSensor";
  pServerData->sDescription      = "title=\"Observable value.\"";
  pServerData->pfGETPayload      = _ObsSensor_GetPayload;
  pServerData->ObsConfig         = IP_COAP_OBS_OBSERVABLE | IP_COAP_OBS_AUTO_NON_ON_MAX_AGE;
  pServerData->ContentFormat     = IP_COAP_CT_OCTET_STREAM;
  pServerData->MaxAge            = 20;
  pServerData->ETagLength        = 2;
  IP_COAP_MEMCPY(&pServerData->ETag[0], "\x17\x18", 2);
  pServerData->DefGetOptMask = IP_COAP_OPTMASK_ETAG | IP_COAP_OPTMASK_CONTENT_FORMAT | IP_COAP_OPTMASK_MAX_AGE;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // BlockTransfer/ObsData:
  //   Example of an observable "big" data that requires block transfer.
  //   Generates a CON message at MaxAge expiry.
  //
  pServerData                    = &_COAPServerData[7];
  pServerData->sURI              = "BlockTransfer/ObsData";
  pServerData->sDescription      = "title=\"Observable data block.\"";
  pServerData->pfGETPayload      = _ObsData_GetPayload;
  pServerData->ObsConfig         = IP_COAP_OBS_OBSERVABLE | IP_COAP_OBS_AUTO_CON_ON_MAX_AGE;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  pServerData->MaxAge            = 15;
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_CONTENT_FORMAT | IP_COAP_OPTMASK_MAX_AGE;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // separate
  //   simple GET support with late reply (no piggyback).
  //
  pServerData                    = &_COAPServerData[8];
  _pSDSeparate                   = pServerData;
  pServerData->sURI              = "separate";
  pServerData->sDescription      = "title=\"simple variable with GET with LATE reply\"";
  pServerData->pfGETPayload      = _Separate_GetPayload;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_CONTENT_FORMAT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // test
  //   Used to test client: reply success to everything.
  //
  pServerData                    = &_COAPServerData[9];
  pServerData->sURI              = "test";
  pServerData->sDescription      = "title=\"test entry\"";
  pServerData->pfGETPayload      = _Test_GetPayload;
  pServerData->pfPUTPayload      = _Test_PutPayload;
  pServerData->pfDELHandler      = _Test_DelHandler;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  pServerData->ETagLength        = 2;
  IP_COAP_MEMCPY(&pServerData->ETag[0], "\x01\x02", 2);
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_ETAG | IP_COAP_OPTMASK_CONTENT_FORMAT;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
  //
  // obs:
  //   Example of a simple observable data.
  //   Generates a CON message at MaxAge expiry.
  //
  pServerData                    = &_COAPServerData[10];
  pServerData->sURI              = "obs";
  pServerData->sDescription      = "title=\"Simple observable data.\"";
  pServerData->pfGETPayload      = _Obs_GetPayload;
  pServerData->ObsConfig         = IP_COAP_OBS_OBSERVABLE | IP_COAP_OBS_AUTO_CON_ON_MAX_AGE;
  pServerData->ContentFormat     = IP_COAP_CT_TXT;
  pServerData->MaxAge            = 10;
  pServerData->DefGetOptMask     = IP_COAP_OPTMASK_CONTENT_FORMAT | IP_COAP_OPTMASK_MAX_AGE;
  //
  IP_COAP_SERVER_AddData(&_COAPServer, pServerData);
}

/*********************************************************************
*
*       _ApplicationProcessing()
*
*  Function description
*    Perform periodic processing of the CoAP server application.
*/
static void _ApplicationProcessing(void) {
  U32 Time;

  //
  // Application example: temperature/average late GET reply.
  // A GET was perform and the data is now ready.
  //
  if (_AverageReady != 0) {
    IP_COAP_SERVER_UpdateData(&_COAPServer, _pSDTempAverage, IP_COAP_TYPE_CON, 0);
  }
  //
  // Application example: BlockTransfer/DelayedBlock late GET reply.
  // A GET was perform and the data is now ready.
  //
  if (_DelayedBlockReady != 0) {
    IP_COAP_SERVER_UpdateData(&_COAPServer, _pSDDelayedBlock, IP_COAP_TYPE_CON, 0);
  }
  //
  // Application example: ObsSensor
  //   Indicate a change of its value periodically.
  //   Generates a new ETag.
  //
  Time = _APP_GetTimeMs();
  if ((Time - _RefTime) > 60000) {  // Warning: Rollover is not handle in this example.
    _RefTime = Time;
    _ObsValue++;
    IP_COAP_SERVER_UpdateData(&_COAPServer, _pSDObsData, IP_COAP_TYPE_CON, 1);
  }
  //
  // Application example: separate
  // Late reply to a GET.
  //
  if (_SeparateReady != 0) {
    IP_COAP_SERVER_UpdateData(&_COAPServer, _pSDSeparate, IP_COAP_TYPE_CON, 0);
  }
}

/*********************************************************************
*
*       Local functions - CoAP functionality.
*
**********************************************************************
*/

/*********************************************************************
*
*       _APP_Receive()
*
*  Function description
*    Receives a UDP packet.
*/
static int _APP_Receive(U8* pBuffer, unsigned BufferSize, IP_COAP_CONN_INFO* pInfo, unsigned* pIsMulticast) {
  fd_set              OriginalSocket;
  fd_set              OriginalStdin;
  fd_set              ReadFDS;
  fd_set              WriteFDS;
  int                 Receive;
  int                 SoError;
  unsigned            SocketFD;
  int                 AddressLength;
  struct sockaddr_in  ClientAddress;

  SocketFD = (int)pInfo->hSock;

  //
  // Clear the sets.
  //
  FD_ZERO(&OriginalSocket);
  FD_ZERO(&OriginalStdin);
  FD_ZERO(&ReadFDS);
  FD_ZERO(&WriteFDS);
  //
  // Add our descriptors to the set. (0 stands for stdin).
  FD_SET(SocketFD, &OriginalSocket);
  FD_SET(SocketFD, &ReadFDS);
  FD_SET(0,        &OriginalStdin);
  FD_SET(0,        &WriteFDS);
  //
  // wait until either socket has data ready to be recv() or timeout (0.5 secs).
  //
  ReadFDS  = OriginalSocket;
  WriteFDS = OriginalStdin;
  Receive  = select(&ReadFDS, NULL, NULL, 1);
  //
  if (Receive == -1) {
    IP_COAP_APP_WARN(("Select error"));
  } else if (Receive == 0) {
    //
    // Timeout.
    //
  } else {
    //
    // One or both of the descriptors have data.
    //
    if (FD_ISSET(SocketFD, &ReadFDS)) {
      FD_CLR(SocketFD, &ReadFDS);
      AddressLength = sizeof(ClientAddress);
      Receive       = recvfrom(SocketFD, (char*)pBuffer, BufferSize, 0,(struct sockaddr *)&ClientAddress, &AddressLength);
      if (Receive == SOCKET_ERROR) {
        getsockopt(SocketFD, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
        IP_COAP_APP_WARN(("recvfrom failed with error: %s", IP_Err2Str(SoError)));
      }
    }
  }
  //
  // Copy the client's address to be used by the CoAP processing.
  //
  pInfo->Family        = IP_COAP_IPV4;
  pInfo->Addr.IPAddrV4 = ClientAddress.sin_addr.s_addr;
  pInfo->Port          = ClientAddress.sin_port;
  //
  // Always indicate it is not multicast/broadcast.
  //
  *pIsMulticast   = 0;

  return Receive;
}

/*********************************************************************
*
*       _APP_Send()
*
*  Function description
*    Receives a UDP packet.
*/
static int _APP_Send(U8* pBuffer, unsigned BufferSize, IP_COAP_CONN_INFO* pInfo) {
  struct sockaddr_in ClientAddress;
  int                AddressLength;
  int                r;

  ClientAddress.sin_addr.s_addr = pInfo->Addr.IPAddrV4;
  ClientAddress.sin_family      = AF_INET;
  ClientAddress.sin_port        = pInfo->Port;
  //
  AddressLength = sizeof(struct sockaddr_in);
  //
  r = sendto((int)pInfo->hSock, (char*)pBuffer, BufferSize, 0, (struct sockaddr *)&ClientAddress, AddressLength);
  //
  if (r == -1) {
    IP_COAP_APP_WARN(("CoAP sending error\n"));
  }
  return r;
}

/*********************************************************************
*
*       _APP_GetTimeMs()
*
*  Function description
*    Returns the time in ms.
*
*  Important note
*    Windows time doesn't rollover on U32.
*
*/
static U32 _APP_GetTimeMs(void) {
  return OS_GetTime();
}

/*********************************************************************
*
*       COAP API
*
*/
static const IP_COAP_API _APP_Api = {
  _APP_Receive,
  _APP_Send,
  _APP_GetTimeMs
};

/*********************************************************************
*
*       _COAP_Server()
*
*  Function description
*    CoAP server application.
*/
static void _COAP_Server(void) {
  struct sockaddr_in   ServerAddress;

  _ConnInfo.hSock = (void*)socket(AF_INET, SOCK_DGRAM, 0);
  if ((int)_ConnInfo.hSock == SOCKET_ERROR) {
    IP_COAP_APP_WARN(("Socket creation error !\n"));
    return;
  }
  _ConnInfo.Family              = IP_COAP_IPV4;
  _ConnInfo.Port                = IP_COAP_DEFAULT_PORT;
  //
  ServerAddress.sin_family      = AF_INET;
  ServerAddress.sin_port        = htons(_ConnInfo.Port);
  ServerAddress.sin_addr.s_addr = INADDR_ANY;
  //
  if (bind((int)_ConnInfo.hSock,(struct sockaddr *)&ServerAddress, sizeof(struct sockaddr)) == -1) {
    IP_COAP_APP_WARN(("Error binding socket\n"));
    return;
  }
  //
  // Initializes the CoAP server.
  //
  IP_COAP_SERVER_Init(&_COAPServer, (U8*)_MsgBuffer, sizeof(_MsgBuffer), &_APP_Api);
  //
  // Configuration of the server.
  //
  _ConfigureServer();
  //
  _RefTime = _APP_GetTimeMs();
  //
  // Main loop.
  //
  while (1) {
    //
    // Call regularly the CoAP processing function.
    //
    IP_COAP_SERVER_Process(&_COAPServer, &_ConnInfo);
    //
    // Perform the application processing.
    //
    _ApplicationProcessing();
  }
}

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Main task executed by the RTOS to create further resources and
*    running the main application.
*/
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(1);
    OS_Delay(50);
  }
  BSP_ClrLED(0);
  _COAP_Server();
}

/*************************** End of file ****************************/
