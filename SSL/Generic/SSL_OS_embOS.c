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
*       emSSL * Embedded Transport Layer Security                    *
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
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SSL_OS_embOS.c
Purpose : Kernel abstraction for embOS

*/

#include "SSL_Int.h"
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

OS_RSEMA  SSL_OS_RSema;     // Public only to allow inlining (direct call from SSL core)

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SSL_OS_Init
*
*  Function description
*    Initialize (create) all objects required for task synchronization.
*/
void SSL_OS_Init(void) {
  if (_IsInited == 0) {
    OS_CREATERSEMA(&SSL_OS_RSema);
    _IsInited = 1;
  }
}

/*********************************************************************
*
*       SSL_OS_Sleep
*/
void SSL_OS_Sleep(unsigned Delay) {
  OS_Delay(Delay);
}

/*********************************************************************
*
*       SSL_OS_DisableInterrupt
*/
void SSL_OS_DisableInterrupt(void) {
  OS_IncDI();
}

/*********************************************************************
*
*       SSL_OS_EnableInterrupt
*/
void SSL_OS_EnableInterrupt(void) {
  OS_DecRI();
}

/*********************************************************************
*
*       SSL_OS_Lock
*
*  Function description
*    The stack requires a single lock, typically a resource semaphore
*    or mutex. This function locks this object, guarding sections of
*    the stack code against other threads.
*    If the entire stack executes from a single task, no
*    functionality is required here.
*/
void SSL_OS_Lock(void) {
  OS_Use(&SSL_OS_RSema);
}

/*********************************************************************
*
*       SSL_OS_Unlock
*
*  Function description
*    Unlocks the single lock used locked by a previous call to
*    SSL_OS_Lock().
*/
void SSL_OS_Unlock(void) {
  OS_Unuse(&SSL_OS_RSema);
}

/*********************************************************************
*
*       SSL_OS_GetTime32()
*
*  Function description
*    Return the current system time in ms.
*    The value will wrap around after app. 49.7 days. This is taken
*    into account by the stack.
*/
U32 SSL_OS_GetTime32(void) {
  return OS_GetTime32();
}

/*********************************************************************
*
*       SSL_OS_GetTaskName()
*
* Function description
*   Retrieves the task name (if available from the OS and not in
*   interrupt) for the currently active task.
*
* Parameters
*   pTask: Pointer to a task identifier such as a task control block.
*
* Return value
*   Terminated string with task name.
*/
const char * SSL_OS_GetTaskName(void *pTask) {
  return OS_GetTaskName((OS_TASK*)pTask);
}

/*************************** End of file ****************************/
