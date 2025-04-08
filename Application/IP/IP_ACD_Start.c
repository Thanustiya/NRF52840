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
----------------------------------------------------------------------
Purpose : Sample program for emNet with ACD
          Demonstrates use of the IP stack to automatically detect
          and solve IP address collisions in a network.

          The sample is intended to be used with a fixed IP. To fully
          test this sample the following criteria have to be met:
            - A fixed IP addr. has to be configured in the IP_X_Config()
            - Another or the same fixed IP has to be configured for the defines
              IP_ADDR and SUBNET_MASK in this sample. If the fixed IP in IP_X_Config()
              is not available, this sample will test IPs starting at IP_ADDR.
            - A PC configured to the same IP addr. as for this target.

          Test procedure:
            - Make sure the other device with the same IP addr. as configured in IP_X_Config()
              for the target is available in the network.
            - Run the sample, the other host will be detected and the IP addr. IP_ADDR
              will be tested and used if free. If it is not free, IP_ADDR is incremented.
              This is to test collision detection on ACD startup.
            - Reconfigure the other device to use the new IP addr. of the target to test
              ACD while running. On collision detection the IP addr. of the
              target is incremented again.

          Notes:
            - Avoiding a conflict by choosing a new IP addr. is the easiest
              way. Defending the IP addr. can be done as well but a strategy
              to defend has to be implemented by the user itself.

          The following is a sample of the output to the terminal window
          of the test procedure described above:

          MainTask - ACD: IP addr. for interface 0 declined: 192.168.88.88 already in use.
          MainTask - ACD: Restart testing with 192.168.88.89 for interface 0.
          MainTask - ACD: IP addr. checked, no conflicts. Using 192.168.88.89 for interface 0.
          IP_Task - ACD: IP conflict detected for interface 0!
          IP_Task - *** Warning *** ACD: Conflicting host: 00:0C:29:56:3B:77 uses IP: 192.168.88.89
          IP_Task - ACD: Send gratuitous ARP to defend 192.168.88.89.
          IP_Task - ACD: IP conflict detected for interface 0!
          IP_Task - ACD: IP addr. checked, no conflicts. Using 192.168.88.90 for interface 0.
--------- END-OF-HEADER --------------------------------------------*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK         0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.
#define SHOW_DEBUG_INFO     0  // Output debug information to visualize state changes.

//
// ACD sample.
//
#define IP_ADDR             "192.168.88.88"                  // Start IP addr. for first ACD test.
#define SUBNET_MASK         "255.255.0.0"                    // Subnet mask.
#define GATEWAY_ADDR        "0.0.0.0"                        // Gateway IP addr. if needed.

#define NUM_PROBES          5           // Number of probes to be sent out to check on startup that the used IP is not in use
#define DEFEND_TIME         5000        // How long [ms] to defend a bound IP addr. against an other host while ACD is active

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

#if SHOW_DEBUG_INFO
static I32                     _LastDebugInfoTime;
#endif

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
*       ACD configuration
*/
static IP_ACD_EX_CONFIG _ACD_Config = {
  .IPAddr                = 0u,                    // StartAddr in host endianess, 0 to use current interface addr.
  .BackgroundPeriod      = 0,                     // BackgroundPeriod [ms], 0 to not continue in background.
  .NumProbes             = NUM_PROBES,            // NumProbes
  .DefendInterval        = DEFEND_TIME,           // DefendInterval [ms]
  .NumAnnouncements      = 0u,                    // NumAnnouncements, number of announcements to send when using a free address. The address can already be used at this point.
                                                  // 0 to use default (typically 2 announcements are sent).
  .AnnounceInterval      = 0u,                    // AnnounceInterval, time [ms] between announcements to send.
                                                  // 0 to use default (typically 2s).
  .AssignAddressManually = 0u,                    // AssignAddressManually, 0 to let the ACD modules assign a free address automatically as soon as possible.
  .InitState             = IP_ACD_STATE_DISABLED  // Initial ACD state to start in when being activated. See documentation for supported initial states.
};

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
*       Static code
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

#if SHOW_DEBUG_INFO

#define IP_ACD_STATE_TO_STR(s) (State==(s)) ? #s

/*********************************************************************
*
*       IP_ACD_State2Str()
*
*  Function description
*    Converts the ACD numercial state to a readable string by simply
*    using the defines name.
*
*  Parameters
*    State: IP_ACD_STATE state.
*
*  Return value
*    Pointer to string of the define name.
*/
const char* IP_ACD_State2Str(IP_ACD_STATE State);
const char* IP_ACD_State2Str(IP_ACD_STATE State) {
  return (
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_DISABLED                    ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_INIT_WAIT_BEFORE_PROBE      ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_INIT_WAIT_FOR_COLLISION     ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_INIT_WAIT_BEFORE_ANNOUNCE   ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_ANNOUNCE_SEND_GARP          ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_ACTIVE_WAIT_BEFORE_BG_PROBES) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_ACTIVE_SEND_BG_PROBES       ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_PASSIVE_WAIT_FOR_COLLISION  ) :
    IP_ACD_STATE_TO_STR(IP_ACD_STATE_COLLISION                   ) :
    "unknown"
  );
}

/*********************************************************************
*
*       _PrintDebugInfo()
*
*  Function description
*    Print verbose debug information about what happens with ACD.
*
*  Parameters
*    IFaceId : Zero-based interface index.
*    pInfo   : Further information of type IP_ACD_INFO about the actual
*              information available.
*/
static void _PrintDebugInfo(unsigned IFaceId, IP_ACD_INFO* pInfo) {
  I32 t0;
  I32 tDiff;

  IP_AddLogFilter(IP_MTYPE_APPLICATION);

  t0                 = (I32)OS_GetTime32();
  tDiff              = t0 - _LastDebugInfoTime;
  _LastDebugInfoTime = t0;
  if (tDiff < 0) {
    tDiff *= -1;
  }
  IP_Logf_Application("[+%6d ms] IFace #%u: State \"%s\" => \"%s\"", tDiff
                                                                   , IFaceId
                                                                   , IP_ACD_State2Str(pInfo->OldState)
                                                                   , IP_ACD_State2Str(pInfo->State)
                     );
}
#endif

/*********************************************************************
*
*       _cbOnInfo()
*
*  Function description
*    Callback executed whenever updated ACD information is available.
*
*  Parameters
*    IFaceId : Zero-based interface index.
*    pInfo   : Further information of type IP_ACD_INFO about the actual
*              information available.
*
*  Additional information
*    Calling API like an ACTIVATE from the callback might produce
*    another callback message. It is the responsibility of the application
*    to avoid infinite recursion. Typically this is no problem as calling
*    ACTIVATE again from the callback reporting the ACTIVATE state makes
*    no sense.
*/
static void _cbOnInfo(unsigned IFaceId, IP_ACD_INFO* pInfo) {
  IP_ACD_STATE State;
  IP_ACD_STATE OldState;

#if SHOW_DEBUG_INFO
  _PrintDebugInfo(IFaceId, pInfo);
#else
  IP_USE_PARA(IFaceId);
#endif

  State    = pInfo->State;
  OldState = pInfo->OldState;

  if (State == IP_ACD_STATE_COLLISION) {
    if ((OldState > IP_ACD_STATE_MODE_INIT_BEGIN) && (OldState < IP_ACD_STATE_MODE_INIT_END)) {
      //
      // Collision during INIT (new tested address was not free).
      //
      _ACD_Config.IPAddr++;                                     // Restart INIT probing with next IP address.
      IP_ACD_ActivateEx(IFaceId, _cbOnInfo, &_ACD_Config, 0u);  // Can be called "blocking" as it is always treated
                                                                // non-blocking from the ACD callback anyhow.
    } else {
      //
      // Collision during run-time (after INIT, using an address that seemed to be free).
      //
      if (pInfo->Defend == IP_ACD_DEFEND_ADDRESS) {  // First collision with a host ?
        //
        // RFC 5227 2.4c suggest to defend once for an AFTER-INIT collision.
        // If RFC 5227 2.4c is the desired implementaiton behavior, nothing
        // else needs to be done here.
        //
        // The actual behavior can be influenced by overriding pInfo->Defend
        // and pInfo->DiscardPacket as they are evaluated by the stack once
        // returning from this callback. For more information about the values
        // that can be written, please refer to the IP_ACD_INFO structure.
        //
      } else {                                       // Every other collision with a host within our defend window.
        //
        // Whether to defend or not to defend an address currently in use
        // and INIT probed to be free before can be configured using the
        // pInfo->Defend and pInfo->DiscardPacket . For more information
        // about the values that can be written, please refer to the IP_ACD_INFO structure.
        //
        _ACD_Config.IPAddr++;                                     // Accept to loose the current IP address and use next.
        IP_ACD_ActivateEx(IFaceId, _cbOnInfo, &_ACD_Config, 0u);  // Can be called "blocking" as it is always treated
                                                                  // non-blocking from the ACD callback anyhow.
      }
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
*       MainTask
*/
void MainTask(void) {
  U32 TargetAddr;
  int r;

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  //
  // Enhance log filter to output ACD related status messages in debug builds
  //
  IP_AddLogFilter(IP_MTYPE_ACD);
  //
  // Wait for link to become ready
  //
  while (IP_GetCurrentLinkSpeedEx(_IFaceId) == 0) {
    OS_Delay(10);
  }
  //
  // Enable ACD configuration.
  //
  r = IP_IPV4_ParseIPv4Addr(IP_ADDR, &TargetAddr);
  if (r < 0) {
    IP_PANIC("Illegal IP Address.");
  }
  _ACD_Config.IPAddr = htonl(TargetAddr);
  IP_ACD_ActivateEx(_IFaceId, _cbOnInfo, &_ACD_Config, 0u);  // Call with blocking API aka wait for ACD probes to finish.
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay (200);
  }
}

/*************************** End of file ****************************/
