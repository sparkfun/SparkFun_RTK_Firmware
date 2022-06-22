@echo off

if [%1]==[] goto usage

@echo Programming for SparkFun RTK Surveyor
@pause
:loop

@echo -
@echo Programming binary: %1 on %2

esptool.exe --chip esp32 --port %2 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect ^
0x1000 ./bin/RTK_Surveyor.ino.bootloader.bin ^
0x8000 ./bin/RTK_Surveyor.ino.partitions.bin ^
0xe000 ./bin/boot_app0.bin ^
0x10000 ./%1

@echo Done programming! Ready for next board.
@pause

goto loop

:usage
@echo Missing the binary file and com port arguments. Ex: batch_program.bat RTK_Surveyor_Firmware_v1_10.bin COM6