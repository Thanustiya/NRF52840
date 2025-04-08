/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_EventObjects.c
Purpose : embOS sample program demonstrating the usage of event objects.
          This sample shows how to send an event from one task to
          multiple tasks.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int StackHP[128], StackLP[128], StackHW[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP, TCBHW;                       // Task control blocks
static OS_EVENT        HW_Event;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  //
  // Wait until event is signaled
  //
  OS_EVENT_GetBlocked(&HW_Event);
  while (1) {
    OS_TASK_Delay(50);
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  //
  // Wait until event is signaled
  //
  OS_EVENT_GetBlocked(&HW_Event);
  while (1) {
    OS_TASK_Delay(200);
  }
}

/*********************************************************************
*
*       HWTask()
*/
static void HWTask(void) {
  //
  // Perform e.g. a hardware setup
  //
  OS_TASK_Delay(100);
  //
  // Init done, send event to waiting tasks
  //
  OS_EVENT_Set(&HW_Event);
  while (1) {
    OS_TASK_Delay(40);
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
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_TASK_CREATE(&TCBHW, "HWTask",   25, HWTask, StackHW);
  OS_EVENT_Create(&HW_Event);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
