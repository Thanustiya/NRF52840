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

Purpose : Sample program for embOS & emNet
          Demonstrates use of the IP_DISCOVER Add-on.
          On Windows (A and AAAA records only):
          > ping -4 server-test.local   -> A request
          > ping -6 server-test2.local  -> AAAA request
          On Linux:
          > avahi-resolve -n server-test.local           -> A request
          > avahi-resolve -n -6 server-test.local        -> AAAA request
          > avahi-resolve -a <ip-address-of-the-server>  -> PTR request
          On Mac:
          > ping server-test.local           -> A request
          > dns-sd -q server-test.local TXT  -> TXT request
          > dns-sd -q server-test.local SRV  -> SRV request
          > dns-sd -q _http._tcp.local PTR   -> PTR request

Notes   : For compatibility with interfaces that need to connect in
          any way this sample calls connect and disconnect routines
          that might not be needed in all cases.

          This sample can be used for Ethernet and dial-up interfaces
          and is configured to use the last registered interface as
          its main interface.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

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
// Server DNS-SD Configuration.
//
#define NUM_SD_CONFIG  9
static IP_DNS_SERVER_CONFIG     _DiscoverConfig;
static IP_DNS_SERVER_SD_CONFIG  _SDConfig[NUM_SD_CONFIG];

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
* Function description
*   Callback that will be notified once the state of an interface
*   changes.
*
* Parameters
*   IFaceId   : Zero-based interface index.
*   AdminState: Is this interface enabled ?
*   HWState   : Is this interface physically ready ?
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
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
* Function description
*   Main task executed by the RTOS to create further resources and
*   running the main application.
*/
void MainTask(void) {
  static char _PTRAddr[32];
#if IP_SUPPORT_IPV6
         U8   NumAddr;
#endif
         U32  IPAddr;

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  //
  // Configures the discover sample. Start with the host name.
  //
  _DiscoverConfig.sHostname    = "server-test.local";
  _DiscoverConfig.TTL          = 120u;
  _DiscoverConfig.apSDConfig   = &_SDConfig[0];
  //
  // No need to have A and AAAA records for the server. But could be added for other
  // names. Set another name at the current IP address.
  //
  _SDConfig[0].Type                   = IP_DNS_SERVER_TYPE_A;
  _SDConfig[0].TTL                    = 0;                       // Use the main TTL value.
  _SDConfig[0].Config.A.sName         = "server-test2.local";
  IPAddr                              = IP_GetIPAddr(_IFaceId);
  _SDConfig[0].Config.A.IPAddr        = IPAddr;
  _SDConfig[0].Flags                  = IP_DNS_SERVER_FLAG_FLUSH;
  //
#if IP_SUPPORT_IPV6
  _SDConfig[1].Type                   = IP_DNS_SERVER_TYPE_AAAA;
  _SDConfig[1].TTL                    = 0;
  _SDConfig[1].Config.A.sName         = "server-test2.local";
  IP_IPV6_GetIPv6Addr(_IFaceId, 0, (IPV6_ADDR*)&_SDConfig[1].Config.AAAA.aIPAddrV6[0], &NumAddr);
  _SDConfig[1].Flags                  = IP_DNS_SERVER_FLAG_FLUSH;
#else
  _SDConfig[1].Type                   = IP_DNS_SERVER_TYPE_A;
  _SDConfig[1].TTL                    = 90;
  _SDConfig[1].Config.A.sName         = "server-test3.local";
  _SDConfig[1].Config.A.IPAddr        = IPAddr;
  _SDConfig[1].Flags                  = IP_DNS_SERVER_FLAG_FLUSH;
#endif
  //
  // Add a PTR record with the IP address..
  //
  SEGGER_snprintf(_PTRAddr, 32, "%d.%d.%d.%d.in-addr.arpa", (IPAddr & 0xFFu), ((IPAddr >> 8) & 0xFFu), ((IPAddr >> 16) & 0xFFu), ((IPAddr >> 24) & 0xFFu));
  _SDConfig[2].Type                   = IP_DNS_SERVER_TYPE_PTR;
  _SDConfig[2].TTL                    = 0;
  _SDConfig[2].Config.PTR.sName       = _PTRAddr;
  _SDConfig[2].Config.PTR.sDomainName = "server-test.local";    // If set to NULL, the host name will be used by default.
  //
  // Add a first TXT record.
  //
  _SDConfig[3].Type                   = IP_DNS_SERVER_TYPE_TXT;
  _SDConfig[3].TTL                    = 0;
  _SDConfig[3].Config.TXT.sName       = NULL;                   // Use the host name.
  _SDConfig[3].Config.TXT.sTXT        = "Key=Value 1";
  //
  // Add a second TXT record. All TXT records for the same sName shall be continuous.
  //
  _SDConfig[4].Type                   = IP_DNS_SERVER_TYPE_TXT;
  _SDConfig[4].TTL                    = 0;
  _SDConfig[4].Config.TXT.sName       = NULL;                   // Use the host name.
  _SDConfig[4].Config.TXT.sTXT        = "Key2=Value 2";
  //
  // Add a SRV record.
  //
  _SDConfig[5].Type                   = IP_DNS_SERVER_TYPE_SRV;
  _SDConfig[5].TTL                    = 0;
  _SDConfig[5].Config.SRV.sName       = "_http._tcp.local";
  _SDConfig[5].Config.SRV.Priority    = 0;
  _SDConfig[5].Config.SRV.Weight      = 0;
  _SDConfig[5].Config.SRV.Port        = 80;
  _SDConfig[5].Config.SRV.sTarget     = NULL;                   // Use the host name.
  //
  // Add another PTR record.
  //
  _SDConfig[6].Type                   = IP_DNS_SERVER_TYPE_PTR;
  _SDConfig[6].TTL                    = 0;
  _SDConfig[6].Config.PTR.sName       = "_http._tcp.local";
  _SDConfig[6].Config.PTR.sDomainName = "server-test._http._tcp.local";
  //
  // Add another SRV record.
  //
  _SDConfig[7].Type                   = IP_DNS_SERVER_TYPE_SRV;
  _SDConfig[7].TTL                    = 0;
  _SDConfig[7].Config.SRV.sName       = "server-test._http._tcp.local";
  _SDConfig[7].Config.SRV.Priority    = 0;
  _SDConfig[7].Config.SRV.Weight      = 0;
  _SDConfig[7].Config.SRV.Port        = 80;
  _SDConfig[7].Config.SRV.sTarget     = NULL;                   // Use the host name.
  //
  // Add a first TXT record.
  //
  _SDConfig[8].Type                   = IP_DNS_SERVER_TYPE_TXT;
  _SDConfig[8].TTL                    = 0;
  _SDConfig[8].Config.TXT.sName       = "server-test._http._tcp.local";
  _SDConfig[8].Config.TXT.sTXT        = "PATH=/";
  //
  _DiscoverConfig.NumConfig           = 9;
  //
  // Start the mDNS server.
  //
  IP_MDNS_SERVER_Start(&_DiscoverConfig);
  //
  // If the target is configured as DHCP server, it is also possible to start a simple
  // DNS server with the following command:
  //
//  IP_DNS_SERVER_Start(&_DiscoverConfig);
  //
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
