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

File    : WiFi_Callbacks.c
Purpose : Generic sample implementation of WiFi callbacks used in IP_Config_*.c files.
*/

#include "IP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Timeouts used for various operations.
//
#ifndef   WIFI_CONNECT_TIMEOUT
  #define WIFI_CONNECT_TIMEOUT     40000u  // Time to wait for being fully connected [ms] .
#endif
#ifndef   WIFI_DISCONNECT_TIMEOUT
  #define WIFI_DISCONNECT_TIMEOUT  10000u  // Time to wait for disconnect to succeed [ms] .
#endif
#ifndef   WIFI_SCAN_TIMEOUT
  #define WIFI_SCAN_TIMEOUT         3000u  // Time to wait for a network scan to finish [ms] .
#endif

//
// Network settings. Select one or no security settings by commenting out
// all not matching ones.
//
#ifndef   WIFI_SSID
  #define WIFI_SSID      "TPLINK01"
#endif

#ifndef   WIFI_WPA_ENTERPRISE_USER
  #define WIFI_WPA_ENTERPRISE_USER  "tester"
#endif

#ifndef   WIFI_WPA_PASS
  #define WIFI_WPA_PASS  "12345678"  // WEP keys need to be configured in "Static const" section.
#endif

#ifndef   WIFI_CHANNEL
  #define WIFI_CHANNEL   0           // Select channel to connect to. 0 means use any channel that sends the desired SSID.
#endif

//#define WIFI_SECURITY_WEP_64_OPEN
//#define WIFI_SECURITY_WEP_64_SHARED
//#define WIFI_SECURITY_WEP_128_OPEN
//#define WIFI_SECURITY_WEP_128_SHARED
#define WIFI_SECURITY_WPA_WPA2

#if (defined(WIFI_SECURITY_WEP_64_OPEN) || defined(WIFI_SECURITY_WEP_64_SHARED) || defined(WIFI_SECURITY_WEP_128_OPEN) || defined(WIFI_SECURITY_WEP_128_SHARED))
  #define WIFI_WEP_KEY_INDEX  0    // WEP key index to use.
#endif

#ifndef   ACCESS_POINT_SSID
  #define ACCESS_POINT_SSID      "Demo"
#endif

#ifndef   ACCESS_POINT_PASS
  #define ACCESS_POINT_PASS      "12345678"
#endif

#ifndef   ACCESS_POINT_CHANNEL
  #define ACCESS_POINT_CHANNEL   3
#endif

#ifndef   ACCESS_POINT_SECURITY
  #define ACCESS_POINT_SECURITY  IP_WIFI_SECURITY_WPA2_AES
#endif

//
// Allowed channel subset of regulatory domains.
//
//#define WIFI_CHANNELS  { 1, 4, 6 }  // Comment out to not restrict channels.

//lint -esym(534, IP_WIFI_Disconnect, IP_NI_GetIFaceType)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_WIFI_SCAN_RESULT  _ScanResult;
static I32                  _LastRssi;
static int                  _ScanResultCode;
static U8                   _IsConnected;
static U8                   _ScanFinished;
static U8                   _NetworkFound;
static U8                   _StateIsChanging;  // (1) In case the connect state changes, prevent further changes until the first has completed.

//
// Security keys to use.
// Generated from pass phrase "segger" on a Netgear router.
//
#if (defined(WIFI_SECURITY_WEP_64_OPEN) || defined(WIFI_SECURITY_WEP_64_SHARED) || defined(WIFI_SECURITY_WEP_128_OPEN) || defined(WIFI_SECURITY_WEP_128_SHARED))
static const IP_WIFI_WEP_KEY _aWEPKey[] = {
// WEP key, needs to be padded to 13 bytes if not 128bit                       | key len | key index
//--------------------------------------------------------------------------------------------------
#if (defined(WIFI_SECURITY_WEP_64_OPEN) || defined(WIFI_SECURITY_WEP_64_SHARED))
   0x55, 0xD0, 0x3E, 0x44, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 5       , 0
  ,0xA1, 0x9A, 0x02, 0x08, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 5       , 1
  ,0x53, 0x97, 0x45, 0x3B, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 5       , 2
  ,0x6D, 0xFF, 0x9C, 0x77, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 5       , 3
#endif
#if (defined(WIFI_SECURITY_WEP_128_OPEN) || defined(WIFI_SECURITY_WEP_128_SHARED))
   0xCB, 0xCD, 0x41, 0x85, 0xD1, 0xA4, 0x7F, 0xEF, 0xB9, 0x9B, 0x80, 0xDC, 0xFC, 13      , 0
  ,0xCB, 0xCD, 0x41, 0x85, 0xD1, 0xA4, 0x7F, 0xEF, 0xB9, 0x9B, 0x80, 0xDC, 0xFC, 13      , 1
  ,0xCB, 0xCD, 0x41, 0x85, 0xD1, 0xA4, 0x7F, 0xEF, 0xB9, 0x9B, 0x80, 0xDC, 0xFC, 13      , 2
  ,0xCB, 0xCD, 0x41, 0x85, 0xD1, 0xA4, 0x7F, 0xEF, 0xB9, 0x9B, 0x80, 0xDC, 0xFC, 13      , 3
#endif
};
#endif

//
// Allowed WiFi channels.
//
#ifdef WIFI_CHANNELS
static const U8 _AllowedChannels[] = WIFI_CHANNELS;
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/********************************************************************
*
*       _BeforeConnectScanResult()
*
*  Function description
*    Callback used to handle the results from a scan that will be
*    triggered before actually connecting to retrieve the security
*    settings from a specific network.
*
*  Parameters
*    IFaceId: Zero-baeed interface index.
*    pResult: Pointer to IP_WIFI_SCAN_RESULT structure of the scan
*             result for the network to connect to.
*    Status :  1: This is the last (empty) result returned for the current scan.
*              0: Next result returned for the current scan.
*             -1: Error, scan aborted.
*/
static void _BeforeConnectScanResult(unsigned IFaceId, const IP_WIFI_SCAN_RESULT* pResult, int Status) {
#ifdef WIFI_CHANNELS
  unsigned IsChannelAllowed;
  unsigned i;
  U8       Channel;
#endif

  IP_USE_PARA(IFaceId);

  if (pResult != NULL) {
    if (IP_MEMCMP(pResult->sSSID, WIFI_SSID, strlen(WIFI_SSID)) == 0) {
#ifdef WIFI_CHANNELS
      //
      // Software check for allowed channels in case the driver/module
      // do not provide a way to configure this.
      //
      Channel          = pResult->Channel;
      IsChannelAllowed = 0u;
      i                = 0u;
      do {
        if (_AllowedChannels[i] == Channel) {
          IsChannelAllowed = 1u;
          break;
        }
      } while (++i < SEGGER_COUNTOF(_AllowedChannels));
      if (IsChannelAllowed != 0u)
#endif
      {
        if (pResult->Rssi < _ScanResult.Rssi) {  // Better signal than before ?
          _NetworkFound = 1u;
          IP_MEMCPY(&_ScanResult, pResult, sizeof(_ScanResult));
        }
      }
    }
  }
  if (Status != 0) {
    //
    // Save any error returned from the driver.
    //
    if (Status < 0) {
      _ScanResultCode = Status;
    } else if (_NetworkFound == 0u) {
      _ScanResultCode = -1;  // Error, no matching network found in scan.
      IP_Warnf_Application("No matching network found to connect to.");
    }
    //
    // Let the connect loop know the scan is done.
    //
    _ScanFinished = 1;
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/********************************************************************
*
*       WIFI_CB_OnLinkChange()
*
*  Function description
*    Callback executed each time the link state (connect state) of
*    the WiFi interface changes.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*    Duplex : Duplex reported for interface.
*    Speed  : Speed reported for interface.
*
*  Notes
*    (1) Recursion (OnDisconnect->IP_Disconnect()->OnDisconnect->...)
*        is prevented by using a locking variable.
*/
void WIFI_CB_OnLinkChange(unsigned IFaceId, U32 Duplex, U32 Speed) {
  if ((Duplex != 0u) && (Speed != 0u)) {
    IP_Logf_Application("IFace %u: Connected", IFaceId);
    _IsConnected = 1;
  } else {
    IP_Logf_Application("IFace %u: Disconnected", IFaceId);
    _IsConnected     = 0;
    _LastRssi        = 0;
    _StateIsChanging = 0;  // (1).
  }
}

/*********************************************************************
*
*       WIFI_CB_Disconnect()
*
*  Function description
*    Disconnects from access point.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*
*  Return value
*    O.K. :   0.
*    Error: < 0.
*
*  Notes
*    (1) Recursion (OnDisconnect->IP_Disconnect()->OnDisconnect->...)
*        is prevented by using a locking variable.
*/
int WIFI_CB_Disconnect(unsigned IFaceId) {
  U32 Timeout;

  if (_StateIsChanging == 0u) {  // (1)
    _StateIsChanging = 1;        // (1)
    IP_WIFI_Disconnect(IFaceId, WIFI_DISCONNECT_TIMEOUT);
    Timeout = IP_OS_GET_TIME() + 200u;
    while ((_IsConnected != 0u) && (IP_IsExpired(Timeout) == 0u)) {
      IP_OS_Delay(1);
    }
    _StateIsChanging = 0;        // (1)
  }
  return 0;
}

/********************************************************************
*
*       WIFI_CB_OnAssociateChange()
*
*  Function description
*    Callback executed each time the associate state (link state) of
*    the WiFi interface changes.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*    pInfo  : Pointer to IP_WIFI_ASSOCIATE_INFO structure. Can be NULL.
*    State  : Associate state. 0: Disassociate event.
*                              1: Associate event.
*/
void WIFI_CB_OnAssociateChange(unsigned IFaceId, const IP_WIFI_ASSOCIATE_INFO* pInfo, U8 State) {
  U8 abBSSID[6];
  U8 Channel;

  //
  // Not all modules support the API to retrieve all
  // connection parameters of the current connection.
  // For the information left we try to use the information
  // we collected with our before connect scan. The
  // AP found in this scan is most likely the one with
  // the best signal.
  //
  if (pInfo != NULL) {
    Channel = pInfo->Channel;
    if (IP_IsAllZero(&pInfo->abBSSID[0], sizeof(pInfo->abBSSID)) != 0u) {
      IP_MEMCPY(&abBSSID[0], &_ScanResult.abBSSID[0], sizeof(abBSSID));
    } else {
      IP_MEMCPY(&abBSSID[0], &pInfo->abBSSID[0], sizeof(abBSSID));
    }
  } else {
    Channel = _ScanResult.Channel;
    IP_MEMCPY(&abBSSID[0], &_ScanResult.abBSSID[0], sizeof(abBSSID));
  }
  if (State != 0u) {
    IP_Logf_Application("IFace %u: Associated to BSSID %02x:%02x:%02x:%02x:%02x:%02x channel %d", IFaceId,
                                                                                                  abBSSID[0],
                                                                                                  abBSSID[1],
                                                                                                  abBSSID[2],
                                                                                                  abBSSID[3],
                                                                                                  abBSSID[4],
                                                                                                  abBSSID[5],
                                                                                                  Channel);
  } else {
    IP_Logf_Application("IFace %u: Disassociated", IFaceId);
//    WIFI_CB_Disconnect(IFaceId);  // Disconnect if access point is not reachable.
  }
}

/********************************************************************
*
*       WIFI_CB_OnSignalChange()
*
*  Function description
*    Callback executed each time the signal strength value of the
*    current connection of the WiFi interface has changed.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*    pInfo  : Structure with signal strength information like
*             RSSI (Receive Signal Strength Indicator), typically
*             given as negative value. Higher (towards 0) means better.
*/
void WIFI_CB_OnSignalChange(unsigned IFaceId, const IP_WIFI_SIGNAL_INFO* pInfo) {
  I32 Rssi;

  if (_IsConnected != 0u) {
    Rssi = pInfo->Rssi;
    if (_LastRssi != Rssi) {
      _LastRssi = Rssi;
      IP_Logf_Application("IFace %u: RSSI changed to %d", IFaceId, Rssi);
    }
  }
}

/*********************************************************************
*
*       WIFI_CB_Connect()
*
*  Function description
*    Connects to a WiFi network.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*
*  Return value
*    O.K. :   0.
*    Error: < 0.
*
*  Notes
*    (1) Recursion (OnDisconnect->IP_Disconnect()->OnDisconnect->...)
*        is prevented by using a locking variable.
*/
int WIFI_CB_Connect(unsigned IFaceId) {
  IP_WIFI_CONNECT_PARAMS ConnectParams;
  U32                    Timeout;
  U32                    t;
  int                    r;

  r = 0;  // Assume O.K.

  if (_StateIsChanging == 0u) {  // (1)
    _StateIsChanging = 1;        // (1)
    _ScanResultCode  = 0;        // O.K.
    _NetworkFound    = 0;
    //
    // Setup general connect parameters.
    //
    IP_MEMSET(&ConnectParams, 0, sizeof(ConnectParams));
    ConnectParams.sSSID   = WIFI_SSID;
    ConnectParams.Mode    = IP_WIFI_MODE_INFRASTRUCTURE;
    ConnectParams.Channel = WIFI_CHANNEL;
    //
    // Setup security access keys.
    //
#ifdef WIFI_SECURITY_WPA_WPA2
  #ifdef WIFI_WPA_ENTERPRISE_USER
    ConnectParams.sEnterpriseUser   = WIFI_WPA_ENTERPRISE_USER;  // Unsused if security is not enterprise but does not hurt.
  #endif
    ConnectParams.sWPAPass          = WIFI_WPA_PASS;
#elif (defined(WIFI_SECURITY_WEP_64_OPEN) || defined(WIFI_SECURITY_WEP_64_SHARED) || defined(WIFI_SECURITY_WEP_128_OPEN) || defined(WIFI_SECURITY_WEP_128_SHARED))
    ConnectParams.paWEPKey          = &_aWEPKey[0];
    ConnectParams.NumWEPKeys        = SEGGER_COUNTOF(_aWEPKey);
    ConnectParams.WEPActiveKeyIndex = 0;
#endif
    //
    // Start counting connect timeout here to include time
    // for a (repeated) network scan and the connect itself.
    //
    t       = IP_OS_GET_TIME();
    Timeout = t + WIFI_CONNECT_TIMEOUT;
    //
    // Setup security type.
    //
#if (defined(WIFI_SECURITY_WEP_64_OPEN) || defined(WIFI_SECURITY_WEP_128_OPEN))
    ConnectParams.Security          = IP_WIFI_SECURITY_WEP_OPEN;
#elif (defined(WIFI_SECURITY_WEP_64_SHARED) || defined(WIFI_SECURITY_WEP_128_SHARED))
    ConnectParams.Security          = IP_WIFI_SECURITY_WEP_SHARED;
#else
    //
    // Retrieve security used by WiFi before connecting.
    //
    _ScanFinished = 0;
    IP_MEMSET(&_ScanResult, 0, sizeof(_ScanResult));
    IP_Logf_Application("Detecting security of SSID: %s...", WIFI_SSID);
    for (;;) {
      if (IP_IsExpired(Timeout) == 1u) {
        _StateIsChanging = 0;  // (1)
        return IP_ERR_TIMEOUT;
      }
      if (IP_WIFI_Scan(IFaceId, WIFI_SCAN_TIMEOUT, _BeforeConnectScanResult, WIFI_SSID, IP_WIFI_CHANNEL_ALL) == 0) {
        break;                 // Scan successfully started.
      }
      IP_OS_Delay(50);         // Grant a little delay before retrying to start scan.
    }
    //
    // Wait for scan to finish.
    //
    for (;;) {
      if (IP_IsExpired(Timeout) == 1u) {
        _StateIsChanging = 0;  // (1).
        return IP_ERR_TIMEOUT;
      }
      if (_ScanFinished == 1u) {
        r = _ScanResultCode;   // 0: O.K., < 0: Error.
        break;                 // Scan successfully finished.
      }
      IP_OS_Delay(10);         // Give other tasks a chance to do something.
    }
    if (r == 0) {  // Still O.K. ?
      IP_Logf_Application("Detected security \"%s\"", IP_WIFI_Security2String(_ScanResult.Security));
      ConnectParams.Security = _ScanResult.Security;
    } else {
      IP_Logf_Application("Connect scan failed");
    }
#endif
    if (r == 0) {  // Still O.K. ?
      //
      // Calculate rest of timeout to be used for connect.
      //
      t = IP_OS_GET_TIME() - t;
      t = WIFI_CONNECT_TIMEOUT - t;
      if ((t > WIFI_CONNECT_TIMEOUT) || (t == 0u)) {
        _StateIsChanging = 0;  // (1).
        return IP_ERR_TIMEOUT;
      }
      //
      // Connect to network.
      //
      IP_Logf_Application("Connecting to SSID: %s", WIFI_SSID);
      if (IP_WIFI_Connect(IFaceId, &ConnectParams, t) != 0) {
        IP_Logf_Application("Could not connect to SSID: %s", WIFI_SSID);
        _StateIsChanging = 0;  // (1).
        return IP_ERR_MISC;
      }
      //
      // Wait to be fully connected.
      //
      for (;;) {
        if (IP_IsExpired(Timeout) == 1u) {
          IP_Logf_Application("Connect failed");
          IP_WIFI_Disconnect(IFaceId, WIFI_DISCONNECT_TIMEOUT);
          _StateIsChanging = 0;  // (1).
          return IP_ERR_TIMEOUT;
        }
        if (_IsConnected == 1u) {
          IP_Logf_Application("Connected to SSID: %s, channel: %u, RSSI: %d", WIFI_SSID, _ScanResult.Channel, _ScanResult.Rssi);
          break;                 // Scan successfully finished.
        }
        IP_OS_Delay(10);         // Give other tasks a chance to do something.
      }
    }
    _StateIsChanging = 0;        // (1).
  }
  return r;
}

/*********************************************************************
*
*       WIFI_CB_Connect_StartAP()
*
*  Function description
*    Starts an access point.
*
*  Parameters
*    IFaceId: Zero-based interface index.
*
*  Return value
*    O.K. :   0.
*    Error: < 0.
*/
int WIFI_CB_Connect_StartAP(unsigned IFaceId) {
  IP_WIFI_CONNECT_PARAMS ConnectParams;
  int                    r;

  IP_MEMSET(&ConnectParams, 0, sizeof(ConnectParams));
  ConnectParams.sSSID    = ACCESS_POINT_SSID;
  ConnectParams.Mode     = IP_WIFI_MODE_ACCESS_POINT;
  ConnectParams.sWPAPass = ACCESS_POINT_PASS;
  ConnectParams.Security = ACCESS_POINT_SECURITY;
  ConnectParams.Channel  = ACCESS_POINT_CHANNEL;
  //
  // Starts the access point.
  //
  r = IP_WIFI_Connect(IFaceId, &ConnectParams, WIFI_CONNECT_TIMEOUT);
  if (r == 0) {
    IP_Logf_Application("Access point started. SSID: %s", ConnectParams.sSSID);
  } else {
    IP_Warnf_Application("Access point start error!");
  }

  return r;
}


/*********************************************************************
*
*       WIFI_CB_OnClientNotification()
*
*  Function description
*    In AP mode, indication of a client connection or disconnection.
*
*  Parameters
*    IFaceId    : Zero-based interface index.
*    pInfo      : Information on the client.
*    IsConnected: Flag for a connection(1) or disconnection (0).
*/
void WIFI_CB_OnClientNotification(unsigned IFaceId, const IP_WIFI_CLIENT_INFO* pInfo, unsigned IsConnected) {
  (void)IFaceId;

  if (IsConnected != 0u) {
    IP_Logf_Application("Client connection : %_h", pInfo->pMacAddress);
  } else {
    IP_Logf_Application("Client disconnection : %_h", pInfo->pMacAddress);
  }
}

/*************************** End of file ****************************/
