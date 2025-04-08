/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2007 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet * TCP/IP stack for embedded applications               *
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
*       emNet version: V3.60.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

Purpose : Demonstrates how to start the emWeb server with UPnP enabled.

Additional information:
  The sample can easily be configured to start the emWeb server with
  different configurations.

  Preparation:
    To connect to the webserver via UPnP on Windows machines
    you must first enable the option "Turn on Network Discovery"
    after which you will be able to find the webserver
    under "Network" in your file explorer.

  Expected behavior:
    This sample starts a webserver that listens on the enabled ports
    and protocols and can be accessed via a web browser using the
    IP(v4/v6) addresses that are printed in the terminal output when
    starting, or by connecting via UPnP.

    Alternatively you can use the UDPDiscover PC executable,
    found at "/Windows/IP/UDPDiscoverGUI", to find
    the IPv4 address of the target on the network (please allow the
    tool to send broadcasts if necessary with your firewall).


  Sample output:
    ...
    6:039 IP_WebServer - WEBS: Using a memory pool of 5120 bytes for 2 connections.
    ...
    <User opens the site in a browser>
    14:985 IP_WebServer - New IPv4 client accepted.
    14:985 Webserver Child - WebS: Get /
    15:014 Webserver Child - WebS: Get /Styles.css
    15:016 Webserver Child - WebS: Get /Logo.gif
    15:016 IP_WebServer - New IPv4 client accepted.
    15:031 Webserver Child - WebS: Get /favicon.ico
    15:042 Webserver Child - WebS: Get /BGround.png
    ...
    <User visits a form sample page and interacts with it>
    64:586 IP_WebServer - New IPv4 client accepted.
    64:587 Webserver Child - WebS: Get /FormGET.htm
    76:650 IP_WebServer - New IPv4 client accepted.
    76:650 Webserver Child - WebS: Get /FormGET.htm
    ...
    <User visits an SSE example page>
    161:362 IP_WebServer - New IPv4 client accepted.
    161:362 Webserver Child - WebS: Get /SSE_Time.htm
    161:384 Webserver Child - WebS: Get /events.js
    161:389 Webserver Child - WebS: Get /SSETime.cgi
    ...
*/

#include "IP.h"
#include "IP_Webserver.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

//
// UPnP VFile hook
//
static WEBS_VFILE_HOOK _UPnP_VFileHook;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _APP_AfterConfig()
*
*  Function description
*    This routine gets executed after the generic configuration of
*    the main sample has been applied. Additional configurations
*    can be applied from here.
*
*  Parameters
*    p: Opaque parameter that can be used to pass sample specific
*       information to this routine.
*/
static void _APP_AfterConfig(void* p) {
  int IFaceId;

  IFaceId = SEGGER_PTR2ADDR(p);

  //
  // Activate UPnP with VFile hook for needed XML files.
  // UPnP does not seem to work with chunked encoding therefore force to use RAW.
  //
  IP_WEBS_AddVFileHook(&_UPnP_VFileHook, (WEBS_VFILE_APPLICATION*)&WebsSample_UPnP_VFileAPI, HTTP_ENCODING_RAW);
  IP_UPNP_Activate(IFaceId, NULL);
}

/*********************************************************************
*
*       Main sample configuration and include
*
**********************************************************************
*/

//
// Include and configure the webserver main sample.
//
#define APP_AFTER_CONFIG  _APP_AfterConfig
#include "IP_WebserverSample.c"
#undef  APP_AFTER_CONFIG

/*************************** End of file ****************************/
