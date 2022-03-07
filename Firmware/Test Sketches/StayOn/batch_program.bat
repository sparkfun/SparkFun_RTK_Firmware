@echo off

if [%1]==[] goto usage

@echo Programming for SparkFun RTK Surveyor
@pause
:loop

@echo -
@echo Programming binary on %1

rem @esptool.exe --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0 RTK_Surveyor_Firmware_v13_combined.bin
@esptool.exe --chip esp32 --port %1 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0 StayOn.bin

@echo Done programming! Ready for next board.
@pause

goto loop

:usage
@echo Missing the binary file and com port arguments. Ex: batch_program.bat COM4