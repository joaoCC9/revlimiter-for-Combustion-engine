################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\libraries\EEPROM\src\EEPROM.cpp 

LINK_OBJ += \
.\libraries\EEPROM\src\EEPROM.cpp.o 

CPP_DEPS += \
.\libraries\EEPROM\src\EEPROM.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
libraries\EEPROM\src\EEPROM.cpp.o: C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\libraries\EEPROM\src\EEPROM.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp-x32\2302/bin/xtensa-esp32-elf-g++" -MMD -c "@C:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-b6b4727c58\esp32/flags/cpp_flags" -w -Os -DF_CPU=240000000L -DARDUINO=10812 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 "-DARDUINO_BOARD=\"ESP32_DEV\"" -DARDUINO_VARIANT="esp32" -DARDUINO_PARTITION_default -DARDUINO_HOST_OS="" -DARDUINO_FQBN="" -DESP32 -DCORE_DEBUG_LEVEL=5 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1  -DARDUINO_USB_CDC_ON_BOOT=0  "@C:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-b6b4727c58\esp32/flags/defines" "-IC:\Users\joao\Documents\GitHub\revlimiter" -iprefix "C:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-b6b4727c58\esp32/include/" "@C:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-b6b4727c58\esp32/flags/includes" "-IC:\DEV\Sloeber\arduinoPlugin\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-b6b4727c58\esp32/qio_qspi/include" -I"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\cores\esp32" -I"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\variants\esp32" -I"C:\DEV\Sloeber\arduinoPlugin\libraries\OneWire\2.3.8" -I"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\libraries\EEPROM\src" -I"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\libraries\Wire\src" -I"C:\DEV\Sloeber\arduinoPlugin\libraries\DallasTemperature\3.9.0" -I"C:\DEV\Sloeber\arduinoPlugin\packages\esp32\hardware\esp32\3.0.4\libraries\BluetoothSerial\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 "@C:\Users\joao\Documents\GitHub\revlimiter\Release/file_opts" "$<" -o "$@"
	@echo 'Finished building: $<'
	@echo ' '


