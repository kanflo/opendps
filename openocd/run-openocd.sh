#!/bin/bash
cd scripts
openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg
