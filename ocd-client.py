#!/usr/bin/python

import socket
import sys

prompt = "> "

ocd_sock = False

def ocd_exchange(str = ""):
    output = ""
    line = ""
    got_prompt = False
    if len(str) > 0:
        ocd_sock.send(str)
    while 1:
        try:
            ch = ocd_sock.recv(1)
#            print "'%s' [%02x]" % (ch, ord(ch))
            if len(ch) > 0:
                if ch == '\d':
                    pass
                elif ch == '\n':
 #                   print "--------------------------"
 #                   print line
                    if "%s\n" % line != str:
                        output += "%s\n" % line
                    line = ""
                elif ord(ch) >= 32 and ord(ch) <= 126:
                    line += ch
#                    print "[%s]" % line
                if line == prompt:
#                    print "==========================="
#                    print line
                    got_prompt = True
                    break
            else:
                break
        except socket.timeout, e:
            break
#    print line

#    print ">>>>>>>>>>>>>>>>>>"
#    print output
#    print "<<<<<<<<<<<<<<<<<<"
#    if got_prompt:
    return output.strip()
#    else:
#        return False

def ocd_sync():
    return ocd_exchange()

def ocd_read(address):
    response = ocd_exchange("mdw 0x%08x 1\n" % (address))
    parts = response.split(":")
    if len(parts) != 2:
        print ("Parsing error: %s" % response)
        return False
    temp_address = int(parts[0].strip(), 16)
    value = int(parts[1].strip(), 16)
    if temp_address != address:
        print ("Address error: %s" % response)
        return False
    return value

def ocd_write(address, value):
    response = ocd_exchange("mww 0x%08x 0x%08x\n" % (address, value))

known_pins = {
    "PA0"  : ["", "U7 "],
    "PA1"  : ["M2 button", ""],
    "PA2"  : ["SEL button", ""],
    "PA3"  : ["M1 button", ""],
    "PA4"  : ["DAC1_OUT", "TL594.2 (1IN-)"],
    "PA5"  : ["DAC2_OUT", "TL594.15 (2IN-)"],
    "PA7"  : ["ADC1_IN7", "R30-U2.7:V_OUT-B (measures Vout)"],
    "PA8"  : ["TFT.7", "(not used by TFT)"],
    "PA14" : ["", "SWDCLK"],
    "PA15" : ["", "R41-TL594.16 (2IN+)"],
    "PB0"  : ["ADC1_IN8", "R7/R2-R14-D4 (measures Vin)"],
    "PB1"  : ["ADC1_IN9", "R33-U2.1:V_OUT-A (measures Iout)"],
    "PB3"  : ["", "R11-R17-R25-U2.5 (V_inB+)"],
    "PB4"  : ["PWR button", ""],
    "PB5"  : ["Rotary press", ""],
    "PB6"  : ["", "NC?"],
    "PB7"  : ["TIM4_CH2", ""],
    "PB8"  : ["Rotary enc", ""],
    "PB9"  : ["Rotary enc", ""],
    "PB11" : ["nPwrEnable", "R29-TFT.2 (TFT_VCC)"],
    "PB12" : ["SPI2_NSS", "TFT_RESET"],
    "PB13" : ["SPI2_SCK", ""],
    "PB14" : ["SPI2_MISO", "TFT_A0"],
    "PB15" : ["SPI2_MOSI", ""],
    "PD1"  : ["", "U7"],
}

ADC1_BASE  = 0x40012400
AFIO_BASE  = 0x40010000
DAC_BASE   = 0x40007400
DMA1_BASE  = 0x40020000
EXTI_BASE  = 0x40010400
GPIOA_BASE = 0x40010800
GPIOB_BASE = 0x40010c00
GPIOC_BASE = 0x40011000
GPIOD_BASE = 0x40011400
GPIOE_BASE = 0x40011800
GPIOF_BASE = 0x40011C00
GPIOG_BASE = 0x40012000
RCC_BASE   = 0x40021000
SPI1_BASE  = 0x40013000
SPI2_BASE  = 0x40003800
TIM1_BASE  = 0x40012C00
TIM2_BASE  = 0x40000000
TIM3_BASE  = 0x40000400
TIM4_BASE  = 0x40000800
TIM6_BASE  = 0x40001000
TIM7_BASE  = 0x40001400
TIM15_BASE = 0x40014000
TIM16_BASE = 0x40014400
TIM17_BASE = 0x40014800

# SPI1 registers
SPI_CR1 = 0x00
SPI_CR2 = 0x04
SPI_SR = 0x08
SPI_DR = 0x0C
SPI_CRCPR = 0x10
SPI_RXCRCR = 0x14
SPI_TXCRCR = 0x18


# TIM2 to TIM5 registers (checked)
TIMx_CR1   = 0x00
TIMx_CR2   = 0x04
TIMx_SMC   = 0x08
TIMx_DIER  = 0x0c
TIMx_SR    = 0x10
TIMx_EGR   = 0x14
TIMx_CCMR1 = 0x18
TIMx_CCMR2 = 0x1c
TIMx_CCER  = 0x20
TIMx_CNT   = 0x24
TIMx_PSC   = 0x28
TIMx_ARR   = 0x2c
TIMx_RCR   = 0x30
TIMx_CCR1  = 0x34
TIMx_CCR2  = 0x38
TIMx_CCR3  = 0x3c
TIMx_CCR4  = 0x40
TIMx_BDTR  = 0x44
TIMx_DCR   = 0x48
TIMx_DMAR  = 0x4c


# ADC registers (checked)
ADC_SR    = 0x00
ADC_CR1   = 0x04
ADC_CR2   = 0x08
ADC_SMPR1 = 0x0C
ADC_SMPR2 = 0x10
ADC_JOFR1 = 0x14
ADC_JOFR2 = 0x18
ADC_JOFR3 = 0x1C
ADC_JOFR4 = 0x20
ADC_HTR   = 0x24
ADC_LTR   = 0x28
ADC_SQR1  = 0x2C
ADC_SQR2  = 0x30
ADC_SQR3  = 0x34
ADC_JSQR  = 0x38
ADC_JDR1  = 0x3C
ADC_JDR2  = 0x40
ADC_JDR3  = 0x44
ADC_JDR4  = 0x48
ADC_DR    = 0x4C

# DAC registers (checked)
DAC_CR      = 0x00
DAC_SWTRIGR = 0x04
DAC_DHR12R1 = 0x08
DAC_DHR12L1 = 0x0C
DAC_DHR8R1  = 0x10
DAC_DHR12R2 = 0x14
DAC_DHR12L2 = 0x18
DAC_DHR8R2  = 0x1c
DAC_DHR12RD = 0x20
DAC_DHR12LD = 0x24
DAC_DHR8RD  = 0x28
DAC_DOR1    = 0x2C
DAC_DOR2    = 0x30
DAC_SR      = 0x34

# GPIO registers (checked)
GPIOx_CRL    = 0x00
GPIOx_CRH    = 0x04
GPIOx_IDR    = 0x08
GPIOx_ODR    = 0x0c
GPIOx_BSRR   = 0x10
GPIOx_BRR    = 0x14
GPIOx_LCKR   = 0x18

# EXTI registers (checked)
EXTI_IMR     = 0x00
EXTI_EMR     = 0x04
EXTI_RTSR    = 0x08
EXTI_FTSR    = 0x0c
EXTI_SWIER   = 0x10
EXTI_PR      = 0x14

# AFIO registers (checked)
AFIO_EVCR    = 0x00
AFIO_MAPR    = 0x04
AFIO_EXTICR1 = 0x08
AFIO_EXTICR2 = 0x0c
AFIO_EXTICR3 = 0x10
AFIO_EXTICR4 = 0x14
AFIO_MAPR2   = 0x18

# RCC registers (checked)
RCC_CR       = 0x00
RCC_CFGR     = 0x04
RCC_CIR      = 0x08
RCC_APB2RSTR = 0x0c
RCC_APB1RSTR = 0x10
RCC_AHBENR   = 0x14
RCC_APB2ENR  = 0x18
RCC_APB1ENR  = 0x1c
RCC_BDCR     = 0x20
RCC_CSR      = 0x24
RCC_CFGR2    = 0x2c

# DMA registers (checked)
DMA_ISR      = 0x00
DMA_IFCR     = 0x04
DMA_CCR1     = 0x08
DMA_CNDTR    = 0x0C
DMA_CPAR1    = 0x10
DMA_CMAR1    = 0x14
#             0x18
DMA_CCR2     = 0x1C
DMA_CNDTR    = 0x20
DMA_CPAR2    = 0x24
DMA_CMAR2    = 0x28
#              0x2C
DMA_CCR3     = 0x30
DMA_CNDTR    = 0x34
DMA_CPAR3    = 0x38
DMA_CMAR3    = 0x3C
#              0x40
DMA_CCR4     = 0x44
DMA_CNDTR    = 0x48
DMA_CPAR4    = 0x4C
DMA_CMAR4    = 0x50
#              0x54
DMA_CCR5     = 0x58
DMA_CNDTR    = 0x5C
DMA_CPAR5    = 0x60
DMA_CMAR5    = 0x64
#              0x68
DMA_CCR6     = 0x6C
DMA_CNDTR    = 0x70
DMA_CPAR6    = 0x74
DMA_CMAR6    = 0x78
#              0x7C
DMA_CCR7     = 0x80
DMA_CNDTR    = 0x84
DMA_CPAR7    = 0x88



register_map = [
  [0x40023000, 0x400233FF, "CRC"],
  [0x40022000, 0x400223FF, "Flash memory interface"],
  [0x40021000, 0x400213FF, "Reset and clock control RCC"],
  [0x40020000, 0x400203FF, "DMA1"],
  [0x40014800, 0x40014BFF, "TIM17 timer"],
  [0x40014400, 0x400147FF, "TIM16 timer"],
  [0x40014000, 0x400143FF, "TIM15 timer"],
  [0x40013800, 0x40013BFF, "USART1"],
  [0x40013000, 0x400133FF, "SPI1"],
  [0x40012C00, 0x40012FFF, "TIM1 timer"],
  [0x40012400, 0x400127FF, "ADC1"],
  [0x40011800, 0x40011BFF, "GPIO Port E"],
  [0x40011400, 0x400117FF, "GPIO Port D"],
  [0x40011000, 0x400113FF, "GPIO Port C"],
  [0x40010C00, 0x40010FFF, "GPIO Port B"],
  [0x40010800, 0x40010BFF, "GPIO Port A"],
  [0x40010400, 0x400107FF, "EXTI"],
  [0x40010000, 0x400103FF, "AFIO"],
  [0x40007800, 0x40007BFF, "CEC"],
  [0x40007400, 0x400077FF, "DAC"],
  [0x40007000, 0x400073FF, "Power control PWR"],
  [0x40006C00, 0x40006FFF, "Backup registers (BKP)"],
  [0x40005800, 0x40005BFF, "I2C2"],
  [0x40005400, 0x400057FF, "I2C1"],
  [0x40004800, 0x40004BFF, "USART3"],
  [0x40004400, 0x400047FF, "USART2"],
  [0x40003800, 0x40003BFF, "SPI2"],
  [0x40003000, 0x400033FF, "Independent watchdog (IWDG)"],
  [0x40002C00, 0x40002FFF, "Window watchdog (WWDG)"],
  [0x40002800, 0x40002BFF, "RTC"],
  [0x40001400, 0x400017FF, "TIM7 timer"],
  [0x40001000, 0x400013FF, "TIM6 timer"],
  [0x40000800, 0x40000BFF, "TIM4 timer"],
  [0x40000400, 0x400007FF, "TIM3 timer"],
  [0x40000000, 0x400003FF, "TIM2 timer"]
]


port_addresses = [GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE, GPIOE_BASE, GPIOF_BASE, GPIOG_BASE]

CRL_CNF_IN = ["An", "Flt", "PuPd", "RFU"]
CRL_CNF_IN_ANALOG = 0
CRL_CNF_IN_FLOAT = 1
CRL_CNF_IN_PUPD = 2
CRL_CNF_IN_RFU = 3

CRL_CNF_OUT = ["PP", "OD", "AF-PP", "AF-OD"]
CRL_CNF_OUT_PP = 0
CRL_CNF_OUT_OD = 1
CRL_CNF_OUT_AF_PP = 2
CRL_CNF_OUT_AF_OD = 3

CRL_MODE = [0, 10, 2, 50]
CRL_MODE_IN = 0
CRL_MODE_OUT_10MHZ = 1
CRL_MODE_OUT_2MHZ = 2
CRL_MODE_OUT_50MHZ = 3

def print_gpio_pin(port_nbr, pin, cnf, mode, level):
    cnf_i = ["GPIO_CNF_INPUT_ANALOG", "GPIO_CNF_INPUT_FLOAT", "GPIO_CNF_INPUT_PULL_UPDOWN", "##### RESERVED #####"]
    cnf_o = ["GPIO_CNF_OUTPUT_PUSHPULL", "GPIO_CNF_OUTPUT_OPENDRAIN", "GPIO_CNF_OUTPUT_ALTFN_PUSHPULL", "GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN"]
    modes = ["GPIO_MODE_INPUT", "GPIO_MODE_OUTPUT_10_MHZ", "GPIO_MODE_OUTPUT_2_MHZ", "GPIO_MODE_OUTPUT_50_MHZ"]

    if mode == 0:
        cnfs = cnf_i
    else:
        cnfs = cnf_o
    a = "gpio_set_mode(GPIO%c, %s, %s, GPIO%d);\n" % (ord('A')+port_nbr, modes[mode], cnfs[cnf], pin)
    if mode == 0 and cnf == 2:
        if level == 0:
            a += "gpio_clear(GPIO%c, GPIO%d);\n" % (ord('A')+port_nbr, pin)
        elif level == 1:
            a += "gpio_set(GPIO%c, GPIO%d);\n" % (ord('A')+port_nbr, pin)
        else:
            a += "rusk!!!!!!!!"
    return a


def dump_port_settings(port_nbr):
#    print("Checking GPIO%c" % (ord('A') + port_nbr))
    crl = ocd_read(port_addresses[port_nbr] + GPIOx_CRL)
    crh = ocd_read(port_addresses[port_nbr] + GPIOx_CRH)
    idr = ocd_read(port_addresses[port_nbr] + GPIOx_IDR)
    odr = ocd_read(port_addresses[port_nbr] + GPIOx_ODR)
    for pin in range(0, 16):
        if pin < 8:
            mode = (crl >> (4*pin)) & 3
            cnf = (crl >> (4*pin+2)) & 3
        else:
            mode = (crh >> (4*(pin-8))) & 3
            cnf = (crh >> (4*(pin-8)+2)) & 3
        pin_s = "P%c%d" % (ord('A')+port_nbr, pin)
        if mode == CRL_MODE_IN:
            level = (idr >> pin) & 1
            info = "%-4s I %d %-5s" % (pin_s, level, CRL_CNF_IN[cnf])
        else:
            level = (odr >> pin) & 1
            info = "%-4s O %d %-5s  (%d Mhz)" % (pin_s, level, CRL_CNF_OUT[cnf], CRL_MODE[mode])
        if pin_s in known_pins:
            desc = known_pins[pin_s]
        else:
            desc = ["", ""]
        setting = print_gpio_pin(port_nbr, pin, cnf, mode, level)
        print("// %-30s %-15s %s" % (info, desc[0], desc[1]))
        print("%s" % (setting))



def dump_gpio_port_settings():
    for port in range(0, 4):
        dump_port_settings(port)


def dump_reg(name, address):
    value = ocd_read(address)
    print("%-8s : 0x%08x  [0x%08x]" % (name, value, address))


"""
CR1      : 0x00000081  [00] ARPE=1 CEN=1
CR2      : 0x00000000  [04]
SMCR     : 0x00000000  [08]
DIER     : 0x00000000  [0c]
SR       : 0x0000001f  [10] CC4IF=1 CC3IF=1 CC2IF=1 CC1IF=1 UIF=1
EGR      : 0x00000000  [14]
CCMR1    : 0x00006800  [18] OC2M=2 OC2PE=1
CCMR2    : 0x00000000  [1c]
CCER     : 0x00000010  [20] CC2P=1
CNT      : 0x000056ed  [24] <counter>
PSC      : 0x00000000  [28]
ARR      : 0x00005dbf  [2c] <auto reload>
CCR1     : 0x00000000  [34]
CCR2     : 0x00005dc0  [38] <TIM2 compare register 2>
CCR3     : 0x00000000  [3c]
CCR4     : 0x00000000  [40]
DCR      : 0x00000000  [48]
DMAR     : 0x00000081  [4c] DMA address

U           ARR         CCR2
1.00        0x00005dbf  0x00005dc0

"""

def dump_tim_settings(name, base_addr):
    print("%s settings" % (name))
    dump_reg("CR1",   base_addr + TIMx_CR1)
    dump_reg("CR2",   base_addr + TIMx_CR2)
    dump_reg("SMC",   base_addr + TIMx_SMC)
    dump_reg("DIER",  base_addr + TIMx_DIER)
    dump_reg("SR",    base_addr + TIMx_SR)
    dump_reg("EGR",   base_addr + TIMx_EGR)
    dump_reg("CCMR1", base_addr + TIMx_CCMR1)
    dump_reg("CCMR2", base_addr + TIMx_CCMR2)
    dump_reg("CCER",  base_addr + TIMx_CCER)
    dump_reg("CNT",   base_addr + TIMx_CNT)
    dump_reg("PSC",   base_addr + TIMx_PSC)
    dump_reg("ARR",   base_addr + TIMx_ARR)
    dump_reg("RCR",   base_addr + TIMx_RCR)
    dump_reg("CCR1",  base_addr + TIMx_CCR1)
    dump_reg("CCR2",  base_addr + TIMx_CCR2)
    dump_reg("CCR3",  base_addr + TIMx_CCR3)
    dump_reg("CCR4",  base_addr + TIMx_CCR4)
    dump_reg("BDTR",  base_addr + TIMx_BDTR)
    dump_reg("DCR",   base_addr + TIMx_DCR)
    dump_reg("DMAR",  base_addr + TIMx_DMAR)
"""
    TIM4 settings

    CR1      : 0x00000081  [0x40000800]
    CR2      : 0x00000000  [0x40000804]
    SMC      : 0x00000000  [0x40000808]
    DIER     : 0x00000000  [0x4000080c]
    SR       : 0x0000001f  [0x40000810]
    EGR      : 0x00000000  [0x40000814]
    CCMR1    : 0x00006800  [0x40000818]
    CCMR2    : 0x00000000  [0x4000081c]
    CCER     : 0x00000010  [0x40000820]
    CNT      : 0x00000ebb  [0x40000824]
    PSC      : 0x00000000  [0x40000828]
    ARR      : 0x00005dbf  [0x4000082c]
    RCR      : 0x00000000  [0x40000830]
    CCR1     : 0x00000000  [0x40000834]
    CCR2     : 0x00001f40  [0x40000838]
    CCR3     : 0x00000000  [0x4000083c]
    CCR4     : 0x00000000  [0x40000840]
    BDTR     : 0x00000000  [0x40000844]
    DCR      : 0x00000000  [0x40000848]
    DMAR     : 0x00000081  [0x4000084c]
"""
def dump_tim4_settings():
    dump_tim_settings("TIM4", TIM4_BASE) # TFT backlight intensity


def dump_dac_settings():
    print("DAC settings")
    dump_reg("CR",      DAC_BASE + DAC_CR)
    dump_reg("SWTRIGR", DAC_BASE + DAC_SWTRIGR)
    dump_reg("DHR12R1", DAC_BASE + DAC_DHR12R1)
    dump_reg("DHR12L1", DAC_BASE + DAC_DHR12L1)
    dump_reg("DHR8R1",  DAC_BASE + DAC_DHR8R1)
    dump_reg("DHR12R2", DAC_BASE + DAC_DHR12R2)
    dump_reg("DHR12L2", DAC_BASE + DAC_DHR12L2)
    dump_reg("DHR8R2",  DAC_BASE + DAC_DHR8R2)
    dump_reg("DHR12RD", DAC_BASE + DAC_DHR12RD)
    dump_reg("DHR12LD", DAC_BASE + DAC_DHR12LD)
    dump_reg("DHR8RD",  DAC_BASE + DAC_DHR8RD)
    dump_reg("DOR1",    DAC_BASE + DAC_DOR1)
    dump_reg("DOR2",    DAC_BASE + DAC_DOR2)
    dump_reg("SR",      DAC_BASE + DAC_SR)
"""
    DAC settings

    CR       : 0x00030003  [0x40007400]
    SWTRIGR  : 0x00000000  [0x40007404]
    DHR12R1  : 0x00000003  [0x40007408]
    DHR12L1  : 0x00000030  [0x4000740c]
    DHR8R1   : 0x00000000  [0x40007410]
    DHR12R2  : 0x0000008c  [0x40007414]
    DHR12L2  : 0x000008c0  [0x40007418]
    DHR8R2   : 0x00000008  [0x4000741c]
    DHR12RD  : 0x008c0003  [0x40007420]
    DHR12LD  : 0x08c00030  [0x40007424]
    DHR8RD   : 0x00000800  [0x40007428]
    DOR1     : 0x00000003  [0x4000742c]
    DOR2     : 0x0000008c  [0x40007430]
    SR       : 0x00000000  [0x40007434]
"""

def dump_adc1_settings():
    print("ADC1 settings")
    dump_reg("SR",      ADC1_BASE + ADC_SR)
    dump_reg("CR1",     ADC1_BASE + ADC_CR1)
    dump_reg("CR2",     ADC1_BASE + ADC_CR2)
    dump_reg("SMPR1",   ADC1_BASE + ADC_SMPR1)
    dump_reg("SMPR2",   ADC1_BASE + ADC_SMPR2)
    dump_reg("JOFR1",   ADC1_BASE + ADC_JOFR1)
    dump_reg("JOFR2",   ADC1_BASE + ADC_JOFR2)
    dump_reg("JOFR3",   ADC1_BASE + ADC_JOFR3)
    dump_reg("JOFR4",   ADC1_BASE + ADC_JOFR4)
    dump_reg("HTR",     ADC1_BASE + ADC_HTR)
    dump_reg("LTR",     ADC1_BASE + ADC_LTR)
    dump_reg("SQR1",    ADC1_BASE + ADC_SQR1)
    dump_reg("SQR2",    ADC1_BASE + ADC_SQR2)
    dump_reg("SQR3",    ADC1_BASE + ADC_SQR3)
    dump_reg("JSQR",    ADC1_BASE + ADC_JSQR)
    dump_reg("JDR1",    ADC1_BASE + ADC_JDR1)
    dump_reg("JDR2",    ADC1_BASE + ADC_JDR2)
    dump_reg("JDR3",    ADC1_BASE + ADC_JDR3)
    dump_reg("JDR4",    ADC1_BASE + ADC_JDR4)
    dump_reg("DR",      ADC1_BASE + ADC_DR)
"""
    ADC1 settings

    SR       : 0x00000010  [0x40012400]
    CR1      : 0x00000000  [0x40012404]
    CR2      : 0x009e0001  [0x40012408] # TSVREF:1, EXTTRIG:1, EXTSEL:111 (SWSTART) ADON:1
    SMPR1    : 0x00000000  [0x4001240c]
    SMPR2    : 0x36c00000  [0x40012410]
    JOFR1    : 0x00000000  [0x40012414]
    JOFR2    : 0x00000000  [0x40012418]
    JOFR3    : 0x00000000  [0x4001241c]
    JOFR4    : 0x00000000  [0x40012420]
    HTR      : 0x00000fff  [0x40012424]
    LTR      : 0x00000000  [0x40012428]
    SQR1     : 0x00000000  [0x4001242c]
    SQR2     : 0x00000000  [0x40012430]
    SQR3     : 0x00000008  [0x40012434]
    SQR4     : 0x00000000  [0x40012438]
    JDR1     : 0x00000000  [0x4001243c]
    JDR2     : 0x00000000  [0x40012440]
    JDR3     : 0x00000000  [0x40012444]
    JDR4     : 0x00000000  [0x40012448]
    DR       : 0x000001cc  [0x4001244c]

    DR: 12c     300     5.0
        168     360     6.0
        19f     415     7.0
        1da     474     8.0
        21a     538     9.0
        254     596     10.0
        2ce     718     12.0

    Linear (tested with https://plot.ly/create/)
"""

def dump_afio_settings():
    print("AFIO settings")
    dump_reg("EVCR",     AFIO_BASE + AFIO_EVCR)
    dump_reg("MAPR",     AFIO_BASE + AFIO_MAPR)
    dump_reg("EXTICR1",  AFIO_BASE + AFIO_EXTICR1)
    dump_reg("EXTICR2",  AFIO_BASE + AFIO_EXTICR2)
    dump_reg("EXTICR3",  AFIO_BASE + AFIO_EXTICR3)
    dump_reg("EXTICR4",  AFIO_BASE + AFIO_EXTICR4)
    dump_reg("MAPR2",    AFIO_BASE + AFIO_MAPR2)
"""
    AFIO settings

    EVCR     : 0x00000000  [0x40010000]
    MAPR     : 0x02008000  [0x40010004]
    EXTICR1  : 0x00000000  [0x40010008]
    EXTICR2  : 0x00000011  [0x4001000c]
    EXTICR3  : 0x00000011  [0x40010010]
    EXTICR4  : 0x00000000  [0x40010014]
    MAPR2    : 0x00000000  [0x40010018]
"""

def dump_exti_settings():
    print("EXTI settings")
    dump_reg("IMR",    EXTI_BASE + EXTI_IMR)
    dump_reg("EMR",    EXTI_BASE + EXTI_EMR)
    dump_reg("RTSR",   EXTI_BASE + EXTI_RTSR)
    dump_reg("FTSR",   EXTI_BASE + EXTI_FTSR)
    dump_reg("SWIER",  EXTI_BASE + EXTI_SWIER)
    dump_reg("PR",     EXTI_BASE + EXTI_PR)
"""
    EXTI settings

    IMR      : 0x0000033e  [0x40010400]
    EMR      : 0x00000000  [0x40010404]
    RTSR     : 0x00000300  [0x40010408]
    FTSR     : 0x0000033e  [0x4001040c]
    SWIER    : 0x00000000  [0x40010410]
    PR       : 0x00000000  [0x40010414]
"""

def dump_rcc_settings():
    print("RCC settings")
    dump_reg("CR",       RCC_BASE + RCC_CR)
    dump_reg("CFGR",     RCC_BASE + RCC_CFGR)
    dump_reg("CIR",      RCC_BASE + RCC_CIR)
    dump_reg("APB2RSTR", RCC_BASE + RCC_APB2RSTR)
    dump_reg("APB1RSTR", RCC_BASE + RCC_APB1RSTR)
    dump_reg("AHBENR",   RCC_BASE + RCC_AHBENR)
    dump_reg("APB2ENR",  RCC_BASE + RCC_APB2ENR)
    dump_reg("APB1ENR",  RCC_BASE + RCC_APB1ENR)
    dump_reg("BDCR",     RCC_BASE + RCC_BDCR)
    dump_reg("CSR",      RCC_BASE + RCC_CSR)
    dump_reg("CFGR2",    RCC_BASE + RCC_CFGR2)

def dump_spi_settings(name, base_addr):
    print("%s settings" % (name))
    dump_reg("CR2",      base_addr + SPI_CR1)
    dump_reg("CR1",      base_addr + SPI_CR2)
    dump_reg("SR",       base_addr + SPI_SR)
    dump_reg("DR",       base_addr + SPI_DR)
    dump_reg("CRCPR",    base_addr + SPI_CRCPR)
    dump_reg("RXCRCR",   base_addr + SPI_RXCRCR)
    dump_reg("TXCRCR",   base_addr + SPI_TXCRCR)

def dump_spi1_settings():
    dump_spi_settings("SPI1", SPI1_BASE)

def dump_spi2_settings():
    dump_spi_settings("SPI2", SPI2_BASE)

def dump_gpio_settings(name, base_addr):
    print("%s settings" % (name))
    dump_reg("CRL",      base_addr + GPIOx_CRL)
    dump_reg("CRH",      base_addr + GPIOx_CRH)
    dump_reg("IDR",      base_addr + GPIOx_IDR)
    dump_reg("ODR",      base_addr + GPIOx_ODR)
    dump_reg("BSRR",     base_addr + GPIOx_BSRR)
    dump_reg("BRR",      base_addr + GPIOx_BRR)
    dump_reg("LCKR",     base_addr + GPIOx_LCKR)

def dump_gpioa_settings():
    dump_gpio_settings("GPIOA", GPIOA_BASE)

def dump_gpiob_settings():
    dump_gpio_settings("GPIOB", GPIOB_BASE)

def dump_gpioc_settings():
    dump_gpio_settings("GPIOC", GPIOC_BASE)

def dump_gpiod_settings():
    dump_gpio_settings("GPIOD", GPIOD_BASE)


"""

DAC settings @ 5V :

CR       : 0x00030003  [00] CH 1/2 output buffer disable,  CH 1/2 enable TSEL=000
SWTRIGR  : 0x00000000  [04]
DHR12R1  : 0x00000169  [08] DAC channel1 12-bit right-aligned data holding register
DHR12L1  : 0x00001690  [0c] DAC channel1 12-bit left aligned data holding register
DHR8R1   : 0x00000016  [10] DAC channel1 8-bit right aligned data holding register
DHR12R2  : 0x000001d6  [14] DAC channel2 12-bit right aligned data holding register
DHR12L2  : 0x00001d60  [18] DAC channel2 12-bit left aligned data holding register
DHR8R2   : 0x0000001d  [1c] DAC channel2 8-bit right-aligned data holding register
DHR12RD  : 0x01d60169  [20] Dual DAC 12-bit right-aligned data holding register
DHR12LD  : 0x1d601690  [24] DUAL DAC 12-bit left aligned data holding register
DHR8RD   : 0x00001d16  [28] DUAL DAC 8-bit right aligned data holding register
DOR1     : 0x00000169  [2c] DAC channel1 data output register
DOR2     : 0x000001d6  [30] DAC channel2 data output register
SR       : 0x00000000  [34]


# 470/V

# 0V
mww 0x40007424 0x1d600000

# 1V
mww 0x40007424 0x1d6004D0

# 2V
mww 0x40007424 0x1d600940

# 3V
mww 0x40007424 0x1d600DB0

# 4V
mww 0x40007424 0x1d601220

# 5V
mww 0x40007424 0x1d601690

# 6V
mww 0x40007424 0x1d601B00

# Full swing (Vin - 1V)
mww 0x40007424 0x1d601f40

# 7.10V @ 7.70V matning
mww 0x40007424 0x1d601fff

"""

def dump_dma_settings():
    print("DMA settings")
    dump_reg("ISR",    DMA1_BASE + DMA_ISR)
    dump_reg("IFCR",   DMA1_BASE + DMA_IFCR)
    dump_reg("CCR1",   DMA1_BASE + DMA_CCR1)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR1",  DMA1_BASE + DMA_CPAR1)
    dump_reg("CMAR1",  DMA1_BASE + DMA_CMAR1)
    dump_reg("CCR2",   DMA1_BASE + DMA_CCR2)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR2",  DMA1_BASE + DMA_CPAR2)
    dump_reg("CMAR2",  DMA1_BASE + DMA_CMAR2)
    dump_reg("CCR3",   DMA1_BASE + DMA_CCR3)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR3",  DMA1_BASE + DMA_CPAR3)
    dump_reg("CMAR3",  DMA1_BASE + DMA_CMAR3)
    dump_reg("CCR4",   DMA1_BASE + DMA_CCR4)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR4",  DMA1_BASE + DMA_CPAR4)
    dump_reg("CMAR4",  DMA1_BASE + DMA_CMAR4)
    dump_reg("CCR5",   DMA1_BASE + DMA_CCR5)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR5",  DMA1_BASE + DMA_CPAR5)
    dump_reg("CMAR5",  DMA1_BASE + DMA_CMAR5)
    dump_reg("CCR6",   DMA1_BASE + DMA_CCR6)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR6",  DMA1_BASE + DMA_CPAR6)
    dump_reg("CMAR6",  DMA1_BASE + DMA_CMAR6)
    dump_reg("CCR7",   DMA1_BASE + DMA_CCR7)
    dump_reg("CNDTR",  DMA1_BASE + DMA_CNDTR)
    dump_reg("CPAR7",  DMA1_BASE + DMA_CPAR7)

def dump_register_map():
    global register_map
    for reg in register_map:
        address = reg[0]
        end = reg[1]
        name = reg[2]
        print("\n# %s (0x%08x..0x%08x)" % (name, address, end))
        while address < end:
            value = ocd_read(address)
            print("[0x%08x] = 0x%08x" % (address, value))
            address += 4

def write_mem():
    if len(sys.argv) != 4:
        print("%s <address> <value>" % (sys.argv[0]))
    else:
        address = int(sys.argv[2], 16)
        value = int(sys.argv[3], 16)
        ocd_write(address, value)

def read_mem():
    if len(sys.argv) < 3:
        print("%s <address> [<length>]" % (sys.argv[0]))
    else:
        length = 1
        address = int(sys.argv[2], 16)
        if len(sys.argv) == 4:
            length = int(sys.argv[3], 16)
        while length > 0:
            value = ocd_read(address)
            print("[0x%08x] = 0x%08x" % (address, value))
            address += 4
            length -= 1

def print_help():
    global commands
    print("Available commands:")
    for cmd in commands:
        print("   %s" % (cmd))

def dump_all():
    global commands
    blocklist = ["reg", "all", "r", "w", "help"]
    for cmd in commands:
        if cmd not in blocklist:
            print ">>>> %s" % cmd
            commands[cmd]()
            print

def parse_mem_dump(data):
    lines = data.split('\n')
    for l in lines:
        parts = l.split(":")
        if len(parts) != 2:
            print ("Parsing error: %s" % l)
            return
        address = int(parts[0].strip(), 16)
        parts[1] = parts[1].strip()
        data = parts[1].split(" ")
        for (i, item) in enumerate(data):
            decode_mem(address+4*i, int(item, 16))
#            data[i] = int(item, 16)
#            print i, item
#        print "0x%08x" % address
#        for d in data:
#           print "   0x%08x" % d


try:
    ocd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ocd_sock.connect(('localhost', 4444))
    ocd_sock.settimeout(1.0)
except socket.error:
    print("Failed connect to Open OCD")
    sys.exit(1)

if not ocd_sync():
    print("Failed to sync with Open OCD")
    sys.exit(1)

commands = {
    "adc1"  : dump_adc1_settings,
    "afio"  : dump_afio_settings,
    "dac"   : dump_dac_settings,
    "dma"   : dump_dma_settings,
    "exti"  : dump_exti_settings,
    "gpio"  : dump_gpio_port_settings,
    "gpioa" : dump_gpioa_settings,
    "gpiob" : dump_gpiob_settings,
    "gpioc" : dump_gpioc_settings,
    "gpiod" : dump_gpiod_settings,
    "reg"   : dump_register_map,
    "rcc"   : dump_rcc_settings,
    "spi1"  : dump_spi1_settings,
    "spi2"  : dump_spi2_settings,
    "tim4"  : dump_tim4_settings,
    "w"     : write_mem,
    "r"     : read_mem,
    "help"  : print_help,
    "all"   : dump_all,
}

if len(sys.argv) >= 2:
    if sys.argv[1] in commands:
        commands[sys.argv[1]]()
    else:
        print_help();
else:
    print_help();
