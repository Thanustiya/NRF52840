/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Start2TasksEx.c
Purpose : embOS sample program running two tasks with one task function.
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

static void TaskEx(void* pData) {
  while (1) {
    OS_TASK_Delay((OS_TIME)pData);
  }
}

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
  OS_TASK_CREATEEX(&TCBHP, "HP Task", 100, TaskEx, StackHP, (void*) 50);
  OS_TASK_CREATEEX(&TCBLP, "LP Task",  50, TaskEx, StackLP, (void*) 200);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
