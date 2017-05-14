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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <dac.h>
#include <timer.h>
#include <rcc.h>
#include <adc.h>
#include <gpio.h>
#include <nvic.h>
#include <exti.h>
#include <usart.h>
#include <stdio.h>
#include "tick.h"
#include "spi_driver.h"
#include "pwrctl.h"
#include "hw.h"
#include "event.h"
#include "dps-model.h"

static void tim2_init(void);
static void clock_init(void);
static void adc1_init(void);
static void usart_init(void);
static void gpio_init(void);
static void exti_init(void);
static void dac_init(void);
static void button_irq_init(void);

static volatile uint16_t i_out_adc;
static volatile uint16_t i_out_trig_adc;
static volatile uint16_t v_in_adc;
static volatile uint16_t v_out_adc;
uint8_t channels[3]; /** for injected sampling, 4 channels max, for regular, 16 max */

/** Used to handle long presses */
#define LONGPRESS_TIME_MS (1000)
static event_t longpress_event;
static uint64_t longpress_start;
static bool longpress_detected;

/** We skip the first 40 samples. For a connected ESP8266 the first sample
  * will read a current draw of ~3A which will trigger the OCP.
  * @todo Investigate if the ESP8266 _really_ draws 3A or if it is a DPS issue
  */
#define STARTUP_SKIP_COUNT   (40)

/** Once we are checking for OCP, we need 20 over currents in a row to trigger
  * OCP. This is due to spikes in the ADC readings.
  * @todo Investigate if the spikes are real or is a DPS issue
  */
#define OCP_FILTER_COUNT (20)

#ifdef CONFIG_ADC_BENCHMARK
static uint64_t adc_tick_start;
#endif // CONFIG_ADC_BENCHMARK

/** Number of ADC conversions performed */
static uint32_t adc_counter;

/** The ADC reading on channel ADC_CHA_IOUT when power out was disabled on the
  * DPS5005 I used to develop OpenDPS. When testing on another unit I noticed
  * the current measurement was quite off, the reason being the ADC reading
  * at 0mA had an offset. For that reason, we calculate the individual offset
  * for the unit we're running on. Might work out... */
#define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
/** How many measurements do we take before calculating adc_i_offset */
#define ADC_I_OFFSET_COUNT  (1000)
/** Offset from golden value when measuring 0.00mA output current on this unit */
static int32_t adc_i_offset;
/** Are we measuring the offset not not? */
static bool measure_i_out = true;
/** Used to calculate mean value of ADC_CHA_IOUT when power out is disabled */
static uint32_t i_offset_calc;

/**
  * @brief Initialize the hardware
  * @retval None
  */
void hw_init(void)
{
    clock_init();
    systick_init();
    gpio_init();
    usart_init();
    adc1_init();
    exti_init();
    spi_init();
    dac_init();
    button_irq_init();

//    AFIO_MAPR |= AFIO_MAPR_PD01_REMAP; /** @todo The original DPS FW does this, things go south if I do it... */
}

/**
  * @brief Read latest ADC mesurements
  * @param i_out_raw latest I_out raw value
  * @param v_in_raw latest V_in raw value
  * @param v_out_raw latest V_out raw value
  * @retval none
  */
void hw_get_adc_values(uint16_t *i_out_raw, uint16_t *v_in_raw, uint16_t *v_out_raw)
{
    *i_out_raw = i_out_adc;
    *v_in_raw = v_in_adc;
    *v_out_raw = v_out_adc;
}

/**
  * @brief Initialize TIM4 that drives the backlight of the TFT
  * @retval None
  */
void hw_enable_backlight(void)
{
    rcc_periph_clock_enable(RCC_TIM4);
    TIM4_CNT = 1;
    TIM4_PSC = 5; // 0 => 6.41kHz 5 => 1.07kHz
    TIM4_CCER = 0x10;
    TIM4_CCMR1 = 0x6800;
    TIM4_ARR = 0x8bdf;
    TIM4_CCR2 = 0x5dc0;
    TIM4_DMAR = 0x81;
    // Set auto reload, start timer.
    TIM4_CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

/**
  * @brief Get the ADC valut that triggered the OCP
  * @retval Trivver value in mA
  */
uint16_t hw_get_itrig_ma(void)
{
    return i_out_trig_adc;
}

/**
  * @brief Check if it current press is a long press, inject event if so
  * @retval None
  */
void hw_longpress_check(void)
{
    if (longpress_event != event_none) {
        if (get_ticks() - longpress_start > LONGPRESS_TIME_MS) {
            event_put(longpress_event, press_long);
            longpress_detected = true;
            longpress_event = event_none;
        }
    }
}

#ifdef CONFIG_ADC_BENCHMARK
/**
  * @brief Print ADC speed
  * @retval none
  */
void hw_print_ticks(void)
{
    uint32_t temp = (uint32_t) get_ticks() - adc_tick_start;
    printf("%lu ADC ticks in %lu ms (%lu Hz)\n", adc_counter, temp, adc_counter/(temp/1000));
}
#endif // CONFIG_ADC_BENCHMARK

/**
  * @brief Check if SEL button is pressed
  * @retval true if SEL button is pressed, false otherwise
  */
bool hw_sel_button_pressed(void)
{
    return !gpio_get(BUTTON_SEL_PORT, BUTTON_SEL_PIN);
}

/**
  * @brief Add some filtering to OCPs
  * @retval None
  */
static void handle_ocp(uint16_t raw)
{
    static uint32_t ocp_count = 0;
    static uint32_t last_tick_counter = 0;
    if (last_tick_counter+1 == adc_counter) {
        ocp_count++;
        last_tick_counter++;
        if (ocp_count == OCP_FILTER_COUNT) {
            i_out_trig_adc = raw;
            pwrctl_enable_vout(false);
            event_put(event_ocp, 0);
        }
    } else {
        ocp_count = 0;
        last_tick_counter = adc_counter;
    }
}

/**
  * @brief ADC1 ISR
  * @retval None
  * @note ADC conversions are performed at a speed of ~21kHz
  */
void adc1_2_isr(void)
{
#ifdef CONFIG_ADC_BENCHMARK
    if (adc_counter == 0) {
        adc_tick_start = get_ticks();
    }
#endif // CONFIG_ADC_BENCHMARK

    // Clear Injected End Of Conversion (JEOC)
    ADC_SR(ADC1) &= ~ADC_SR_JEOC;
    // If pwrctl_i_limit_raw == 0, the setting hasn't been read from past yet
    adc_counter++;
    uint32_t i = adc_read_injected(ADC1, 1);
    /** @todo Make sure power out is not enabled during this measurement */
    if (measure_i_out) {
        if (adc_counter < ADC_I_OFFSET_COUNT) {
            i_offset_calc += i;
        } else {
            adc_i_offset = ADC_CHA_IOUT_GOLDEN_VALUE - (i_offset_calc / ADC_I_OFFSET_COUNT);
            measure_i_out = false;
        }
    }
    if (pwrctl_i_limit_raw) {
        if (adc_counter >= STARTUP_SKIP_COUNT) {
            i += adc_i_offset;
            if (i > pwrctl_i_limit_raw) { /** OCP! */
                handle_ocp(i);
            }
            i_out_adc = i;
        }
    }
    v_in_adc  = adc_read_injected(ADC1, 2);
    v_out_adc = adc_read_injected(ADC1, 3);
}

/**
  * @brief Handle USART1 interrupts
  * @retval None
  */
void usart1_isr(void)
{
    if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) &&
        ((USART_SR(USART1) & USART_SR_RXNE) != 0)) {
        uint8_t ch = usart_recv(USART1);
        event_put(event_uart_rx, ch);
    }

#ifdef TX_IRQ
    if (((USART_CR1(USART1) & USART_CR1_TXEIE) != 0) &&
        ((USART_SR(USART1) & USART_SR_TXE) != 0)) {
        int32_t data;
        data = ringbuf_read_ch(&output_ring, NULL);
        if (data == -1) {
            USART_CR1(USART1) &= ~USART_CR1_TXEIE;
        } else {
            usart_send(USART1, data);
        }
    }
#endif // TX_IRQ
}
/**
  * @brief Enable clocks
  * @retval None
  */
static void clock_init(void)
{
    rcc_clock_setup_in_hsi_out_48mhz();
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_AFIO);
}

/**
  * @brief Enable ADC1
  * @retval None
  */
static void adc1_init(void)
{
    int i;
    nvic_set_priority(NVIC_ADC1_2_IRQ, 0);
    nvic_enable_irq(NVIC_ADC1_2_IRQ);
    rcc_periph_clock_enable(RCC_ADC1);
    adc_power_off(ADC1); // Make sure the ADC doesn't run during config.

    /** @todo Use scan mode which does all channels in one sweep and generates the interrupt/EOC/JEOC flags set at the end of all channels, not each one. */
    adc_enable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    // Use TIM2 TRGO as injected conversion trigger
    adc_enable_external_trigger_injected(ADC1,ADC_CR2_JEXTSEL_TIM2_TRGO);
    // Generate the ADC1_2_IRQ
    adc_enable_eoc_interrupt_injected(ADC1);
    adc_set_right_aligned(ADC1);
    //adc_enable_temperature_sensor(); /** @todo Use internal temperature sensor for monitoring */
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

    channels[0] = ADC_CHA_IOUT;
    channels[1] = ADC_CHA_VIN;
    channels[2] = ADC_CHA_VOUT;
    // channels[3] = 16; // Temperature sensor
    adc_set_injected_sequence(ADC1, 3, channels);

    adc_power_on(ADC1);

    // Wait for ADC starting up.
    for (i = 0; i < 800000; i++) {
        __asm__("nop");
    }

    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);

    tim2_init(); // Start TIM2 trigger now that the ADC is online
}

/**
  * @brief Initialize USART1
  * @retval None
  */
static void usart_init(void)
{
    rcc_periph_clock_enable(RCC_USART1);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    nvic_enable_irq(NVIC_USART1_IRQ);
    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    // Enable USART1 Receive interrupt.
    USART_CR1(USART1) |= USART_CR1_RXNEIE;

    usart_enable(USART1);
}

/**
  * @brief Initialize GPIO
  * @retval None
  */
static void gpio_init(void)
{
    // PA0  O 1 OD     (50 Mhz)                       U7
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO0);

    // PA1  I 1 PuPd                  M2 button
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO1);
    gpio_set(GPIOA, GPIO1);

    // PA2  I 1 PuPd                  SEL button
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO2);
    gpio_set(GPIOA, GPIO2);

    // PA3  I 1 PuPd                  M1 button
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO3);
    gpio_set(GPIOA, GPIO3);

    // DAC1_OUT and DAC2_OUT controls the output voltage via a TL594 Pulse-Width-Modulation Control Circuit
    // http://www.ti.com/product/tl594?qgpn=tl594
    // PA4  I 0 An                    DAC1_OUT        TL594.2 (1IN-)
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);

    // PA5  I 0 An                    DAC2_OUT        TL594.15 (2IN-)
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO5);

    // PA6  I 1 PuPd
//tft    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6);
//tft    gpio_set(GPIOA, GPIO6);

    // PA7  I 0 An                    ADC1_IN7        R30-U2.7:V_OUT-B
//tft    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO7);

    // PA8  O 0 PP     (50 Mhz)       TFT.7           (not used by TFT)
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);

    // PA9  I 1 Flt
//    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO9);

    // PA10 I 1 Flt
//    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);

    // PA11 I 1 Flt
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);

    // PA12 I 1 Flt
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);

    // PA13 I 0 PuPd
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO13);
    gpio_clear(GPIOA, GPIO13);

    // PA14 I 1 PuPd                                  SWDCLK
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO14);
    gpio_set(GPIOA, GPIO14);

    // PA15 O 1 OD     (50 Mhz)                       R41-TL594.16 (2IN+)
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO15);

    // PB0  I 0 An                                    R7/R2-R14-D4
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);

    // PB1  I 0 An                    ADC1_IN9        R33-U2.1:V_OUT-A
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);

    // PB2  I 1 Flt
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO2);

    // PB3  O 1 OD     (50 Mhz)                       R11-R17-R25-U2.5 (V_inB+)
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO3);

    // PB4  I 1 PuPd                  PWR button
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO4);
    gpio_set(GPIOB, GPIO4);

    // PB5  I 1 PuPd                  Rotary press
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO5);
    gpio_set(GPIOB, GPIO5);

    // PB6  O 0 PP     (50 Mhz)                       NC?
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6);

    // PB7  O 0 AF-PP  (50 Mhz)       TIM4_CH2
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);

    // PB8  I 1 PuPd                  Rotary enc
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO8);
    gpio_set(GPIOB, GPIO8);

    // PB9  I 1 PuPd                  Rotary enc
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);
    gpio_set(GPIOB, GPIO9);

    // PB10 I 1 Flt
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);

    // PB11 O 1 PP     (50 Mhz)       nPwrEnable      R29-TFT.2
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);

    // PB12 O 1 PP     (50 Mhz)       SPI2_NSS        TFT_RESET
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_set(GPIOB, GPIO12);

    // PB13 O 1 AF-PP  (50 Mhz)       SPI2_SCK
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13);

    // PB14 O 1 PP     (50 Mhz)       SPI2_MISO       TFT_A0
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO14);

    // PB15 O 1 AF-PP  (50 Mhz)       SPI2_MOSI
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO15);

    // PC0  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO0);

    // PC1  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO1);

    // PC2  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO2);

    // PC3  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO3);

    // PC4  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO4);

    // PC5  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO5);

    // PC6  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

    // PC7  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO7);

    // PC8  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO8);

    // PC9  I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO9);

    // PC10 I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);

    // PC11 I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);

    // PC12 I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);

    // PC13 I 0 Flt
//    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO13);
#ifdef DPS5015
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
#endif

	// PC14 I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO14);

    // PC15 I 0 Flt
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15);


    // PD0  I 1 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO0);

    // PD1  O 1 PP     (50 Mhz)                       U7
    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);

    // PD2  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO2);

    // PD3  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO3);

    // PD4  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO4);

    // PD5  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO5);

    // PD6  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

    // PD7  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO7);

    // PD8  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO8);

    // PD9  I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO9);

    // PD10 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);

    // PD11 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);

    // PD12 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);

    // PD13 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO13);

    // PD14 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO14);

    // PD15 I 0 Flt
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15);
}

/**
  * @brief Enable EXTI where most buttons are connected
  * @retval None
  */
static void exti_init(void)
{
    AFIO_EXTICR2 = 0x11;
    AFIO_EXTICR3 = 0x11;
}

/**
  * @brief Initialize the DAC that is used to control voltage output
  * @retval None
  */
static void dac_init(void)
{
    rcc_periph_clock_enable(RCC_DAC);
    DAC_CR       = 0;          // 0x40007400 disable
    DAC_SWTRIGR  = 0x00000000; // 0x40007404
    DAC_DHR12R1  = 0x00000000; // 0x40007408 0V output
    DAC_DHR12R2  = 0x00000000; // 0x40007414
    DAC_CR       = 0x00030003; // 0x40007400 BOFF2, EN2, BOFF1, EN1
}

/**
  * @brief Set up TIM2 for injected ADC1 sampling
  * @retval None
  */
static void tim2_init(void)
{
    uint32_t timer = TIM2;
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_mode(timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(timer, 0xFF);
    timer_set_prescaler(timer, 0x8);
    timer_set_clock_division(timer, 0x0);
    timer_set_master_mode(timer, TIM_CR2_MMS_UPDATE); // Generate TRGO on every update.
    timer_enable_counter(timer);
}

/**
  * @brief Start a (possible) long press
  * @param event the event that will be triggered by a long press
  * @retval None
  */
static void longpress_begin(event_t event)
{
    longpress_event = event;
    longpress_detected = false;
    longpress_start = get_ticks();
}

/**
  * @brief End long press check
  * @retval true if it was a long press
  */
static bool longpress_end(void)
{
    bool temp = longpress_detected;
    longpress_event = event_none;
    longpress_start = 0;
    longpress_detected = false;
    return temp;
}

/**
  * @brief Select button ISR
  * @note BUTTON_SEL_isr & friends defined in hw.h
  * @retval None
  */
void BUTTON_SEL_isr(void)
{
    static bool falling = true;
    exti_reset_request(BUTTON_SEL_EXTI);
    if (falling) {
        longpress_begin(event_button_sel);
        exti_set_trigger(BUTTON_SEL_EXTI, EXTI_TRIGGER_RISING);
    } else {
        if (!longpress_end()) {
            // Not a long press, send short press
            event_put(event_button_sel, press_short);
        }
        exti_set_trigger(BUTTON_SEL_EXTI, EXTI_TRIGGER_FALLING);
    }
    falling = !falling;
}

/**
  * @brief M1 button ISR
  * @retval None
  */
void BUTTON_M1_isr(void)
{
    static bool falling = true;
    exti_reset_request(BUTTON_M1_EXTI);
    if (falling) {
        exti_set_trigger(BUTTON_M1_EXTI, EXTI_TRIGGER_RISING);
    } else {
        event_put(event_button_m1, press_short);
        exti_set_trigger(BUTTON_M1_EXTI, EXTI_TRIGGER_FALLING);
    }
    falling = !falling;
}

/**
  * @brief M2 button ISR
  * @retval None
  */
void BUTTON_M2_isr(void)
{
    static bool falling = true;
    exti_reset_request(BUTTON_M2_EXTI);
    if (falling) {
        exti_set_trigger(BUTTON_M2_EXTI, EXTI_TRIGGER_RISING);
    } else {
        event_put(event_button_m2, press_short);
        exti_set_trigger(BUTTON_M2_EXTI, EXTI_TRIGGER_FALLING);
    }
    falling = !falling;
}

/**
  * @brief Enable button ISR
  * @retval None
  */
void BUTTON_ENABLE_isr(void)
{
    static bool falling = true;
    exti_reset_request(BUTTON_ENABLE_EXTI);
    if (falling) {
        exti_set_trigger(BUTTON_ENABLE_EXTI, EXTI_TRIGGER_RISING);
    } else {
        event_put(event_button_enable, press_short);
        exti_set_trigger(BUTTON_ENABLE_EXTI, EXTI_TRIGGER_FALLING);
    }
    falling = !falling;
}

/**
  * @brief Rotare encoder ISR
  * @retval None
  */
void BUTTON_ROTARY_isr(void)
{
    if (exti_get_flag_status(BUTTON_ROT_PRESS_EXTI)) {
        exti_reset_request(BUTTON_ROT_PRESS_EXTI);
        static bool falling = true;
        if (falling) {
            longpress_begin(event_rot_press);
            exti_set_trigger(BUTTON_ROT_PRESS_EXTI, EXTI_TRIGGER_RISING);
        } else {
            if (!longpress_end()) {
                // Not a long press, send short press
                event_put(event_rot_press, press_short);
            }
            exti_set_trigger(BUTTON_ROT_PRESS_EXTI, EXTI_TRIGGER_FALLING);
        }
        falling = !falling;
    }

    if (exti_get_flag_status(BUTTON_ROT_A_EXTI)) {
        exti_reset_request(BUTTON_ROT_A_EXTI);
        bool a = (((uint16_t) GPIO_IDR(BUTTON_ROT_A_PORT)) & BUTTON_ROT_A_PIN) ? 1 : 0; // Slightly faster than gpio_get(...)
        bool b = (((uint16_t) GPIO_IDR(BUTTON_ROT_B_PORT)) & BUTTON_ROT_B_PIN) ? 1 : 0;
        if (a == b) {
            event_put(event_rot_left, press_short);
        } else {
            event_put(event_rot_right, press_short);
        }
    }

    if (exti_get_flag_status(BUTTON_ROT_B_EXTI)) {
        exti_reset_request(BUTTON_ROT_B_EXTI);
    }
}

/**
  * @brief Initialize IRQs for the buttons
  * @retval None
  */
static void button_irq_init(void)
{
    nvic_enable_irq(BUTTON_SEL_NVIC);
    exti_select_source(BUTTON_SEL_EXTI, BUTTON_SEL_PORT);
    exti_set_trigger(BUTTON_SEL_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(BUTTON_SEL_EXTI);

    nvic_enable_irq(BUTTON_M1_NVIC);
    exti_select_source(BUTTON_M1_EXTI, BUTTON_M1_PORT);
    exti_set_trigger(BUTTON_M1_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(BUTTON_M1_EXTI);

    nvic_enable_irq(BUTTON_M2_NVIC);
    exti_select_source(BUTTON_M2_EXTI, BUTTON_M2_PORT);
    exti_set_trigger(BUTTON_M2_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(BUTTON_M2_EXTI);

    nvic_enable_irq(BUTTON_ENABLE_NVIC);
    exti_select_source(BUTTON_ENABLE_EXTI, BUTTON_ENABLE_PORT);
    exti_set_trigger(BUTTON_ENABLE_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(BUTTON_ENABLE_EXTI);

    nvic_enable_irq(BUTTON_ROTARY_NVIC);

    exti_select_source(BUTTON_ROT_A_EXTI, BUTTON_ROT_A_PORT);
    exti_set_trigger(BUTTON_ROT_A_EXTI, EXTI_TRIGGER_BOTH);
    exti_enable_request(BUTTON_ROT_A_EXTI);

    exti_select_source(BUTTON_ROT_B_EXTI, BUTTON_ROT_B_PORT);
    exti_set_trigger(BUTTON_ROT_B_EXTI, EXTI_TRIGGER_BOTH);
    exti_enable_request(BUTTON_ROT_B_EXTI);

    exti_select_source(BUTTON_ROT_PRESS_EXTI, BUTTON_ROT_PRESS_PORT);
    exti_set_trigger(BUTTON_ROT_PRESS_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(BUTTON_ROT_PRESS_EXTI);
}
