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
-------------------------- END-OF-HEADER -----------------------------

Purpose : Header file for SEGGER's FIND protocol.
*/

#ifndef IP_FIND_H             // Avoid multiple inclusion.
#define IP_FIND_H

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif


/*********************************************************************
*
*       API functions / Function prototypes
*
**********************************************************************
*/

int IP_FIND_Init(void);

#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

#endif                        // Avoid multiple inclusion.

/*************************** End of file ****************************/
