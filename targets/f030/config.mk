SERIES = F0
DEFINES += -DSTM32F030x6 -DPROTO_V2 -DPSCREEN
CFLAGS += -mcpu=cortex-m0

#OPENOCD ?= openocd -f interface/cmsis-dap.cfg -f target/stm32f0x.cfg
OPENOCD ?= openocd -f interface/stlink-v2.cfg -f target/stm32f0x.cfg

HALPREF = $(DRV)/Src
HALSRC =  \
$(HALPREF)/stm32f0xx_ll_adc.c \
$(HALPREF)/stm32f0xx_ll_comp.c \
$(HALPREF)/stm32f0xx_ll_crc.c \
$(HALPREF)/stm32f0xx_ll_crs.c \
$(HALPREF)/stm32f0xx_ll_dac.c \
$(HALPREF)/stm32f0xx_ll_dma.c \
$(HALPREF)/stm32f0xx_ll_exti.c \
$(HALPREF)/stm32f0xx_ll_gpio.c \
$(HALPREF)/stm32f0xx_ll_i2c.c \
$(HALPREF)/stm32f0xx_ll_pwr.c \
$(HALPREF)/stm32f0xx_ll_rcc.c \
$(HALPREF)/stm32f0xx_ll_rtc.c \
$(HALPREF)/stm32f0xx_ll_spi.c \
$(HALPREF)/stm32f0xx_ll_tim.c \
$(HALPREF)/stm32f0xx_ll_usart.c \
$(HALPREF)/stm32f0xx_ll_usb.c \
$(HALPREF)/stm32f0xx_ll_utils.c \

