docker build -t rtk_firmware --no-cache-filter deployment .
docker create --name=rtk_container rtk_firmware:latest
docker cp rtk_container:/RTK_Surveyor.ino.bin .
docker cp rtk_container:/RTK_Surveyor.ino.elf .
docker cp rtk_container:/RTK_Surveyor.ino.bootloader.bin .
docker container rm rtk_container
