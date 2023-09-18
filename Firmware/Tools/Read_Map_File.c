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

typedef struct _PARSE_TABLE_ENTRY
{
    const char * string;
    size_t stringLength;
    void (* routine)(char * line);
} PARSE_TABLE_ENTRY;

//----------------------------------------
// Constants
//----------------------------------------

#define C_PLUS_PLUS_HEADER      "_Z"
#define HALF_READ_BUFFER_SIZE   65536
#define STDIN                   0

#define nullptr                 ((void *)0)

//----------------------------------------
// Globals
//----------------------------------------

int exitStatus;
size_t fileLength;
char line[HALF_READ_BUFFER_SIZE];
int lineNumber;
int mapFile;
char readBuffer[HALF_READ_BUFFER_SIZE * 2];
size_t readBufferBytes;
bool readBufferEndOfFile;
size_t readBufferHead;
size_t readBufferTail;
char symbolBuffer[HALF_READ_BUFFER_SIZE];
int symbolEntries;
SYMBOL_TYPE * symbolListHead;
SYMBOL_TYPE * symbolListTail;

//----------------------------------------
// Macros
//----------------------------------------

#define PARSE_ENTRY(string, routine) \
    {string, sizeof(string) - 1, routine}

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

int readFromFile()
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

ssize_t readLineFromFile(char * buffer, size_t maxLineLength)
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
            status = readFromFile();
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

void symbolAdd(const char * symbol, uint64_t baseAddress, uint64_t length)
{
    SYMBOL_TYPE * entry;

    entry = malloc(sizeof(SYMBOL_TYPE) + 7 + strlen(symbol) + 1);
    if (entry)
    {
        // Build the symbol entry
        entry->nextSymbol = nullptr;
        entry->address = baseAddress;
        entry->length = length;
        entry->name = &((char *)entry)[sizeof(SYMBOL_TYPE)];
        strcpy(entry->name, symbol);

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
//        printf("0x%08lx-0x%08lx: %s\n",
//               baseAddress, baseAddress + length - 1, symbol);
    }
}

char * symbolGetName(char * buffer)
{
    int c;
    size_t headerLength;
    char * symbolName;

    // Determine if this is a c symbol
    headerLength = sizeof(C_PLUS_PLUS_HEADER) - 1;
    c = strncmp(buffer, C_PLUS_PLUS_HEADER, headerLength);

    // Remove the c++ header from the symbol
    if (!c)
    {
        buffer += headerLength;
        if (*buffer == 'N')
            buffer++;
        if (*buffer == 'K')
            buffer++;

        // Remove the value
        while ((*buffer >= '0') && (*buffer <= '9'))
            buffer++;
    }

    // Get the symbol name
    symbolName = &symbolBuffer[0];
    while (*buffer && (*buffer != ' ') && (*buffer != '\t'))
        *symbolName++ = *buffer++;
    *symbolName = 0;

    // Remove the last character of a c++ symbol
    if ((!c) && (symbolBuffer[0]))
        *--symbolName = 0;

    // Return the next position in the buffer
    return buffer;
}

void parseIram1(char * buffer)
{
    uint64_t baseAddress;
    uint64_t length;

    /* There are 4 different forms of iram1 lines:

 .iram1.30      0x0000000040084f00       0x5c /home/.../libheap.a(heap_caps.c.obj)
                0x0000000040084f00                heap_caps_malloc_prefer

 .iram1.4       0x00000000400851b0       0x21 /home/.../libesp_hw_support.a(cpu_util.c.obj)
                0x00000000400851b0                esp_cpu_reset
 *fill*         0x00000000400851d1        0x3

 .iram1.34      0x0000000040084f5c       0x36 /home/.../libheap.a(heap_caps.c.obj)
                                         0x3e (size before relaxing)
                0x0000000040084f5c                heap_caps_free

 .iram1.5       0x00000000400851d4       0x31 /home/.../libesp_hw_support.a(cpu_util.c.obj)
                                         0x35 (size before relaxing)
                0x00000000400851d4                esp_cpu_set_watchpoint
 *fill*         0x0000000040085205        0x3
        */

    do
    {
        // Skip the rest of the iram1 stuff
        buffer = skipOverText(buffer);

        // Remove the white space
        buffer = removeWhiteSpace(buffer);

        // Get the symbol address
        if (sscanf(buffer, "0x%016lx", &baseAddress) != 1)
            break;

        // Validate the symbol address
        if (baseAddress < 0x40000000)
            break;

        // Get the symbol length
        buffer = skipOverText(buffer);
        buffer = removeWhiteSpace(buffer);
        if (sscanf(buffer, "0x%lx", &length) != 1)
            break;

        // Read a line from the map file
        if (readLineFromFile(line, sizeof(line)) <= 0)
            break;
        lineNumber += 1;
        buffer = &line[16];

        // Locate the symbol value
        buffer += 16;
        if (*buffer != '0')
        {
            // Read a line from the map file
            if (readLineFromFile(line, sizeof(line)) <= 0)
                break;
            lineNumber += 1;
            buffer = &line[16];
        }

        // Get the symbol name
        buffer = skipOverText(buffer);
        buffer = removeWhiteSpace(buffer);
        buffer = symbolGetName(buffer);
        symbolAdd(symbolBuffer, baseAddress, length);
    } while (0);
}

void parseTextLine(char * buffer)
{
    uint64_t baseAddress;
    uint64_t length;
    size_t lineOffset;
    int offset;
    char * symbol;
    int value;

    /* There are 6 different forms of iram1 lines:
 .text.i2cRead  0x0000000040132094       0xfc /tmp/.../core.a(esp32-hal-i2c.c.o)
                                        0x107 (size before relaxing)
                0x0000000040132094                i2cRead

 .text.i2cRead  0x0000000040132094       0xff /tmp/.../core.a(esp32-hal-i2c.c.o)
                                        0x107 (size before relaxing)
                0x0000000040132094                i2cRead
 *fill*         0x0000000040132193        0x1

 .text._Z21createMessageListBaseR6String
                0x00000000400dc990      0x150 /tmp/.../RTK_Surveyor.ino.cpp.o
                0x00000000400dc990                _Z21createMessageListBaseR6String

 .text._Z17ntripClientUpdatev
                0x00000000400dfe24      0x462 /tmp/.../RTK_Surveyor.ino.cpp.o
                0x00000000400dfe24                _Z17ntripClientUpdatev
 *fill*         0x00000000400e0286        0x2

 .text._Z12printUnknownh
                0x00000000400e1058       0x18 /tmp/.../RTK_Surveyor.ino.cpp.o
                                         0x20 (size before relaxing)
                0x00000000400e1058                _Z12printUnknownh

 .text._Z12printUnknowni
                0x00000000400e1070       0x13 /tmp/.../RTK_Surveyor.ino.cpp.o
                                         0x1a (size before relaxing)
                0x00000000400e1070                _Z12printUnknowni
 *fill*         0x00000000400e1083        0x1
    */

    do
    {
        // Get the symbol name
        buffer = symbolGetName(buffer);

        // Remove the white space
        buffer = removeWhiteSpace(buffer);

        // Read the symbol address and length from next line if necessary
        if (!*buffer)
        {
            // Read a line from the map file
            if (readLineFromFile(line, sizeof(line)) <= 0)
                break;
            lineNumber += 1;
            buffer = line;
        }

        // Remove the white space
        buffer = removeWhiteSpace(buffer);

        // Get the symbol address
        if (sscanf(buffer, "0x%016lx", &baseAddress) != 1)
            break;

        // Remove the white space
        buffer = skipOverText(buffer);
        buffer = removeWhiteSpace(buffer);

        // Get the symbol length
        if (sscanf(buffer, "0x%lx", &length) == 1)
        {
            // Routines are in flash which starts at 0x40000000
            if (baseAddress >= 0x40000000)
                    symbolAdd(symbolBuffer, baseAddress, length);
        }
    } while (0);
}

// Define the search text and routine to process the line when found
const PARSE_TABLE_ENTRY parseTable[] =
{
    PARSE_ENTRY(" .iram1.",  parseIram1),
    PARSE_ENTRY(" .text.", parseTextLine),
};
const int parseTableEntries = sizeof(parseTable) / sizeof(parseTable[0]);

// Parse the map file to locate the symbols and their addresses and lengths
int parseMapFile()
{
    const PARSE_TABLE_ENTRY * entry;
    int index;
    ssize_t lineLength;

    do
    {
        // Read a line from the map file
        lineLength = readLineFromFile(line, sizeof(line));
        if (lineLength <= 0)
            break;
        lineNumber += 1;

        // Locate the matching text in the map file
        for (index = 0; index < parseTableEntries; index++)
        {
            // Locate the routine names
            entry = &parseTable[index];
            if (strncmp(line, entry->string, entry->stringLength) == 0)
            {
                parseTable[index].routine(&line[entry->stringLength]);
                break;
            }
        }
    } while (readBufferBytes);

/*
    uint64_t baseAddress;
    char * buffer;
    uint64_t length;
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
        lineLength = readLineFromFile(line, sizeof(line));
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
                lineLength = readLineFromFile(line, sizeof(line));
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
*/

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
        status = parseMapFile();
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
