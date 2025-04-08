/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Semaphores.c
Purpose : embOS sample program demonstrating the usage of semaphores.
          Suppose a library has 3 identical study rooms, to be used by
          one student at a time. Students must request a room from the
          front desk if they wish to use a study room. If no rooms are
          free, students wait at the desk until someone relinquishes a
          room. When a student has finished using a room, the student
          must return to the desk and indicate that one room has become
          free.
*/

#include "RTOS.h"
#include <stdio.h>

#define NUM_OF_ROOMS     3
#define NUM_OF_STUDENTS  5

static OS_STACKPTR int Stack[NUM_OF_STUDENTS][128];  // Task stack
static OS_TASK         TCB[NUM_OF_STUDENTS];         // Task-control-block
static OS_SEMAPHORE    Sema;                         // Semaphore

static void _PrintDec(unsigned Value) {
  unsigned Digit;
  char*    s;
  int      r;
  char     ac[10] = {0};

  s     = &ac[0];
  Digit = 10;
  while (Digit < Value) {
    Digit *= 10;
  }
  do {
    Digit /= 10;
    r      = Value / Digit;
    Value -= r * Digit;
    *s++   = (r + '0');
  } while (Value | (Digit > 1));
  OS_COM_SendString(ac);
}

static void Student(void* pData) {
  unsigned int StudentNo;
  StudentNo = (unsigned int)pData;
  while(1) {
    OS_SEMAPHORE_TakeBlocked(&Sema);  // Get a key for a room and wait at the desk if no room is free
    OS_COM_SendString("\nStudent ");
    _PrintDec(StudentNo);
    OS_COM_SendString(" enters room.");
    OS_TASK_Delay(100);               // Occupy room for while
    OS_COM_SendString("\nStudent ");
    _PrintDec(StudentNo);
    OS_COM_SendString(" leaves room.");
    OS_SEMAPHORE_Give(&Sema);         // Bring the key back to the desk
    OS_TASK_Yield();                  // Go to canteen...
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
  OS_TASK_CREATEEX(&TCB[0], "Student 1", 100, Student, Stack[0], (void*)1);
  OS_TASK_CREATEEX(&TCB[1], "Student 2", 100, Student, Stack[1], (void*)2);
  OS_TASK_CREATEEX(&TCB[2], "Student 3", 100, Student, Stack[2], (void*)3);
  OS_TASK_CREATEEX(&TCB[3], "Student 4", 100, Student, Stack[3], (void*)4);
  OS_TASK_CREATEEX(&TCB[4], "Student 5", 100, Student, Stack[4], (void*)5);
  OS_SEMAPHORE_Create(&Sema, NUM_OF_ROOMS);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
