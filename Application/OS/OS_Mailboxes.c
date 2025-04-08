/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Mailboxes.c
Purpose : embOS sample program demonstrating the usage of mailboxes.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define MAX_MSG_SIZE  (8)  // Max. number of bytes per message
#define MAX_MSG_NUM   (2)  // Max. number of messages per Mailbox

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_MAILBOX      MyMailbox;
static char            MyMailboxBuffer[MAX_MSG_SIZE * MAX_MSG_NUM];

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
  char aData[MAX_MSG_SIZE];

  while (1) {
    OS_MAILBOX_GetBlocked(&MyMailbox, (void *)aData);
    OS_COM_SendString(aData);
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    OS_MAILBOX_PutBlocked(&MyMailbox, "\nHello\0");
    OS_MAILBOX_PutBlocked(&MyMailbox, "\nWorld!\0");
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
  OS_MAILBOX_Create(&MyMailbox, MAX_MSG_SIZE, MAX_MSG_NUM, &MyMailboxBuffer);
  OS_COM_SendString("embOS OS_Mailbox example");
  OS_COM_SendString("\n\nDemonstrating message passing\n");
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
