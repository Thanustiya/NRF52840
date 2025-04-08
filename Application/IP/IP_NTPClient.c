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
          Demonstrates time synchronization from NTP servers.
          At least one NTP clock must be in the local network
          and/or a DNS server must be present to resolve
          the pool address.

          The application will print the current time every 5s.

          The NTP client can be used with NTP_USE_SIMPLER_VERSION
          flag set to 0 or 1.

          The NTP client needs the flag IP_SUPPORT_PACKET_TIMESTAMP
          to be set.

Notes   : For compatibility with interfaces that need to connect in
          any way this sample calls connect and disconnect routines
          that might not be needed in all cases.

          This sample can be used for Ethernet and dial-up interfaces
          and is configured to use the last registered interface as
          its main interface.

Example output:
          ...
          8:025 NTPClient - --- UTC time: 2022.09.05  09:39:01 ---
          13:025 NTPClient - --- UTC time: 2022.09.05  09:39:06 ---
          18:025 NTPClient - --- UTC time: 2022.09.05  09:39:11 ---
          ...
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
   TASK_PRIO_IP_NTPC = 150
  ,TASK_PRIO_IP_TASK         // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK      // Must be the highest priority of all IP related tasks.
#endif
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD     0
#endif

#if IP_SUPPORT_PACKET_TIMESTAMP == 0
  #error "Packet timestamp IP_SUPPORT_PACKET_TIMESTAMP needs to be activated"
#endif

#define JAN_2017   3692217600uL  // EPOCH is 1st Jan 1900.

#define POOL_0     "0.pool.ntp.org"
#define POOL_1     "1.pool.ntp.org"

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
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];              // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                     // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];         // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                   // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _NTPcStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];   // Stack of the NTP client.
static OS_TASK         _NTPcTCB;                                                   // Task-Control-Block of the NTP client.

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
*       _PrettyPrint()
*
* Function description
*   Print the current date and time.
*
* Parameters
*   Status : Status of the API call.
*   Seconds: Number of seconds since the EPOCH.
*/
static void _PrettyPrint(int Status, U32 Seconds) {
  static const unsigned DaysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  static const unsigned DaysPerYear[]  = {365 /*2017*/, 365 /*2018*/, 365 /*2019*/, 366 /*2020*/};
               U32       Minutes;
               U32       Hours;
               U32       Days;
               U32       Month;
               U32       Year;
               unsigned  i;

  if (Status != 0) {
    IP_Logf_Application("Not synced");
  } else {
    Seconds -= JAN_2017;
    Minutes  = Seconds / 60;
    Seconds -= Minutes * 60;
    Hours    = Minutes / 60;
    Minutes -= Hours * 60;
    Days     = Hours / 24;
    Hours   -= Days * 24;
    Year     = 2017;
    //
    i = 0;
    for (;;) {
      if (Days < DaysPerYear[i]) {
        break;
      }
      Days -= DaysPerYear[i];
      i++;
      Year++;
      if (i >= 4u) {
        i = 0;
      }
    }
    //
    for (Month = 0u; Month < 12u; Month++) {
      i = 0u;
      //
      // Check the leap year.
      //
      if (Month == 1u) {
        if (DaysPerYear[(Year - 2017) % 4u] == 366) {
          i = 1u;
        }
      }
      if (Days < (DaysPerMonth[Month] + i)) {
        break;
      }
      Days -= DaysPerMonth[Month] + i;
    }
    //
    IP_Logf_Application("--- UTC time: %d.%02d.%02d  %02d:%02d:%02d ---", Year, Month + 1, Days + 1, Hours, Minutes, Seconds);
  }
}

/*********************************************************************
*
*       _NTPcTask()
*
* Function description
*   Client task requesting the current timestamp from an NTP server
*   and disconnecting the interface if possible.
*/
static void _NTPcTask(void) {
  IP_NTP_TIMESTAMP Timestamp;
  I32              Time;
  int              Status;


  //
  // Wait until link is up and interface is configured.
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  //
  // Start the NTP client and configure some pool.
  //
  IP_NTP_CLIENT_Start();
  IP_NTP_CLIENT_AddServerPool(_IFaceId, POOL_0);
  IP_NTP_CLIENT_AddServerPool(_IFaceId, POOL_1);
  Time = IP_OS_GetTime32() + 5000;
  //
  for (;;) {
    //
    // Run the NTP state machine.
    //
    IP_NTP_CLIENT_Run();
    //
    // Check NTP time every 5 seconds.
    //
    if (IP_IsExpired(Time) != 0) {
      Time += 5000;
      //
      // Get the current NTP time.
      //
      Status = IP_NTP_GetTimestamp(&Timestamp);
      //
      // Print the current time if the synchro with server clocks is done.
      //
      _PrettyPrint(Status, Timestamp.Seconds);
    }
    OS_Delay(10);
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
  IP_Init();
  IP_AddLogFilter(IP_MTYPE_APPLICATION);
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                               // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                       // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB   , "IP_Task"   , IP_Task   , TASK_PRIO_IP_TASK   , _IPStack);     // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB , "IP_RxTask" , IP_RxTask , TASK_PRIO_IP_RX_TASK, _IPRxStack);   // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_NTPcTCB, "NTPClient", _NTPcTask, TASK_PRIO_IP_NTPC  , _NTPcStack);       // Start the NTP client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                    // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                     // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
