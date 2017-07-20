#ifndef __DPS_MODEL_H__
#define __DPS_MODEL_H__

// if no other DPSxxxx model is specified, we will assume DPS5005
//#define DPS5015 

/*
 * K - angle factor
 * C - offset
 * for ADC
 * K = (I1-I2)/(ADC1-ADC2)
 * C = I1-K*ADC1
 * for DAC
 * K = (DAC1-DAC2)/(I1-I2)
 * C = DAC1-K*I1
 */

/* Exmaple for voltage ADC calibration
 * you can see real ADC value in CLI mode's stat:
 *                         ADC(U): 394
 * and you have to measure real voltage with reference voltmeter.
 * ADC value -> Voltage calculation
 * ADC  394 =  5001 mV
 * ADC  782 = 10030 mV
 * ADC 1393 = 18000 mV
 * K = (U1-U2)/(ADC1-ADC2) = (18000-5001)/(1393-394) = 76.8461538
 * C = U1-K*ADC1= 5001-K*394  = -125.732
 *
 * */

/* Example for voltage DAC calibration 
 * DAC value -> Voltage
 * You can write DAC values to DHR12R1 register 0x40007408
 * over openocd console:
 * mww 0x40007408 77
 * and measure real voltage with reference voltmeter.
 *
 * DAC   77 =  1004 mV
 *      872 = 12005 mV
 *  K = (DAC1-DAC2)/(U1-U2) = (77-872)/(1004-12005) = .072266
 *  C = DAC1-K*U1 = 77-(K*1004) = 4.44477774
 *
 * */


#ifdef DPS5015 
// DPS5015

#define ADC_CHA_IOUT_GOLDEN_VALUE  (59)
#define A_ADC_K (double)6.8403
#define A_ADC_C (double)-394.06
#define A_DAC_K (double)0.166666
#define A_DAC_C (double)261.6666


#define V_ADC_K (double)13.012
#define V_ADC_C (double)-125.732


#define V_DAC_K (double)0.072266
#define V_DAC_C (double)4.444777

#else
// DPS5005

#define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
#define A_ADC_K (double)1.713
#define A_ADC_C (double)-118.51
#define A_DAC_K (double)0.652
#define A_DAC_C (double)288.611

#define V_DAC_K (double)0.072
#define V_DAC-C (double)1.85
#define V_ADC_K (double)13.164
#define V_ADC_C (double)-100.751

#endif



#endif // __DPS_MODEL_H__
