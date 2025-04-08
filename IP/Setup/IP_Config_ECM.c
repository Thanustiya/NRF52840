/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device * USB Device stack for embedded applications    *
*                                                                    *
*       Please note: Knowledge of this file may under no             *
*       circumstances be used to write a similar product.            *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device version: V3.66.0                                *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
Purpose : Configuration file for emNet via USB ECM.

          The Ethernet Control Model (ECM) is one of the
          Communication Device Class protocols defined by usb.org to
          create a virtual Ethernet connection between a USB
          device and a host PC.

          Any OS_IP_* sample application can be used with this
          configuration to run via USB.
-------------------------- END-OF-HEADER -----------------------------
*/

#include "BSP.h"
#include "IP.h"
#include "USB_ECM.h"
#include "USB_Driver_embOS_IP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define ALLOC_SIZE                 0x6000                      // Size of memory dedicated to the stack in bytes
#define DRIVER                     &USB_IP_Driver              // Driver used for target.

#ifndef USBD_USE_FIXED_IP
  #define USBD_USE_FIXED_IP 0
#endif
//
// As soon as we are enumerated, we know that the USB host has assigned a USB address to us.
// We will use this USB address in order to assign the emNet interface a IP address.
// The IP address is selected from a so-called IP pool for CGN (carrier grade NAT) - RFC6598
// The IP pool for this is 100.64.0.0/10.
// We will use 100.127.<USBAddr>.0/29 -> gives us 8 IP addresses, whereas 6 are assignable.
// This is enough for a client/host configuration.
// Therefore we will have the following scenario
// emNet interface: IP: 100.127.<USBAddr>.1
// Host interface: 100.127.<USBAddr>.2 - assigned by our DHCP server.
// SubnetMask = 255.255.255.248
// DHCP Server starts assigns addresses from 100.128.<USBAddr>.2 with a range of 4 addresses.
//
// If this conflicts with any software on your USB host system, be free to change it to
// a more convenient address pool that would fit your USB host system.
//
#if USBD_USE_FIXED_IP == 0
  #define SERVER_IP_ADDR(x)                  IP_BYTES2ADDR(100, 127, x, 1)
  #define DNS_SERVER_ADDR(x)                 IP_BYTES2ADDR(100, 127, x, 1)
  #define SUBNET_MASK                        IP_BYTES2ADDR(255, 255, 255, 248)
  #define DHCP_START_IP_ADDR(x)              IP_BYTES2ADDR(100, 127, x, 2)
  #define DHCP_NUM_ADDRS                     4
#else
  //
  // This can be used when you do not want your target to change IPs depending on USBAddr.
  //
  #define SERVER_IP_ADDR(x)                  IP_BYTES2ADDR(10,  0, 0, 10)
  #define DNS_SERVER_ADDR(x)                 IP_BYTES2ADDR(10,  0, 0, 10)
  #define SUBNET_MASK                        IP_BYTES2ADDR(255, 0, 0,  0)
  #define DHCP_START_IP_ADDR(x)              IP_BYTES2ADDR(10,  0, 0, 11)
  #define DHCP_NUM_ADDRS                     4
#endif

#if 0
  //
  // For use with EcoGamma driver on windows
  //
  #define _USB_VENDOR_ID     0x085F
  #define _USB_PRODUCT_ID    0x2130
#else
  //
  // For use on linux or with Thesycon driver on windows
  //
  #define _USB_VENDOR_ID     0x8765
  #define _USB_PRODUCT_ID    0x3003
#endif
#define _USB_VENDOR_NAME   "SEGGER"
#define _USB_PRODUCT_NAME  "SEGGER ECM device"
#define _USB_SERIAL_NO     "13245678"

//
// The serial define can be manipulated for each individual target.
// This allows multiple targets to be connected to the same PC.
//
// In the default configuration of this sample the target would be available
// through the URL usb.local or usb01.local
//
#define SERVER_NAME_SERIAL          "01"
#define SERVER_NAME                 "usb"
#define SERVER_NAME2                SERVER_NAME SERVER_NAME_SERIAL


static IP_DNS_SERVER_SD_CONFIG _aSDConfig[] = {
  //
  // Secondary entry, main entry is automatically created through _DNSConfig.sHostname
  //
  { .Type = IP_DNS_SERVER_TYPE_A,
    .TTL = 0,
    .Config.A = {
      .sName  = SERVER_NAME2,
      .IPAddr = 0, // Set automatically.
    }
  },
  #if IP_SUPPORT_IPV6
  { .Type = IP_DNS_SERVER_TYPE_AAAA,
    .TTL = 0,
    .Config.AAAA = {
      .sName = SERVER_NAME2,
      .aIPAddrV6 = {0}, // Set automatically.
    }
  }
  #endif
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Information that are used during enumeration
*/
static const USB_DEVICE_INFO _DeviceInfo = {
  _USB_VENDOR_ID,
  _USB_PRODUCT_ID,
  _USB_VENDOR_NAME,
  _USB_PRODUCT_NAME,
  _USB_SERIAL_NO
};


//
// USB ECM related variables
//
static U32  _abReceiveBuffer[512 * 3 / sizeof(U32)]; // OUT endpoint has to be able to contain one full Ethernet packet!
//
// IP generic related
//
static U32 _aPool[ALLOC_SIZE / 4];            // This is the memory area used by the IP stack.
static unsigned   _IFaceId;
static IP_HOOK_ON_STATE_CHANGE _Hook;
static IP_DNS_SERVER_CONFIG  _DNSConfig;

enum {
  IP_SET_ADDRESS_MASK_OPERATION,
  IP_DHCPS_CONFIGPOOL_OPERATION,
  IP_DHCPS_CONFIGDNSADDR_OPERATION,
  IP_DHCPS_START_OPERATION,
  IP_MDNS_SERVER_START_OPERATION,
  IP_DNS_SERVER_START_OPERATION,
  IP_DHCPS_HALT_OPERATION,
  IP_MDNS_SERVER_STOP_OPERATION,
  IP_DNS_SERVER_STOP_OPERATION,
  IP_MAX_OPERATION
} DelayedOperation;

static IP_EXEC_DELAYED _aIPExecDelayed[IP_MAX_OPERATION];

static U32                    _DNSServer;
static IP_EXEC_DELAYED_PARA_3 _SetAddrMaskExPara;
static IP_EXEC_DELAYED_PARA_4 _DHCPS_ConfigPoolPara;
static IP_EXEC_DELAYED_PARA_3 _DHCPS_ConfigDNSAddPara;


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _AddECM
*
*  Function description
*    Adds and configures ECM communication device class to USB stack.
*/
static void _AddECM(void) {
  USB_ECM_INIT_DATA InitData;

  IP_MEMSET(&InitData, 0, sizeof(InitData));
  InitData.EPOut      = USBD_AddEP(USB_DIR_OUT, USB_TRANSFER_TYPE_BULK, 0, (U8 *)_abReceiveBuffer, sizeof(_abReceiveBuffer));
  InitData.EPIn       = USBD_AddEP(USB_DIR_IN,  USB_TRANSFER_TYPE_BULK, 0, NULL, 0);
  InitData.EPInt      = USBD_AddEP(USB_DIR_IN,  USB_TRANSFER_TYPE_INT, 32, NULL, 0);
  InitData.pDriverAPI = &USB_Driver_IP_NI;
  InitData.DriverData.pDriverData    = (void *)_IFaceId;
  //
  // Not used as the NI module itself can generate a MAC address itself.
  // If this is necessary to use some fix addresses, please uncomment the next two line
  // and assign the Host Ethernet Interface a MAC Id.
  //
  //InitData.DriverData.NumBytesHWAddr = 6;                           // Number of bytes in the HW address. Typically 6 bytes.
  //InitData.DriverData.pDriverData = "\xDE\xAD\xBE\xEF\x0B\xAD";     // Pointer to a user context.
  USBD_ECM_Add(&InitData);
}

/*********************************************************************
*
*       _OnDone
*
*  Function description
*    Is called when the operation has been completed.
*/
static void _OnDone(IP_EXEC_DELAYED* pED, void* pContext) {
  IP_USE_PARA(pED);
  IP_USE_PARA(pContext);
  IP_LOG((IP_MTYPE_INIT, "%s done", pContext));
}

/*********************************************************************
*
*       _UpdateIPServiceOperation
*
*  Function description
*    Starts and stops the dedicated server services for IP depending on the link state.
*/
static void _UpdateIPServiceOperation(unsigned IFaceId, unsigned LinkStatus) {
  U32 ServerIpAddr;
  U32 SubnetMask;
#if USBD_USE_FIXED_IP == 0
  U8  USBAddr;
#endif

  if (IFaceId == _IFaceId) {
    if (LinkStatus) {
      //
      // Setup the emNET own IP address (aka SERVER) and subnet mask
      //
#if USBD_USE_FIXED_IP == 0
      USBAddr = USBD_GetUSBAddr();
#endif
      ServerIpAddr = SERVER_IP_ADDR(USBAddr);
      SubnetMask = SUBNET_MASK;
      //
      // Configure the delayed exec parameters
      //
      //
      // IP_SetAddrMaskEx parameters
      //
      _SetAddrMaskExPara.Para0  = SEGGER_PTR2ADDR(IFaceId);
      _SetAddrMaskExPara.Para1  = SEGGER_PTR2ADDR(ServerIpAddr);
      _SetAddrMaskExPara.Para2  = SEGGER_PTR2ADDR(SubnetMask);
      //
      // IP_DHCPS_ConfigPool parameters
      //
      _DHCPS_ConfigPoolPara.Para0 = SEGGER_PTR2ADDR(IFaceId);
      _DHCPS_ConfigPoolPara.Para1 = SEGGER_PTR2ADDR(DHCP_START_IP_ADDR(USBAddr));
      _DHCPS_ConfigPoolPara.Para2 = SEGGER_PTR2ADDR(SubnetMask);
      _DHCPS_ConfigPoolPara.Para3 = SEGGER_PTR2ADDR(DHCP_NUM_ADDRS);
      //
      // IP_DHCPS_ConfigDNS parameters
      //
      _DNSServer = DNS_SERVER_ADDR(USBAddr);
      _DHCPS_ConfigDNSAddPara.Para0 = SEGGER_PTR2ADDR(IFaceId);
      _DHCPS_ConfigDNSAddPara.Para1 = SEGGER_PTR2ADDR(&_DNSServer);
      _DHCPS_ConfigDNSAddPara.Para2 = SEGGER_PTR2ADDR(1);
      //
      // IP_DHCPS_Start(m)DNS parameters
      //
      IP_MEMSET(&_DNSConfig, 0, sizeof(_DNSConfig));
      _DNSConfig.sHostname    = SERVER_NAME;
      _DNSConfig.TTL          = 60;
      _DNSConfig.apSDConfig   = _aSDConfig;
      _DNSConfig.NumConfig    = SEGGER_COUNTOF(_aSDConfig);
      //
      // Queue now the the delayed exec commands
      //
      // Assign a local address 100.127.<USBAddr>.1/8
      IP_ExecDelayed(&_aIPExecDelayed[IP_SET_ADDRESS_MASK_OPERATION],
                    IP_SetAddrMaskEx_Delayed,
                    &_SetAddrMaskExPara,
                    (void *)"SetAddrMaskEx",
                    _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_DHCPS_CONFIGPOOL_OPERATION],
                     IP_DHCPS_ConfigPool_Delayed,
                     &_DHCPS_ConfigPoolPara,
                     (void *)"IP_DHCPS_ConfigPool",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_DHCPS_CONFIGDNSADDR_OPERATION],
                     IP_DHCPS_ConfigDNSAddr_Delayed,
                     &_DHCPS_ConfigDNSAddPara,
                     (void *)"IP_DHCPS_ConfigDNSAddr",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_DHCPS_START_OPERATION],
                     IP_DHCPS_Start_Delayed,
                     SEGGER_ADDR2PTR(void, _IFaceId),
                     (void *)"IP_DHCPS_Start",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_MDNS_SERVER_START_OPERATION],
                     IP_MDNS_SERVER_Start_Delayed,
                     &_DNSConfig,
                     (void *)"IP_MDNS_SERVER_Start",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_DNS_SERVER_START_OPERATION],
                     IP_DNS_SERVER_Start_Delayed,
                     &_DNSConfig,
                     (void *)"IP_DNS_SERVER_Start",
                     _OnDone);
    }
    if (LinkStatus == 0) {
      IP_ExecDelayed(&_aIPExecDelayed[IP_DHCPS_HALT_OPERATION],
                     IP_DHCPS_Halt_Delayed,
                     SEGGER_ADDR2PTR(void, _IFaceId),
                     (void *)"IP_DHCPS_Halt",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_MDNS_SERVER_STOP_OPERATION],
                     IP_MDNS_SERVER_Stop_Delayed,
                     NULL,
                     (void *)"IP_MDNS_SERVER_Stop",
                     _OnDone);
      IP_ExecDelayed(&_aIPExecDelayed[IP_DNS_SERVER_STOP_OPERATION],
                     IP_DNS_SERVER_Stop_Delayed,
                     NULL,
                     (void *) "IP_DNS_SERVER_Stop",
                     _OnDone);
    }
  }
}

/*********************************************************************
*
*       _Connect()
*
* Function description
*   Performs a first time initialization for the USB IP.
*
* Parameters
*   IFaceId: Zero-based interface index.
*
* Return value
*   O.K. :   0.
*   Error: < 0.
*/
static int _Connect(unsigned IFaceId) {
  //
  // Initialize the DHCP Server for this interface.
  //
  IP_DHCPS_Init(IFaceId);
  //
  // Initialize the USB stack
  // and prepare to use the IP-over-USB connection
  //
  USBD_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  //
  // Enable IAD makes sure that the device is properly enumerated on Windows 7 on USB 3 Controller
  //
  USBD_EnableIAD();
  _AddECM();
  USBD_Start();
  return 0;                   // Successfully connected.
}

/*********************************************************************
*
*       _OnHWChange
*
*  Function description
*    Signals the IP Service Task that the connection state has been changed.
*    Based on that the IP Service Task then configure and starts
*    or stop the dedicated Services.
*    In the current stat these are:
*         + DHCP Server
*         + MDNS Server
*         + DNS Server
*/
static void _OnHWChange(unsigned IFaceId, U8 AdminState, U8 HWState) {
  unsigned IsReady;

  IP_USE_PARA(AdminState);
  IsReady = (HWState == 1 && AdminState == 1) ? 1 : 0;
  _UpdateIPServiceOperation(IFaceId, IsReady);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       IP_X_Config
*
*  Function description
*    This function is called by the IP stack during IP_Init().
*
*  Typical memory/buffer configurations:
*    Microcontroller system, size optimized
*      #define ALLOC_SIZE 0x3000                    // 12KBytes RAM
*      mtu = 576;                                   // 576 is minimum acc. to RFC, 1500 is max. for Ethernet
*      IP_SetMTU(0, mtu);                           // Maximum Transmission Unit is 1500 for ethernet by default
*      IP_AddBuffers(8, 256);                       // Small buffers.
*      IP_AddBuffers(4, mtu + 16);                  // Big buffers. Size should be mtu + 16 byte for ethernet header (2 bytes type, 2*6 bytes MAC, 2 bytes padding)
*      IP_ConfTCPSpace(1 * (mtu-40), 1 * (mtu-40)); // Define the TCP Tx and Rx window size
*
*    Microcontroller system, speed optimized or multiple connections
*      #define ALLOC_SIZE 0x6000                    // 24 KBytes RAM
*      mtu = 1500;                                  // 576 is minimum acc. to RFC, 1500 is max. for Ethernet
*      IP_SetMTU(0, mtu);                           // Maximum Transmission Unit is 1500 for ethernet by default
*      IP_AddBuffers(12, 256);                      // Small buffers.
*      IP_AddBuffers(6, mtu + 16);                  // Big buffers. Size should be mtu + 16 byte for ethernet header (2 bytes type, 2*6 bytes MAC, 2 bytes padding)
*      IP_ConfTCPSpace(3 * (mtu-40), 3 * (mtu-40)); // Define the TCP Tx and Rx window size
*
*    System with lots of RAM
*      #define ALLOC_SIZE 0x20000                   // 128 KBytes RAM
*      mtu = 1500;                                  // 576 is minimum acc. to RFC, 1500 is max. for Ethernet
*      IP_SetMTU(0, mtu);                           // Maximum Transmission Unit is 1500 for ethernet by default
*      IP_AddBuffers(50, 256);                      // Small buffers.
*      IP_AddBuffers(50, mtu + 16);                 // Big buffers. Size should be mtu + 16 byte for ethernet header (2 bytes type, 2*6 bytes MAC, 2 bytes padding)
*      IP_ConfTCPSpace(5 * (mtu-40), 5 * (mtu-40)); // Define the TCP Tx and Rx window size
*/
void IP_X_Config(void) {
  int mtu;
  int IFaceId;

  IP_AssignMemory(_aPool, sizeof(_aPool));         // Assigning memory should be the first thing
  IFaceId = IP_AddEtherInterface(DRIVER);          // Add driver for your hardware.
  IP_SetIFaceConnectHook(IFaceId, _Connect);
  IP_AddStateChangeHook(&_Hook, _OnHWChange);
  _IFaceId = IFaceId;
  //
  // Run-time configure buffers.
  // The default setup will do for most cases.
  //
  mtu = 1500;                                  // 576 is minimum acc. to RFC, 1500 is max. for Ethernet
  IP_SetMTU(IFaceId, mtu);                     // Maximum Transmission Unit is 1500 for ethernet by default
  IP_AddBuffers(12, 256);                      // Small buffers.
  IP_AddBuffers( 9, mtu + 16);                 // Big buffers. Size should be mtu + 16 byte for ethernet header (2 bytes type, 2*6 bytes MAC, 2 bytes padding)
  IP_ConfTCPSpace(3 * (mtu-40), 3 * (mtu-40)); // Define the TCP Tx and Rx window size
  IP_SOCKET_SetDefaultOptions(0
//                              | SO_TIMESTAMP   // Send TCP timestamp to optimize the round trip time measurement. Normally not used in LAN.
                              | SO_KEEPALIVE   // Enable keepalives by default for TCP sockets.
                             );
  //
  // Define log and warn filter.
  // Note: The terminal I/O emulation might affect the timing of your
  //       application, since most debuggers need to stop the target
  //       for every terminal I/O output unless you use another
  //       implementation such as DCC or SWO.
  //
  IP_SetWarnFilter(0xFFFFFFFF);              // 0xFFFFFFFF: Do not filter: Output all warnings.
  IP_SetLogFilter(0
                  | IP_MTYPE_APPLICATION     // Output application messages.
                  | IP_MTYPE_INIT            // Output all messages from init.
                  | IP_MTYPE_LINK_CHANGE     // Output a message if link status changes.
                  | IP_MTYPE_PPP             // Output all PPP/PPPoE related messages.
                  | IP_MTYPE_DHCP            // Output general DHCP status messages.
#if IP_SUPPORT_IPV6
                  | IP_MTYPE_IPV6            // Output IPv6 address related messages
#endif
//                  | IP_MTYPE_DHCP_EXT        // Output additional DHCP messages.
//                  | IP_MTYPE_CORE            // Output log messages from core module.
//                  | IP_MTYPE_ALLOC           // Output log messages for memory allocation.
//                  | IP_MTYPE_DRIVER          // Output log messages from driver.
//                  | IP_MTYPE_ARP             // Output log messages from ARP layer.
//                  | IP_MTYPE_IP              // Output log messages from IP layer.
//                  | IP_MTYPE_TCP_CLOSE       // Output a log messages if a TCP connection has been closed.
//                  | IP_MTYPE_TCP_OPEN        // Output a log messages if a TCP connection has been opened.
//                  | IP_MTYPE_TCP_IN          // Output TCP input logs.
//                  | IP_MTYPE_TCP_OUT         // Output TCP output logs.
//                  | IP_MTYPE_TCP_RTT         // Output TCP round trip time (RTT) logs.
//                  | IP_MTYPE_TCP_RXWIN       // Output TCP RX window related log messages.
//                  | IP_MTYPE_TCP             // Output all TCP related log messages.
//                  | IP_MTYPE_UDP_IN          // Output UDP input logs.
//                  | IP_MTYPE_UDP_OUT         // Output UDP output logs.
//                  | IP_MTYPE_UDP             // Output all UDP related messages.
//                  | IP_MTYPE_ICMP            // Output ICMP related log messages.
//                  | IP_MTYPE_NET_IN          // Output network input related messages.
//                  | IP_MTYPE_NET_OUT         // Output network output related messages.
//                  | IP_MTYPE_DNS             // Output all DNS related messages.
//                  | IP_MTYPE_SOCKET_STATE    // Output socket status messages.
//                  | IP_MTYPE_SOCKET_READ     // Output socket read related messages.
//                  | IP_MTYPE_SOCKET_WRITE    // Output socket write related messages.
//                  | IP_MTYPE_SOCKET          // Output all socket related messages.
                 );
  //
  // Add protocols to the stack.
  //
  IP_TCP_Add();
  IP_UDP_Add();
  IP_ICMP_Add();
#if IP_SUPPORT_IPV6
  IP_IPV6_Add(IFaceId);
#endif
}

/*************************** End of file ****************************/
