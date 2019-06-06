/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Johan Kanflo (github.com/kanflo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __HW_H__
#define __HW_H__

/** We can provide max 800mV below Vin */
#define V_IO_DELTA (800)

#define ADC_CHA_IOUT  (7)
#define ADC_CHA_VIN   (8)
#define ADC_CHA_VOUT  (9)

#define TFT_RST_PORT GPIOB
#define TFT_RST_PIN  GPIO12
#define TFT_A0_PORT  GPIOB
#define TFT_A0_PIN   GPIO14

#define BUTTON_SEL_PORT GPIOA
#define BUTTON_SEL_PIN  GPIO2
#define BUTTON_SEL_EXTI EXTI2
#define BUTTON_SEL_isr  exti2_isr
#define BUTTON_SEL_NVIC NVIC_EXTI2_IRQ

#define BUTTON_M1_PORT GPIOA
#define BUTTON_M1_PIN  GPIO3
#define BUTTON_M1_EXTI EXTI3
#define BUTTON_M1_isr  exti3_isr
#define BUTTON_M1_NVIC NVIC_EXTI3_IRQ

#define BUTTON_M2_PORT GPIOA
#define BUTTON_M2_PIN  GPIO1
#define BUTTON_M2_EXTI EXTI1
#define BUTTON_M2_isr  exti1_isr
#define BUTTON_M2_NVIC NVIC_EXTI1_IRQ

#define BUTTON_ENABLE_PORT GPIOB
#define BUTTON_ENABLE_PIN  GPIO4
#define BUTTON_ENABLE_EXTI EXTI4
#define BUTTON_ENABLE_isr  exti4_isr
#define BUTTON_ENABLE_NVIC NVIC_EXTI4_IRQ

#define BUTTON_ROT_PRESS_PORT GPIOB
#define BUTTON_ROT_PRESS_PIN  GPIO5
#define BUTTON_ROT_PRESS_EXTI EXTI5
#define BUTTON_ROT_A_PORT     GPIOB
#define BUTTON_ROT_A_PIN      GPIO8
#define BUTTON_ROT_A_EXTI     EXTI8
#define BUTTON_ROT_B_PORT     GPIOB
#define BUTTON_ROT_B_PIN      GPIO9
#define BUTTON_ROT_B_EXTI     EXTI9
#define BUTTON_ROTARY_isr     exti9_5_isr
#define BUTTON_ROTARY_NVIC    NVIC_EXTI9_5_IRQ


/**
  * @brief Initialize the hardware
  * @retval None
  */
void hw_init(void);

/**
  * @brief Read latest ADC mesurements
  * @param i_out_raw latest I_out raw value
  * @param v_in_raw latest V_in raw value
  * @param v_out_raw latest V_out raw value
  * @retval none
  */
void hw_get_adc_values(uint16_t *i_out_raw, uint16_t *v_in_raw, uint16_t *v_out_raw);

/**
  * @brief Set the output voltage DAC value
  * @param v_dac the value to set to
  * @retval none
  */
void hw_set_voltage_dac(uint16_t v_dac);

/**
  * @brief Set the output current DAC value
  * @param i_dac the value to set to
  * @retval none
  */
void hw_set_current_dac(uint16_t i_dac);

/**
  * @brief Initialize TIM4 that drives the backlight of the TFT
  * @retval None
  */
void hw_enable_backlight(uint8_t brighness);

/**
  * @brief Set TFT backlight value
  * @retval None
  */
void hw_set_backlight(uint8_t brightness);

/**
  * @brief Get TFT backlight value
  * @retval Brightness percentage
  */
uint8_t hw_get_backlight(void);

/**
  * @brief Get the ADC value that triggered the OCP
  * @retval Trigger value in mA
  */
uint16_t hw_get_itrig_ma(void);

/**
  * @brief Get the ADC value that triggered the OVP
  * @retval Trigger value in mV
  */
uint16_t hw_get_vtrig_mv(void);

/**
  * @brief Check if it current press is a long press, inject event if so
  * @retval None
  */
void hw_longpress_check(void);

/**
  * @brief Check if SEL button is pressed
  * @retval true if SEL button is pressed, false otherwise
  */
bool hw_sel_button_pressed(void);

#ifdef CONFIG_ADC_BENCHMARK
/**
  * @brief Print ADC speed
  * @retval none
  */
void hw_print_ticks(void);
#endif // CONFIG_ADC_BENCHMARK

#ifdef CONFIG_FUNCGEN_ENABLE
/**
  * @brief A pointer to the function generator's that's called upon each ADC update
  * @retval none
  */
extern void (*funcgen_tick)(void);
/** 
  * @brief An empty function. This is used to avoid testing in the ISR and branch in all cases (fixed latency)
  * @retval none
  */  
void fg_noop(void);

/**
  * @brief The current time in microsecond unit, updated by a timer
  */
uint32_t cur_time_us(void); 
#endif

#endif // __HW_H__
