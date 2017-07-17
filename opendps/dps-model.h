#ifndef __DPS_MODEL_H__
#define __DPS_MODEL_H__

// if no other DPSxxxx model is specified, we will assume DPS5005

#define DPS5015 

/*
 * K - angle factor
 * C - offset
 * K = (I1-I2)/(ADC1-ADC2)
 * C = I1-K*ADC1
 */

#ifdef DPS5015 
// DPS5015

#define ADC_CHA_IOUT_GOLDEN_VALUE  (59)
#define CUR_ADC_K (double)6.8403
//#define CUR_ADC_C (double)-460.00
#define CUR_ADC_C (double)-394.06
/*
#define CUR_ADC_K (double)6.7770
#define CUR_ADC_C (double)-394.06
*/

#define CUR_DAC_K (double)0.166666
#define CUR_DAC_C (double)261.6666

#else
// DPS5005

#define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
#define CUR_ADC_K (double)1.713
#define CUR_ADC_C (double)-118.51
#define CUR_DAC_K (double)0.652
#define CUR_DAC_C (double)288.611

#endif



#endif // __DPS_MODEL_H__
