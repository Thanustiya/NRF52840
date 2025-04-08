/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_ObjectIdentifier.c
Purpose : embOS sample program for Human readable object identifiers
*/

#include "RTOS.h"
#include <stdio.h>

static OS_STACKPTR int StackHP[128];  // Task stacks
static OS_TASK         TCBHP;         // Task control blocks
static OS_U8           MailboxA_Buffer[10];
static OS_U8           MailboxB_Buffer[10];
static OS_MAILBOX      MailboxA, MailboxB;
static OS_OBJNAME      MailboxA_Name, MailboxB_Name;

static void _PrintMailboxNames(void) {
  OS_COM_SendString("\nMailboxA: ");
  OS_COM_SendString(OS_DEBUG_GetObjName(&MailboxA));
  OS_COM_SendString("\nMailboxB: ");
  OS_COM_SendString(OS_DEBUG_GetObjName(&MailboxB));
}

static void HPTask(void) {
  _PrintMailboxNames();
  while (1) {
    OS_TASK_Delay(50);
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
  OS_CREATEMB(&MailboxA, 5, 2, MailboxA_Buffer);
  OS_CREATEMB(&MailboxB, 5, 2, MailboxB_Buffer);
  OS_DEBUG_SetObjName(&MailboxA_Name, &MailboxA, "Mailbox A");
  OS_DEBUG_SetObjName(&MailboxB_Name, &MailboxB, "Mailbox B");
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
