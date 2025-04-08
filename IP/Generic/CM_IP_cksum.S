;//**********************************************************************
;//*               SEGGER MICROCONTROLLER SYSTEME GmbH                  *
;//*       Solutions for real time microcontroller applications         *
;//**********************************************************************
;//*                                                                    *
;//*       (C) 2007 - 2018    SEGGER Microcontroller Systeme GmbH       *
;//*                                                                    *
;//*       www.segger.com     Support: support@segger.com               *
;//*                                                                    *
;//**********************************************************************
;//*                                                                    *
;//*       TCP/IP stack for embedded applications                       *
;//*                                                                    *
;//**********************************************************************
;//*
;//* Purpose : Efficient implementation of IP-Checksum code
;//*
;//* It has been tested with
;//*   - emIDE V2.20
;//*   - SEGGER Embedded Studio V1.0.1
;//*   - IAR EW V6.40E
;//*   - IAR EW V7.40C
;//*   - Keil uVision V5.14
;//*
;//--------- END-OF-HEADER ---------------------------------*/

#ifdef __IAR_SYSTEMS_ASM__

;/*********************************************************************
;*
;*       These defines are necessary to be able to
;*       let run code in ram.
;*       As they are not defined in any IAR header file
;*       we need to define them here.
;*  References:  ARM ELF manual
;*               Document number: SWS ESPC 0003 B-02
;*               IAR ARM Assembler reference manual
;*               EWARM_AssemblerReference.ENU.pdf
;*/
#define SHT_PROGBITS   0x01
#define SHF_WRITE      0x01
#define SHF_EXECINSTR  0x04

;/*********************************************************************
;*
;*       IAR V5: Code in program memory
;*
;*/
#if   (__VER__ / 1000000) >= 5
        PUBLIC CM_IP_cksum
        SECTION .textrw:CODE:NOROOT(2)  ; // Code is in RAM
        SECTION_TYPE SHT_PROGBITS, SHF_WRITE | SHF_EXECINSTR
        THUMB

;/*********************************************************************
;*
;*       IAR V4: Locate Code in RAM
;*
;*/
#elif (__VER__ / 100) == 4
        PUBLIC CM_IP_cksum
        PUBWEAK ?init?tab?CODE_I
        RSEG CODE_I:CODE:NOROOT(2)
        RSEG CODE_ID:CODE:NOROOT(2)
`?*?CODE_ID`:

        RSEG INITTAB:CODE:ROOT(2)
        DATA
?init?tab?CODE_I:
        DCD      sfe(CODE_I) - sfb(CODE_I), sfb(CODE_I), sfb(CODE_ID)
        RSEG CODE_I:CODE:NOROOT(2)      ; // Code is in RAM
        THUMB
#else
#error Unsupported version of IAR toolchain
#endif

#elif (defined(__GNUC__) || defined(__SEGGER_CC__))

;/*********************************************************************
;*
;*       GNU
;*
;*/
        .text
        .type CM_IP_cksum, function
        .global  CM_IP_cksum
        .thumb
        .syntax unified
        .section .text, "ax"

#elif defined(__CC_ARM)

;/*********************************************************************
;*
;*       Keil MDK
;*
;*/
        AREA    cksumcode, CODE, READONLY
        EXPORT  CM_IP_cksum
        PRESERVE8
        THUMB

#else
#error Unsupported Assembler
#endif

;/*********************************************************************
;*
;*       Public code
;*
;**********************************************************************
;*/
;/*********************************************************************
;*
;*       CM_IP_cksum
;*
;* Function description
;*   Computes U16 internet checksum if start checksum is 0 or compares
;*   given checksum and returns 0 if valid.
;*
;* Register usage:
;*   R0    pData     - Must be HWord aligned
;*   R1    NumHWords
;*   R2    Sum
;*   R3 - R9, R12   Used for data transfers
;*   R13   SP
;*   R14   LR (contains return address)
;*   R15   PC
;*/

;//U32 IP_cksum(void * ptr, unsigned NumHWords, U32 Sum) {
#if (defined(__GNUC__) || defined(__SEGGER_CC__))
CM_IP_cksum:
#else
CM_IP_cksum
#endif
;//
;// 32-bit align pointer
;//
;// if ((int)pData & 2) {
;//   Sum += *pData++;
;//   NumHWords--;
;// }
        TST      R0,#2
        ITTT     NE
        LDRHNE   R3,[R0],#2
        ADDNE    R2,R2,R3
        SUBNE    R1,R1,#1
;//
;// Fast loop, 8 words = 16 halfwords at a time
;//
        SUBS     R1,R1,#16
        BCC      FastLoop8Done
        STMDB    SP!,{R4-R9}            ; // Push preserved registers. Note: R3, R12 are temp and do not need to be saved.
#if (defined(__GNUC__) || defined(__SEGGER_CC__))
FastLoop8:
#else
FastLoop8
#endif
        ldmia    R0!,{R3-R9, R12}       ; // load 32 bytes
        ADDS     R2,R2,R3
        ADCS     R2,R2,R4
        ADCS     R2,R2,R5
        ADCS     R2,R2,R6
        ADCS     R2,R2,R7
        ADCS     R2,R2,R8
        ADCS     R2,R2,R9
        ADCS     R2,R2,R12
        ADC      R2,R2,#0
        SUBS     R1,R1,#16
        BCS      FastLoop8
        ldmia    SP!,{R4-R9}            ; // Pop preserved registers
#if (defined(__GNUC__) || defined(__SEGGER_CC__))
FastLoop8Done:
#else
FastLoop8Done
#endif
;//
;// Handle remaining full words (between 0 and 7)
;//
        ADDS     R1,R1,#16
        ADDS     R1,R1,#0               ; // Clear carry
        LSR      R3,R1,#1               ; // NumWordsRem
        MOV      R12,#6
        MUL      R12,R12,R3
        SUB      R1,R1,R3,LSL #1        ; // Correct NumHWords
        LDR      R3,=DoneHandleRemWords ; // Max jump, in case no more words need to be handled
        SUB      R3,R3,R12              ; // <DoneHandleRemWords> - 6 * NumWords. Each word needs 6 bytes of instructions to be handled. LDR => 4 bytes, ADCS => 2 bytes
        ORR      R3,R3,#1               ; // Correct to uneven address
        BX       R3
        LDR      R3,[R0],#4             ; // 7 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 6 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 5 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 4 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 3 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 2 words
        ADCS     R2,R2,R3
        LDR      R3,[R0],#4             ; // 1 words
        ADCS     R2,R2,R3
#if (defined(__GNUC__) || defined(__SEGGER_CC__))
DoneHandleRemWords:
#else
DoneHandleRemWords
#endif
        ADC      R2,R2,#0               ; // 0 words
;//
;// Do final half word
;//
        TST      R1,#1
        BEQ      FoldCheckSum
        LDRH     R3,[R0,#+0]
        ADDS     R2,R2,R3
        ADCS     R2,R2,#0
;//
;//  Fold 32-bit sum to 16 bits
;//
;//  while (Sum >> 16) {
;//    Sum = (Sum & 0xffff) + (Sum >> 16);
;//  }
#if (defined(__GNUC__) || defined(__SEGGER_CC__))
FoldCheckSum:
#else
FoldCheckSum
#endif
        MOV      R3,R2,ROR #16
        ADD      R2,R2,R3
        MOV      R2,R2,LSR #16
;//
;//   return Sum;
;//
        MOV      R0,R2
        BX       LR

#if (defined(__GNUC__) || defined(__SEGGER_CC__))
        .end
#else
        END
#endif

/**************************** End of file ***************************/
