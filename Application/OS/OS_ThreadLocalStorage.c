/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_ThreadLocalStorage.c
Purpose : embOS sample application to demonstrate the usage of TLS.
          TLS support is CPU and compiler specific and may not be
          implemented in all embOS ports.
*/

#include "RTOS.h"
#include <errno.h>

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128], StackLP[128], StackMP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP, TCBMP;                       // Task control blocks

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       HPTask() with thread local storage
*/
static void HPTask(void) {
  //
  // Initialize TLS for this task
  //
  OS_TASK_SetContextExtensionTLS();
  if (errno != 0) {
    while (1) {
      //
      // errno is local to this task, hence we should not arrive here
      //
    }
  }
  //
  // Simulate a task specific error
  //
  errno = 3;
  while (1) {
    OS_TASK_Delay(10);
    if (errno != 3) {
      while (1) {
        //
        // errno is local to this task, hence we should not arrive here
        //
      }
    }
  }
}

/*********************************************************************
*
*       MPTask() with thread local storage
*/
static void MPTask(void) {
  //
  // Initialize TLS for this task
  //
  OS_TASK_SetContextExtensionTLS();
  if (errno != 0) {
    while (1) {
      //
      // errno is local to this task, hence we should not arrive here
      //
    }
  }
  //
  // Simulate a task specific error
  //
  errno = 2;
  while (1) {
    OS_TASK_Delay(10);
    if (errno != 2) {
      while (1) {
        //
        // errno is local to this task, hence we should not arrive here
        //
      }
    }
  }
}

/*********************************************************************
*
*       LPTask() without thread local storage
*/
static void LPTask(void) {
  if (errno != 1) {
    while (1) {
      //
      // errno is not local to this task, hence we expect the global
      // value that was set in main() and should not arrive here
      //
    }
  }
  while (1) {
    OS_TASK_Delay(50);
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
  errno = 1;    // Simulate an error
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBMP, "MP Task",  70, MPTask, StackMP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
