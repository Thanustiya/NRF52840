/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Mutexes.c
Purpose : embOS sample program demonstrating the usage of mutexes.
*/

#include "RTOS.h"
#include <stdio.h>

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_MUTEX        Mutex;

/*********************************************************************
*
*       _Write()
*/
static void _Write(char const* s) {
  OS_MUTEX_LockBlocked(&Mutex);
  OS_COM_SendString(s);
  OS_MUTEX_Unlock(&Mutex);
}

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  while (1) {
    _Write("HPTask\n");
    OS_TASK_Delay(50);
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    _Write("LPTask\n");
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
  OS_MUTEX_Create(&Mutex);  // Creates mutex
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
