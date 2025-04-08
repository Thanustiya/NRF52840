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
          The sample will setup a DHCP server that can assign a network
          configuration to a client requesting one. A DHCP server shall
          be used with a static IP addr. only and shall not request its
          own IP addr. from another DHCP server.

          The following is a sample of the output to the terminal window:

          0:000 MainTask - INIT: Init started. Version 2.13.11
          0:002 MainTask - DRIVER: Found PHY with Id 0x181 at addr 0x1F
          0:003 MainTask - INIT: Link is down
          0:003 MainTask - INIT: Init completed
          0:003 IP_Task - INIT: IP_Task started
          3:000 IP_Task - LINK: Link state changed: Full duplex, 100 MHz
          8:250 IP_Task - DHCPs: DISCOVER from 00:26:9E:E2:C8:44 on IFace 0
          8:250 IP_Task - DHCPs: Lease time [s] requested 0, granted 60
          8:250 IP_Task - DHCPs: OFFER 10.1.0.11
          8:252 IP_Task - DHCPs: REQUEST from 00:26:9E:E2:C8:44 on IFace 0
          8:252 IP_Task - DHCPs: Requested 10.1.0.11
          8:253 IP_Task - DHCPs: Lease time [s] requested 0, granted 60
          8:253 IP_Task - DHCPs: ACK
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

#define USE_RX_TASK          0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// DHCP server sample configuration, mandatory.
//
#define IP_POOL_START_ADDR   IP_BYTES2ADDR(10, 1, 0, 11)  // First addr. of IP pool to assign to clients.
#define IP_POOL_SUBNET_MASK  0xFFFFFF00                   // Subnet mask assigned to clients.
#define IP_POOL_SIZE         20                           // Number of IP addr. in pool starting from IP_POOL_START_ADDR .

//
// DHCP server sample configuration, optional.
//
#define DISABLE_NETBIOS  1                                // Try to disable NetBIOS over TCP/IP for Microsoft clients via
                                                          // DHCP server options. Has to be enabled (default) on the client.

#define DNS_ADDR_0       IP_BYTES2ADDR(10, 1, 0, 1)       // First DNS server to offer to client.
#define DNS_ADDR_1       IP_BYTES2ADDR(10, 1, 0, 2)       // Second DNS server to offer to client.
#define GW_ADDR          IP_BYTES2ADDR(10, 1, 0, 1)       // Gateway to offer to client.
#define MAX_LEASE_TIME   60                               // Max lease time override [s]. Default 2h.

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

#if (DISABLE_NETBIOS != 0)
/*********************************************************************
*
*       _cbDHCPs_AddVendorOptions()
*
*  Function description
*    Adds DHCP vendor specific options to our server replies.
*
*  Parameters
*    IFaceId : Zero-based interface index.
*    pInfo   : Further information about the vendor of the client.
*    ppOption: Pointer to the pointer where to add further options.
*              The dereferenced pointer needs to be incremented
*              by the number of bytes added. Type and length bytes
*              need to be added by the callback as well.
*    NumBytes: Number of free bytes that can be used to store
*              options from the callback.
*/
static void _cbDHCPs_AddVendorOptions(unsigned IFaceId, IP_DHCPS_GET_VENDOR_OPTION_INFO* pInfo, U8** ppOption, unsigned NumBytes) {
  U8*      pVendorClassId;
  U8*      pOption;
  unsigned VendorClassIdLen;

  IP_USE_PARA(IFaceId);

  pOption = *ppOption;  // Get the location where to add our options aka borrow the pointer.
  //
  // Parse the vendor class id.
  //
  pVendorClassId   = pInfo->pVendorClassId;        // Points to the type which should always be DHCP option 60.
  pVendorClassId++;                                // proceed to the length field.
  VendorClassIdLen = (unsigned)*pVendorClassId++;  // Get the length byte and proceed to the actual non-terminated vendor string.
  //
  // Check if the vendor class identifier is known to us.
  //
  if ((IP_MEMCMP(pVendorClassId, "MSFT 5.0", VendorClassIdLen) == 0) &&
      (NumBytes >= 8u)) {  // Also check if we have enough space to add the option.
    //
    // Identified a Microsoft device that supports vendor-specific options.
    // More information about this can be found at the following location:
    //   * https://msdn.microsoft.com/en-us/library/cc227279.aspx
    //
    // Information about the vendor-specific options supported for Microsoft
    // devices can be found here:
    //   * [1] https://msdn.microsoft.com/en-us/library/cc227275.aspx
    //   * [2] https://msdn.microsoft.com/en-us/library/cc227276.aspx
    //
    // A common task is to disable NetBIOS (over TCP/IP) via DHCP
    // if your clients primarily use other techniques and you want
    // to speed up discovery of them by name. Typically one method
    // will be tested after each other which means that each method
    // used costs additional time before your desired discovery
    // method finally might be used.
    //
    *pOption++  =  43u;                    // Add an option field of type 43 "Vendor-Specific Information".
    *pOption++  =   6u;                    // Add length field with value 6 for the actual 6 bytes vendor-specific content.
    *pOption++  = 0x01;                    // [1] "Microsoft Disable NetBIOS Option (section 2.2.2.1)"
    *pOption++  = 0x04;                    // [2] "Vendor-specific Option Length"
    IP_StoreU32BE(pOption, 0x00000002uL);  // [2] "Vendor-specific Option Data" "Disables NetBIOS over TCP/IP for that network interface."
    pOption    += 4;
  }
  *ppOption = pOption;  // Write back the borrowed pointer so the DHCP server internal code knows where to continue.
}
#endif

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
#ifdef DNS_ADDR_0
  U32 aDNSAddr[] = {
     DNS_ADDR_0
#ifdef DNS_ADDR_1
    ,DNS_ADDR_1
#endif
  };
#endif

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                             // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                     // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);    // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                              // Register hook to be notified on disconnects.
  //
  // Do not wait for interface ready as a DHCP server
  // shall only be used with a static IP configured.
  //
  IP_DHCPS_ConfigPool(_IFaceId, IP_POOL_START_ADDR, IP_POOL_SUBNET_MASK, IP_POOL_SIZE);  // Setup IP pool to distribute.
#ifdef GW_ADDR
  IP_DHCPS_ConfigGWAddr(_IFaceId, GW_ADDR);                                              // Configure default GW addr. if any. Optional.
#endif
#ifdef DNS_ADDR_0
  IP_DHCPS_ConfigDNSAddr(_IFaceId, &aDNSAddr[0], SEGGER_COUNTOF(aDNSAddr));              // Configure DNS addr. if any. Optional.
#endif
#ifdef MAX_LEASE_TIME
  IP_DHCPS_ConfigMaxLeaseTime(_IFaceId, MAX_LEASE_TIME);                                 // Configure maximum lease time to grant. Default is 2h.
#endif
  IP_DHCPS_Init(_IFaceId);                                                               // Initialize server.
#if (DISABLE_NETBIOS != 0)
  IP_DHCPS_SetVendorOptionsCallback(_cbDHCPs_AddVendorOptions);                          // Prevent Microsoft clients from using NetBIOS.
#endif
  IP_DHCPS_Start(_IFaceId);                                                              // Start server.
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
