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

Purpose : Sample program for embOS & emNet demonstrating a possible
          usage of the CoAP client.

Additional information:
  This sample starts a CoAP client and sends
  a couple of commands to the CoAP server specified
  via the "COAP_SERVER" and "COAP_PORT" defines.

  Example output:
    .
    .
    .
    4:076 MainTask - Test 0: Sending PING
    4:076 MainTask - Test 0: Success!
    4:077 MainTask - Test 1: Sending Discover
    4:098 MainTask - /obs: Simple observable data.
    /test: test entry
    /separate: simple variable with GET with LATE reply
    /BlockTransfer/ObsData: Observable data block.
    /ObsSensor:
    4:098 MainTask - Test 1: Success!
    4:099 MainTask - Test 2: Sending GET separate
    4:100 MainTask - 00:00:00
    4:100 MainTask - Test 2: Success!
    4:100 MainTask - Test 3: Sending NON GET block of 16 bytes
    4:101 MainTask - Test data
    4:101 MainTask - Test 3: Success!
    4:101 MainTask - Test 4: PUT test
    4:102 MainTask - Test 4: Success!
    4:103 MainTask - Test 5: PUT that should fail
    4:103 MainTask - received error: 4.05
    4:103 MainTask - Test 5: Success!
    4:103 MainTask - Test 6: DELETE test
    4:104 MainTask - Test 6: Success!
    4:104 MainTask - Test 7: POST test
    4:105 MainTask - Test 7: Success!
    4:105 MainTask - Test 8: Observe
    4:106 MainTask - Obs data
    4:106 MainTask - GET 1 from Observe completed
    .
    .
    .
    44:106 MainTask - Obs data
    44:106 MainTask - GET 5 from Observe completed
    44:106 MainTask - Test 8: Success!
    .
    .
    .
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_COAP.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK                0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

#define COAP_PORT     IP_COAP_DEFAULT_PORT
#define COAP_SERVER   "192.168.101.203"

//
// Application
//
#define DESCRIPTION_SIZE 2048
#define LAST_TEST        1000

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
static IP_COAP_CLIENT_CONTEXT  _COAPClient;
static IP_COAP_CONN_INFO       _ConnInfo;
static U32                     _MsgBuffer[1500 / sizeof(U32)];     // U32 to make sure we have a word alignment.

static IP_COAP_CLIENT_OBS      _Observer;

//
// Application's data
//
static int                     _TestId;
static int                     _CmdOnGoing;
static U8                      _CmdOnGoingIndex;
static int                     _ErrorDetected;
static int                     _NumObs;

static char                    _Description[DESCRIPTION_SIZE];
static U8                      _Payload[] = "<------0-------><------1------>\n<------2-------><------3------>\n";

static int                     _ObsOnGoing;
static char                    _ObsURI[] = "obs";

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
*       _GET_Handler()
*
* Function description
*   Handles a GET.
*/
static int _GET_Handler(IP_COAP_CLIENT_CONTEXT* pContext, U8** ppPayload, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  (void)pContext;
  (void)ppPayload;
  (void)pLength;
  (void)pParam;

  IP_COAP_APP_LOG(("\n  %.*s", *pLength, *ppPayload));
  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _PUT_Handler()
*
* Function description
*   Handles a PUT / POST.
*/
static int _PUT_Handler(IP_COAP_CLIENT_CONTEXT* pContext, U8** ppPayload, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  int Offset;
  U8* pBuf;
  U16 Length;
  U16 l;
  int r;
  int PayloadLength;

  (void)pContext;

  pBuf          = *ppPayload;
  Length        = *pLength;
  r             = IP_COAP_RETURN_SEND_END;          // By default, this is the last block.
  PayloadLength = IP_COAP_STRLEN((char*)_Payload);  // Don't count the 0 from the null-terminated string
  //
  Offset = pParam->pBlock->Index * pParam->pBlock->Size;
  if (Offset >= PayloadLength) {
    _ErrorDetected = 1;
    return IP_COAP_RETURN_NO_PAYLOAD;
  }
  //
  // Compute the remainging length of the payload not sent yet.
  //
  l = PayloadLength - Offset;
  //
  // If the remaining length of the payload is bigger than this
  // block size, reduce it to the block size.
  //
  if (l > Length) {
    r = IP_COAP_RETURN_SEND_BLOCK;  // More data to send in a next block.
    l = Length;
  }
  IP_COAP_MEMCPY(pBuf, _Payload + Offset, l);
  pBuf   += l;
  Length -= l;
  //
  *ppPayload = pBuf;
  *pLength   = Length;

  return r;
}

/*********************************************************************
*
*       _DISCOVER_Handler()
*
* Function description
*   Handles a GET .weel-known/core. Called for each block of data.
*   Stores the data in a static with the following format:
*   URI: description\n
*/
static int _DISCOVER_Handler(IP_COAP_CLIENT_CONTEXT* pContext, U8** ppPayload, U16* pLength, IP_COAP_CALLBACK_PARAM* pParam) {
  static int       InName;
  static int       InDesc;
         U8*       pBuf;
         U16       Length;
         unsigned  i;
         char      c;
  const  char      Title[] = "title=";
  static int       TitleCounter;
  static int       DescSize;
  static int       SkipFirst;

  (void)pContext;

  //
  // Reset the static on the first block and check the content format.
  //
  if (pParam->pBlock->Index == 0) {
    InName       = 0;
    InDesc       = 0;
    TitleCounter = 0;
    DescSize     = 0;
    SkipFirst    = 1;
    //
    // Check the content format (presence is mandatory).
    //
    if (IP_COAP_CheckContentFormat(pParam, IP_COAP_CT_LINK_FORMAT, 1) != IP_COAP_RETURN_OK) {
      IP_COAP_APP_LOG(("Test %d: Wrong content format!", _TestId));
      _ErrorDetected = 1;
    }
  }
  //
  pBuf   = *ppPayload;
  Length = *pLength;
  //
  for (i = 0; i < Length; i++) {
    c = *pBuf++;
    if (c == '<') {
      if (SkipFirst == 0) {
        if (DescSize < DESCRIPTION_SIZE) {
          _Description[DescSize++] = '\n';
        }
      }
      SkipFirst = 0;
      InName    = 1;
      continue;
    }
    if (c == '>') {
      InName = 0;
      if ((DescSize+2) < DESCRIPTION_SIZE) {
        _Description[DescSize++] = ':';
        _Description[DescSize++] = ' ';
      }
      continue;
    }
    if (c == '\"') {
      if ((InDesc == 0) && (TitleCounter == 6)) {
        InDesc = 1;
        continue;
      } else if (InDesc == 1) {
        InDesc = 0;
        continue;
      }
    }
    if (c == Title[TitleCounter]) {
      TitleCounter++;
    } else {
      TitleCounter = 0;
    }
    if (InName || InDesc) {
      if (c != '\n') {
        if (DescSize < DESCRIPTION_SIZE) {
          _Description[DescSize++] = c;
        }
      }
    }
  }
  //
  // Terminate the string with a 0.
  //
  _Description[DescSize] = 0;

  return IP_COAP_RETURN_OK;
}

/*********************************************************************
*
*       _HandleResult()
*
*  Function description
*    Print the result of the command.
*/
static void _HandleResult(int Index, U8 ResultCode, U8* pError, U16 ErrorLength) {

  //
  // In this example pError and ErrorLength are not used, but in some
  // error cases, it may contain a string with an explanation for the
  // error (to precise the error code).
  //
  (void)pError;
  (void)ErrorLength;

  //
  // Is this a reply corresponding to the sent request.
  //
  if (((_CmdOnGoing == 0) || (_CmdOnGoingIndex != Index)) && (_ObsOnGoing == 0)) {
    IP_COAP_APP_WARN(("Unrequested reply"));
    return;
  }
  _CmdOnGoing = 0;
  //
  switch (_TestId) {
    case 0:
      //
      // Expected reply: answer to a ping.
      //
      if (ResultCode == IP_COAP_CODE_EMPTY) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 1:
      //
      // Expected answer to the discover.
      //
      if ((ResultCode == IP_COAP_CODE_SUCCESS_CONTENT) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("%s", _Description));
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 2:
      //
      // Expected answer to a GET separate.
      //
      if ((ResultCode == IP_COAP_CODE_SUCCESS_CONTENT) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 3:
      //
      // Expected answer to a GET with a NON.
      //
      if ((ResultCode == IP_COAP_CODE_SUCCESS_CONTENT) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 4:
      //
      // Expected answer to a PUT.
      //
      if ((ResultCode == IP_COAP_CODE_SUCCESS_CHANGED) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 5:
      //
      // Expected reject of a PUT.
      //
      if ((IP_COAP_GET_CLASS(ResultCode) != IP_COAP_CLASS_SUCCESS) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("received error: %d.%02d", IP_COAP_GET_CLASS(ResultCode), ResultCode & 0x1F));
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 6:
      //
      // Expected answer to a DELETED.
      //
      if ((ResultCode == IP_COAP_CODE_SUCCESS_DELETED) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 7:
      //
      // Expected answer to a PUT.
      //
      if (((ResultCode == IP_COAP_CODE_SUCCESS_CREATED) || (ResultCode == IP_COAP_CODE_SUCCESS_CHANGED)) && (_ErrorDetected == 0)) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 8:
      //
      // End of an observable request.
      //
      _ObsOnGoing = 0;
      //
      if (_ErrorDetected == 0) {
        IP_COAP_APP_LOG(("Test %d: Success!", _TestId));
      } else {
        IP_COAP_APP_LOG(("Test %d: Error!!!", _TestId));
      }
      break;
    case 9:
      if (_ErrorDetected != 0) {
        IP_COAP_APP_LOG(("Error !!!"));
      }
      break;
    default:
      break;

  }
  _TestId++;
}

/*********************************************************************
*
*       _OBS_EndHandler()
*
* Function description
*   Indication of an observer end.
*/
static void _OBS_EndHandler(U8 ReturnCode, int IsFinal, void* pParam) {
  int r;
  U8  Index;

  Index = (U8)(U32)pParam;
  //
  if (ReturnCode != IP_COAP_CODE_SUCCESS_CONTENT) {
    _ErrorDetected = 1;
  }
  //
  // When the 5th GET is received, observation was cancelled, so
  // no more reply is expected.
  //
  if (_NumObs == 5) {
    _ErrorDetected = 1;
  }
  _NumObs++;
  //
  // IsFinal indicates the observation is aborted, either
  // the server replied to the GET but the value was not
  // observable or an error occured.
  //
  if (IsFinal != 0) {
    _ErrorDetected = 1;
    IP_COAP_APP_LOG(("\n  Observe terminated"));
  } else {
    IP_COAP_APP_LOG(("\n  GET %d from Observe completed", _NumObs));
    //
    // After 5 observation, cancel the observe request with an active abort.
    //
    if (_NumObs == 5) {
      //
      // Abort the observe.
      //
      IP_COAP_CLIENT_OBS_Abort(&_COAPClient, Index, &_Observer, 1);
      //
      // And actually send the abort reusing the on-going request.
      //
      IP_COAP_CLIENT_SetCommand(&_COAPClient, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_GET);
      IP_COAP_CLIENT_SetOptionURIPath(&_COAPClient, Index, (U8*)&_ObsURI[0], (U8)IP_COAP_STRLEN(_ObsURI));
      //
      // Use the same payload handler as other GET.
      //
      IP_COAP_CLIENT_SetPayloadHandler(&_COAPClient, Index, _GET_Handler);
      //
      r = IP_COAP_CLIENT_BuildAndSend(&_COAPClient, Index);
      if (r != IP_COAP_RETURN_OK) {
        _ErrorDetected = 1;
        _TestId++;
      }
    }
  }
}

/*********************************************************************
*
*       _BuildNewRequest()
*
*  Function description
*    Sends a new request to the server.
*    Note that the following example shall be adapt depending
*    on the used CoAP server structure.
*/
static void _BuildNewRequest(unsigned Index) {
  IP_COAP_CLIENT_CONTEXT* pContext;
  int                     r;

  pContext       = &_COAPClient;
  _ErrorDetected = 0;
  //
  if (_CmdOnGoing != 0) {
    return;
  }
  //
  switch (_TestId) {
    case 0:
      //
      // First send a simple PING (CON, EMPTY) to the server.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_EMPTY);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: Sending PING", _TestId));
      break;
    case 1:
      //
      // Change the default block size.
      //
      IP_COAP_CLIENT_SetDefaultBlockSize(&_COAPClient, 64);
      //
      // Send a "discover": GET on ".well-known/core"
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_GET);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)".well-known/core", 16);
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _DISCOVER_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: Sending Discover", _TestId));
      break;
    case 2:
      //
      // Send a GET with a separate reply (no piggyback).
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_GET);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"separate", 8);
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _GET_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: Sending GET separate", _TestId));
      break;
    case 3:
      //
      // Send a GET test with a NON.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_NON, IP_COAP_CODE_REQ_GET);
      //
      // Set a block size (will use a block of 16 for this request but
      // won't change the default of 64).
      //
      IP_COAP_CLIENT_SetOptionBlock(pContext, Index, 16);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"test", 4);
      IP_COAP_CLIENT_SetOptionAccept(pContext, Index, IP_COAP_CT_TXT);
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _GET_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: Sending NON GET block of 16 bytes", _TestId));
      //
      // Change the block size for the request
      //
      break;
    case 4:
      //
      // Change the default block size.
      //
      IP_COAP_CLIENT_SetDefaultBlockSize(&_COAPClient, 32);
      //
      // Send a PUT.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_PUT);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"test", 4);
      IP_COAP_CLIENT_SetOptionSize1(pContext, Index, IP_COAP_STRLEN((char*)_Payload));
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _PUT_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: PUT test", _TestId));
      break;
    case 5:
      //
      // Send a PUT that will fail.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_PUT);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"separate", 8);
      IP_COAP_CLIENT_SetOptionSize1(pContext, Index, sizeof((char*)_Payload));
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _PUT_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: PUT that should fail", _TestId));
      break;
    case 6:
      //
      // Send a DELETE.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_DEL);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"test", 4);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: DELETE test", _TestId));
      break;
    case 7:
      //
      // Send a POST.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_POST);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)"test", 4);
      IP_COAP_CLIENT_SetOptionSize1(pContext, Index, sizeof((char*)_Payload));
      IP_COAP_CLIENT_SetOptionContentFormat(pContext, Index, IP_COAP_CT_TXT);
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _PUT_Handler);
      //
      IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      //
      _CmdOnGoing      = 1;
      _CmdOnGoingIndex = Index;
      IP_COAP_APP_LOG(("\nTest %d: POST test", _TestId));
      break;
    case 8:
      //
      // Observable data.
      //
      if (_ObsOnGoing != 0) {
        break;
      }
      _NumObs = 0;
      //
      // Observe specific initialization.
      //
      IP_COAP_CLIENT_OBS_Init(pContext, Index, &_Observer, 1);
      IP_COAP_CLIENT_OBS_SetEndCallback(&_Observer, _OBS_EndHandler, (void*)(U32)Index);
      //
      // Regular request initialization. Observe uses a GET request.
      //
      IP_COAP_CLIENT_SetCommand(pContext, Index, IP_COAP_TYPE_CON, IP_COAP_CODE_REQ_GET);
      IP_COAP_CLIENT_SetOptionURIPath(pContext, Index, (U8*)&_ObsURI[0], (U8)IP_COAP_STRLEN(_ObsURI));
      IP_COAP_CLIENT_SetOptionBlock(pContext, Index, 16);
      //
      // Use the same payload handler as other GET.
      //
      IP_COAP_CLIENT_SetPayloadHandler(pContext, Index, _GET_Handler);
      //
      IP_COAP_APP_LOG(("\nTest %d: Observe", _TestId));
      //
      r = IP_COAP_CLIENT_BuildAndSend(pContext, Index);
      if (r != IP_COAP_RETURN_OK) {
        IP_COAP_APP_LOG(("Error starting the observer."));
        //
        // Go to next test.
        //
        _TestId++;
      } else {
        _ObsOnGoing = 1;
      }
      break;
    case 9:
      _TestId = LAST_TEST;
      break;
    default:
      break;
  }
}


/*********************************************************************
*
*       _RunApplication()
*
*  Function description
*    User interface handling.
*/
static int _RunApplication(void) {
  unsigned Index;
  U8       ResultCode;
  U8*      pError;
  U16      ErrorLength;
  int      r;

  //
  // Check if there is a pending result.
  //
  r = IP_COAP_CLIENT_GetLastResult(&_COAPClient, &ResultCode, &pError, &ErrorLength);
  if (r >= 0) {
    _HandleResult(r, ResultCode, pError, ErrorLength);
  }
  //
  // Check if there is a free request to execute a new one.
  //
  if (IP_COAP_CLIENT_GetFreeRequestIdx(&_COAPClient, &Index) == IP_COAP_RETURN_OK) {
    //
    // There is a free request: build a new request to be sent to the server.
    //
    _BuildNewRequest(Index);
  }
  //
  if (_TestId == LAST_TEST) {
    r = 1;
  } else {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       _ConfigureClient()
*
*  Function description
*    Configures the application.
*/
static void _ConfigureClient(void) {
  //
  // Get server address.
  //
  IP_COAP_CLIENT_SetServerAddress(&_COAPClient, &_ConnInfo);
  //
  // Select with which test to start.
  //
  _TestId = 0;
}

/*********************************************************************
*
*       Local functions - CoAP functionality.
*
**********************************************************************
*/

/*********************************************************************
*
*       _IsIPAddress()
*
*  Function description
*    Checks if string is a dot-decimal IP address, for example 192.168.1.1
*/
static unsigned _IsIPAddress(const char* sIPAddr) {
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
      if ((NumDots < 3) || (i > 15)) { // Error, every dot-decimal IP address includes 3 '.' and is not longer as 15 characters.
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
*    IP as 32-bit number in host endianness.
*/
static long _ParseIPAddr(const char* sIPAddr) {
  long     IPAddr;
  unsigned Value;
  unsigned NumDots;
  unsigned i;
  unsigned j;
  char     acDigits[4];
  char     c;

  IPAddr = 0;
  //
  // Check if string is a valid IP address.
  //
  Value = _IsIPAddress(sIPAddr);
  if (Value) {
    //
    // Parse the IP address.
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
  struct sockaddr_in  ServerAddress;

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
      AddressLength = sizeof(ServerAddress);
      Receive       = recvfrom(SocketFD, (char*)pBuffer, BufferSize, 0,(struct sockaddr *)&ServerAddress, &AddressLength);
      if (Receive == SOCKET_ERROR) {
        getsockopt(SocketFD, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
        IP_COAP_APP_WARN(("recvfrom failed with error: %s", IP_Err2Str(SoError)));
      }
    }
  }
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

  ClientAddress.sin_addr.s_addr = htonl(pInfo->Addr.IPAddrV4);
  ClientAddress.sin_family      = AF_INET;
  ClientAddress.sin_port        = htons(pInfo->Port);
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
static void _COAP_Client(void) {
  struct sockaddr_in   Address;
  struct hostent*      pHostEntry;
  long                 Ip;
  int                  Exit;

  //
  // Get the CoAP server IP address.
  //
  if (_IsIPAddress(COAP_SERVER)) {
    Ip = _ParseIPAddr(COAP_SERVER);
  } else {
    //
    // Convert host into IP address.
    //
    pHostEntry = gethostbyname((char*)COAP_SERVER);
    if (pHostEntry == NULL) {
      IP_COAP_APP_WARN(("APP: gethostbyname failed: %s\n", COAP_SERVER));
      return ;
    }
    Ip = *(unsigned*)(*pHostEntry->h_addr_list);
    Ip = htonl(Ip);
  }
  //
  // Initializes the connection.
  //
  _ConnInfo.hSock = (void*)socket(AF_INET, SOCK_DGRAM, 0);
  if ((int)_ConnInfo.hSock == SOCKET_ERROR) {
    IP_COAP_APP_WARN(("Socket creation error !\n"));
    return;
  }
  _ConnInfo.Family        = IP_COAP_IPV4;
  _ConnInfo.Port          = COAP_PORT;
  _ConnInfo.Addr.IPAddrV4 = Ip;
  //
  Address.sin_family      = AF_INET;
  Address.sin_port        = 0;
  Address.sin_addr.s_addr = INADDR_ANY;
  //
  if (bind((int)_ConnInfo.hSock,(struct sockaddr *)&Address, sizeof(struct sockaddr)) == -1) {
    IP_COAP_APP_WARN(("Error bind\n"));
    return;
  }
  //
  // Initializes the CoAP client.
  //
  IP_COAP_CLIENT_Init(&_COAPClient, (U8*)&_MsgBuffer[0], sizeof(_MsgBuffer), &_APP_Api);
  //
  // Configures the application.
  //
  _ConfigureClient();
  //
  // Main loop
  //
  Exit = 0;
  //
  while (Exit == 0) {
    //
    // Call the process. It will handle reception, retry, observe...
    //
    IP_COAP_CLIENT_Process(&_COAPClient);
    //
    // And call the application. CoAP client is not thread safe,
    // if a separate task is used, it is needed to protext the calls
    // to CoAP client function.
    //
    Exit = _RunApplication();
  };
  //
  // Note: If "observe" is not used, and there is only one request
  // at a time (IP_COAP_NSTART == 1), it is not needed to call
  // IP_COAP_CLIENT_Process() continuously.
  // Another way to write a client would be in this case for each
  // request:
  //   //
  //   // Get a free request.
  //   //
  //   IP_COAP_CLIENT_GetFreeRequestIdx(...)
  //   //
  //   // Initializes the request.
  //   //
  //   IP_COAP_CLIENT_SetCommand(...)
  //   IP_COAP_CLIENT_SetOptionXxx(...)
  //   ...
  //   //
  //   // Send the request.
  //   //
  //   IP_COAP_CLIENT_BuildAndSend(...)
  //   //
  //   // Do CoAP processing until the reply is received.
  //   //
  //   while (IP_COAP_CLIENT_GetLastResult(...) < 0) {
  //     IP_COAP_CLIENT_Process(...);
  //   };
  //
  //
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
  _COAP_Client();
  //
  while (1) {
    BSP_ToggleLED(0);
    OS_Delay(500);
  };
}

/*************************** End of file ****************************/
