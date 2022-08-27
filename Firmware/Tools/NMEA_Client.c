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
    int nmeaSocket;
    struct sockaddr_in serverIpAddress;


    // Display the help text
    if (argc != 2)
    {
        printf ("%s   serverIpAddress\n", argv[0]);
        return -1;
    }

    // Initialize the server address
    memset (&serverIpAddress, '0', sizeof(serverIpAddress));
    serverIpAddress.sin_family = AF_INET;
    serverIpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverIpAddress.sin_port = htons(1958);
    if (inet_pton (AF_INET, argv[1], &serverIpAddress.sin_addr) <= 0)
    {
        perror ("ERROR - Invalid IPv4 address!\n");
        return -2;
    }

    //Create the socket
    nmeaSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (nmeaSocket < 0)
    {
        perror ("ERROR - Unable to create the client socket!\n");
        return -3;
    }

    if (connect (nmeaSocket, (struct sockaddr *)&serverIpAddress, sizeof(serverIpAddress)) < 0)
    {
       perror("Error : Failed to connect to NMEA server!\n");
       return -4;
    }

    // Read the NMEA data from the client
    while ((bytesRead = read (nmeaSocket, rxBuffer, sizeof(rxBuffer)-1)) > 0)
    {
        // Zero terminate the NMEA string
        rxBuffer[bytesRead] = 0;

        // Output the NMEA buffer
        if (fputs ((char *)rxBuffer, stdout) == EOF)
        {
            perror ("ERROR - Failed to write to stdout!\n");
            return -5;
        }
    }

    if (bytesRead < 0)
    {
        perror ("ERROR - Failed reading from NMEA server!\n");
        return -6;
    }

    // Successful execution
    return 0;
}
