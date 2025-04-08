/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*  Copyright 2018 - SEGGER Microcontroller GmbH - www.segger.com     *
*                                                                    *
*  Redistribution and use in source form, with or without            *
*  modification, are permitted provided that the following           *
*  condition is met:                                                 *
*                                                                    *
*  1. Redistributions of source code must retain the above copyright *
*     notice, this list of conditions and the following disclaimer.  *
*                                                                    *
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            *
*  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       *
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          *
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          *
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR             *
*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,      *
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT  *
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF  *
*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED   *
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT       *
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING    *
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF    *
*  THE POSSIBILITY OF SUCH DAMAGE.                                   *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
Purpose : Sample implementation of SEGGER's FIND protocol.
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "IP_FIND.h"
#include "IP.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define TARGET_NAME                     "MyTarget"              // Target Name. Mandatory, usually a fixed string
#define TARGET_SERIAL_NUMBER            12345678                // Serial number. Mandatory, the define is a number, the value transmitted
                                                                // is an ASCII string. Typically, a function call will be used to get the
                                                                // serial number.
#define FIND_RESPONSE_BUFFER_SIZE       128                     // Maximum FIND packet size is 512 bytes.
                                                                // It usually makes sense to limit this since the work buffer is located
                                                                // on the stack, and a bigger buffer would increase stack requirements.

#define ADD_IP_ADDRESSES                0

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define FIND_RECEIVE_IDENTIFIER         "FINDReq=1;"
#define FIND_SEND_IDENTIFIER            "FIND=1;"
#define FIND_DEF_PORT                   50022


/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnRx()
*
* Function description
*   Default FIND protocol client callback. Called from stack whenever
*   we get a FIND request.
*
* Parameters
*   pInPacket: Pointer to incoming packet.
*   pContext : Context set during callback registration.
*
* Return value
*   IP_OK      : Packet processed. pInPacket will be freed.
*
* Notes
*   (1) Freeing pInPacket:
*       With either return value, the IN-packet is freed by the stack
*       and therefore can not be re-used nor has it to be freed by
*       this routine.
*
*       Incoming packet example:
*         "FINDReq=1;"
*
*       Outgoing packet example:
*         "FIND=1;IP=192.168.2.124,FE80:0000:0000:0000:0222:C7FF:FE89:8745;HWADDR=00:22:C7:89:87:45;DeviceName=MyTarget;SN=12345678;"
*/
static int _OnRx(IP_PACKET* pInPacket, void* pContext) {
  unsigned      IFaceId;
  U32           TargetAddr;
  int           Pos;
  char*         pOutData;
  char*         pInData;
  IP_PACKET*    pOutPacket;
  unsigned char aMacData[6];
  int           SerialNumber;
  char          aFINDResponse[FIND_RESPONSE_BUFFER_SIZE];
#if ADD_IP_ADDRESSES
  U32           IPAddr;
#if IP_SUPPORT_IPV6
  U8            NumAddr;
  U8            AddrIndex;
  IPV6_ADDR     IPv6Addr;
#endif
#endif

  IP_USE_PARA(pContext);
  pInData  = (char*)IP_UDP_GetDataPtr(pInPacket);           // Get the pointer to the UDP payload.
  if (memcmp(pInData, FIND_RECEIVE_IDENTIFIER, 10) == 0) {  // Check if this is a known FIND packet.
    IFaceId = IP_UDP_GetIFIndex(pInPacket);
    //
    // Fill packet with data, containing IPAddr, HWAddr, S/N and name.
    //
    memset(aFINDResponse, 0, FIND_RESPONSE_BUFFER_SIZE);                                   // Make sure all fields are cleared.
    Pos = SEGGER_snprintf(aFINDResponse, FIND_RESPONSE_BUFFER_SIZE, FIND_SEND_IDENTIFIER); // Answer string for a discover response.
#if ADD_IP_ADDRESSES
    //
    // Add IPv4 information.
    //
    IPAddr = IP_GetIPAddr(IFaceId);
    IPAddr = htonl(IPAddr);
    Pos   += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, "IP=%i", IPAddr);
#if IP_SUPPORT_IPV6
    //
    // Add IPv6 information, in systems without IPv6 support this should be eliminated.
    //
    AddrIndex = 0;
    IP_IPV6_GetIPv6Addr(IFaceId, AddrIndex, NULL, &NumAddr);  // Get number of configured IPv6 adresses
    if (NumAddr > 0) {
    //
    // Add IPv6 addresses
    // If your target uses more as one IPv6 address, please check if the size of the working buffer is sufficient to store them all.
    //
    do {
      IP_IPV6_GetIPv6Addr(IFaceId, AddrIndex, &IPv6Addr, &NumAddr);
      Pos += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, ",%n", &IPv6Addr.Union.aU8[0]);
      AddrIndex++;
    } while(NumAddr > AddrIndex);
    Pos += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, ";");
    }
#endif // IP_SUPPORT_IPV6
#endif // ADD_IP_ADDRESSES
    //
    // Add MAC information.
    //
    IP_GetHWAddr(IFaceId, aMacData, 6);
    Pos += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, "HWADDR=%_h;", &aMacData[0]);
    //
    // Add device name.
    //
    Pos += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, "DeviceName=%s;", TARGET_NAME);
    //
    // Add serial number.
    //
    SerialNumber = TARGET_SERIAL_NUMBER;
    Pos += SEGGER_snprintf(&aFINDResponse[Pos], FIND_RESPONSE_BUFFER_SIZE - Pos, "SN=%d;", SerialNumber);
    //
    // Allocate and send response packet.
    //
    pOutPacket = IP_UDP_AllocEx(IFaceId, Pos + 1);                                               // Allocate packet on the same interface
    if (pOutPacket) {
      pOutData = (char*)IP_UDP_GetDataPtr(pOutPacket);                                           // Get data pointer
      SEGGER_memcpy(pOutData, aFINDResponse, Pos + 1);                                           // Copy response into packet buffer
      IP_UDP_GetSrcAddr(pInPacket, &TargetAddr, sizeof(TargetAddr));                             // Get the src address of the IN-packet
      (void)IP_UDP_SendAndFree(IFaceId, TargetAddr, FIND_DEF_PORT, FIND_DEF_PORT, pOutPacket);   // Send packet
    }
  }
  return IP_OK;
}


/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       IP_FIND_Init()
*
*  Function description
*    Adds the FIND protocol to your application.
*
*  Additional information
*    Opens a UDP socket listening on port xxxxx
*
*  Return value
*    ==  0: O.K.
*    == -1: Error.
*/
int IP_FIND_Init(void) {
  IP_UDP_CONNECTION* pCon;
  int                r;

  r = 0;
  pCon = IP_UDP_Open(0uL /* any foreign host */, FIND_DEF_PORT, FIND_DEF_PORT, _OnRx, NULL);  // Use default sample implementation
  if (pCon == NULL) {
    r = -1;
  }
  return r;
}

/****** End Of File *************************************************/
