/*********************************************************************
*               SEGGER MICROCONTROLLER SYSTEME GmbH                  *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*       (C) 2003 - 2007   SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       USB device stack for embedded applications                   *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : USB_Config_Nordic_nRF52840_DK.c
Purpose : Config file for NordicSemi nRF52840-DK eval board
--------  END-OF-HEADER  ---------------------------------------------
*/

#include <stdio.h>
#include "USB.h"
#include "BSP_USB.h"
#include "nrf.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines, non-configurable, sfrs
*
**********************************************************************
*/

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _EnableISR
*/
static void _EnableISR(USB_ISR_HANDLER * pfISRHandler) {
  BSP_USB_InstallISR(pfISRHandler);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Setup which target USB driver shall be used
*/

/*********************************************************************
*
*       USBD_X_Config
*/
void USBD_X_Config(void) {
  //
  // Clear event
  //
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  //
  // Start clock
  //
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  //
  // Wait for clock
  //
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
  }
  //
  // Init driver
  //
  USBD_AddDriver(&USB_Driver_Nordic_nRF52xxx);
  USBD_SetISRMgmFuncs(_EnableISR, NULL, NULL);
}


/*************************** End of file ****************************/
