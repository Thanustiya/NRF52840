/*********************************************************************
*               SEGGER MICROCONTROLLER GmbH & Co KG                  *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2011  SEGGER Microcontroller GmbH & Co KG         *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: Internal                                         *
*                                                                    *
*       Current version number will be inserted here                 *
*       when shipment is built.                                      *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : BSP_USB.c
Purpose : BSP for nRF52840-DK eval board
          Functions for USB controllers
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "BSP_USB.h"
#include "RTOS.h"
#include "nrf.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef void (ISR_HANDLER)(void);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static ISR_HANDLER * _pfUSBISRHandler;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {
#endif
  void USBD_IRQHandler(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
/*********************************************************************
*
*       USBD_IRQHandler
*/
void USBD_IRQHandler(void) {
  OS_EnterInterrupt();
  if (_pfUSBISRHandler) {
    _pfUSBISRHandler();
  }
  OS_LeaveInterrupt();
}

/*********************************************************************
*
*       BSP_USB_InstallISR
*/
void BSP_USB_InstallISR(void (*pfISR)(void)) {
  _pfUSBISRHandler = pfISR;
  NVIC_SetPriority(USBD_IRQn, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ(USBD_IRQn);
}

/*********************************************************************
*
*       BSP_USB_Init
*/
void BSP_USB_Init(void) {
}

/****** End Of File *************************************************/
