/^(\.iram0\.text|\.iram0\.vectors|\.dram0\.data|\.flash\.text|\.flash\.rodata)/ {arduino_size += $2 }
/^(\.dram0\.data|\.dram0\.bss|\.noinit)/ {arduino_data += $2 }
// {arduino_eeprom += $2 }
END { print "\nSketch uses " arduino_size " bytes (" int(arduino_size/1310720*100+0.5)  "%)  of program storage space. Maximum is " 1310720 " bytes.\nGlobal variables use "arduino_data" bytes ("int(arduino_data/327680*100+0.5)"%) of dynamic memory, leaving "327680-arduino_data" bytes for local variables. Maximum is "327680" bytes.\n"}