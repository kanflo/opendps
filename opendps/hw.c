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
#include <scb.h>
#include "tick.h"
#include "spi_driver.h"
#include "pwrctl.h"
#include "hw.h"
#include "event.h"
#include "dps-model.h"

/** Linker file symbols */
extern uint32_t *_ram_vect_start;
extern uint32_t *_ram_vect_end;
extern uint32_t *vector_table;


static void common_timer_init(enum rcc_periph_clken rcc, uint32_t timer, uint32_t period, uint32_t prescaler);
static void tim2_init(void);
static void clock_init(void);
static void adc1_init(void);
static void usart_init(void);
static void gpio_init(void);
static void exti_init(void);
static void dac_init(void);
static void button_irq_init(void);
static void copy_vectors(void);
#ifdef CONFIG_FUNCGEN_ENABLE
static void tim3_init(void);
void (*funcgen_tick)(void) = &fg_noop;
volatile uint16_t cur_time_hiw;
#endif

static volatile uint16_t i_out_adc;
static volatile uint16_t i_out_trig_adc;
static volatile uint16_t v_in_adc;
static volatile uint16_t v_out_adc;
static volatile uint16_t v_out_trig_adc;
static volatile uint64_t last_button_down;

typedef enum {
    adc_cha_i_out = 0,
    adc_cha_v_in,
    adc_cha_v_out,
    adc_cha_max,

} adc_channel_t;
_Static_assert (adc_cha_max <= 4, "Max 4 channels for injected sampling");

const uint8_t channels[adc_cha_max] = { ADC_CHA_IOUT, ADC_CHA_VIN, ADC_CHA_VOUT }; /** Must have the same order as adc_channel_t */

/** Used to handle long presses */
#define LONGPRESS_TIME_MS (1000)
static volatile event_t longpress_event;
static volatile uint64_t longpress_start;
static volatile bool longpress_detected;
/** Used to filter SET press from SET + ROT */
static volatile bool set_pressed = false;
static volatile bool set_skip = false;
static volatile bool m1_pressed = false;
static volatile bool m2_pressed = false;
static volatile bool m1_and_m2_pressed = false;

#define DEBOUNCE_TIME_MS    (30)

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

/** Once we are checking for OVP, we need 20 over voltages in a row to trigger
  * OVP. This is due to spikes in the ADC readings.
  * @todo Investigate if the spikes are real or is a DPS issue
  */
#define OVP_FILTER_COUNT (20)

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
#ifndef ADC_CHA_IOUT_GOLDEN_VALUE
 #define ADC_CHA_IOUT_GOLDEN_VALUE  (0x45)
#endif
/** How many measurements do we take before calculating adc_i_offset */
#define ADC_I_OFFSET_COUNT  (1000)
/** Offset from golden value when measuring 0.00mA output current on this unit */
static int32_t adc_i_offset;
/** Are we measuring the offset or not? */
static bool measure_i_out = true;
/** Used to calculate mean value of ADC_CHA_IOUT when power out is disabled */
static uint32_t i_offset_calc;

/**
  * @brief Initialize the hardware
  * @retval None
  */
void hw_init(void)
{
    copy_vectors();
    clock_init();
    systick_init();
    gpio_init();
    usart_init();
    adc1_init();
    exti_init();
    spi_init();
    dac_init();
    button_irq_init();
#ifdef CONFIG_FUNCGEN_ENABLE
    tim3_init();
#endif

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
  * @brief Set the output voltage DAC value
  * @param v_dac the value to set to
  * @retval none
  */
void hw_set_voltage_dac(uint16_t v_dac)
{
    DAC_DHR12R1(DAC1) = v_dac;
}

/**
  * @brief Set the output current DAC value
  * @param i_dac the value to set to
  * @retval none
  */
void hw_set_current_dac(uint16_t i_dac)
{
    DAC_DHR12R2(DAC1) = i_dac;
}

/**
  * @brief Initialize TIM4 that drives the backlight of the TFT
  * @retval None
  */
void hw_enable_backlight(uint8_t brightness)
{
    rcc_periph_clock_enable(RCC_TIM4);
    TIM4_CNT = 1;
    TIM4_PSC = 5; // 0 => 6.41kHz 5 => 1.07kHz
    TIM4_CCER = 0x10;
    TIM4_CCMR1 = 0x6800;
    TIM4_ARR = 0x8bdf;
    //TIM4_CCR2 = 0x5dc0;
    // no brightness increase above 0x7FFF, so 1% would be 0x147
    TIM4_CCR2 = brightness * 0x147;
    TIM4_DMAR = 0x81;
    // Set auto reload, start timer.
    TIM4_CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}
/**
  * @brief Set TFT backlight value
  * @retval None
  */
void hw_set_backlight(uint8_t brightness)
{
    TIM4_CCR2 = brightness * 0x147;
}

/**
  * @brief Get TFT backlight value
  * @retval Brightness percentage
  */
uint8_t hw_get_backlight(void)
{
    return TIM4_CCR2 / 0x147;
}

/**
  * @brief Get the ADC valut that triggered the OCP
  * @retval Trigger value in mA
  */
uint16_t hw_get_itrig_ma(void)
{
    return i_out_trig_adc;
}

/**
  * @brief Get the ADC value that triggered the OVP
  * @retval Trigger value in mV
  */
uint16_t hw_get_vtrig_mv(void)
{
    return v_out_trig_adc;
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
    dbg_printf("%u ADC ticks in %u ms (%u Hz)\n", adc_counter, temp, adc_counter/(temp/1000));
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
  * @brief Add some filtering to OVPs
  * @retval None
  */
static void handle_ovp(uint16_t raw)
{
    static uint32_t ovp_count = 0;
    static uint32_t last_tick_counter = 0;
    if (last_tick_counter+1 == adc_counter) {
        ovp_count++;
        last_tick_counter++;
        if (ovp_count == OVP_FILTER_COUNT) {
            v_out_trig_adc = raw;
            pwrctl_enable_vout(false);
            event_put(event_ovp, 0);
        }
    } else {
        ovp_count = 0;
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
    uint32_t i = adc_read_injected(ADC1, adc_cha_i_out + 1); // Yes, this is correct

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
            if (i > pwrctl_i_limit_raw && pwrctl_vout_enabled()) { /** OCP! */
                handle_ocp(i);
            }
            i_out_adc = i;
        }
    }

    v_in_adc = adc_read_injected(ADC1, adc_cha_v_in + 1); // Yes, this is correct
    v_out_adc = adc_read_injected(ADC1, adc_cha_v_out + 1); // Yes, this is correct

    /** Check to see if an over voltage limit has been triggered */
    if (pwrctl_v_limit_raw) {
        if (v_out_adc > pwrctl_v_limit_raw && pwrctl_vout_enabled()) { /** OVP! */
            handle_ovp(v_out_adc);
        }
    }

#ifdef CONFIG_FUNCGEN_ENABLE
    (*funcgen_tick)();
#endif
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
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);
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
    adc_set_injected_sequence(ADC1, adc_cha_max, (uint8_t*) channels);
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
    usart_set_baudrate(USART1, CONFIG_BAUDRATE); /** Baudrate set in makefile */
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
    /** @todo: replace all gpio_set_mode with a struct (uint32_t gpioport, uint8_t mode, uint8_t cnf, uint16_t gpios)*/
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
#ifdef TFT_CSN_PORT
    gpio_set_mode(TFT_CSN_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, TFT_CSN_PIN);
    gpio_set(TFT_CSN_PORT, TFT_CSN_PIN);
#else
    
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
#endif
//    gpio_clear(GPIOA, GPIO8); /** @todo DPS5005 comms version with fw 1.3 does this, check functions */

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
#if defined(DPS5015) || defined(DPS5020)
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
    DAC_CR(DAC1)      = 0;          // 0x40007400 disable
    DAC_SWTRIGR(DAC1) = 0x00000000; // 0x40007404
    DAC_DHR12R1(DAC1) = 0x00000000; // 0x40007408 0V output
    DAC_DHR12R2(DAC1) = 0x00000000; // 0x40007414
    DAC_CR(DAC1)      = 0x00030003; // 0x40007400 BOFF2, EN2, BOFF1, EN1
}

/**
  * @brief Common code to initialize a timer to avoid code duplication.
  * @param timer      the timer to initialize
  * @param period     the period to use
  * @param prescaler  the prescaler divider - 1
  */
static void common_timer_init(enum rcc_periph_clken rcc, uint32_t timer, uint32_t period, uint32_t prescaler)
{
    rcc_periph_clock_enable(rcc);
    rcc_periph_reset_pulse(((rcc >> 5) << 4) | (rcc & 0x1F)); /* There's no conversion macro here, but the formula is simple */
    timer_set_mode(timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(timer, period);
    timer_set_prescaler(timer, prescaler);
    timer_set_clock_division(timer, 0x0);
} 

/**
  * @brief Set up TIM2 for injected ADC1 sampling
  * This timer fires at 20915Hz (that is 48MHz / 9 / 255)
  * @retval None
  */
static void tim2_init(void)
{
    uint32_t timer = TIM2;
    common_timer_init(RCC_TIM2, timer, 0xFF, 8);
    timer_set_master_mode(timer, TIM_CR2_MMS_UPDATE); // Generate TRGO on every update.
    timer_enable_counter(timer);
}

#ifdef CONFIG_FUNCGEN_ENABLE
/**
  * @brief Set up TIM3 for function generator sampling
  * This timer fires at 1000000Hz (that is 48MHz / 1 / 48) 
  * @retval None
  */
static void tim3_init(void)
{
    uint32_t timer = TIM3;
    common_timer_init(RCC_TIM3, timer, 0xFFFF, 47);
    timer_enable_counter(timer);
    nvic_enable_irq(NVIC_TIM3_IRQ);
    nvic_set_priority(NVIC_TIM3_IRQ, 1);
    timer_enable_irq(timer, TIM_DIER_UIE); /* Update IRQ enable */
}

void tim3_isr(void) {
  if (timer_get_flag(TIM3, TIM_DIER_UIE)) {
      timer_clear_flag(TIM3, TIM_DIER_UIE);
  }
  cur_time_hiw ++;
}

uint32_t cur_time_us(void)
{
    uint32_t t = timer_get_counter(TIM3);
    t |= (cur_time_hiw << 16U);
    return t;
}
/**
  * @brief Do nothing
  * This avoid to test a (shared) variable and branch in an isr, and instead, branch to a function in all cases
  */
void fg_noop(void) {
}
#endif

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
  * @brief Detect if button is bouncing
  * @retval true if button is bounding 
  */
static bool is_bouncing(void)
{
    uint64_t t = get_ticks();
    if (t - last_button_down < DEBOUNCE_TIME_MS)
        return true;
    last_button_down = t;
    return false;
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
        if (is_bouncing()) return;
        set_pressed = true;
        set_skip = false;
        longpress_begin(event_button_sel);
        exti_set_trigger(BUTTON_SEL_EXTI, EXTI_TRIGGER_RISING);
    } else {
        set_pressed = false;
        if (!set_skip) {
            if (!longpress_end()) {
                // Not a long press, send short press
                event_put(event_button_sel, press_short);
            }
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
        if (is_bouncing()) return;
        m1_pressed = true;
        if (m2_pressed)
            m1_and_m2_pressed = true;
        exti_set_trigger(BUTTON_M1_EXTI, EXTI_TRIGGER_RISING);
    } else {
        m1_pressed = false;

        if (!m2_pressed) {
            if (m1_and_m2_pressed) {
                m1_and_m2_pressed = false;
                event_put(event_buttom_m1_and_m2, press_short);
            } else {
                event_put(event_button_m1, press_short);
            }
        }

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
        if (is_bouncing()) return;
        m2_pressed = true;
        if (m1_pressed)
            m1_and_m2_pressed = true;
        exti_set_trigger(BUTTON_M2_EXTI, EXTI_TRIGGER_RISING);
    } else {
        m2_pressed = false;

        if (!m1_pressed) {
            if (m1_and_m2_pressed) {
                m1_and_m2_pressed = false;
                event_put(event_buttom_m1_and_m2, press_short);
            } else {
                event_put(event_button_m2, press_short);
            }
        }

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
        if (is_bouncing()) return;
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
            if (is_bouncing()) return;
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
            if (set_pressed) {
                set_skip = true;
                (void) longpress_end();
                event_put(event_rot_left_set, press_short);
            } else {
                event_put(event_rot_left, press_short);
            }
        } else {
            if (set_pressed) {
                set_skip = true;
                (void) longpress_end();
                event_put(event_rot_right_set, press_short);
            } else {
                event_put(event_rot_right, press_short);
            }
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

/**
  * @brief Relocate the vector table to the internal SRAM
  * @retval None
  */
static void copy_vectors(void)
{
    uint32_t v_size = (uint32_t) &_ram_vect_end - (uint32_t) &_ram_vect_start;
    volatile uint32_t *v_rom = (uint32_t*) &vector_table;
    volatile uint32_t *v_ram = (uint32_t*) &_ram_vect_start;

    for(uint32_t i = 0; i < v_size/4; i++) {
        v_ram[i] = v_rom[i];
    }
 
    /* Enable the SYSCFG peripheral clock*/
//    rcc_periph_clock_enable(RCC_SYSCFG);
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // Not Reset, Clock
 
    SCB_VTOR = (uint32_t) &_ram_vect_start;
    /* Remap SRAM at 0x20000000 */
//    SYSCFG_CFGR1 &= SYSCFG_CFGR1_MEM_MODE;
//    SYSCFG_CFGR1 |= SYSCFG_CFGR1_MEM_MODE_SRAM;
}

