// Split_Messages.c

#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "crc24q.h"
#include "crc24q.c"
//#include "Crc24q.h"

#define COMPUTE_CRC24Q(parse, data)  (((parse)->crc << 8) ^ crc24q[data ^ (((parse)->crc >> 16) & 0xff)])

#define DISPLAY_BAD_CHARACTERS          0
#define DISPLAY_BAD_CHARACTER_OFFSETS   1
#define DISPLAY_RTCM_MESSAGE_LIST       1
#define DISPLAY_UBX_MESSAGE_LIST        1
#define DISPLAY_BOUNDARY                0
#define DISPLAY_DATA_BYTES              0
#define DISPLAY_LIST                    0
#define DISPLAY_MESSAGE_TYPE            0
#define DISPLAY_MESSAGE_TYPE_LIST       1
#define DISPLAY_NMEA_MESSAGES           1
#define DISPLAY_STRINGS                 0

#define MESSAGE_LENGTH(length)          (3 + length + 3)

#define BINARY_MESSAGE_START            0xd3

#define MAX_BAD_CHARACTERS              1000

uint32_t bad_characters[256>>5];
uint32_t bad_character_count[256];
uint32_t bad_character_offset[MAX_BAD_CHARACTERS];
uint32_t bad_character_length[MAX_BAD_CHARACTERS];
int32_t bad_character_offset_count = -1;

char buffer[65536];
char string[256];
uint8_t * file_data;
uint32_t rtcm_messages[4096 >> 5];
uint32_t rtcm_message_count[4096];
uint32_t rtcm_max_message_length[4096];
uint32_t ubx_messages[65536 >> 5];
uint32_t ubx_message_count[65536];
uint32_t ubx_max_message_length[65536];
int bad_checksum_header;
int nmea_checksum_errors;
int rtcm_crc_errors;
int ubx_checksum_errors;

typedef struct _NMEA_MESSAGE {
    struct _NMEA_MESSAGE * next;
    uint8_t * message;
    uint32_t count;
    uint32_t max_length;
} NMEA_MESSAGE;

NMEA_MESSAGE * nmea_list;

enum SentenceTypes
{
SENTENCE_TYPE_NONE = 0,
SENTENCE_TYPE_NMEA,
SENTENCE_TYPE_UBX,
SENTENCE_TYPE_RTCM
} currentSentence = SENTENCE_TYPE_NONE;

typedef struct _PARSE_STATE * P_PARSE_STATE;

//Parse routine
typedef uint8_t (* PARSE_ROUTINE)(P_PARSE_STATE parse,  //Parser state
                                  uint8_t data);        //Incoming data byte

//End of message callback routine
typedef void (* EOM_CALLBACK)(P_PARSE_STATE parse,      //Parser state
                                 uint8_t type);         //Message type

#define PARSE_BUFFER_LENGTH       0x10000

typedef struct _PARSE_STATE
{
  PARSE_ROUTINE state;            //Parser state routine
  EOM_CALLBACK eomCallback;       //End of message callback routine
  const char * parserName;        //Name of parser
  uint32_t crc;                   //RTCM computed CRC
  uint32_t rtcmCrc;               //Computed CRC value for the RTCM message
  uint32_t invalidRtcmCrcs;       //Number of bad RTCM CRCs detected
  uint16_t bytesRemaining;        //Bytes remaining in RTCM CRC calculation
  uint16_t length;                //Message length including line termination
  uint16_t maxLength;             //Maximum message length including line termination
  uint16_t message;               //RTCM message number
  uint16_t nmeaLength;            //Length of the NMEA message without line termination
  uint8_t buffer[PARSE_BUFFER_LENGTH];  //Buffer containing the message
  uint8_t nmeaMessageName[16];    //Message name
  uint8_t nmeaMessageNameLength;  //Length of the message name
  uint8_t ck_a;                   //U-blox checksum byte 1
  uint8_t ck_b;                   //U-blox checksum byte 2
  bool computeCrc;                //Compute the CRC when true
} PARSE_STATE;

//Forward declaration
uint8_t waitForPreamble(PARSE_STATE * parse, uint8_t data);
uint64_t offset;
uint64_t file_offset;

void
dump_message (
    unsigned char * data
    )
{
    unsigned int actual;
    unsigned int crc;
    int index;
    int length;
    int offset;

    //
    //    +----------+--------+----------------+---------+----------+---------+
    //    | Preamble |  Fill  | Message Length | Message |   Fill   | CRC-24Q |
    //    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits | 24 bits |
    //    |   0xd3   | 000000 |                |         |   zeros  |         |
    //    +----------+--------+----------------+---------+----------+---------+
    //

    // Get the number of bytes
    length = (data[1] << 8) | data[2];
    if (DISPLAY_DATA_BYTES) {
        // Dump the message
        offset = data - file_data;
        printf ("0x%08x: %02x %02x %02x\n", offset, data[0], data[1], data[2]);
        offset += 3;
        index = 0;
        if (length > 0) {
            do
            {
                // Display the offset at the beginning of the line
                if (!(index & 0xf))
                    printf ("0x%08x: ", offset);

                // Display the data bytes
                printf ("%02x ", data[index+3]);
                offset += 1;

                // Terminate the lines
                if (!(++index % (DISPLAY_DATA_BYTES ? DISPLAY_DATA_BYTES : 1)))
                    printf ("\n");
            } while (index < length);

            // Terminate a short line
            if (index % (DISPLAY_DATA_BYTES ? DISPLAY_DATA_BYTES : 1))
                printf ("\n");
        }

        // Display the Cyclic Redundancy Check (CRC)
        printf ("0x%08x: %02x %02x %02x CRC: %s\n",
                offset, data[3 + index], data[3 + index + 1], data[3 + index + 2],
                crc24q_check (data, MESSAGE_LENGTH(length)) ? "Matches" : "Does not match!");
    } else {
        if (!crc24q_check (data, MESSAGE_LENGTH(length))) {
            if (!bad_checksum_header) {
                bad_checksum_header = 1;
                printf ("Bad checksums:\n");
            }
            length = MESSAGE_LENGTH(length);
            crc = crc24q_hash(data, length - 3);
            actual = (data[length - 3] << 16) | (data[length - 2] << 8) | data[length - 1];
            printf ("    0x%08lx: Binary message, expected CRC: 0x%06x, actual CRC: 0x%06x\n",
                    data - file_data, crc, actual);
            rtcm_crc_errors += 1;
        }
    }
}

void
display_string (
    unsigned char * string,
    int length
    )
{
    char * temp;
    char * temp_end;

    // Copy the strings into the buffer
    memcpy (buffer, string, length);
    buffer[length] = 0;

    // Terminate the strings
    temp = buffer;
    temp_end = &temp[length];
    do {
        if ((*temp == '\r') || (*temp == '\n'))
            *temp = 0;
    } while (++temp < temp_end);

    // Display the strings
    temp = buffer;
    while (temp < temp_end) {
        if (*temp)
            printf ("%s\n", temp);
        temp += strlen(temp) + 1;
    }
}

unsigned char *
process_nmea_message (
    unsigned char * data,
    unsigned char * data_end
    )
{
    unsigned char checksum;
    char checksum_char[2];
    NMEA_MESSAGE * message;
    NMEA_MESSAGE * previous;
    unsigned char * start;

    // Check for the beginning of a NMEA message ($)
    if (*data != '$') {
        if ((*data != '\r') && (*data != '\n')) {
            bad_character_count[*data] += 1;
            bad_characters[*data >> 5] |= 1 << (*data & 0x1f);
            if ((bad_character_offset_count < 0)
                || ((bad_character_offset[bad_character_offset_count] + bad_character_length[bad_character_offset_count]) != (data - file_data))) {
                bad_character_offset_count += 1;
                bad_character_offset[bad_character_offset_count] = data - file_data;
            }
            bad_character_length[bad_character_offset_count] += 1;
        }
        return data + 1;
    }

    // Skip over the dollar sign ($)
    start = data++;

    // Scan for comma or end of message
    checksum = 0;
    while ((*data != ',') && (*data != '\r') && (*data != '\n')
        && (*data != BINARY_MESSAGE_START))
        checksum ^= *data++;

    // Return if this is the start of a binary message
    if (*data == BINARY_MESSAGE_START)
        return data;

    // Remember the message if a comma was found
    if (*data == ',') {
        if (DISPLAY_LIST)
            printf ("----------------------\r\n");

        // Build the zero terminated string
        memset (string, 0, sizeof(string));
        strncpy (string, (char *)start, data - start);

        /*
            list --> NULL

            previous = NULL
            string: $GNGST --.
                             V
                     +------------+
            list --> | $GNGST (1) | --> NULL
                     +------------+

            previous = $GNGST
            string: $GNGGA --.
                             V
                     +------------+     +------------+
            list --> | $GNGGA (1) | --> | $GNGST (1) | --> NULL
                     +------------+     +------------+

            previous = $GNGSA
            string: $GNGSA --------------------.
                                               V
                     +------------+     +------------+     +------------+
            list --> | $GNGGA (1) | --> | $GNGSA (1) | --> | $GNGST (1) | --> NULL
                     +------------+     +------------+     +------------+

            previous = $GNGSA
            string: $GNGSA --------------------.
                                               V
                     +------------+     +------------+     +------------+
            list --> | $GNGGA (1) | --> | $GNGSA (2) | --> | $GNGST (1) | --> NULL
                     +------------+     +------------+     +------------+

            previous = $GNGST
            string: $GNRMC ---------------------------------------------------------.
                                                                                    V
                     +------------+     +------------+     +------------+     +------------+
            list --> | $GNGGA (1) | --> | $GNGSA (2) | --> | $GNGST (1) | --> | $GNRMC (1) | --> NULL
                     +------------+     +------------+     +------------+     +------------+

         */

        // Display the list
        if (DISPLAY_LIST) {
            printf ("list: ");
            for (previous = nmea_list; previous; previous = previous->next)
                printf ("%s(%d)-->", previous->message, previous->count);
            printf ("NULL\n");
        }

        // Find the location for insertion
        // Check for something in the list
        previous = NULL;
        if (nmea_list) {
            if (strcmp((char *)nmea_list->message, string) <= 0) {
                previous = nmea_list;
                while (previous->next && (strcmp((char *)previous->next->message, string) <= 0))
                    previous = previous->next;
            }
        }

        // Display the insertion decision
        if (DISPLAY_LIST) {
            printf ("previous: %s\n", ((previous != NULL) ? (char *)previous->message : (char *)"NULL"));
            printf ("string: %s\n", string);
        }

        // Check for a duplicate message
        if (previous && (strcmp ((char *)previous->message, string) == 0)) {
            if (DISPLAY_LIST)
                printf ("Duplicate found: %d\n", previous->count);
            previous->count += 1;
            message = previous;
        } else {
            // Add the new message
            message = malloc (sizeof(NMEA_MESSAGE) + strlen(string) + 1);
            message->message = (uint8_t *)(message + 1);
            strcpy((char *)message->message, string);
            message->count = 1;

            // Message insertion position
            //      previous == NULL; ==> list head
            //      previous != NULL; ==> middle or end of list
            if (!previous) {
                if (DISPLAY_LIST)
                    printf ("Add to head of the list\n");

                // Add this message at the start of the list
                message->next = nmea_list;
                nmea_list = message;
            } else {
                if (DISPLAY_LIST)
                    printf ("Add to middle of the list\n");

                // Insert this message into the middle of the list
                message->next = previous->next;
                previous->next = message;
            }
        }

        // Display the new list
        if (DISPLAY_LIST) {
            printf ("New list: ");
            for (previous = nmea_list; previous; previous = previous->next)
                printf ("%s(%d)-->", previous->message, previous->count);
            printf ("NULL\n");
        }
    }

    // Scan for asterisk or end of message
    while ((*data != '*') && (*data != '\r') && (*data != '\n')
        && (*data != BINARY_MESSAGE_START))
        checksum ^= *data++;

    // Check for end of NMEA message, validate the checksum
    if (*data == '*') {
        checksum_char[0] = ((checksum >> 4) & 0xf) + '0';
        if (checksum_char[0] > '9')
            checksum_char[0] += 'A' - '0' - 10;
        checksum_char[1] = (checksum & 0xf) + '0';
        if (checksum_char[1] > '9')
            checksum_char[1] += 'A' - '0' - 10;
        if ((toupper(data[1]) != checksum_char[0]) || (toupper(data[2]) != checksum_char[1])) {
            if (!bad_checksum_header) {
                bad_checksum_header = 1;
                printf ("Bad checksums:\n");
            }
            printf ("    0x%08lx: NMEA %s, expected CRC: 0x%02x, actual CRC: 0x%c%c\n",
                    data - file_data, &string[1], checksum, data[1], data[2]);
            nmea_checksum_errors += 1;
        }
        data += 3;
    }

    // Scan for end of message
    while ((*data != '\r') && (*data != '\n') && (*data != BINARY_MESSAGE_START))
        data++;

    return data;
}

uint8_t *
find_gnss_header (
    uint8_t * data,
    uint8_t * data_end
    )
{
    int length;

    do {
        //  From RTCM 10403.2 Section 4
        //
        //    +----------+--------+----------------+---------+----------+---------+
        //    | Preamble |  Fill  | Message Length | Message |   Fill   | CRC-24Q |
        //    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits | 24 bits |
        //    |   0xd3   | 000000 |                |         |   zeros  |         |
        //    +----------+--------+----------------+---------+----------+---------+
        //

        // Locate the beginning of the message
        while ((data < data_end) && (*data != BINARY_MESSAGE_START))
            data = process_nmea_message (data, data_end);

        // Check for end of data
        if (data >= data_end)
            break;

        // Get the number of bytes
        length = (data[1] << 8) | data[2];
        if ((data + MESSAGE_LENGTH(length)) <= data_end) {

            // Verify the CRC
            if (crc24q_check (data, MESSAGE_LENGTH(length)))
                break;
        }

        // Skip this preamble byte
        data++;
    } while (data < data_end);

    // Return the address of the next preamble byte or end of data
    return data;
}

void processNemaMessage(PARSE_STATE * parse)
{
    NMEA_MESSAGE * message;
    NMEA_MESSAGE * previous;

    //Display the NMEA message
//    dumpBuffer(parse->buffer, parse->length);
//    printf("    %s NMEA %s, %d bytes\r\n", parse->parserName, parse->nmeaMessageName, parse->length);

    // Display the list
    if (DISPLAY_LIST) {
        printf ("list: ");
        for (previous = nmea_list; previous; previous = previous->next)
            printf ("%s(%d)-->", previous->message, previous->count);
        printf ("NULL\n");
    }

    // Find the location for insertion
    // Check for something in the list
    previous = NULL;
    if (nmea_list) {
        if (strcmp((char *)nmea_list->message, (char *)parse->nmeaMessageName) <= 0) {
            previous = nmea_list;
            while (previous->next && (strcmp((char *)previous->next->message, (char *)parse->nmeaMessageName) <= 0))
                previous = previous->next;
        }
    }

    // Display the insertion decision
    if (DISPLAY_LIST) {
        printf ("previous: %s\n", ((previous != NULL) ? (char *)previous->message : (char *)"NULL"));
        printf ("string: %s\n", parse->nmeaMessageName);
    }

    // Check for a duplicate message
    if (previous && (strcmp ((char *)previous->message, (char *)parse->nmeaMessageName) == 0)) {
        if (DISPLAY_LIST)
            printf ("Duplicate found: %d\n", previous->count);
        previous->count += 1;
        message = previous;
        if (message->max_length < parse->length)
            message->max_length = parse->length;
    } else {
        // Add the new message
        message = malloc (sizeof(NMEA_MESSAGE) + strlen((char *)parse->nmeaMessageName) + 1);
        message->message = (uint8_t *)(message + 1);
        strcpy((char *)message->message, (char *)parse->nmeaMessageName);
        message->count = 1;
        message->max_length = parse->length;

        // Message insertion position
        //      previous == NULL; ==> list head
        //      previous != NULL; ==> middle or end of list
        if (!previous) {
            if (DISPLAY_LIST)
                printf ("Add to head of the list\n");

            // Add this message at the start of the list
            message->next = nmea_list;
            nmea_list = message;
        } else {
            if (DISPLAY_LIST)
                printf ("Add to middle of the list\n");

            // Insert this message into the middle of the list
            message->next = previous->next;
            previous->next = message;
        }
    }

    // Display the new list
    if (DISPLAY_LIST) {
        printf ("New list: ");
        for (previous = nmea_list; previous; previous = previous->next)
            printf ("%s(%d)-->", previous->message, previous->count);
        printf ("NULL\n");
    }
}

void processRtcmMessage(PARSE_STATE * parse)
{
//    dumpBuffer(parse->buffer, parse->length);
//    printf("    %s RTCM %d, %d bytes\r\n", parse->parserName, parse->message, parse->length);
    rtcm_messages[parse->message >> 5] |= 1 << (parse->message & 0x1f);
    rtcm_message_count[parse->message] += 1;
    if (rtcm_max_message_length[parse->message] < parse->length)
        rtcm_max_message_length[parse->message] = parse->length;
}

void processUbxMessage(PARSE_STATE * parse)
{
//    dumpBuffer(parse->buffer, parse->length);
//    printf("    %s UBX %d.%d, %d bytes\r\n", parse->parserName, parse->message >> 8, parse->message & 0xff, parse->length);
    ubx_messages[parse->message >> 5] |= 1 << (parse->message & 0x1f);
    ubx_message_count[parse->message] += 1;
    if (ubx_max_message_length[parse->message] < parse->length)
      ubx_max_message_length[parse->message] = parse->length;
}

//Process the message
void processMessage(PARSE_STATE * parse, uint8_t type)
{
    switch (type)
    {
    case SENTENCE_TYPE_NMEA:
        processNemaMessage(parse);
        break;

    case SENTENCE_TYPE_RTCM:
        processRtcmMessage(parse);
        break;

    case SENTENCE_TYPE_UBX:
        processUbxMessage(parse);
        break;

    default:
        printf ("Unknown message type: %d\r\n", type);
        break;
    }
}

//Convert nibble to ASCII
uint8_t nibbleToAscii(int nibble)
{
  nibble &= 0xf;
  return (nibble > 9) ? nibble + 'a' - 10 : nibble + '0';
}

//Convert nibble to ASCII
int AsciiToNibble(int data)
{
  //Convert the value to lower case
  data |= 0x20;
  if ((data >= 'a') && (data <= 'f'))
    return data - 'a' + 10;
  if ((data >= '0') && (data <= '9'))
    return data - '0';
  return -1;
}

void dumpBuffer(uint8_t * buffer, uint16_t length)
{
  unsigned int bytes;
  uint8_t * end;
  unsigned int index;

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

//Read the line termination
uint8_t nmeaLineTermination(PARSE_STATE * parse, uint8_t data)
{
  unsigned int checksum;

  //Process the line termination
  if ((data != '\r') && (data != '\n'))
  {
    //Don't include this character in the buffer
    parse->length--;

    //Convert the checksum characters into binary
    checksum = AsciiToNibble(parse->buffer[parse->nmeaLength - 2]) << 4;
    checksum |= AsciiToNibble(parse->buffer[parse->nmeaLength - 1]);

    //Validate the checksum
    if (checksum == parse->crc)
      parse->crc = 0;
    if (parse->crc)
    {
      nmea_checksum_errors += 1;
      dumpBuffer(parse->buffer, parse->length);
      if ((AsciiToNibble(parse->buffer[parse->nmeaLength-2]) >= 0)
        && (AsciiToNibble(parse->buffer[parse->nmeaLength-1]) >= 0))
        printf ("    %s NMEA %s, %2d bytes, bad checksum, expecting 0x%c%c, computed: 0x%02x\r\n",
                       parse->parserName,
                       parse->nmeaMessageName,
                       parse->length,
                       parse->buffer[parse->nmeaLength-2],
                       parse->buffer[parse->nmeaLength-1],
                       parse->crc);
      else
        printf ("    %s NMEA %s, %2d bytes, invalid checksum bytes 0x%02x 0x%02x, computed: 0x%02x\r\n",
                       parse->parserName,
                       parse->nmeaMessageName,
                       parse->length,
                       parse->buffer[parse->nmeaLength-2],
                       parse->buffer[parse->nmeaLength-1],
                       parse->crc);
    }

    //Process this message
    parse->eomCallback(parse, SENTENCE_TYPE_NMEA);

    //Add this character to the beginning of the buffer
    parse->buffer[0] = data;
    parse->length = 1;
    return waitForPreamble(parse, data);
  }
  return SENTENCE_TYPE_NMEA;
}

//Read the linefeed
uint8_t nmeaLinefeed(PARSE_STATE * parse, uint8_t data)
{
  unsigned int checksum;
  uint8_t sentenceType;

  //Convert the checksum characters into binary
  checksum = AsciiToNibble(parse->buffer[parse->nmeaLength - 2]) << 4;
  checksum |= AsciiToNibble(parse->buffer[parse->nmeaLength - 1]);

  //Update the maximum message length
  if (parse->length > parse->maxLength)
  {
    parse->maxLength = parse->length;
    printf("maxLength: %d bytes\r\n", parse->maxLength);
  }

  //Validate the checksum
  if (checksum == parse->crc)
    parse->crc = 0;
  if (parse->crc)
  {
    nmea_checksum_errors += 1;
    dumpBuffer(parse->buffer, parse->length);
    printf ("    %s NMEA %s, %2d bytes, bad checksum, expecting 0x%c%c, computed: 0x%02x\r\n",
                   parse->parserName,
                   parse->nmeaMessageName,
                   parse->length,
                   parse->buffer[parse->nmeaLength-2],
                   parse->buffer[parse->nmeaLength-1],
                   parse->crc);
  }

  //Process this message
  parse->state = waitForPreamble;
  parse->eomCallback(parse, SENTENCE_TYPE_NMEA);

  //Search for another preamble byte
  parse->length = 0;
  parse->crc = 0;
  return SENTENCE_TYPE_NONE;
}

//Read the carriage return
uint8_t nmeaCarriageReturn(PARSE_STATE * parse, uint8_t data)
{
  parse->state = nmeaLinefeed;
  return SENTENCE_TYPE_NMEA;
}

//Read the second checksum byte
uint8_t nmeaChecksumByte2(PARSE_STATE * parse, uint8_t data)
{
  parse->nmeaLength = parse->length;
//  parse->state = nmeaLineTermination;
  parse->state = nmeaCarriageReturn;
  return SENTENCE_TYPE_NMEA;
}

//Read the first checksum byte
uint8_t nmeaChecksumByte1(PARSE_STATE * parse, uint8_t data)
{
  parse->state = nmeaChecksumByte2;
  return SENTENCE_TYPE_NMEA;
}

//Read the message data
uint8_t nmeaFindAsterisk(PARSE_STATE * parse, uint8_t data)
{
  if (data != '*')
    parse->crc ^= data;
  else
    parse->state = nmeaChecksumByte1;
  return SENTENCE_TYPE_NMEA;
}

//Read the message name
uint8_t nmeaFindFirstComma(PARSE_STATE * parse, uint8_t data)
{
  parse->crc ^= data;
  if ((data != ',') || (parse->nmeaMessageNameLength == 0))
  {
    if ((data < 'A') || (data > 'Z'))
    {
      //Display the invalid data
      dumpBuffer(parse->buffer, parse->length - 1);
      printf ("    %s Invalid NMEA data, %d bytes\r\n",
                   parse->parserName, parse->length - 1);

      parse->buffer[0] = data;
      parse->crc = 0;
      parse->length = 1;
      return waitForPreamble (parse, data);
    }

    //Save the message name
    parse->nmeaMessageName[parse->nmeaMessageNameLength++] = data;
  }
  else
  {
    //Zero terminate the message name
    parse->nmeaMessageName[parse->nmeaMessageNameLength++] = 0;
    parse->state = nmeaFindAsterisk;
  }
  return SENTENCE_TYPE_NMEA;
}

//Read the CRC
uint8_t rtcmReadCrc(PARSE_STATE * parse, uint8_t data)
{
  uint16_t dataSent;

  //Account for this data byte
  parse->bytesRemaining -= 1;

  //Wait until all the data is received
  if (parse->bytesRemaining > 0)
    return SENTENCE_TYPE_RTCM;

  //Update the maximum message length
  if (parse->length > parse->maxLength)
  {
    parse->maxLength = parse->length;
    printf("maxLength: %d bytes\r\n", parse->maxLength);
  }

  //Display the RTCM messages with bad CRC
  parse->crc &= 0x00ffffff;
  if (parse->crc)
  {
    rtcm_crc_errors += 1;
    dumpBuffer(parse->buffer, parse->length);
    printf ("    %s RTCM %d, %2d bytes, bad CRC, expecting 0x%02x%02x%02x, computed: 0x%06x\r\n",
                   parse->parserName,
                   parse->message,
                   parse->length,
                   parse->buffer[parse->length-3],
                   parse->buffer[parse->length-2],
                   parse->buffer[parse->length-1],
                   parse->rtcmCrc);
  }

  //Process the message
  parse->state = waitForPreamble;
  parse->eomCallback(parse, SENTENCE_TYPE_RTCM);

  //Search for another preamble byte
  parse->length = 0;
  parse->computeCrc = false;
  parse->crc = 0;
  return SENTENCE_TYPE_NONE;
}

//Read the rest of the message
uint8_t rtcmReadData(PARSE_STATE * parse, uint8_t data)
{
  uint16_t dataSent;

  //Account for this data byte
  parse->bytesRemaining -= 1;

  //Wait until all the data is received
  if (parse->bytesRemaining <= 0)
  {
    parse->rtcmCrc = parse->crc & 0x00ffffff;
    parse->bytesRemaining = 3;
    parse->state = rtcmReadCrc;
  }
  return SENTENCE_TYPE_RTCM;
}

//Read the lower 4 bits of the message number
uint8_t rtcmReadMessage2(PARSE_STATE * parse, uint8_t data)
{
  parse->message |= data >> 4;
  parse->bytesRemaining -= 1;
  parse->state = rtcmReadData;
  return SENTENCE_TYPE_RTCM;
}

//Read the upper 8 bits of the message number
uint8_t rtcmReadMessage1(PARSE_STATE * parse, uint8_t data)
{
  parse->message = data << 4;
  parse->bytesRemaining -= 1;
  parse->state = rtcmReadMessage2;
  return SENTENCE_TYPE_RTCM;
}

//Read the lower 8 bits of the length
uint8_t rtcmReadLength2(PARSE_STATE * parse, uint8_t data)
{
  parse->bytesRemaining |= data;
  parse->state = rtcmReadMessage1;
  return SENTENCE_TYPE_RTCM;
}

//Read the upper two bits of the length
uint8_t rtcmReadLength1(PARSE_STATE * parse, uint8_t data)
{
  //Verify the length byte
  if (data & (~3))
  {
    //Display the invalid data
    dumpBuffer(parse->buffer, parse->length - 1);
    printf ("    %s Invalid RTCM data, %d bytes\r\n", parse->parserName, parse->length - 1);

    //Invalid length, place this byte at the beginning of the buffer
    parse->buffer[0] = data;
    parse->length = 1;
    parse->computeCrc = false;
    parse->crc = 0;

    //Start searching for a preamble byte
    return waitForPreamble(parse, data);
  }

  //Save the upper 2 bits of the length
  parse->bytesRemaining = data << 8;
  parse->state = rtcmReadLength2;
  return SENTENCE_TYPE_RTCM;
}

//Read the CK_B byte
uint8_t ubloxCkB(PARSE_STATE * parse, uint8_t data)
{
  bool badChecksum;

  //Update the maximum message length
  if (parse->length > parse->maxLength)
  {
    parse->maxLength = parse->length;
    printf("maxLength: %d bytes\r\n", parse->maxLength);
  }

  //Validate the checksum
  badChecksum = ((parse->buffer[parse->length - 2] != parse->ck_a)
              || (parse->buffer[parse->length - 1] != parse->ck_b));
  if (badChecksum)
  {
    ubx_checksum_errors += 1;
    dumpBuffer(parse->buffer, parse->length);
    printf ("    %s U-Blox %d.%d, %2d bytes, bad checksum, expecting 0x%02x%02x, computed: 0x%02x%02x\r\n",
                   parse->parserName,
                   parse->message >> 8,
                   parse->message & 0xff,
                   parse->length,
                   parse->buffer[parse->nmeaLength-2],
                   parse->buffer[parse->nmeaLength-1],
                   parse->ck_a,
                   parse->ck_b);
  }

  //Process this message
  parse->state = waitForPreamble;
  parse->eomCallback(parse, SENTENCE_TYPE_UBX);

  //Search for the next preamble byte
  parse->length = 0;
  parse->crc = 0;
  parse->ck_a = 0;
  parse->ck_b = 0;
  return SENTENCE_TYPE_NONE;
}

//Read the CK_A byte
uint8_t ubloxCkA(PARSE_STATE * parse, uint8_t data)
{
  parse->state = ubloxCkB;
  return SENTENCE_TYPE_UBX;
}

//Read the payload
uint8_t ubloxPayload(PARSE_STATE * parse, uint8_t data)
{
  //Compute the checksum over the payload
  if (parse->bytesRemaining--)
  {
    //Calculate the checksum
    parse->ck_a += data;
    parse->ck_b += parse->ck_a;
    return SENTENCE_TYPE_UBX;
  }
  return ubloxCkA(parse, data);
}

//Read the second length byte
uint8_t ubloxLength2(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the second length byte
  parse->bytesRemaining |= ((uint16_t)data) << 8;
  parse->state = ubloxPayload;
  return SENTENCE_TYPE_UBX;
}

//Read the first length byte
uint8_t ubloxLength1(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the first length byte
  parse->bytesRemaining = data;
  parse->state = ubloxLength2;
  return SENTENCE_TYPE_UBX;
}

//Read the ID byte
uint8_t ubloxId(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the ID as the lower 8-bits of the message
  parse->message |= data;
  parse->state = ubloxLength1;
  return SENTENCE_TYPE_UBX;
}

//Read the class byte
uint8_t ubloxClass(PARSE_STATE * parse, uint8_t data)
{
  //Start the checksum calculation
  parse->ck_a = data;
  parse->ck_b = data;

  //Save the class as the upper 8-bits of the message
  parse->message = ((uint16_t)data) << 8;
  parse->state = ubloxId;
  return SENTENCE_TYPE_UBX;
}

//Read the second sync byte
uint8_t ubloxSync2(PARSE_STATE * parse, uint8_t data)
{
  //Verify the sync 2 byte
  if (data != 0x62)
  {
    //Display the invalid data
    dumpBuffer(parse->buffer, parse->length - 1);
    printf ("    %s Invalid UBX data, %d bytes\r\n", parse->parserName, parse->length - 1);

    //Invalid sync 2 byte, place this byte at the beginning of the buffer
    parse->length = 0;
    parse->buffer[parse->length++] = data;

    //Start searching for a preamble byte
    return waitForPreamble(parse, data);
  }

  parse->state = ubloxClass;
  return SENTENCE_TYPE_UBX;
}

//Wait for the preamble byte (0xd3)
uint8_t waitForPreamble(PARSE_STATE * parse, uint8_t data)
{
  //Verify that this is the preamble byte
  offset = file_offset;
  switch(data)
  {
  case '$':

    //
    //    NMEA Message
    //
    //    +----------+---------+--------+---------+----------+----------+
    //    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
    //    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
    //    |     $    |         |    ,   |         |          |          |
    //    +----------+---------+--------+---------+----------+----------+
    //    |                                                  |
    //    |<------------------- Checksum ------------------->|
    //

    parse->crc = 0;
    parse->computeCrc = false;
    parse->nmeaMessageNameLength = 0;
    parse->state = nmeaFindFirstComma;
    return SENTENCE_TYPE_NMEA;

  case 0xb5:

    //
    //    U-BLOX Message
    //
    //    |<-- Preamble --->|
    //    |                 |
    //    +--------+--------+---------+--------+---------+---------+--------+--------+
    //    |  SYNC  |  SYNC  |  Class  |   ID   | Length  | Payload |  CK_A  |  CK_B  |
    //    | 8 bits | 8 bits |  8 bits | 8 bits | 2 bytes | n bytes | 8 bits | 8 bits |
    //    |  0xb5  |  0x62  |         |        |         |         |        |        |
    //    +--------+--------+---------+--------+---------+---------+--------+--------+
    //                      |                                      |
    //                      |<------------- Checksum ------------->|
    //
    //  8-Bit Fletcher Algorithm, which is used in the TCP standard (RFC 1145)
    //  http://www.ietf.org/rfc/rfc1145.txt
    //  Checksum calculation
    //      Initialization: CK_A = CK_B = 0
    //      CK_A += data
    //      CK_B += CK_A
    //

    parse->state = ubloxSync2;
    return SENTENCE_TYPE_UBX;

  case 0xd3:

    //
    //    RTCM Standard 10403.2 - Chapter 4, Transport Layer
    //
    //    |<------------- 3 bytes ------------>|<----- length ----->|<- 3 bytes ->|
    //    |                                    |                    |             |
    //    +----------+--------+----------------+---------+----------+-------------+
    //    | Preamble |  Fill  | Message Length | Message |   Fill   |   CRC-24Q   |
    //    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits |   24 bits   |
    //    |   0xd3   | 000000 |   (in bytes)   |         |   zeros  |             |
    //    +----------+--------+----------------+---------+----------+-------------+
    //    |                                                                       |
    //    |<-------------------------------- CRC -------------------------------->|
    //

    //Start the CRC with this byte
    parse->crc = 0;
    parse->crc = COMPUTE_CRC24Q(parse, data);
    parse->computeCrc = true;

    //Get the message length
    parse->state = rtcmReadLength1;
    return SENTENCE_TYPE_RTCM;
  }

  //preamble byte not found
  dumpBuffer(parse->buffer, parse->length);
  printf ("    %s invalid byte 0x%02x\r\n", parse->parserName, data);
  parse->length = 0;
  parse->state = waitForPreamble;
  return SENTENCE_TYPE_NONE;
}

int
main (
    int argc,
    char ** argv
    )
{
    int bit;
    unsigned char * data;
    unsigned char * data_end;
    int file;
    char * filename;
    off_t file_size;
    int index;
    int length;
    int message_number;
    int message_type;
    static PARSE_STATE parse;
    int total;
    uint8_t value;

    // Initialize the parser
    parse.state = waitForPreamble;
    parse.eomCallback = processMessage;
    parse.parserName = "Tx";

    // Open the log file
    filename = argv[1];
    file = open (filename, O_RDONLY);
    if (file < 0) {
        perror ("ERROR - Failed to open the file");
        return -1;
    }

    // Determine the file length
    file_size = lseek (file, 0, SEEK_END);

    // Get the file buffer
    file_data = malloc (file_size);
    if (!file_data) {
        fprintf (stderr, "ERROR - Failed to allocate file buffer!\n");
        return -2;
    }

    // Read the file into memory
    lseek (file, 0, SEEK_SET);
    if (read (file, file_data, file_size) != file_size) {
        fprintf (stderr, "ERROR - Failed to read the file into memory!\n");
    }

    // Close the file
    close (file);

    // Skip the first byte to force unaligned start
    data = file_data;
    data_end = &data[file_size];
    while (data < data_end) {
        file_offset = data - file_data;

        //Save the data byte
        value = *data;
        parse.buffer[parse.length++] = value;

        //Compute the CRC value for the message
        if (parse.computeCrc)
            parse.crc = COMPUTE_CRC24Q(&parse, value);

        //Parse this message
        parse.state(&parse, value);
        data++;
    }

    // Display the checksum and CRC errors
    if (nmea_checksum_errors)
        printf ("    Total NMEA checksum errors: %d\n", nmea_checksum_errors);
    if (rtcm_crc_errors)
        printf ("    Total RTCM message CRC errors: %d\n", rtcm_crc_errors);
    if (ubx_checksum_errors)
        printf ("    Total UBX message checksum errors: %d\n", ubx_checksum_errors);

    // Display the NMEA message list
    if (DISPLAY_NMEA_MESSAGES) {
        printf ("NMEA Message List:\n");
        NMEA_MESSAGE * message = nmea_list;
        while (message) {
            printf ("    %s: %d %s, max length: %d bytes\n", message->message, message->count,
                    (message->count == 1) ? "time" : "times", message->max_length);
            message = message->next;
        }
    }

    // Display the RTCM message type list
    if (DISPLAY_RTCM_MESSAGE_LIST) {
        printf ("RTCM Message List:\n");
        for (index = 0; index < (int)(sizeof(rtcm_messages) / sizeof(rtcm_messages[0])); index++) {
            if (rtcm_messages[index])
                for (bit = 0; bit < 32; bit++) {
                    message_number = (index << 5) | bit;
                    if (rtcm_messages[index] & (1 << bit))
                        printf ("    %d (%02x %xx): %d %s, max length: %d bytes\n", message_number,
                                message_number >> 4, message_number & 0xf,
                                rtcm_message_count[message_number],
                                (rtcm_message_count[message_number] == 1) ? "time" : "times",
                                rtcm_max_message_length[message_number]);
                }
        }
    }

    // Display the UBX message type list
    if (DISPLAY_UBX_MESSAGE_LIST) {
        printf ("UBX Message List:\n");
        for (index = 0; index < (int)(sizeof(ubx_messages) / sizeof(ubx_messages[0])); index++) {
            if (ubx_messages[index])
                for (bit = 0; bit < 32; bit++) {
                    message_number = (index << 5) | bit;
                    if (ubx_messages[index] & (1 << bit))
                        printf ("    %d.%d (0x%02x.%02x): %d %s, max length: %d bytes\n",
                                message_number >> 8, message_number & 0xff,
                                message_number >> 8, message_number & 0xff,
                                ubx_message_count[message_number],
                                (ubx_message_count[message_number] == 1) ? "time" : "times",
                                ubx_max_message_length[message_number]);
                }
        }
    }

    // Display the bad characters
    if (DISPLAY_BAD_CHARACTERS) {
        printf ("Bad characters:\n");
        total = 0;
        for (index = 0; index < 256; index++) {
            if (bad_characters[index >> 5] & (1 << (index & 0x1f))) {
                printf ("    0x%02x: %d\n", index, bad_character_count[index]);
                total += bad_character_count[index];
            }
        }
        printf ("    Total: %d\n", total);
    }

    // Display the bad character offsets
    if (DISPLAY_BAD_CHARACTER_OFFSETS) {
        printf ("Bad character offsets:\n");
        total = 0;
        for (index = 0; index <= bad_character_offset_count; index++) {
            printf ("    0x%08x: %d bytes\n", bad_character_offset[index], bad_character_length[index]);
            total += bad_character_length[index];
        }
        printf ("    Total: %d\n", total);
    }
}
