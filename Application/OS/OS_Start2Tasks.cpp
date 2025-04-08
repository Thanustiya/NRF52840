/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Start2Tasks.cpp
Purpose : embOS C++ sample program running two simple tasks.
*/

#include "RTOS.h"
#include "stdlib.h"

/*********************************************************************
*
*       Class definition
*
**********************************************************************
*/
class CThread:private OS_TASK {
public:
  CThread(char* s, unsigned int p, unsigned int t):sName(s), Prio(p), Time(t) {
    void* pTaskStack = malloc(256u);
    if (pTaskStack != NULL) {
      OS_TASK_CreateEx(this, sName, Prio, CThread::run, reinterpret_cast<OS_STACKPTR void*>(pTaskStack), 256u, 2u, reinterpret_cast<void*>(Time));
    }
  }

private:
  char*        sName;
  unsigned int Prio;
  unsigned int Time;

  static void run(void* t) {
    while (1) {
      OS_TASK_Delay(reinterpret_cast<unsigned long>(t));
    }
  }
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
CThread *HPTask, *LPTask;

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
  HPTask = new CThread(const_cast<char*>("HPTask"), 100u, 50u);
  LPTask = new CThread(const_cast<char*>("LPTask"), 50u, 200u);
  OS_TASK_Terminate(NULL);
}

/*************************** End of file ****************************/
