/*************************************************************************
 *  © 2015 Microchip Technology Inc.
 *
 *  Project Name:    FFT
 *  FileName:        fft_.s
 *  Dependencies:    None
 *  Processor:       PIC24, dsPIC
 *  Compiler:        XC16
 *  IDE:             MPLAB® X
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Description: This file contains 2 points Cooley-Tukey butterfly code.
 *
 *************************************************************************/
/**************************************************************************
 * MICROCHIP SOFTWARE NOTICE AND DISCLAIMER: You may use this software, and
 * any derivatives created by any person or entity by or on your behalf,
 * exclusively with Microchip's products in accordance with applicable
 * software license terms and conditions, a copy of which is provided for
 * your referencein accompanying documentation. Microchip and its licensors
 * retain all ownership and intellectual property rights in the
 * accompanying software and in all derivatives hereto.
 *
 * This software and any accompanying information is for suggestion only.
 * It does not modify Microchip's standard warranty for its products. You
 * agree that you are solely responsible for testing the software and
 * determining its suitability. Microchip has no obligation to modify,
 * test, certify, or support the software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE, ITS INTERACTION WITH
 * MICROCHIP'S PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY
 * APPLICATION.
 *
 * IN NO EVENT, WILL MICROCHIP BE LIABLE, WHETHER IN CONTRACT, WARRANTY,
 * TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), STRICT
 * LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE,
 * FOR COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE,
 * HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY
 * OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWABLE BY LAW,
 * MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
 * SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID
 * DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF
 * THESE TERMS.
 *************************************************************************
 *
 * written by Anton Alkhimenok 04/02/2015
 *
 *************************************************************************/

.global _FFT2

.section .text

_FFT2:
    push.d w8
    push.d w10

    ; w4 -> coeff RE
    ; w5 -> coeff IM
    ; w0 -> w8  - first RE address
    ; w1 -> w9  - first IM address
    ; w2 -> w10 - second RE address
    ; w3 -> w11 - second IM address
    mov    w0, w8
    mov    w1, w9
    mov    w2, w10
    mov    w3, w11

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; PLUS PATH
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ; w0 - first RE
    ; w1 - first IM
    ; w2 - second RE
    ; w3 - second IM
    mov  [w8],  w0
    mov  [w9],  w1
    mov  [w10], w2
    mov  [w11], w3

    asr  w0, #1, w0
    asr  w1, #1, w1
    asr  w2, #1, w2
    asr  w3, #1, w3

    add  w0, w2, w6    ; real

    ; SAVE W6 RESULT (REAL)
    mov  w6, [w8]

    add  w1, w3, w7    ; imaginary

    ; SAVE W7 RESULT (IMAGINARY)
    mov  w7, [w9]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; MINUS PATH
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    sub  w0, w2, w0    ; real  
    sub  w1, w3, w1    ; imaginary

    ; multiply by coefficient
    mul.ss w0, w4, w2 ; re by re
    mul.ss w1, w5, w6 ; im by im
    sub    w2, w6, w2
    subb   w3, w7, w3

    ; SAVE RESULT W3 HERE (REAL)
    sl     w3, #1, w3
    btsc   w2, #15
    bset   w3, #0
    mov    w3, [w10]

    ; multiply by coefficient
    mul.ss w0, w5, w2 ; re by im
    mul.ss w1, w4, w6 ; im by re
    add    w2, w6, w2
    addc   w3, w7, w3

    ; SAVE RESULT W3 HERE (IMAGINARY)
    sl     w3, #1, w3
    btsc   w2, #15
    bset   w3, #0
    mov    w3, [w11]

    pop.d w10
    pop.d w8


    return

.end
