/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2019  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       IoT Toolkit * HTTP Client and JSON Parser                    *
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
*       IoT Toolkit version: V2.32                                   *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : IOT_ConfigIO.c
Purpose     : Configuration of I/O for SSH library.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include "IOT.h"

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

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

#if IOT_DEBUG > 1
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
    (void)putchar(c);
#endif
  }
#endif
}
#endif
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       IOT_Panic()
*
*  Function description
*    Deal with an unexpected situation detected by emSSH.
*/
void IOT_Panic(void) {
#if IOT_DEBUG > 1
#if USE_SYSTEMVIEW
  SEGGER_SYSVIEW_Error("*** Fatal error, System halted: ");
  SEGGER_SYSVIEW_PrintfTargetEx("", SEGGER_SYSVIEW_ERROR | (1 << 8));  // FIXME
#else
  _puts("*** Fatal error, System halted");
#endif
#endif
  for (;;) {
    /* Hang */
  }
}

/*************************** End of file ****************************/
