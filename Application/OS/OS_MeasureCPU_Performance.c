/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_MeasureCPU_Performance.c
Purpose : embOS sample program that checks the performance of the
          entire system, outputting one result value per second. Higher
          values indicate better performance.
          The program computes the first 1000 prime numbers in a loop.
          The output value is the number of times this could be performed
          within one second.
          Version 2.0, July-14 2008
----------------------------------------------------------------------
Reference values -  RAM configuration:
ARM926-EJS fully cached, 201.0MHz, IAR EWARM V5.20,   Thumb Mode => 11834 Loops/sec => 58.9 Loops/sec/MHz
ARM926-EJS fully cached, 201.0MHz, IAR EWARM V5.20,   ARM   Mode => 12497 Loops/sec => 62.2 Loops/sec/MHz
ARM926-EJS fully cached, 180.0MHz, IAR EWARM V5.30.2, ARM   Mode => 16829 Loops/sec => 93.5 Loops/sec/MHz
ARM7-TDMI                 47.9MHz, IAR EWARM V5.20,   Thumb Mode =>  2656 Loops/sec => 55.4 Loops/sec/MHz
ARM7-TDMI                 47.9MHz, IAR EWARM V5.20,   ARM   Mode =>  2646 Loops/sec => 55.2 Loops/sec/MHz
SH2A-7264                144.0MHz, HEW V4.07                     => 11372 Loops/sec => 78.9 Loops/sec/MHz
*/

#include "RTOS.h"
#include <stdio.h>
#include <stdlib.h>

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128];  // Task stack
static OS_TASK         TCBHP;         // Task control block
static char            aIsPrime[1000];
static unsigned int    NumPrimes;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _CalcPrimes()
*/
static void _CalcPrimes(unsigned int NumItems) {
  unsigned int i;
  unsigned int j;

  //
  // Mark all as potential prime numbers
  //
  memset(aIsPrime, 1, NumItems);
  //
  // 2 deserves a special treatment
  //
  for (i = 4; i < NumItems; i += 2) {
    aIsPrime[i] = 0;         // Cross it out: not a prime
  }
  //
  // Cross out multiples of every prime starting at 3. Crossing out starts at i^2.
  //
  for (i = 3; i * i < NumItems; i++) {
    if (aIsPrime[i]) {
      j = i * i;             // The square of this prime is the first we need to cross out
      do {
        aIsPrime[j] = 0;     // Cross it out: not a prime
        j += 2 * i;          // Skip even multiples (only 3*, 5*, 7* etc)
      } while (j < NumItems);
    }
  }
  //
  // Count prime numbers
  //
  NumPrimes = 0;
  for (i = 2; i < NumItems; i++) {
    if (aIsPrime[i]) {
      NumPrimes++;
    }
  }
}

/*********************************************************************
*
*       _PrintDec()
*/
static void _PrintDec(unsigned int v) {
  unsigned int Digit;
  unsigned int r;

  Digit = 10;
  while (Digit < v) {
    Digit *= 10;
  }
  do {
    Digit /= 10;
    r = v / Digit;
    v -= r * Digit;
    putchar(r + '0');
  } while (v | (Digit > 1));
}

/*********************************************************************
*
*       _PrintResult()
*/
static void _PrintResult(unsigned int Cnt) {
  if (NumPrimes != 168) {
    puts("Error");
  } else {
    puts("Loops/sec:");
    _PrintDec(Cnt);
  }
  puts("\n");
}

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  unsigned int Cnt;
  int          TestTime;

  while(1) {
    Cnt = 0;
    OS_TASK_Delay(1);  // Sync to tick
    TestTime = OS_TIME_GetTicks32() + 1000;
    while ((TestTime - OS_TIME_GetTicks()) >= 0) {
      _CalcPrimes(sizeof(aIsPrime));
      Cnt++;
    }
    _PrintResult(Cnt);
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
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
