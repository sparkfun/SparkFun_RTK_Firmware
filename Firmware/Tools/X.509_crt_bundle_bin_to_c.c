/*
 * X.509_crt_bundle_bin_to_c.c
 *
 * Program to convert the .bin file into a "c" data structure
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define BYTES_MAX           16

int main(int argc, char ** argv)
{
    uint8_t buffer[1024];
    int bundleFile;
    ssize_t bytesRead;
    int bytesToDisplay;
    uint8_t * data;
    uint8_t * dataEnd;
    int fileOffset;
    int index;
    ssize_t offset;
    int status;

    bundleFile = 0;
    do
    {
        // Display the help text
        if (argc != 2)
        {
            printf("%s   x509_crt_bundle_bin_file\n", argv[0]);
            status = -1;
            break;
        }

        // Open the binary file containing the certificate bundle
        bundleFile = open(argv[1], O_RDONLY);
        if (bundleFile < 0)
        {
            status = errno;
            perror("ERROR: Unable to open certificate bundle file\n");
            break;
        }

        // Display the header
        printf("const uint8_t x509CertificateBundle[] =\n");
        printf("{\n");

        // Assume success
        status = 0;

        // Walk through the BIN file data
        fileOffset = 0;
        while (1)
        {
            // Read more data from the file
            bytesRead = read(bundleFile, buffer, sizeof(buffer));
            if (bytesRead < 0)
            {
                status = errno;
                perror("ERROR: Failed during read of BIN file\n");
                break;
            }

            // Check for end of file
            if (!bytesRead)
            {
                printf("};\n\n");
                break;
            }

            // Display the data read from the file
            data = &buffer[0];
            dataEnd = &data[bytesRead];
            while (data < dataEnd)
            {
                // Display the row of data
                bytesToDisplay = dataEnd - data;
                if (bytesToDisplay > BYTES_MAX)
                    bytesToDisplay = BYTES_MAX;
                printf("    ");
                for ( index = 0; index < bytesToDisplay; index++)
                    printf("%s0x%02x", index ? ", " : "", *data++);
                printf(",");
                fileOffset += bytesToDisplay;
                printf("  // %5d\n", fileOffset);
            }
        };
    } while (0);

    // Close the certificate bundle
    if (bundleFile > 0)
        close(bundleFile);
    return status;
}

