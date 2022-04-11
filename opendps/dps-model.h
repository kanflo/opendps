#ifndef __DPS_MODEL_H__
#define __DPS_MODEL_H__

// if no other DPSxxxx model is specified, we will assume DPS5005

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

/** Contribution by @cleverfox */
#if defined(DPS5020)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (20000) // Please note that the UI currently does not handle settings larger that 9.99A
 #endif
 #define CURRENT_DIGITS 2
 #define CURRENT_DECIMALS 2
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (59)
 #define A_ADC_K (float)6.75449f
 #define A_ADC_C (float)-358.73f
 #define A_DAC_K (float)0.16587f
 #define A_DAC_C (float)243.793f
 #define V_ADC_K (float)13.2930f
 #define V_ADC_C (float)-179.91f
 #define V_DAC_K (float)0.07528f
 #define V_DAC_C (float)6.68949f

 #define VIN_ADC_K (float)16.956f
 #define VIN_ADC_C (float)6.6895f
#elif defined(DPS5015)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (15000)
 #endif
 #define CURRENT_DIGITS 2
 #define CURRENT_DECIMALS 2
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (59)
 #define A_ADC_K (float)6.8403f
 #define A_ADC_C (float)-394.06f
 #define A_DAC_K (float)0.166666f
 #define A_DAC_C (float)261.6666f
 #define V_ADC_K (float)13.012f
 #define V_ADC_C (float)-125.732f
 #define V_DAC_K (float)0.072266f
 #define V_DAC_C (float)4.444777f
#elif defined(DPS5005)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (5000)
 #endif
 #define CURRENT_DIGITS 1
 #define CURRENT_DECIMALS 3
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
 #define A_ADC_K (float)1.713f
 #define A_ADC_C (float)-118.51f
 #define A_DAC_K (float)0.652f
 #define A_DAC_C (float)288.611f
 #define V_DAC_K (float)0.072f
 #define V_DAC_C (float)1.85f
 #define V_ADC_K (float)13.164f
 #define V_ADC_C (float)-100.751f
#elif defined(DP50V5A)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (5000)
 #endif
 #define CURRENT_DIGITS 1
 #define CURRENT_DECIMALS 3
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
 #define A_DAC_K (float)0.6402f
 #define A_DAC_C (float)299.5518f
 #define A_ADC_K (float)1.74096f
 #define A_ADC_C (float)-121.3943805f
 #define V_DAC_K (float)0.07544f
 #define V_DAC_C (float)2.1563f
 #define V_ADC_K (float)13.253f
 #define V_ADC_C (float)-103.105f
#elif defined(DPS3005)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (5000)
 #endif
 #define CURRENT_DIGITS 1
 #define CURRENT_DECIMALS 3
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (0x00)
 #define A_ADC_K (float)1.751f
 #define A_ADC_C (float)-1.101f
 #define A_DAC_K (float)0.653f
 #define A_DAC_C (float)262.5f
 #define V_DAC_K (float)0.0761f
 #define V_DAC_C (float)2.2857f
 #define V_ADC_K (float)13.131f
 #define V_ADC_C (float)-111.9f
#elif defined(DPS3003)
 #ifndef CONFIG_DPS_MAX_CURRENT
  #define CONFIG_DPS_MAX_CURRENT (3000)
 #endif
 #define CURRENT_DIGITS 1
 #define CURRENT_DECIMALS 3
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (0x00)
 #define A_ADC_K (float)0.99676f
 #define A_ADC_C (float)-44.3156f
 #define A_DAC_K (float)1.12507f
 #define A_DAC_C (float)256.302f
 #define V_DAC_K (float)0.12237f
 #define V_DAC_C (float)10.1922f
 #define V_ADC_K (float)8.16837f
 #define V_ADC_C (float)-115.582f
 #define VIN_ADC_K (float)16.7897f
 #define VIN_ADC_C (float)16.6448f
#else
 #error "Please set MODEL to the device you want to build for"
#endif // MODEL

/** These are constant across all models currently but may require tuning for each model */
#ifndef VIN_ADC_K
 #define VIN_ADC_K (float)16.746f
#endif

#ifndef VIN_ADC_C
 #define VIN_ADC_C (float)64.112f
#endif

#define VIN_VOUT_RATIO (float)1.1f /** (Vin / VIN_VOUT_RATIO) = Max Vout */

#endif // __DPS_MODEL_H__
