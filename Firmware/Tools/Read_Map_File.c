/**********************************************************************
* Read_Map_File.c
*
* Program to read the map file and process the ESP32 backtrace
**********************************************************************/
/*
  Linux:

  1.  From the top level RTK directory, build the RTK_Surveyor.ino.bin file
      using the Arduino CLI, an example:

    export DEBUG_LEVEL=debug
    export ENABLE_DEVELOPER=true
    export FIRMWARE_VERSION_MAJOR=3
    export FIRMWARE_VERSION_MINOR=7

    ~/Arduino/arduino-cli compile --fqbn "esp32:esp32:esp32":DebugLevel=$DEBUG_LEVEL \
    ./Firmware/RTK_Surveyor/RTK_Surveyor.ino  --build-property \
    build.partitions=app3M_fat9M_16MB  --build-property upload.maximum_size=3145728 \
    --build-property "compiler.cpp.extra_flags=\"-DPOINTPERFECT_TOKEN=$POINTPERFECT_TOKEN\" \"-DFIRMWARE_VERSION_MAJOR=$FIRMWARE_VERSION_MAJOR\" \"-DFIRMWARE_VERSION_MINOR=$FIRMWARE_VERSION_MINOR\" \"-DENABLE_DEVELOPER=$ENABLE_DEVELOPER\"" \
    --export-binaries

  2.  Load the firmware into the RTK, an example:

    ~/SparkFun/new-firmware.sh ttyUSB0  Firmware/RTK_Surveyor/build/esp32.esp32.esp32/RTK_Surveyor.ino.bin

  3.  Capture the backtrace using a terminal connected to the RTK

  4.  Run the Read_Map_File application and give it the backtrace, an example:

    Firmware/Tools/Read_Map_File   Firmware/RTK_Surveyor/build/esp32.esp32.esp32/RTK_Surveyor.ino.map
    Backtrace:0x40117bc4:0x3ffec6600x40117c05:0x3ffec680 0x400d941e:0x3ffec6a0 0x400dbedb:0x3ffec6c0 0x400dc413:0x3ffec6f0
*/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//----------------------------------------
// Data structures
//----------------------------------------

typedef struct _SYMBOL_TYPE
{
    struct _SYMBOL_TYPE * nextSymbol;
    uint64_t address;
    uint64_t length;
    char * name;
} SYMBOL_TYPE;

//----------------------------------------
// Constants
//----------------------------------------

#define HALF_READ_BUFFER_SIZE   65536
#define STDIN                   0

#define nullptr                 ((void *)0)

//----------------------------------------
// Globals
//----------------------------------------

int exitStatus;
size_t fileLength;
char readBuffer[HALF_READ_BUFFER_SIZE * 2];
size_t readBufferBytes;
bool readBufferEndOfFile;
size_t readBufferHead;
size_t readBufferTail;
int symbolEntries;
SYMBOL_TYPE * symbolListHead;
SYMBOL_TYPE * symbolListTail;

//----------------------------------------
// Support routines
//----------------------------------------

void dumpBuffer(char * buffer, size_t length)
{
  ssize_t bytes;
  char * end;
  unsigned int index;
  ssize_t offset;

  offset = 0;
  end = &buffer[length];
  while (buffer < end)
  {
    //Determine the number of bytes to display on the line
    bytes = end - buffer;
    if (bytes > (16 - (offset & 0xf)))
      bytes = 16 - (offset & 0xf);

    //Display the offset
    printf("0x%08lx: ", offset);

    //Skip leading bytes
    for (index = 0; index < (offset & 0xf); index++)
      printf("   ");

    //Display the data bytes
    for (index = 0; index < bytes; index++)
      printf("%02x ", buffer[index]);

    //Separate the data bytes from the ASCII
    for (; index < (16 - (offset & 0xf)); index++)
      printf("   ");
    printf(" ");

    //Skip leading bytes
    for (index = 0; index < (offset & 0xf); index++)
      printf(" ");

    //Display the ASCII values
    for (index = 0; index < bytes; index++)
      printf("%c", ((buffer[index] < ' ') || (buffer[index] >= 0x7f))
                   ? '.' : buffer[index]);
    printf("\r\n");

    //Set the next line of data
    buffer += bytes;
    offset += bytes;
  }
}

// Remove the white space
char * removeWhiteSpace(char * buffer)
{
    while (*buffer && ((*buffer == ' ') || (*buffer == '\t')))
        buffer++;
    return buffer;
}

// Skip over the text
char * skipOverText(char * buffer)
{
    while (*buffer && (*buffer != ' ') && (*buffer != '\t'))
        buffer++;
    return buffer;
}

//----------------------------------------
// File read routines
//----------------------------------------

int readFromFile(int mapFile)
{
    ssize_t bytesRead;

    exitStatus = 0;
    bytesRead = read(mapFile, &readBuffer[readBufferHead], HALF_READ_BUFFER_SIZE);
    if (bytesRead < 0)
    {
        exitStatus = errno;
        perror("ERROR: Failed to read from file!\n");
    }
    else
    {
        // Adjust the read pointer
        readBufferBytes += bytesRead;
        readBufferHead += bytesRead;
        fileLength += bytesRead;
        if (readBufferHead >= sizeof(readBuffer))
            readBufferHead -= sizeof(readBuffer);

        // Flag the end-of-file
        if (bytesRead < HALF_READ_BUFFER_SIZE)
            readBufferEndOfFile = true;
    }
    return exitStatus;
}

ssize_t readLineFromFile(int mapFile, char * buffer, size_t maxLineLength)
{
    char * bufferEnd;
    char * bufferStart;
    char byte;
    int status;
    static size_t offset;

    bufferStart = buffer;
    bufferEnd = &buffer[maxLineLength - 1];
    while (buffer < bufferEnd)
    {
        // Fill the readBuffer if necessary
        if ((!readBufferEndOfFile)
            && (readBufferBytes <= HALF_READ_BUFFER_SIZE))
        {
            status = readFromFile(mapFile);
            if (status)
                return -1;
        }

        // Check for end-of-file
        if (readBufferEndOfFile && (readBufferHead == readBufferTail))
            break;

        // Get the next byte from the buffer
        readBufferBytes -= 1;
        byte = readBuffer[readBufferTail++];
        if (readBufferTail >= sizeof(readBuffer))
            readBufferTail = 0;

        // Done when the line feed or NULL is found
        if ((byte == '\n') || (!byte))
            break;

        // Skip carriage returns
        if (byte == '\r')
            continue;

        // Place the rest of the data into the buffer
        *buffer++ = byte;
    }

    // Zero terminate the string
    *buffer = 0;
    return bufferEnd - bufferStart;
}

//----------------------------------------
// Map file parsing routines
//----------------------------------------

int parseMapFile(int mapFile)
{
    uint64_t baseAddress;
    char * buffer;
    uint64_t length;
    static char line[HALF_READ_BUFFER_SIZE];
    ssize_t lineLength;
    static size_t lineNumber;
    size_t lineOffset;
    int offset;
    char * symbol;
    static char symbolBuffer[HALF_READ_BUFFER_SIZE];
    const char * text = " .text._Z";
    int textLength;
    int value;

    textLength = strlen(text);
    do
    {
        // Read a line from the map file
        lineLength = readLineFromFile(mapFile, line, sizeof(line));
        if (lineLength <= 0)
            break;
        lineNumber += 1;

        // Locate the routine names
        if (strncmp(line, text, textLength) == 0)
        {
            // Remove the value
            buffer = &line[textLength];
            value = 0;
            while ((*buffer >= '0') && (*buffer <= '9'))
            {
                value *= 10;
                value += *buffer++ - '0';
            }
            if (!value)
                continue;

            // Get the symbol name
            symbol = &symbolBuffer[0];
            while (*buffer && (*buffer != ' ') && (*buffer != '\t'))
                *symbol++ = *buffer++;
            *symbol = 0;

            // Remove the last character of the symbol
            if (symbolBuffer[0])
                *--symbol = 0;

            // Remove the white space
            buffer = removeWhiteSpace(buffer);

            // Read the symbol address and length from next line if necessary
            if (!*buffer)
            {
                // Read a line from the map file
                lineLength = readLineFromFile(mapFile, line, sizeof(line));
                if (lineLength <= 0)
                    break;
                lineNumber += 1;
                buffer = line;
            }

            // Remove the white space
            buffer = removeWhiteSpace(buffer);

            // Get the symbol address
            if (sscanf(buffer, "0x%016lx", &baseAddress) != 1)
                continue;

            // Remove the white space
            buffer = skipOverText(buffer);
            buffer = removeWhiteSpace(buffer);

            // Get the symbol length
            if (sscanf(buffer, "0x%lx", &length) == 1)
            {
                // Routines are in flash which starts at 0x40000000
                if (baseAddress >= 0x40000000)
                {
                    SYMBOL_TYPE * entry;

                    entry = malloc(sizeof(SYMBOL_TYPE) + 7 + strlen(symbolBuffer) + 1);
                    if (entry)
                    {
                        // Build the symbol entry
                        entry->nextSymbol = nullptr;
                        entry->address = baseAddress;
                        entry->length = length;
                        entry->name = &((char *)entry)[sizeof(SYMBOL_TYPE)];
                        strcpy(entry->name, symbolBuffer);

                        // Add the symbol to the list
                        if (!symbolListHead)
                        {
                            // This is the first entry in the list
                            symbolListHead = entry;
                            symbolListTail = entry;
                        }
                        else
                        {
                            // Add this entry of the end of the list
                            symbolListTail->nextSymbol = entry;
                            symbolListTail = entry;
                        }
                        symbolEntries += 1;
//                        printf("0x%08lx-0x%08lx: %s\n",
//                               baseAddress, baseAddress + length - 1, symbolBuffer);
                    }
                }
            }
        }
    } while (readBufferBytes);
    return exitStatus;
}

// Find the matching symbol
char * findSymbolName(uint64_t pc)
{
    SYMBOL_TYPE * symbol;

    // Walk the symbol list
    symbol = symbolListHead;
    while (symbol)
    {
        if ((symbol->address <= pc) && ((symbol->address + symbol->length) > pc))
            return symbol->name;
        symbol = symbol->nextSymbol;
    }
    return nullptr;
}

// Enable the user to enter the backtrace and display the symbols
int processBacktrace()
{
    char * buffer;
    const char * const backtrace = "Backtrace:";
    SYMBOL_TYPE * backtraceEntry;
    SYMBOL_TYPE * backtraceList;
    ssize_t bytesRead;
    char * line;
    size_t lineLength;
    uint64_t pc;
    uint64_t stackPointer;
    int status;
    char * symbolName;

    do
    {
        // Read the backtrace from the input
        backtraceList = nullptr;
        line = nullptr;
        lineLength = 0;
        status = 0;
        bytesRead = getline(&line, &lineLength, stdin);
        if (bytesRead < 0)
        {
            status = errno;
            break;
        }

        // Skip over the Backtrace:
        buffer = line;
        if (strncmp(buffer, backtrace, strlen(backtrace)) == 0)
            buffer += strlen(backtrace);

        // Skip any white space
        buffer = removeWhiteSpace(buffer);

        // The backtrace line looks like the following:
        //      Backtrace:0x4010d9b4:0x3ffebe300x4010d9f5:0x3ffebe50 0x400d8c81:0x3ffebe70 0x400db357:0x3ffebe90 0x400db81f:0x3ffebec0

        // Parse the backtrace
        while (1)
        {
            // Get the program counter address
            if (sscanf(&buffer[2], "%08lx", &pc) != 1)
                break;

            // Skip over the previous value
            //        0x value :  0x
            buffer +=  2 + 8;
            if (*buffer == ':')
                buffer++;
            buffer +=  2;

            // Get the stack pointer value
            if (sscanf(buffer, "%08lx", &stackPointer) != 1)
                break;

            // Process the PC value
            backtraceEntry = malloc(sizeof(*backtraceEntry));
            if (backtraceEntry)
            {
                // Reverse the order of the backtrace entries
                backtraceEntry->nextSymbol = backtraceList;
                backtraceEntry->address = pc;
                backtraceEntry->length = stackPointer;
                backtraceList = backtraceEntry;
            }

            // Skip any white space
            buffer +=  8;
            buffer = removeWhiteSpace(buffer);
        }
    } while(0);

    // Display the backtrace
    if (backtraceList)
    {
        // Table header
        printf("\n");
        printf("    PC        Stack Ptr   Symbol\n");
        printf("----------   ----------   --------------------\n");

        // Walk the backtrace symbol list
        backtraceEntry = backtraceList;
        while (backtraceEntry)
        {
            symbolName = findSymbolName(backtraceEntry->address);
            printf("0x%08lx   0x%08lx", backtraceEntry->address, backtraceEntry->length);
            if (symbolName)
                printf("   %s", symbolName);
            printf("\n");
            backtraceEntry = backtraceEntry->nextSymbol;
        }
    }

    // Done with the line
    if (line)
        free(line);
    return status;
}

//----------------------------------------
// Application
//----------------------------------------

int main(int argc, char ** argv)
{
    char * filename;
    int mapFile;
    int status;

    do
    {
        status = -1;

        // Display the help text
        if (argc != 2)
        {
            printf ("%s  filename\n", argv[0]);
            return -1;
        }

        // Open the file
        filename = argv[1];
        mapFile = open(filename, O_RDONLY);
        if (mapFile < 0)
        {
            status = errno;
            perror("ERROR: Unable to open the file\n");
        }

        // Parse the map file
        status = parseMapFile(mapFile);
    } while (0);

    if (!status)
    {
        // Display the number of symbols
        printf("symbolEntries: %d\n", symbolEntries);

        // Process the backtrace
        printf("Enter the backtrace:\n");
        status = processBacktrace();
    }

    // Close the file
    if (mapFile >= 0)
        close(mapFile);

    return status;
}
