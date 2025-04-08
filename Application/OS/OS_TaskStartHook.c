/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_TaskStartHook.c
Purpose : embOS sample program showiing how to setup a task start hook
          routine
*/

#include "RTOS.h"
#include "BSP.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

static void _Hook(void) {
  if (OS_TASK_GetID() == &TCBHP) {
    BSP_SetLED(0);
  }
  if (OS_TASK_GetID() == &TCBLP) {
    BSP_SetLED(1);
  }
}

static void HPTask(void) {
  while (1) {
    OS_TASK_Delay(50);
  }
}

static void LPTask(void) {
  while (1) {
    OS_TASK_Delay(200);
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
  OS_TASK_SetDefaultStartHook(_Hook);
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
