################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/netif/etharp.c \
../lwip/src/netif/ethernetif.c 

OBJS += \
./lwip/src/netif/etharp.o \
./lwip/src/netif/ethernetif.o 

C_DEPS += \
./lwip/src/netif/etharp.d \
./lwip/src/netif/ethernetif.d 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/netif/%.o: ../lwip/src/netif/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -D__USE_LPCOPEN -DCORE_M3 -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\lpc_chip_175x_6x\inc" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\lpc_board_nxp_lpcxpresso_1769\inc" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\WebServerTest\lwip\inc" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\WebServerTest\lwip\inc\ipv4" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\WebServerTest\freertos\inc" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\WebServerTest\webserver\inc" -I"C:\Users\Kevin\Documents\LPCXpresso_8.2.0_647\workspace\WebServerTest\fatfs\inc" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


