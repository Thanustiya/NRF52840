/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2013 - 2018  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       emModbus * Modbus stack for embedded applications            *
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
*       emModbus version: V1.02h                                     *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : MB_OS_embOS.c
Purpose : Kernel abstraction for embOS. Do not modify to allow easy updates!
*/

#include "MB_Int.h"
#include "RTOS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// The default tick is expected to be 1ms. For a finer tick
// like 1us a multiplicator has to be configured. The tick
// should match the OS tick.
// Examples:
//   - 1ms   = 1
//   - 100us = 10
//   - 10us  = 100
//
#define TICK_MULTIPLICATOR  1  // Default, 1 = 1ms.

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

typedef struct MASTER_WAIT {
  struct MASTER_WAIT* pNext;
  struct MASTER_WAIT* pPrev;
  void*               pWaitItem;
#if (OS_VERSION >= 38606)  // OS_AddOnTerminateHook() is supported since embOS v3.86f .
  OS_TASK*            pTask;
#endif
  OS_EVENT            Event;
} MASTER_WAIT;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static MASTER_WAIT*         _pFirstMasterWait;  // Head of List. One entry per waiting task.
static char                 _NumInits;
static char                 _MasterIsInited;
static char                 _SlaveIsInited;

#if (OS_VERSION >= 38606)  // OS_AddOnTerminateHook() is supported since embOS v3.86f .
static OS_ON_TERMINATE_HOOK _OnTerminateTaskHook;
#endif

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/

OS_EVENT MB_OS_NetEvent;  // Public only to allow inlining (direct call from Modbus-Stack).

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _DLIST_RemoveDelete()
*
*  Function description
*    Removes a waitable object from the doubly linked list and deletes
*    its wait object from embOS lists.
*
*  Parameters
*    pMasterWait: Pointer to element to remove from list.
*
*  Additional information
*    Function is called from MB_OS_WaitItemTimed() and _OnTerminateTask().
*    Calling functions have to make sure that it is not called recursive
*    by disabling task switch before calling this routine.
*/
static void _DLIST_RemoveDelete(MASTER_WAIT* pMasterWait) {
  //
  // Remove entry from doubly linked list.
  //
  if (pMasterWait->pPrev) {
    pMasterWait->pPrev->pNext = pMasterWait->pNext;
  } else {
    _pFirstMasterWait = pMasterWait->pNext;
  }
  if (pMasterWait->pNext) {
    pMasterWait->pNext->pPrev = pMasterWait->pPrev;
  }
  //
  // Delete the event object.
  //
  OS_EVENT_Set(&pMasterWait->Event);  // Set event to prevent error on removing an unsignalled event.
  OS_EVENT_Delete(&pMasterWait->Event);
}

#if (OS_VERSION >= 38606)  // OS_AddOnTerminateHook() is supported since embOS v3.86f .
/*********************************************************************
*
*       _OnTerminateTask()
*
*  Function description
*    This routine removes the registered wait objects from the doubly
*    linked list of wait objects upon termination of its task. This
*    is necessary due to the fact that the element is publically known
*    due to a doubly linked list but is stored on a task stack. In case
*    this task gets terminated we need to gracefully remove the element
*    from all resources and even remove it from any embOS list.
*
*  Parameters
*    pTask: Task handle of task that will be terminated.
*
*  Additional information
*    Function is called from an application task via OS hook with
*    task switching disabled.
*/
static void _OnTerminateTask(OS_CONST_PTR OS_TASK *pTask) {
  MASTER_WAIT* pMasterWait;

  if (_MasterIsInited == 0) {
    return;  // As there is no API to remove the on terminate hook we will simply return here.
  }
  for (pMasterWait = _pFirstMasterWait; pMasterWait; pMasterWait = pMasterWait->pNext) {
    if (pMasterWait->pTask == pTask) {
      //
      // Prior to deleting an event object it needs to be set to be unused
      // (no task waiting for it). Setting the EVENT object is safe as in
      // all cases only one the task that created the object on its stack
      // is waiting for the event and task switching is disabled. Therefore
      // we will stay in this routine and finish our work.
      //
      OS_EVENT_Set(&pMasterWait->Event);
      _DLIST_RemoveDelete(pMasterWait);
      break;
    }
  }
}
#endif

/*********************************************************************
*
*       _DeInit()
*
*  Function description
*    De-Initialize (remove) all objects required for task
*    synchronisation required by both, master and slave if no further
*    reference.
*/
static void _DeInit(void) {
  if (--_NumInits == 0) {  // Is this the last reference ?
    //
    // Prior to deleting an event object it needs to be set to be unused
    // (no task waiting for it). Setting the EVENT object is safe as in
    // all cases only one the task that created the object on its stack
    // is waiting for the event and task switching is disabled. Therefore
    // we will stay in this routine and finish our work.
    //
    OS_EVENT_Set(&MB_OS_NetEvent);
    OS_EVENT_Delete(&MB_OS_NetEvent);
  }
}

/*********************************************************************
*
*       _Init()
*
*  Function description
*    Initialize (create) all objects required for task
*    synchronisation required by both, master and slave.
*    This is one event for waking up tasks waiting for a NET-event to
*    occur.
*/
static void _Init(void) {
  if (_NumInits++ == 0) {  // Is this the first init done ?
    OS_EVENT_Create(&MB_OS_NetEvent);
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
*       MB_OS_DeInitMaster()
*
* Function description
*   De-Initialize (remove) all objects required for task
*   syncronisation of the master.
*/
void MB_OS_DeInitMaster(void) {
  if (_MasterIsInited) {
    _DeInit();
    _MasterIsInited   = 0;
    _pFirstMasterWait = 0;
  }
}

/*********************************************************************
*
*       MB_OS_InitMaster()
*
*  Function description
*    Initialize (create) all objects required for task
*    syncronisation of the master.
*    This is one hook in case a task currently executing Modbus
*    master API is terminated.
*/
void MB_OS_InitMaster(void) {
  if (_MasterIsInited == 0) {
    _Init();
#if (OS_VERSION >= 38606)  // OS_AddOnTerminateHook() is supported since embOS v3.86f .
    OS_AddOnTerminateHook(&_OnTerminateTaskHook, _OnTerminateTask);
#endif
    _MasterIsInited = 1;
  }
}

/*********************************************************************
*
*       MB_OS_DeInitSlave()
*
*  Function description
*    De-Initialize (remove) all objects required for task
*    syncronisation and signalling of the slave.
*/
void MB_OS_DeInitSlave(void) {
  if (_SlaveIsInited) {
    _DeInit();
    _SlaveIsInited = 0;
  }
}

/*********************************************************************
*
*       MB_OS_InitSlave()
*
*  Function description
*    Initialize (create) all objects required for task
*    synchronisation and signalling of the slave.
*    This is one semaphore for protection of critical code which may
*    not be executed from multiple task at the same time.
*/
void MB_OS_InitSlave(void) {
  if (_SlaveIsInited == 0) {
    _Init();
    _SlaveIsInited = 1;
  }
}

/*********************************************************************
*
*       MB_OS_DisableInterrupt()
*
*  Function description
*    Disables interrupts to lock against calls from interrupt routines.
*/
void MB_OS_DisableInterrupt(void) {
  OS_IncDI();
}

/*********************************************************************
*
*       MB_OS_EnableInterrupt()
*
*  Function description
*    Enables interrupts that have previously been disabled.
*/
void MB_OS_EnableInterrupt(void) {
  OS_DecRI();
}

/*********************************************************************
*
*       MB_OS_GetTime()
*
*  Function description
*    Return the current system time in ms.
*    The value will wrap around after app. 49.7 days. This is taken
*    into account by the stack.
*
*  Return value
*    U32 timestamp in system ticks (typically 1ms).
*/
U32 MB_OS_GetTime(void) {
  return OS_GetTime32();
}

/*********************************************************************
*
*       MB_OS_SignalNetEvent()
*
*  Function description
*    Wakes the MB_SLAVE_Task() waiting for a NET-event or timeout in
*    the function MB_OS_WaitNetEvent().
*/
void MB_OS_SignalNetEvent(void) {
  OS_EVENT_Set(&MB_OS_NetEvent);
}

/*********************************************************************
*
*       MB_OS_WaitNetEvent()
*
*  Function description
*    Called from MB_SLAVE_Task() only.
*    Blocks until a NET-event occurs, meaning MB_OS_SignalNetEvent()
*    is called from another task or ISR. The event is expected to be
*    cleared after triggering a waiting object for resume. For embOS
*    wait objects this is the default and we do not need to clear it
*    manually. However it might happen that an interrupt signals the
*    event again just after the waiting task has been signalled,
*    resulting in a second loop through the code. As this typically
*    just means looping over a small linked slave channel list and
*    should only occur rarely this is not a problem.
*
*  Parameters
*    Time to wait for a NET-event to occur in ms. 0 for infinite.
*/
void MB_OS_WaitNetEvent(unsigned ms) {
#if (TICK_MULTIPLICATOR != 1)
  ms = ms * TICK_MULTIPLICATOR;
#endif
  if (ms) {
    OS_EVENT_WaitTimed(&MB_OS_NetEvent, ms);
  } else {
    OS_EVENT_Wait(&MB_OS_NetEvent);
  }
}

/*********************************************************************
*
*       MB_OS_WaitItemTimed()
*
*  Function description
*    Suspends a task which needs to wait for an object.
*    This object is identified by a pointer to it and can be of any
*    type, e.g. channel.
*
*  Parameters
*    pWaitItem: Item to wait for.
*    Timeout  : Timeout for waiting in system ticks (typically 1ms).
*
*  Additional information
*    Function is called from an application task and is locked in
*    every case.
*/
void MB_OS_WaitItemTimed(void* pWaitItem, unsigned Timeout) {
  MASTER_WAIT MasterWait;

#if (TICK_MULTIPLICATOR != 1)
  Timeout = Timeout * TICK_MULTIPLICATOR;
#endif
  //
  // Create the wait object which contains the OS-Event object.
  //
  MasterWait.pWaitItem = pWaitItem;
  OS_EVENT_Create(&MasterWait.Event);
  //
  // Add to beginning of doubly-linked list.
  //
  MasterWait.pPrev = NULL;
  OS_EnterRegion();        // Disable task switching to prevent being preempted by a task being killed while modifying the linked list or simply by another task.
#if (OS_VERSION >= 38606)  // OS_AddOnTerminateHook() is supported since embOS v3.86f .
  MasterWait.pTask = OS_GetpCurrentTask();
#endif
  MasterWait.pNext  = _pFirstMasterWait;
  _pFirstMasterWait = &MasterWait;
  if (MasterWait.pNext) {
    MasterWait.pNext->pPrev = &MasterWait;
  }
  OS_LeaveRegion();
  //
  //  Suspend this task.
  //
  if (Timeout == 0) {
    OS_EVENT_Wait(&MasterWait.Event);
  } else {
    OS_EVENT_WaitTimed(&MasterWait.Event, Timeout);
  }
  //
  // Remove it from doubly linked list and delete event object.
  //
  OS_EnterRegion();  // Disable task switching to prevent being preempted by a task while modifying the linked list.
  _DLIST_RemoveDelete(&MasterWait);
  OS_LeaveRegion();
}

/*********************************************************************
*
*       MB_OS_SignalItem()
*
*  Function description
*    Sets an object to signaled state, or resumes tasks which are
*    waiting at the event object.
*
*  Parameters
*    pWaitItem: Item to signal.
*
*  Additional information
*    Function is called from a task, not an ISR and is locked in
*    every case.
*/
void MB_OS_SignalItem(void* pWaitItem) {
  MASTER_WAIT* pMasterWait;

  OS_EnterRegion();  // Disable task switching to prevent reading the list while it is modified.
  for (pMasterWait = _pFirstMasterWait; pMasterWait; pMasterWait = pMasterWait->pNext) {
    if (pMasterWait->pWaitItem == pWaitItem) {
      OS_EVENT_Set(&pMasterWait->Event);
    }
  }
  OS_LeaveRegion();
}

/*********************************************************************
*
*       MB_OS_GetTaskName()
*
*  Function description
*    Retrieves the task name (if available from the OS and not in
*    interrupt) for the currently active task.
*
*  Parameters
*    pTask: Pointer to a task identifier such as a task control block.
*
*  Return value
*    Terminated string with task name.
*/
const char* MB_OS_GetTaskName(void* pTask) {
  return OS_GetTaskName((OS_TASK*)pTask);
}

/*************************** End of file ****************************/
