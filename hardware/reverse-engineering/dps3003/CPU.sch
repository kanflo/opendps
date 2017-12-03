EESchema Schematic File Version 2
LIBS:stm32
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:my_conn
LIBS:my_supply
LIBS:my_something_else
LIBS:my_ics
LIBS:my_rcld
LIBS:my_switches
LIBS:my_sensors
LIBS:my_transistors
LIBS:DPS3203-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 3
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Button_SP_small SW?
U 1 1 596961C3
P 7275 2275
F 0 "SW?" H 7275 2425 50  0000 C CNN
F 1 "V/UP" H 7275 2150 50  0000 C CNN
F 2 "" H 7275 2275 50  0000 C CNN
F 3 "" H 7275 2275 50  0000 C CNN
	1    7275 2275
	-1   0    0    -1  
$EndComp
$Comp
L Rotary_Encoder_Switch SW?
U 1 1 596961CA
P 1175 4900
F 0 "SW?" H 1175 5160 50  0000 C CNN
F 1 "Rotary_Encoder_Switch" H 1175 4640 50  0000 C CNN
F 2 "" H 1075 5060 50  0001 C CNN
F 3 "" H 1175 5160 50  0001 C CNN
	1    1175 4900
	1    0    0    -1  
$EndComp
$Comp
L MC78L05ACH U?
U 1 1 596961D1
P 1125 7150
F 0 "U?" H 925 7350 50  0000 C CNN
F 1 "HT7133" H 1125 7350 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT89-3_Housing" H 1125 7250 50  0001 C CIN
F 3 "" H 1125 7150 50  0001 C CNN
	1    1125 7150
	1    0    0    -1  
$EndComp
$Comp
L STM32F100C8 U?
U 1 1 596961DE
P 3900 3625
F 0 "U?" H 2600 5275 60  0000 C CNN
F 1 "STM32F100C8" H 4950 1975 60  0000 C CNN
F 2 "LQFP48" H 3900 3625 40  0000 C CIN
F 3 "" H 3900 3625 60  0000 C CNN
	1    3900 3625
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR?
U 1 1 596962A1
P 650 6950
F 0 "#PWR?" H 650 6800 50  0001 C CNN
F 1 "+5V" H 650 7090 50  0000 C CNN
F 2 "" H 650 6950 50  0001 C CNN
F 3 "" H 650 6950 50  0001 C CNN
	1    650  6950
	1    0    0    -1  
$EndComp
$Comp
L C_Small C16
U 1 1 596962C4
P 650 7300
F 0 "C16" H 660 7370 50  0000 L CNN
F 1 "C_Small" H 660 7220 50  0000 L CNN
F 2 "" H 650 7300 50  0001 C CNN
F 3 "" H 650 7300 50  0001 C CNN
	1    650  7300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 5969638B
P 1125 7575
F 0 "#PWR?" H 1125 7325 50  0001 C CNN
F 1 "GND" H 1125 7425 50  0000 C CNN
F 2 "" H 1125 7575 50  0001 C CNN
F 3 "" H 1125 7575 50  0001 C CNN
	1    1125 7575
	1    0    0    -1  
$EndComp
Wire Wire Line
	650  6950 650  7100
Wire Wire Line
	650  7100 650  7200
Wire Wire Line
	725  7100 650  7100
Connection ~ 650  7100
Wire Wire Line
	650  7400 650  7500
Wire Wire Line
	650  7500 1125 7500
Wire Wire Line
	1125 7500 1625 7500
Wire Wire Line
	1125 7400 1125 7500
Wire Wire Line
	1125 7500 1125 7575
Connection ~ 1125 7500
$Comp
L C_Small C01
U 1 1 5969645A
P 1625 7350
F 0 "C01" H 1635 7420 50  0000 L CNN
F 1 "C_Small" H 1635 7270 50  0000 L CNN
F 2 "" H 1625 7350 50  0001 C CNN
F 3 "" H 1625 7350 50  0001 C CNN
	1    1625 7350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1525 7100 1625 7100
Wire Wire Line
	1625 6925 1625 7100
Wire Wire Line
	1625 7100 1625 7250
Wire Wire Line
	1625 7500 1625 7450
$Comp
L +3.3V #PWR?
U 1 1 596964CA
P 1625 6925
F 0 "#PWR?" H 1625 6775 50  0001 C CNN
F 1 "+3.3V" H 1625 7065 50  0000 C CNN
F 2 "" H 1625 6925 50  0001 C CNN
F 3 "" H 1625 6925 50  0001 C CNN
	1    1625 6925
	1    0    0    -1  
$EndComp
Connection ~ 1625 7100
$Comp
L +3.3V #PWR?
U 1 1 59696712
P 3600 1425
F 0 "#PWR?" H 3600 1275 50  0001 C CNN
F 1 "+3.3V" H 3600 1565 50  0000 C CNN
F 2 "" H 3600 1425 50  0001 C CNN
F 3 "" H 3600 1425 50  0001 C CNN
	1    3600 1425
	1    0    0    -1  
$EndComp
$Comp
L C_Small C27
U 1 1 596968AF
P 3700 1575
F 0 "C27" H 3710 1645 50  0000 L CNN
F 1 "C_Small" H 3710 1495 50  0000 L CNN
F 2 "" H 3700 1575 50  0001 C CNN
F 3 "" H 3700 1575 50  0001 C CNN
	1    3700 1575
	1    0    0    -1  
$EndComp
Wire Wire Line
	3600 1425 3600 1450
Wire Wire Line
	3600 1450 3600 1875
Wire Wire Line
	3700 1475 3700 1450
Wire Wire Line
	3600 1450 3700 1450
Wire Wire Line
	3700 1450 3900 1450
Wire Wire Line
	3900 1450 4025 1450
Wire Wire Line
	4025 1450 4200 1450
Wire Wire Line
	4200 1450 4325 1450
Connection ~ 3600 1450
$Comp
L C_Small C19
U 1 1 59696A81
P 4025 1575
F 0 "C19" H 4035 1645 50  0000 L CNN
F 1 "C_Small" H 4035 1495 50  0000 L CNN
F 2 "" H 4025 1575 50  0001 C CNN
F 3 "" H 4025 1575 50  0001 C CNN
	1    4025 1575
	1    0    0    -1  
$EndComp
Wire Wire Line
	4025 1450 4025 1475
Connection ~ 3700 1450
Wire Wire Line
	3750 1875 3750 1850
Wire Wire Line
	3750 1850 3900 1850
Wire Wire Line
	3900 1450 3900 1850
Wire Wire Line
	3900 1850 3900 1875
Connection ~ 3900 1450
$Comp
L GND #PWR?
U 1 1 59696C14
P 3400 1800
F 0 "#PWR?" H 3400 1550 50  0001 C CNN
F 1 "GND" H 3400 1650 50  0000 C CNN
F 2 "" H 3400 1800 50  0001 C CNN
F 3 "" H 3400 1800 50  0001 C CNN
	1    3400 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 1800 3400 1725
Wire Wire Line
	3400 1725 3700 1725
Wire Wire Line
	3700 1725 4025 1725
Wire Wire Line
	4025 1725 4325 1725
Wire Wire Line
	4025 1725 4025 1675
Wire Wire Line
	3700 1675 3700 1725
Connection ~ 3700 1725
Wire Wire Line
	4200 1450 4200 1875
Connection ~ 4025 1450
Connection ~ 3900 1850
$Comp
L C_Small Cx
U 1 1 59696D5D
P 4325 1575
F 0 "Cx" H 4335 1645 50  0000 L CNN
F 1 "C_Small" H 4335 1495 50  0000 L CNN
F 2 "" H 4325 1575 50  0001 C CNN
F 3 "" H 4325 1575 50  0001 C CNN
	1    4325 1575
	1    0    0    -1  
$EndComp
Wire Wire Line
	4325 1450 4325 1475
Connection ~ 4200 1450
Wire Wire Line
	4325 1725 4325 1675
Connection ~ 4025 1725
$Comp
L C_Small C21
U 1 1 59696F67
P 6850 2475
F 0 "C21" H 6860 2545 50  0000 L CNN
F 1 "C_Small" H 6860 2395 50  0000 L CNN
F 2 "" H 6850 2475 50  0001 C CNN
F 3 "" H 6850 2475 50  0001 C CNN
	1    6850 2475
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 59696FC6
P 6850 2650
F 0 "#PWR?" H 6850 2400 50  0001 C CNN
F 1 "GND" H 6850 2500 50  0000 C CNN
F 2 "" H 6850 2650 50  0001 C CNN
F 3 "" H 6850 2650 50  0001 C CNN
	1    6850 2650
	-1   0    0    -1  
$EndComp
Wire Wire Line
	6400 2275 6850 2275
Wire Wire Line
	6850 2275 7100 2275
Wire Wire Line
	6850 2275 6850 2375
Wire Wire Line
	6850 2575 6850 2625
Wire Wire Line
	6850 2625 6850 2650
Wire Wire Line
	6850 2625 7575 2625
Wire Wire Line
	7575 2625 7575 2275
Wire Wire Line
	7575 2275 7450 2275
Connection ~ 6850 2625
Connection ~ 6850 2275
$Comp
L Button_SP_small SW?
U 1 1 5969720C
P 7275 1700
F 0 "SW?" H 7275 1850 50  0000 C CNN
F 1 "SET" H 7275 1575 50  0000 C CNN
F 2 "" H 7275 1700 50  0000 C CNN
F 3 "" H 7275 1700 50  0000 C CNN
	1    7275 1700
	-1   0    0    -1  
$EndComp
$Comp
L C_Small C22
U 1 1 59697212
P 6850 1900
F 0 "C22" H 6860 1970 50  0000 L CNN
F 1 "C_Small" H 6860 1820 50  0000 L CNN
F 2 "" H 6850 1900 50  0001 C CNN
F 3 "" H 6850 1900 50  0001 C CNN
	1    6850 1900
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 59697218
P 6850 2075
F 0 "#PWR?" H 6850 1825 50  0001 C CNN
F 1 "GND" H 6850 1925 50  0000 C CNN
F 2 "" H 6850 2075 50  0001 C CNN
F 3 "" H 6850 2075 50  0001 C CNN
	1    6850 2075
	-1   0    0    -1  
$EndComp
Wire Wire Line
	6325 1700 6850 1700
Wire Wire Line
	6850 1700 7100 1700
Wire Wire Line
	6850 1700 6850 1800
Wire Wire Line
	6850 2000 6850 2050
Wire Wire Line
	6850 2050 6850 2075
Wire Wire Line
	6850 2050 7575 2050
Wire Wire Line
	7575 2050 7575 1700
Wire Wire Line
	7575 1700 7450 1700
Connection ~ 6850 2050
Connection ~ 6850 1700
$Comp
L Button_SP_small SW?
U 1 1 596973AF
P 7275 1125
F 0 "SW?" H 7275 1275 50  0000 C CNN
F 1 "A/DWN" H 7275 1000 50  0000 C CNN
F 2 "" H 7275 1125 50  0000 C CNN
F 3 "" H 7275 1125 50  0000 C CNN
	1    7275 1125
	-1   0    0    -1  
$EndComp
$Comp
L C_Small C23
U 1 1 596973B5
P 6850 1325
F 0 "C23" H 6860 1395 50  0000 L CNN
F 1 "C_Small" H 6860 1245 50  0000 L CNN
F 2 "" H 6850 1325 50  0001 C CNN
F 3 "" H 6850 1325 50  0001 C CNN
	1    6850 1325
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 596973BB
P 6850 1500
F 0 "#PWR?" H 6850 1250 50  0001 C CNN
F 1 "GND" H 6850 1350 50  0000 C CNN
F 2 "" H 6850 1500 50  0001 C CNN
F 3 "" H 6850 1500 50  0001 C CNN
	1    6850 1500
	-1   0    0    -1  
$EndComp
Wire Wire Line
	6225 1125 6850 1125
Wire Wire Line
	6850 1125 7100 1125
Wire Wire Line
	6850 1125 6850 1225
Wire Wire Line
	6850 1425 6850 1475
Wire Wire Line
	6850 1475 6850 1500
Wire Wire Line
	6850 1475 7575 1475
Wire Wire Line
	7575 1475 7575 1125
Wire Wire Line
	7575 1125 7450 1125
Connection ~ 6850 1475
Connection ~ 6850 1125
$Comp
L Button_SP_small SW?
U 1 1 596973C9
P 950 3875
F 0 "SW?" H 950 4025 50  0000 C CNN
F 1 "ON/OFF" H 950 3750 50  0000 C CNN
F 2 "" H 950 3875 50  0000 C CNN
F 3 "" H 950 3875 50  0000 C CNN
	1    950  3875
	1    0    0    -1  
$EndComp
$Comp
L C_Small C24
U 1 1 596973CF
P 1375 4075
F 0 "C24" H 1385 4145 50  0000 L CNN
F 1 "C_Small" H 1385 3995 50  0000 L CNN
F 2 "" H 1375 4075 50  0001 C CNN
F 3 "" H 1375 4075 50  0001 C CNN
	1    1375 4075
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 596973D5
P 1375 4250
F 0 "#PWR?" H 1375 4000 50  0001 C CNN
F 1 "GND" H 1375 4100 50  0000 C CNN
F 2 "" H 1375 4250 50  0001 C CNN
F 3 "" H 1375 4250 50  0001 C CNN
	1    1375 4250
	1    0    0    -1  
$EndComp
Wire Wire Line
	1125 3875 1375 3875
Wire Wire Line
	1375 3875 2400 3875
Wire Wire Line
	1375 3875 1375 3975
Wire Wire Line
	1375 4175 1375 4225
Wire Wire Line
	1375 4225 1375 4250
Wire Wire Line
	1375 4225 650  4225
Wire Wire Line
	650  4225 650  3875
Wire Wire Line
	650  3875 775  3875
Connection ~ 1375 4225
Connection ~ 1375 3875
Wire Wire Line
	5400 2675 6400 2675
Wire Wire Line
	6400 2675 6400 2275
Wire Wire Line
	6325 1700 6325 2575
Wire Wire Line
	6325 2575 5400 2575
Wire Wire Line
	5400 2475 6225 2475
Wire Wire Line
	6225 2475 6225 1125
Wire Wire Line
	1475 4800 1725 4800
Wire Wire Line
	1725 3975 1725 4800
Wire Wire Line
	1725 4800 1725 4850
Wire Wire Line
	1725 3975 2400 3975
$Comp
L GND #PWR?
U 1 1 596986A7
P 1175 5300
F 0 "#PWR?" H 1175 5050 50  0001 C CNN
F 1 "GND" H 1175 5150 50  0000 C CNN
F 2 "" H 1175 5300 50  0001 C CNN
F 3 "" H 1175 5300 50  0001 C CNN
	1    1175 5300
	1    0    0    -1  
$EndComp
Wire Wire Line
	1175 5300 1175 5250
Wire Wire Line
	700  5250 1175 5250
Wire Wire Line
	1175 5250 1525 5250
Wire Wire Line
	1525 5250 1725 5250
Wire Wire Line
	1525 5250 1525 5000
Wire Wire Line
	1525 5000 1475 5000
Wire Wire Line
	875  4900 700  4900
Wire Wire Line
	700  4900 700  5250
Connection ~ 1175 5250
$Comp
L C_Small C20
U 1 1 596987A7
P 1725 4950
F 0 "C20" H 1735 5020 50  0000 L CNN
F 1 "C_Small" H 1735 4870 50  0000 L CNN
F 2 "" H 1725 4950 50  0001 C CNN
F 3 "" H 1725 4950 50  0001 C CNN
	1    1725 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	1725 5250 1725 5050
Connection ~ 1525 5250
Connection ~ 1725 4800
$Comp
L C_Small C25
U 1 1 59698A89
P 600 5550
F 0 "C25" H 610 5620 50  0000 L CNN
F 1 "C_Small" H 610 5470 50  0000 L CNN
F 2 "" H 600 5550 50  0001 C CNN
F 3 "" H 600 5550 50  0001 C CNN
	1    600  5550
	1    0    0    -1  
$EndComp
$Comp
L C_Small C26
U 1 1 59698AFE
P 825 5500
F 0 "C26" H 835 5570 50  0000 L CNN
F 1 "C_Small" H 835 5420 50  0000 L CNN
F 2 "" H 825 5500 50  0001 C CNN
F 3 "" H 825 5500 50  0001 C CNN
	1    825  5500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 59698C90
P 600 5700
F 0 "#PWR?" H 600 5450 50  0001 C CNN
F 1 "GND" H 600 5550 50  0000 C CNN
F 2 "" H 600 5700 50  0001 C CNN
F 3 "" H 600 5700 50  0001 C CNN
	1    600  5700
	1    0    0    -1  
$EndComp
Wire Wire Line
	600  5650 600  5675
Wire Wire Line
	600  5675 600  5700
Wire Wire Line
	600  5675 825  5675
Wire Wire Line
	825  5675 825  5600
Connection ~ 600  5675
Wire Wire Line
	2400 4275 1775 4275
Wire Wire Line
	1775 4275 1775 4500
Wire Wire Line
	1775 4500 600  4500
Wire Wire Line
	600  4500 600  4800
Wire Wire Line
	600  4800 600  5450
Wire Wire Line
	875  4800 600  4800
Connection ~ 600  4800
Wire Wire Line
	875  5000 825  5000
Wire Wire Line
	825  4550 825  5000
Wire Wire Line
	825  5000 825  5400
Wire Wire Line
	825  4550 1825 4550
Wire Wire Line
	1825 4550 1825 4375
Wire Wire Line
	1825 4375 2400 4375
Connection ~ 825  5000
$Comp
L CONN_02X04 J?
U 1 1 596992FB
P 2950 5975
F 0 "J?" H 2950 6225 50  0000 C CNN
F 1 "CONN_02X04" H 2950 5725 50  0000 C CNN
F 2 "" H 2950 4775 50  0001 C CNN
F 3 "" H 2950 4775 50  0001 C CNN
	1    2950 5975
	1    0    0    -1  
$EndComp
Wire Wire Line
	2400 4875 2300 4875
Wire Wire Line
	2300 4875 2300 5825
Wire Wire Line
	2300 5825 2700 5825
$Comp
L +3.3VP #PWR?
U 1 1 59699D37
P 3275 5725
F 0 "#PWR?" H 3425 5675 50  0001 C CNN
F 1 "+3.3VP" H 3275 5825 50  0000 C CNN
F 2 "" H 3275 5725 50  0001 C CNN
F 3 "" H 3275 5725 50  0001 C CNN
	1    3275 5725
	1    0    0    -1  
$EndComp
Wire Wire Line
	3275 5725 3275 5825
Wire Wire Line
	3275 5825 3200 5825
Wire Wire Line
	2700 5925 2375 5925
Wire Wire Line
	2375 5925 2375 4975
Wire Wire Line
	2375 4975 2400 4975
$Comp
L R_Small R27
U 1 1 5969C110
P 2975 5575
F 0 "R27" H 3005 5595 50  0000 L CNN
F 1 "10k(01C)" H 3005 5535 50  0000 L CNN
F 2 "" H 2975 5575 50  0001 C CNN
F 3 "" H 2975 5575 50  0001 C CNN
	1    2975 5575
	0    1    -1   0   
$EndComp
Wire Wire Line
	2225 4175 2225 5575
Wire Wire Line
	2225 4175 2400 4175
$Comp
L Q_NPN_BEC Q3
U 1 1 5969CC0A
P 4025 6175
F 0 "Q3" H 4225 6225 50  0000 L CNN
F 1 "SS8050(Y1)" H 4200 6125 50  0000 L CNN
F 2 "" H 4225 6275 50  0001 C CNN
F 3 "" H 4025 6175 50  0001 C CNN
	1    4025 6175
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 5969CD39
P 4200 5475
F 0 "#PWR?" H 4200 5225 50  0001 C CNN
F 1 "GND" H 4200 5325 50  0000 C CNN
F 2 "" H 4200 5475 50  0001 C CNN
F 3 "" H 4200 5475 50  0001 C CNN
	1    4200 5475
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 5375 4200 5425
Wire Wire Line
	4200 5425 4200 5475
Wire Wire Line
	3600 5425 3750 5425
Wire Wire Line
	3750 5425 3900 5425
Wire Wire Line
	3900 5425 4200 5425
Wire Wire Line
	3600 5425 3600 5375
Connection ~ 4200 5425
Wire Wire Line
	3750 5375 3750 5425
Connection ~ 3750 5425
Wire Wire Line
	3900 5375 3900 5425
Connection ~ 3900 5425
$Comp
L GND #PWR?
U 1 1 5969D119
P 3925 6425
F 0 "#PWR?" H 3925 6175 50  0001 C CNN
F 1 "GND" H 3925 6275 50  0000 C CNN
F 2 "" H 3925 6425 50  0001 C CNN
F 3 "" H 3925 6425 50  0001 C CNN
	1    3925 6425
	1    0    0    -1  
$EndComp
Wire Wire Line
	3925 6375 3925 6425
Wire Wire Line
	3925 5975 3925 5925
Wire Wire Line
	3925 5925 3200 5925
Wire Wire Line
	3075 5575 3425 5575
Wire Wire Line
	3425 5575 3425 5825
Wire Wire Line
	3425 5825 4225 5825
Wire Wire Line
	4225 5825 4225 6175
Wire Wire Line
	2700 6025 2150 6025
Wire Wire Line
	2150 6025 2150 4775
Wire Wire Line
	2150 4775 2400 4775
Wire Wire Line
	2400 4675 2075 4675
Wire Wire Line
	2075 4675 2075 6375
Wire Wire Line
	2075 6375 3400 6375
Wire Wire Line
	3400 6375 3400 6025
Wire Wire Line
	3400 6025 3200 6025
Wire Wire Line
	5400 3175 6475 3175
Wire Wire Line
	6475 3175 6475 6625
Wire Wire Line
	6475 6625 2625 6625
Wire Wire Line
	2625 6625 2625 6125
Wire Wire Line
	2625 6125 2700 6125
$Comp
L GND #PWR?
U 1 1 5969D8BC
P 3275 6175
F 0 "#PWR?" H 3275 5925 50  0001 C CNN
F 1 "GND" H 3275 6025 50  0000 C CNN
F 2 "" H 3275 6175 50  0001 C CNN
F 3 "" H 3275 6175 50  0001 C CNN
	1    3275 6175
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 6125 3275 6125
Wire Wire Line
	3275 6125 3275 6175
Wire Wire Line
	2225 5575 2875 5575
Text GLabel 5575 3375 2    60   Input ~ 0
RXD
Text GLabel 5600 3275 2    60   Input ~ 0
TXD
Wire Wire Line
	5600 3275 5400 3275
Wire Wire Line
	5400 3375 5575 3375
$Comp
L CONN_01X04 J?
U 1 1 5969E723
P 2650 7175
F 0 "J?" H 2650 7425 50  0000 C CNN
F 1 "CONN_01X04" V 2750 7175 50  0000 C CNN
F 2 "" H 2650 7175 50  0001 C CNN
F 3 "" H 2650 7175 50  0001 C CNN
	1    2650 7175
	1    0    0    -1  
$EndComp
$Comp
L +3.3VP #PWR?
U 1 1 5969E796
P 2400 7000
F 0 "#PWR?" H 2550 6950 50  0001 C CNN
F 1 "+3.3VP" H 2400 7100 50  0000 C CNN
F 2 "" H 2400 7000 50  0001 C CNN
F 3 "" H 2400 7000 50  0001 C CNN
	1    2400 7000
	1    0    0    -1  
$EndComp
Wire Wire Line
	2450 7025 2400 7025
Wire Wire Line
	2400 7025 2400 7000
Text GLabel 2300 7225 0    60   Input ~ 0
TXD
Wire Wire Line
	2300 7125 2450 7125
Wire Wire Line
	2450 7225 2300 7225
Text GLabel 2300 7125 0    60   Input ~ 0
RXD
$Comp
L GND #PWR?
U 1 1 5969EC48
P 2375 7350
F 0 "#PWR?" H 2375 7100 50  0001 C CNN
F 1 "GND" H 2375 7200 50  0000 C CNN
F 2 "" H 2375 7350 50  0001 C CNN
F 3 "" H 2375 7350 50  0001 C CNN
	1    2375 7350
	-1   0    0    -1  
$EndComp
Wire Wire Line
	2450 7325 2375 7325
Wire Wire Line
	2375 7325 2375 7350
$Comp
L CONN_01X05 J?
U 1 1 5969F04C
P 3200 7225
F 0 "J?" H 3200 7525 50  0000 C CNN
F 1 "CONN_01X05" V 3300 7225 50  0000 C CNN
F 2 "" H 3200 7225 50  0001 C CNN
F 3 "" H 3200 7225 50  0001 C CNN
	1    3200 7225
	-1   0    0    -1  
$EndComp
$Comp
L +3.3V #PWR?
U 1 1 5969F127
P 3475 6975
F 0 "#PWR?" H 3475 6825 50  0001 C CNN
F 1 "+3.3V" H 3475 7115 50  0000 C CNN
F 2 "" H 3475 6975 50  0001 C CNN
F 3 "" H 3475 6975 50  0001 C CNN
	1    3475 6975
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 7025 3475 7025
Wire Wire Line
	3475 7025 3475 6975
Text GLabel 2350 2225 0    60   Input ~ 0
~NRST
Wire Wire Line
	2350 2225 2400 2225
Text GLabel 3550 7125 2    60   Input ~ 0
~NRST
Wire Wire Line
	3550 7125 3400 7125
Text GLabel 5575 3775 2    60   Input ~ 0
SWCLK
Wire Wire Line
	5575 3775 5400 3775
Text GLabel 3550 7325 2    60   Input ~ 0
SWCLK
Wire Wire Line
	3550 7325 3400 7325
$Comp
L GND #PWR?
U 1 1 596A01B3
P 4075 7400
F 0 "#PWR?" H 4075 7150 50  0001 C CNN
F 1 "GND" H 4075 7250 50  0000 C CNN
F 2 "" H 4075 7400 50  0001 C CNN
F 3 "" H 4075 7400 50  0001 C CNN
	1    4075 7400
	-1   0    0    -1  
$EndComp
Wire Wire Line
	4075 7400 4075 7225
Wire Wire Line
	4075 7225 3400 7225
Text GLabel 5575 3675 2    60   Input ~ 0
SWDIO
Wire Wire Line
	5575 3675 5400 3675
Text GLabel 3550 7425 2    60   Input ~ 0
SWDIO
Wire Wire Line
	3550 7425 3400 7425
Text GLabel 8400 3375 2    60   Input ~ 0
V_OUT
$Comp
L R_Small R3
U 1 1 596A0C0A
P 8350 3600
F 0 "R3" H 8380 3620 50  0000 L CNN
F 1 "100k" H 8380 3560 50  0000 L CNN
F 2 "" H 8350 3600 50  0001 C CNN
F 3 "" H 8350 3600 50  0001 C CNN
	1    8350 3600
	1    0    0    1   
$EndComp
$Comp
L R_Small R4
U 1 1 596A0D9D
P 8350 3875
F 0 "R4" H 8380 3895 50  0000 L CNN
F 1 "10k" H 8380 3835 50  0000 L CNN
F 2 "" H 8350 3875 50  0001 C CNN
F 3 "" H 8350 3875 50  0001 C CNN
	1    8350 3875
	1    0    0    1   
$EndComp
$Comp
L GND #PWR?
U 1 1 596A0E96
P 8350 4025
F 0 "#PWR?" H 8350 3775 50  0001 C CNN
F 1 "GND" H 8350 3875 50  0000 C CNN
F 2 "" H 8350 4025 50  0001 C CNN
F 3 "" H 8350 4025 50  0001 C CNN
	1    8350 4025
	-1   0    0    -1  
$EndComp
Wire Wire Line
	8400 3375 8350 3375
Wire Wire Line
	8350 3375 8350 3500
Wire Wire Line
	8350 3700 8350 3775
Wire Wire Line
	8350 3975 8350 4025
Text GLabel 5600 2775 2    60   Input ~ 0
V_SET
Wire Wire Line
	5600 2775 5400 2775
Text GLabel 875  2875 0    60   Input ~ 0
V_In
$Comp
L R_Small R2
U 1 1 596ABE5F
P 975 3075
F 0 "R2" H 1005 3095 50  0000 L CNN
F 1 "100k" H 1005 3035 50  0000 L CNN
F 2 "" H 975 3075 50  0001 C CNN
F 3 "" H 975 3075 50  0001 C CNN
	1    975  3075
	-1   0    0    1   
$EndComp
$Comp
L R_Small R7
U 1 1 596ABFF0
P 975 3325
F 0 "R7" H 1005 3345 50  0000 L CNN
F 1 "5k1" H 1005 3285 50  0000 L CNN
F 2 "" H 975 3325 50  0001 C CNN
F 3 "" H 975 3325 50  0001 C CNN
	1    975  3325
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR?
U 1 1 596AC124
P 975 3475
F 0 "#PWR?" H 975 3225 50  0001 C CNN
F 1 "GND" H 975 3325 50  0000 C CNN
F 2 "" H 975 3475 50  0001 C CNN
F 3 "" H 975 3475 50  0001 C CNN
	1    975  3475
	1    0    0    -1  
$EndComp
Wire Wire Line
	875  2875 975  2875
Wire Wire Line
	975  2875 975  2975
Wire Wire Line
	975  3175 975  3200
Wire Wire Line
	975  3200 975  3225
Wire Wire Line
	975  3425 975  3475
Wire Wire Line
	975  3200 1450 3200
Wire Wire Line
	1450 3200 1450 3475
Wire Wire Line
	1450 3475 2400 3475
Connection ~ 975  3200
Text GLabel 5600 2875 2    60   Input ~ 0
I_SET
Wire Wire Line
	5600 2875 5400 2875
Text GLabel 5600 3075 2    60   Input ~ 0
I_ADC
Wire Wire Line
	5600 3075 5400 3075
Text GLabel 2350 3575 0    60   Input ~ 0
V_ADC
Wire Wire Line
	2350 3575 2400 3575
$Comp
L GND #PWR?
U 1 1 596B63C1
P 1725 3675
F 0 "#PWR?" H 1725 3425 50  0001 C CNN
F 1 "GND" H 1725 3525 50  0000 C CNN
F 2 "" H 1725 3675 50  0001 C CNN
F 3 "" H 1725 3675 50  0001 C CNN
	1    1725 3675
	1    0    0    -1  
$EndComp
Wire Wire Line
	1725 3675 2400 3675
$Comp
L R_Small R12
U 1 1 596B6794
P 2050 2500
F 0 "R12" H 2080 2520 50  0000 L CNN
F 1 "10k(01C)" H 2080 2460 50  0000 L CNN
F 2 "" H 2050 2500 50  0001 C CNN
F 3 "" H 2050 2500 50  0001 C CNN
	1    2050 2500
	-1   0    0    1   
$EndComp
Wire Wire Line
	2400 2375 2050 2375
Wire Wire Line
	2050 2375 2050 2400
$Comp
L GND #PWR?
U 1 1 596B6B89
P 2050 2625
F 0 "#PWR?" H 2050 2375 50  0001 C CNN
F 1 "GND" H 2050 2475 50  0000 C CNN
F 2 "" H 2050 2625 50  0001 C CNN
F 3 "" H 2050 2625 50  0001 C CNN
	1    2050 2625
	1    0    0    -1  
$EndComp
Wire Wire Line
	2050 2600 2050 2625
Wire Wire Line
	2400 2725 2150 2725
Wire Wire Line
	2150 2725 2150 2825
Wire Wire Line
	2150 2825 1625 2825
Wire Wire Line
	1625 2825 1625 825 
Wire Wire Line
	1625 825  3075 825 
Wire Wire Line
	3375 825  5500 825 
Wire Wire Line
	5500 825  5500 2375
Wire Wire Line
	5400 2375 5500 2375
Wire Wire Line
	5500 2375 5625 2375
$Comp
L R_Small RL
U 1 1 596BE4D7
P 5625 2175
F 0 "RL" H 5655 2195 50  0000 L CNN
F 1 "10k(01C)" H 5655 2135 50  0000 L CNN
F 2 "" H 5625 2175 50  0001 C CNN
F 3 "" H 5625 2175 50  0001 C CNN
	1    5625 2175
	1    0    0    1   
$EndComp
$Comp
L +3.3V #PWR?
U 1 1 596BE923
P 5625 2025
F 0 "#PWR?" H 5625 1875 50  0001 C CNN
F 1 "+3.3V" H 5625 2165 50  0000 C CNN
F 2 "" H 5625 2025 50  0001 C CNN
F 3 "" H 5625 2025 50  0001 C CNN
	1    5625 2025
	1    0    0    -1  
$EndComp
Wire Wire Line
	5625 2375 5625 2275
Connection ~ 5500 2375
Wire Wire Line
	5625 2075 5625 2025
$Comp
L GND #PWR?
U 1 1 596BEE28
P 3250 1000
F 0 "#PWR?" H 3250 750 50  0001 C CNN
F 1 "GND" H 3250 850 50  0000 C CNN
F 2 "" H 3250 1000 50  0001 C CNN
F 3 "" H 3250 1000 50  0001 C CNN
	1    3250 1000
	1    0    0    -1  
$EndComp
Wire Notes Line
	3075 725  3075 1000
Wire Notes Line
	3075 1000 3400 1000
Wire Notes Line
	3400 1000 3400 725 
Wire Notes Line
	3400 725  3075 725 
Text Notes 3225 875  0    60   ~ 0
?
Wire Wire Line
	2000 4575 2400 4575
Wire Wire Line
	2000 4575 2000 6375
Wire Wire Line
	2000 6375 1175 6375
Text Notes 1100 6375 0    60   ~ 0
not popul. R29, Q2\nmaybe FAN control
Wire Wire Line
	5400 3875 5575 3875
Wire Wire Line
	2400 3775 2275 3775
Text GLabel 2275 3775 0    60   Input ~ 0
PIN_39
Text GLabel 5575 3875 2    60   Input ~ 0
PIN_38
$EndSCHEMATC
