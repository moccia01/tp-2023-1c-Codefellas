################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/comunicacion.c \
../src/init.c \
../src/kernel.c \
../src/planificador.c 

C_DEPS += \
./src/comunicacion.d \
./src/init.d \
./src/kernel.d \
./src/planificador.d 

OBJS += \
./src/comunicacion.o \
./src/init.o \
./src/kernel.o \
./src/planificador.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Codefellas/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/comunicacion.d ./src/comunicacion.o ./src/init.d ./src/init.o ./src/kernel.d ./src/kernel.o ./src/planificador.d ./src/planificador.o

.PHONY: clean-src

