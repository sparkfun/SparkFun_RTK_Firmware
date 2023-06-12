//------------------------------------------------------------------------------
// Lee Leahy
// 12 June 2023
//
// Program to reset the ESP32
//------------------------------------------------------------------------------
//
// Starting from the SparkFun RTK Surveyor
//
//      https://www.sparkfun.com/products/18443
//
// Schematic
//
//      https://cdn.sparkfun.com/assets/c/1/b/d/8/SparkFun_RTK_Surveyor-v13.pdf
//
//              ___     ___    GPIO-0   PU
//              DTR     RTS     BOOT    EN
//               0       0       1       1      Run flash code
//               0       1       0       1      Download code, write to flash
//               1       0       1       0      Power off
//               1       1       0       0      Power off
//
// To the ESP-WROOM-32 datasheet
//
//      https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf
//
// To the ESP-32 Series datasheet
//
//      https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
//
//      PU - 0: Power off, 1: Power on/up
//
//      GPIO-O: Saved at power up, 0 = download image, 1 = boot from flash
//
//------------------------------------------------------------------------------

#define MICROSECONDS                1
#define MILLISECONDS                (1000 * MICROSECONDS)
#define POWER_OFF_DELAY             (250 * MILLISECONDS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

const int dtrPin = TIOCM_DTR;
const int rtsPin = TIOCM_RTS;

//                             GPIO-0   PU
//              DTR     RTS     BOOT    EN
//               1       1       1       1      Run flash code
//               1       0       0       1      Download code, write to flash
//               0       1       1       0      Power off
//               0       0       0       0      Power off

int bootFromFlash(int serialPort)
{
    int status;

    status = ioctl(serialPort, TIOCMBIS, &rtsPin);
    if (status)
        fprintf(stderr, "ERROR: Failed to select the boot from flash\n");
    return status;
}

int downloadImage(int serialPort)
{
    int status;

    status = ioctl(serialPort, TIOCMBIC, &rtsPin);
    if (status)
        fprintf(stderr, "ERROR: Failed to select download image\n");
    return status;
}

int powerOff(int serialPort)
{
    int status;

    status = ioctl(serialPort, TIOCMBIC, &dtrPin);
    if (status)
        fprintf(stderr, "ERROR: Failed to power off the ESP-32\n");
    return status;
}

int powerOn(int serialPort)
{
    int status;

    status = ioctl(serialPort, TIOCMBIS, &dtrPin);
    if (status)
        fprintf(stderr, "ERROR: Failed to power on the ESP-32\n");
    return status;
}

int main (int argc, const char ** argv)
{
    struct termios params;
    int serialPort;
    const char * serialPortName;
    int status;

    serialPort = -1;
    status = 0;
    do
    {
        // Display the help if necessary
        if (argc != 2)
        {
            status = -1;
            printf("%s   serial_port\n", argv[0]);
            break;
        }

        // Open the serial port
        serialPortName = argv[1];
        serialPort = open (serialPortName, O_RDWR);
        if (serialPort < 0)
        {
            status = errno;
            perror ("ERROR: Failed to open the terminal");
            break;
        }

        // Get the terminal attributes
        if (tcgetattr(serialPort, &params) != 0)
        {
            status = errno;
            perror("ERROR: tcgetattr failed!");
            break;
        }

        // Power off the RTK's ESP-32
        status = powerOff(serialPort);
        if (status)
            break;
        usleep(POWER_OFF_DELAY);

        // Select the boot device
        status = bootFromFlash(serialPort);
        if (status)
            break;

        // Power on the RTK's ESP-32
        status = powerOn(serialPort);
    } while (0);

    // Close the terminal
    if (serialPort >= 0)
        close(serialPort);
    return status;
}
