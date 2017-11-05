/*************************************************************************
 *  © 2015 Microchip Technology Inc.                                       
 *  
 *  Project Name:    FFT
 *  FileName:        fft.h
 *  Dependencies:    xc.h, stdint.h
 *  Processor:       PIC24, dsPIC
 *  Compiler:        XC16
 *  IDE:             MPLAB® X                        
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Description: This file contains declarations for the FFT library functions.
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

#ifndef _FFT_H_
#define _FFT_H_

#include <xc.h>
#include <stdint.h>

/*************************************************************************
 This parameter defines number of points/samples for the calculation.
 The valid values are 8, 16, 32, 64, 128, 256, 512, 1024 and 2048. 
 If required transform size doesn’t match exactly the valid sizes
 then the size should be rounded to largest valid size and
 not used points should be set to zero.
 *************************************************************************/
#define FFT_POINTS_NUMBER      64

/*************************************************************************
 Function:

 void FFT(int16_t* pRe, int16_t* pIm)

 Description:
 
 This function calculates FFT for the points provided in real and imaginary parts arrays.
 In most cases the input signal is real (not complex), the data points must be stored in
 the real part array and the imaginary array elements must be set to zero.
 The sizes of the real and imaginary arrays must be equal to the transform size defined
 by FFT_POINTS_NUMBER in fft.h file. After calculation the result (harmonics) are written
 to the same input data arrays.

 Parameters:

 int16_t* re – pointer to array with the input signal vector (real part).
 int16_t* im – pointer to array with the input signal vector (imaginary part),
             for real signals this vector values must be set to zero.

 Returned data:

 int16_t* re – pointer to array where the harmonics are stored (real part).
 int16_t* im – pointer to array where the harmonics are stored (imaginary part).
*************************************************************************/
void FFT(int16_t* pRe, int16_t* pIm);

/*************************************************************************
 Function:

 uint16_t FFTAmplitude(int16_t re, int16_t im)

 Description:
 
 This function calculates absolute value (harmonic amplitude) of
 the complex number (A = SQRT(Re^2 + Im^2)).

 Parameters:

 int16_t re – real part of the complex number.
 int16_t im –imaginary part of the complex number.

 Returned data:

 The function returns the absolute value (harmonic amplitude)
 for the complex number entered.
*************************************************************************/
uint16_t FFTAmplitude(int16_t re, int16_t im);

/*************************************************************************
 Function:

 int16_t FFTPhase(int16_t re, int16_t im);

 Description:
 
 This function calculates an argument (harmonic phase) of the complex number
 (D = (180/PI)*ARCTAN(Im/Re)) in degrees with maximum error +-1 degree.
 
 Parameters:

 short re – real part of the complex number.
 short im – imaginary part of the complex number.

 Returned data:
 The function returns an argument (harmonic phase) in degrees
 for the complex number entered.
*************************************************************************/
int16_t FFTPhase(int16_t re, int16_t im);

#endif
