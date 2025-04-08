/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSL * Embedded Transport Layer Security                    *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product for in-house use.         *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSL_ConfigIO.c
Purpose     : Configuration of I/O for SSL library.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include "SSL.h"

#ifndef   USE_RTT
  #define USE_RTT         0
#endif
#ifndef   USE_SYSTEMVIEW
  #define USE_SYSTEMVIEW  0
#endif
#ifndef   USE_DCC
  #define USE_DCC         0
#endif
#ifndef   USE_EMBOS_VIEW
  #define USE_EMBOS_VIEW  0
#endif

#if (USE_RTT != 0)
#include "SEGGER_RTT.h"
#endif
#if (USE_SYSTEMVIEW != 0)
#include "SEGGER_SYSVIEW.h"
#endif
#if (USE_DCC != 0)
#include "JLINKDCC.h"
#endif

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

//
// By default assume that the time (and task name) will be
// applied by an external output routine like SYS_ .
//
#ifndef   SHOW_TASK
  #define SHOW_TASK 1
#endif

#ifndef   SHOW_TIME
  #define SHOW_TIME 1
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _puts()
*
*  Function description
*    Local (static) replacement for puts.
*    The reason why puts is not used is that puts always appends a NL
*    character, which screws up our formatting.
*
*  Parameters
*    s: String to output.
*/
#if USE_SYSTEMVIEW == 0
static void _puts(const char* s) {
#if (USE_EMBOS_VIEW != 0)
  //
  // Prevent using OS_SendString() from an interrupt as this is not
  // valid. Might happen in case we are overrun by packets.
  // OS_InInterrupt() might not be supported by ALL embOS ports but
  // should be support by typically any newer embOS port.
  //
  if (OS_InInterrupt() == 0) {
    OS_SendString(s);
  }
#elif (USE_RTT != 0)
  SEGGER_RTT_WriteString(0, s);
#else
  char c;

  for (;;) {
    c = *s++;
    if (c == 0) {
      break;
    }
#if USE_DCC
    JLINKDCC_SendChar(c);
#else
    (void)(putchar)(c);
#endif
  }
#endif
}
#endif

#if USE_SYSTEMVIEW == 0 && SHOW_TIME
/*********************************************************************
*
*       _WriteUnsigned
*/
static char * _WriteUnsigned(char *s, U32 v, int NumDigits) {
  unsigned Base;
  unsigned Div;
  U32      Digit;

  Digit = 1;
  Base  = 10;
  //
  // Count how many digits are required
  //
  for (;;) {
    if (NumDigits <= 1) {
      Div = v / Digit;
      if (Div < Base) {
        break;
      }
    }
    NumDigits--;
    Digit *= Base;
  }
  //
  // Output digits
  //
  do {
    Div = v / Digit;
    v  -= Div * Digit;
    *s++ = (char)('0' + Div);
    Digit /= Base;
  } while (Digit);
  *s = 0;
  return s;
}
#endif

/*********************************************************************
*
*       _ShowStamp()
*
*  Function description
*    Outputs a timestamp and task name as configured.
*/
#if USE_SYSTEMVIEW == 0
static void _ShowStamp(void) {
#if SHOW_TIME
  {
    I32   Time;
    char* sBuffer;
    char  ac[20];

    sBuffer    = &ac[0];
    Time       = (I32)SSL_OS_GetTime32();
    sBuffer    = _WriteUnsigned(sBuffer, Time / 1000, 0);
    *sBuffer++ = ':';
    sBuffer    = _WriteUnsigned(sBuffer, Time % 1000, 3);
    *sBuffer++ = ' ';
    *sBuffer++ = 0;
    _puts(ac);
  }
#endif

#if SHOW_TASK
  {
    const char* s;
    s = SSL_OS_GetTaskName(NULL);
    if (s != NULL) {
      _puts(s);
      _puts(" - ");
    }
  }
#endif
}
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SSL_Panic()
*
*  Function description
*    Deal with an unexpected situation detected by emSSL.
*/
void SSL_Panic(int Status) {
  SSL_OS_DisableInterrupt();
#if SSL_DEBUG > 1
#if USE_SYSTEMVIEW
  SEGGER_SYSVIEW_Error("*** Fatal error, System halted: ");
  SEGGER_SYSVIEW_PrintfTargetEx("", SEGGER_SYSVIEW_ERROR | (1 << 8));  // FIXME
#else
  _puts("*** Fatal error, System halted");
#endif
#endif
  while (Status) {
    ;
  }
}

/*********************************************************************
*
*       SSL_Log()
*
*  Function description
*    Log data from emSSL.
*/
void SSL_Log(const char *sText) {
  SSL_OS_DisableInterrupt();
#if USE_SYSTEMVIEW
  SEGGER_SYSVIEW_Print(sText);
#else
  _ShowStamp();
  _puts(sText);
  _puts("\n");
#endif
  SSL_OS_EnableInterrupt();
}

/*********************************************************************
*
*       SSL_Warn()
*
*  Function description
*    Log warning from emSSL.
*/
void SSL_Warn(const char *sText) {
  SSL_OS_DisableInterrupt();
#if USE_SYSTEMVIEW
  SEGGER_SYSVIEW_Warn(sText);
#else
  _ShowStamp();
  _puts("*** Warning *** ");
  _puts(sText);
  _puts("\n");
#endif
  SSL_OS_EnableInterrupt();
}

/*************************** End of file ****************************/
