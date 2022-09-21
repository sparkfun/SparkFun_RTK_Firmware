/*
 * NMEA_Client.c
 *
 * Program to display the NMEA messages from the RTK Express
 */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

uint8_t rxBuffer[2048];

int
main (
    int argc,
    char ** argv
    )
{
    int bytesRead;
    int bytesWritten;
    int displayHelp;
    struct sockaddr_in rtkServerIpAddress;
    int rtkSocket;
    int status;
    struct sockaddr_in vespucciServerIpAddress;
    int vespucciSocket;

    do {
        displayHelp = 1;
        status = 0;
        if ((argc < 2) || (argc > 3))
        {
            status = -1;
            break;
        }

        // Initialize the RTK server address
        memset (&rtkServerIpAddress, '0', sizeof(rtkServerIpAddress));
        rtkServerIpAddress.sin_family = AF_INET;
        rtkServerIpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        rtkServerIpAddress.sin_port = htons(1958);
        if (inet_pton (AF_INET, argv[1], &rtkServerIpAddress.sin_addr) <= 0)
        {
            perror ("ERROR - Invalid RTK server IPv4 address!\n");
            status = -2;
            break;
        }

        if (argc == 3)
        {
            // Initialize the Vespucci server address
            memset (&vespucciServerIpAddress, '0', sizeof(vespucciServerIpAddress));
            vespucciServerIpAddress.sin_family = AF_INET;
            vespucciServerIpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            vespucciServerIpAddress.sin_port = htons(1958);
            if (inet_pton (AF_INET, argv[2], &vespucciServerIpAddress.sin_addr) <= 0)
            {
                perror ("ERROR - Invalid Vespucci IPv4 address!\n");
                status = -3;
                break;
            }
            displayHelp = 0;

            //Create the vespucci socket
            vespucciSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (vespucciSocket < 0)
            {
                perror ("ERROR - Unable to create the Vespucci client socket!\n");
                status = -4;
                break;
            }

            if (connect (vespucciSocket, (struct sockaddr *)&vespucciServerIpAddress, sizeof(vespucciServerIpAddress)) < 0)
            {
               perror("Error : Failed to connect to Vespucci NMEA server!\n");
               status = -5;
               break;
            }
        }
        displayHelp = 0;

        //Create the RTK socket
        rtkSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (rtkSocket < 0)
        {
            perror ("ERROR - Unable to create the RTK client socket!\n");
            status = -6;
            break;
        }

        if (connect (rtkSocket, (struct sockaddr *)&rtkServerIpAddress, sizeof(rtkServerIpAddress)) < 0)
        {
           perror("Error : Failed to connect to RTK NMEA server!\n");
           status = -7;
           break;
        }

        // Read the NMEA data from the RTK server
        while ((bytesRead = read (rtkSocket, rxBuffer, sizeof(rxBuffer)-1)) > 0)
        {
            // Zero terminate the NMEA string
            rxBuffer[bytesRead] = 0;

            // Output the NMEA buffer
            if (fputs ((char *)rxBuffer, stdout) == EOF)
            {
                perror ("ERROR - Failed to write to stdout!\n");
                status = -8;
                break;
            }

            // Forward the NMEA data to the Vespucci server
            if (argc == 3)
            {
                if ((bytesWritten = write (vespucciSocket, rxBuffer, bytesRead)) != bytesRead)
                {
                    perror ("ERROR - Failed to write to Vespucci socket");
                    status = -9;
                    break;
                }
            }
        }

        if (bytesRead <= 0)
        {
            perror ("ERROR - Failed reading from NMEA server!\n");
            status = -10;
        }
    } while (0);

    // Display the help text
    if (displayHelp)
        printf ("%s   RtkServerIpAddress   [VespucciServerIpAddress]\n", argv[0]);

    return status;
}
