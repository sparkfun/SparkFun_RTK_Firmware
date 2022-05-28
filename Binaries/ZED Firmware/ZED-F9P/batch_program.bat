@echo off

if [%1]==[] goto usage

@echo Program for upgrading ZED Firmware on the SparkFun RTK Products
@pause
:loop

@echo -
@echo Programming binary: %1 on %2

ubxfwupdate.exe -p \\.\%2 -b 9600:9600:921600 --no-fis 1 -s 0 -t 1 -v 1 "%1"

@echo Done programming! Ready for next board.
@pause

goto loop

:usage
@echo Missing the binary file and com port arguments. Ex: batch_program.bat UBX_F9_100_HPG132.df73486d99374142f3aabf79b7178f48.bin COM37