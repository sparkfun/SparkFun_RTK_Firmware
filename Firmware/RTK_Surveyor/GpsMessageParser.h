/*------------------------------------------------------------------------------
GpsMessageParser.h

  Constant and routine declarations for the GPS message parser.
------------------------------------------------------------------------------*/

#ifndef __GPS_MESSAGE_PARSER_H__
#define __GPS_MESSAGE_PARSER_H__

#include <stdint.h>

#include "crc24q.h" // 24-bit CRC-24Q cyclic redundancy checksum for RTCM parsing

//----------------------------------------
// Constants
//----------------------------------------

#define PARSE_BUFFER_LENGTH 3000 // Some USB RAWX messages can be > 2k

enum
{
    SENTENCE_TYPE_NONE = 0,

    // Add new sentence types below in alphabetical order
    SENTENCE_TYPE_NMEA,
    SENTENCE_TYPE_RTCM,
    SENTENCE_TYPE_UBX,
};

//----------------------------------------
// Types
//----------------------------------------

typedef struct _PARSE_STATE *P_PARSE_STATE;

// Parse routine
typedef uint8_t (*PARSE_ROUTINE)(P_PARSE_STATE parse, // Parser state
                                 uint8_t data);       // Incoming data byte

// End of message callback routine
typedef void (*PARSE_EOM_CALLBACK)(P_PARSE_STATE parse, // Parser state
                             uint8_t type);       // Message type

typedef struct _PARSE_STATE
{
    PARSE_ROUTINE state;                 // Parser state routine
    PARSE_EOM_CALLBACK eomCallback;      // End of message callback routine
    const char *parserName;              // Name of parser
    uint32_t crc;                        // RTCM computed CRC
    uint32_t rtcmCrc;                    // Computed CRC value for the RTCM message
    uint32_t invalidRtcmCrcs;            // Number of bad RTCM CRCs detected
    uint16_t bytesRemaining;             // Bytes remaining in RTCM CRC calculation
    uint16_t length;                     // Message length including line termination
    uint16_t maxLength;                  // Maximum message length including line termination
    uint16_t message;                    // RTCM message number
    uint16_t nmeaLength;                 // Length of the NMEA message without line termination
    uint8_t buffer[PARSE_BUFFER_LENGTH]; // Buffer containing the message
    uint8_t nmeaMessageName[16];         // Message name
    uint8_t nmeaMessageNameLength;       // Length of the message name
    uint8_t ck_a;                        // U-blox checksum byte 1
    uint8_t ck_b;                        // U-blox checksum byte 2
    bool computeCrc;                     // Compute the CRC when true
} PARSE_STATE;

//----------------------------------------
// Macros
//----------------------------------------

#ifdef  PARSE_NMEA_MESSAGES
#define NMEA_PREAMBLE       nmeaPreamble,
#else
#define NMEA_PREAMBLE
#endif  // PARSE_NMEA_MESSAGES

#ifdef  PARSE_RTCM_MESSAGES
#define RTCM_PREAMBLE       rtcmPreamble,
#else
#define RTCM_PREAMBLE
#endif  // PARSE_RTCM_MESSAGES

#ifdef  PARSE_UBLOX_MESSAGES
#define UBLOX_PREAMBLE      ubloxPreamble,
#else
#define UBLOX_PREAMBLE
#endif  // PARSE_UBLOX_MESSAGES

#define GPS_PARSE_TABLE                 \
PARSE_ROUTINE const gpsParseTable[] =   \
{                                       \
    NMEA_PREAMBLE                       \
    RTCM_PREAMBLE                       \
    UBLOX_PREAMBLE                      \
};                                      \
                                        \
const int gpsParseTableEntries = sizeof(gpsParseTable) / sizeof(gpsParseTable[0]);

//----------------------------------------
// External values
//----------------------------------------

extern PARSE_ROUTINE const gpsParseTable[];
extern const int gpsParseTableEntries;

//----------------------------------------
// External routines
//----------------------------------------

// Main parser routine
uint8_t gpsMessageParserFirstByte(PARSE_STATE *parse, uint8_t data);

// NMEA parse routines
uint8_t nmeaPreamble(PARSE_STATE *parse, uint8_t data);
uint8_t nmeaFindFirstComma(PARSE_STATE *parse, uint8_t data);
uint8_t nmeaFindAsterisk(PARSE_STATE *parse, uint8_t data);
uint8_t nmeaChecksumByte1(PARSE_STATE *parse, uint8_t data);
uint8_t nmeaChecksumByte2(PARSE_STATE *parse, uint8_t data);
uint8_t nmeaLineTermination(PARSE_STATE *parse, uint8_t data);

// RTCM parse routines
uint8_t rtcmPreamble(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadLength1(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadLength2(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadMessage1(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadMessage2(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadData(PARSE_STATE *parse, uint8_t data);
uint8_t rtcmReadCrc(PARSE_STATE *parse, uint8_t data);

// u-blox parse routines
uint8_t ubloxPreamble(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxSync2(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxClass(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxId(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxLength1(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxLength2(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxPayload(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxCkA(PARSE_STATE *parse, uint8_t data);
uint8_t ubloxCkB(PARSE_STATE *parse, uint8_t data);

// External print routines
void printNmeaChecksumError(PARSE_STATE *parse);
void printRtcmChecksumError(PARSE_STATE *parse);
void printRtcmMaxLength(PARSE_STATE *parse);
void printUbloxChecksumError(PARSE_STATE *parse);
void printUbloxInvalidData(PARSE_STATE *parse);

#endif  // __GPS_MESSAGE_PARSER_H__
