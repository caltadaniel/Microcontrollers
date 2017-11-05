/*************************************************************************
 *  © 2015 Microchip Technology Inc.
 *
 *  Project Name:    FFT
 *  FileName:        demo_fft_main.c
 *  Dependencies:    xc.h, stdint.h, math.h, fft.h
 *  Processor:       PIC24, dsPIC
 *  Compiler:        XC16
 *  IDE:             MPLAB® X
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Description: This file contains a demo code for the FFT library functions.
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

#include <xc.h>
#include <stdint.h>
#include <math.h>
#include "fft.h"

// PIC24FJ128GA010 CONFIGURATION SETTINGS
_CONFIG2(POSCMOD_HS & FNOSC_PRIPLL)
_CONFIG1(WDTPS_PS128 & FWPSA_PR128 & WINDIS_ON & FWDTEN_OFF & ICS_PGx2 & JTAGEN_OFF)

#define PIx2  ((double)2*3.14159265)

// arrays for input/output FFT
volatile int16_t re[FFT_POINTS_NUMBER];
volatile int16_t im[FFT_POINTS_NUMBER];

// arrays for harmonics' amplitudes and phases
// only harmonics from 0(DC) to FFT_POINTS_NUMBER/2 are valid
volatile uint16_t amplitude[FFT_POINTS_NUMBER/2];
volatile int16_t phase[FFT_POINTS_NUMBER/2];

int main(void)
{

uint16_t i;

    // load the arrays with a sum of sine waves
    for(i=0; i<FFT_POINTS_NUMBER; i++)
    {
        re[i] = (int16_t)(
                 2048.0+ // DC amplitude 2048
                 8192.0*cos(PIx2*i/(FFT_POINTS_NUMBER/2)+(PIx2*10/360))+ // 2nd harmonic amplitude 8192, phase 10 deg
                 8192.0*sin(PIx2*i/(FFT_POINTS_NUMBER/4))+ // 4th harmonic amplitude 8192, phase -90 deg (sin(x) = cos(x-90deg))
                 8192.0*cos(PIx2*i/(FFT_POINTS_NUMBER/7))  // 7th harmonic amplitude 8192, phase 0 deg
                );
                
        // imaginary part must be zero for the real signal
        im[i] = 0;
    }

    FFT((int16_t*)re,(int16_t*)im);

    // calculate harmonics amplitudes and phases
    // only harmonics from 0(DC) to FFT_POINTS_NUMBER/2 are valid
    for(i=0; i<FFT_POINTS_NUMBER/2; i++)
    {
        // DC amplitude in the frequency dimension must match the amplitude
        // in time dimension (2048/1 = 2048)
        // Harmonics amplitudes in the frequency dimension must be half of
        // amplitudes in time dimension (8192/2 = 4096)
        amplitude[i] = FFTAmplitude(re[i],im[i]);
        phase[i] = FFTPhase(re[i],im[i]);
    }

    while(1);

    return 1;
}
