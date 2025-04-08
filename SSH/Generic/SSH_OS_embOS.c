/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSH * Embedded Secure Shell                                *
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
*       emSSH version: V2.56.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SSH_OS_embOS.c
Purpose : Kernel abstraction for embOS

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH_Int.h"
#include "RTOS.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U8 _IsInited;

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/

OS_RSEMA SSH_OS_RSema;     // Public only to allow inlining (direct call from SSH core)

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SSH_OS_Init()
*
*  Function description
*    Initialize SSH-OS interface.
*
*  Additional information
*    Creates and initializes all objects required for task
*    synchronization.
*/
void SSH_OS_Init(void) {
  if (_IsInited == 0) {
    OS_CREATERSEMA(&SSH_OS_RSema);
    _IsInited = 1;
  }
}

/*********************************************************************
*
*       SSH_OS_DisableInterrupt()
*
*  Function description
*    Disables interrupts.
*
*  Additional information
*    It is required that the implementation maintains a count of the
*    number of times the interrupt has been disabled.  Only when
*    all interrupt disables have been matched with corresponding
*    enables will interrupts be serviced.
*/
void SSH_OS_DisableInterrupt(void) {
  OS_IncDI();
}

/*********************************************************************
*
*       SSH_OS_EnableInterrupt()
*
*  Function description
*    Enables interrupts.
*
*  Additional information
*    This function is paired with SSH_OS_DisableInterrupt to enable
*    interrupts previously disabled by SSH_OS_DisableInterrupt.
*/
void SSH_OS_EnableInterrupt(void) {
  OS_DecRI();
}

/*********************************************************************
*
*       SSH_OS_Lock
*
*  Function description
*    Lock emSSH.
*
*  Additional information
*    emSSH requires a single lock, typically a resource semaphore or
*    mutex. This function locks this object, guarding sections of
*    emSSH against other threads.
*
*    It is required that the lock is "recursive" or "counts" and
*    can be locked and unlocked several times by the same calling task.
*/
void SSH_OS_Lock(void) {
  OS_Use(&SSH_OS_RSema);
}

/*********************************************************************
*
*       SSH_OS_Unlock
*
*  Function description
*    Unlock emSSH.
*
*  Additional information
*    This function is paired with SSH_OS_Lock to unlock the resource
*    semaphore or mutex previously locked by SSH_OS_Lock.
*/
void SSH_OS_Unlock(void) {
  OS_Unuse(&SSH_OS_RSema);
}

/*********************************************************************
*
*       SSH_OS_GetTime32()
*
*  Function description
*    Return the current system time in ms.
*
*  Return value
*    System time in ms.
*/
U32 SSH_OS_GetTime32(void) {
  return OS_GetTime32();
}

/*********************************************************************
*
*       SSH_OS_GetTaskName()
*
*  Function description
*    Retrieves the task name.
*
*  Parameters
*    pTask - Pointer to a task identifier such as a task control block.
*
*  Return value
*    Pointer to zero-terminated string containing task name.
*/
const char * SSH_OS_GetTaskName(void *pTask) {
  return OS_GetTaskName((OS_TASK*)pTask);
}

/*************************** End of file ****************************/
