/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_ExtendedTask.c
Purpose : embOS sample program demonstrating the extension of tasks.
          This sample application is described in the generic manual
          in chapter:
          "Extending the task context by using own task structures"
*/

#include "RTOS.h"
#include <stdio.h>

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

//
// Custom task structure with extended task context.
//
typedef struct {
  OS_TASK Task;     // OS_TASK has to be the first element
  OS_TIME Timeout;  // Any other data type may be used to extend the context
  char*   pString;  // Any number of elements may be used to extend the context
} MY_APP_TASK;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static MY_APP_TASK     TCBHP, TCBLP;                // Task-control-blocks

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MyTask()
*/
static void MyTask(void) {
  MY_APP_TASK* pThis;
  OS_TIME      Timeout;
  char*        pString;

  pThis = (MY_APP_TASK*)OS_TASK_GetID();
  while (1) {
    Timeout = pThis->Timeout;
    pString = pThis->pString;
    OS_COM_SendString(pString);
    OS_TASK_Delay(Timeout);
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
*/
#ifdef __cplusplus
extern "C" {     // Make sure we have C-declarations in C++ programs
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  OS_TASK_EnterRegion();
  //
  // Create the extended tasks just as normal tasks.
  // Note that the first parameter has to be of type OS_TASK
  //
  OS_TASK_CREATE(&TCBHP.Task, "HP Task", 100, MyTask, StackHP);
  OS_TASK_CREATE(&TCBLP.Task, "LP Task",  50, MyTask, StackLP);
  //
  // Give task contexts individual data
  //
  TCBHP.Timeout = 200;
  TCBHP.pString = "HP task running\n";
  TCBLP.Timeout = 500;
  TCBLP.pString = "LP task running\n";
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
