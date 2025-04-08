/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Watchdog.c
Purpose : embOS sample program for using the watchdog module
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_WD           WatchdogHP, WatchdogLP;
static OS_TICK_HOOK    Hook;

static void _TriggerWatchDog(void) {
  //
  // Trigger the hardware watchdog
  //
}

static void _Reset(OS_CONST_PTR OS_WD* pWD) {
  OS_USEPARA(pWD);  // Applications can use pWD to detect WD expiration cause.
  //
  // Reboot microcontroller
  //
  while (1) {       // Dummy loop, you can set a breakpoint here
  }
}

static void HPTask(void) {
  OS_WD_Add(&WatchdogHP, 50);
  while (1) {
    OS_TASK_Delay(50);
    OS_WD_Trigger(&WatchdogHP);
  }
}

static void LPTask(void) {
  OS_WD_Add(&WatchdogLP, 200);
  while (1) {
    OS_TASK_Delay(200);
    OS_WD_Trigger(&WatchdogLP);
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
  OS_WD_Config(&_TriggerWatchDog, &_Reset);
  OS_TICK_AddHook(&Hook, OS_WD_Check);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
