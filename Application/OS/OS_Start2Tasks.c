/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Start2Tasks.c
Purpose : embOS sample program running two simple tasks.
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

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
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
