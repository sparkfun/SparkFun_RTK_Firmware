#ifdef COMPILE_ETHERNET

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Extra code for the W5500 (vs. w5100.h)

const uint16_t w5500RTR = 0x0019;   // Retry Count Register - Common Register Block
const uint16_t w5500SIR = 0x0017;   // Socket Interrupt Register - Common Register Block
const uint16_t w5500SIMR = 0x0018;  // Socket Interrupt Mask Register - Common Register Block
const uint16_t w5500SnIR = 0x0002;  // Socket n Interrupt Register - Socket Register Block
const uint16_t w5500SnIMR = 0x002C; // Socket n Interrupt Mask Register - Socket Register Block

const uint8_t w5500CommonRegister = 0x00 << 3;  // Block Select
const uint8_t w5500Socket0Register = 0x01 << 3; // Block Select
const uint8_t w5500Socket1Register = 0x05 << 3; // Block Select
const uint8_t w5500Socket2Register = 0x09 << 3; // Block Select
const uint8_t w5500Socket3Register = 0x0D << 3; // Block Select
const uint8_t w5500Socket4Register = 0x11 << 3; // Block Select
const uint8_t w5500Socket5Register = 0x15 << 3; // Block Select
const uint8_t w5500Socket6Register = 0x19 << 3; // Block Select
const uint8_t w5500Socket7Register = 0x1D << 3; // Block Select
const uint8_t w5500RegisterWrite = 0x01 << 2;   // Read/Write bit
const uint8_t w5500VDM = 0x00 << 0;             // Variable Data Length Mode
const uint8_t w5500FDM1 = 0x01 << 0;            // Fixed Data Length Mode 1 Byte
const uint8_t w5500FDM2 = 0x02 << 0;            // Fixed Data Length Mode 2 Byte
const uint8_t w5500FDM4 = 0x03 << 0;            // Fixed Data Length Mode 4 Byte

const uint8_t w5500SocketRegisters[] = {w5500Socket0Register, w5500Socket1Register, w5500Socket2Register,
                                        w5500Socket3Register, w5500Socket4Register, w5500Socket5Register,
                                        w5500Socket6Register, w5500Socket7Register};

const uint8_t w5500SIR_ClearAll = 0xFF;
const uint8_t w5500SIMR_EnableAll = 0xFF;

const uint8_t w5500SnIR_ClearAll = 0xFF;

const uint8_t w5500SnIMR_CON = 0x01 << 0;
const uint8_t w5500SnIMR_DISCON = 0x01 << 1;
const uint8_t w5500SnIMR_RECV = 0x01 << 2;
const uint8_t w5500SnIMR_TIMEOUT = 0x01 << 3;
const uint8_t w5500SnIMR_SENDOK = 0x01 << 4;

void w5500write(SPIClass &spiPort, const int cs, uint16_t address, uint8_t control, uint8_t *data, uint8_t len)
{
    // Apply settings
    spiPort.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

    // Signal communication start
    digitalWrite(cs, LOW);

    spiPort.transfer(address >> 8); // Address Phase
    spiPort.transfer(address & 0xFF);

    spiPort.transfer(control | w5500RegisterWrite | w5500VDM); // Control Phase

    for (uint8_t i = 0; i < len; i++)
    {
        spiPort.transfer(*data++); // Data Phase
    }

    // End communication
    digitalWrite(cs, HIGH);
    spiPort.endTransaction();
}

void w5500read(SPIClass &spiPort, const int cs, uint16_t address, uint8_t control, uint8_t *data, uint8_t len)
{
    // Apply settings
    spiPort.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

    // Signal communication start
    digitalWrite(cs, LOW);

    spiPort.transfer(address >> 8); // Address Phase
    spiPort.transfer(address & 0xFF);

    spiPort.transfer(control | w5500VDM); // Control Phase

    for (uint8_t i = 0; i < len; i++)
    {
        *data++ = spiPort.transfer(0x00); // Data Phase
    }

    // End communication
    digitalWrite(cs, HIGH);
    spiPort.endTransaction();
}

void w5500ClearSocketInterrupts()
{
    // Clear all W5500 socket interrupts.
    for (uint8_t i = 0; i < (sizeof(w5500SocketRegisters) / sizeof(uint8_t)); i++)
    {
        w5500write(SPI, pin_Ethernet_CS, w5500SnIR, w5500SocketRegisters[i], (uint8_t *)&w5500SnIR_ClearAll, 1);
    }
    w5500write(SPI, pin_Ethernet_CS, w5500SIR, w5500CommonRegister, (uint8_t *)&w5500SIR_ClearAll, 1);
}

void w5500ClearSocketInterrupt(uint8_t sockIndex)
{
    // Clear the interrupt for sockIndex only

    // Sn_IR indicates the status of Socket Interrupt such as establishment, termination,
    // receiving data, timeout). When an interrupt occurs and the corresponding bit of
    // Sn_IMR is ‘1’, the corresponding bit of Sn_IR becomes ‘1’.
    // In order to clear the Sn_IR bit, the host should write the bit to ‘1’.
    w5500write(SPI, pin_Ethernet_CS, w5500SnIR, w5500SocketRegisters[sockIndex], (uint8_t *)&w5500SnIR_ClearAll, 1);

    // SIR indicates the interrupt status of Socket. Each bit of SIR be still ‘1’ until Sn_IR is
    // cleared by the host. If Sn_IR is not equal to ‘0x00’, the n-th bit of SIR is ‘1’ and INTn
    // PIN is asserted until SIR is ‘0x00’.
    uint8_t SIR = 1 << sockIndex;
    w5500write(SPI, pin_Ethernet_CS, w5500SIR, w5500CommonRegister, &SIR, 1);
}

bool w5500CheckSocketInterrupt(uint8_t sockIndex)
{
    // Check the interrupt for sockIndex only
    uint8_t S_INT = 1 << sockIndex;
    uint8_t SIR;
    w5500read(SPI, pin_Ethernet_CS, w5500SIR, w5500CommonRegister, &SIR, 1);
    return ((S_INT & SIR) > 0);
}

void w5500EnableSocketInterrupts()
{
    // Enable the RECV interrupt on all eight sockets
    for (uint8_t i = 0; i < (sizeof(w5500SocketRegisters) / sizeof(uint8_t)); i++)
    {
        w5500write(SPI, pin_Ethernet_CS, w5500SnIMR, w5500SocketRegisters[i], (uint8_t *)&w5500SnIMR_RECV, 1);
    }

    w5500write(SPI, pin_Ethernet_CS, w5500SIMR, w5500CommonRegister, (uint8_t *)&w5500SIMR_EnableAll,
               1); // Enable the socket interrupt on all eight sockets
}

void w5500EnableSocketInterrupt(uint8_t sockIndex)
{
    w5500write(SPI, pin_Ethernet_CS, w5500SnIMR, w5500SocketRegisters[sockIndex], (uint8_t *)&w5500SnIMR_RECV,
               1); // Enable the RECV interrupt for sockIndex only

    // Read-Modify-Write
    uint8_t SIMR;
    w5500read(SPI, pin_Ethernet_CS, w5500SIMR, w5500CommonRegister, &SIMR, 1);
    SIMR |= 1 << sockIndex;
    w5500write(SPI, pin_Ethernet_CS, w5500SIMR, w5500CommonRegister, &SIMR, 1); // Enable the socket interrupt
}

void w5500DisableSocketInterrupt(uint8_t sockIndex)
{
    w5500write(SPI, pin_Ethernet_CS, w5500SnIMR, w5500SocketRegisters[sockIndex], (uint8_t *)&w5500SnIMR_RECV,
               1); // Enable the RECV interrupt for sockIndex only

    // Read-Modify-Write
    uint8_t SIMR;
    w5500read(SPI, pin_Ethernet_CS, w5500SIMR, w5500CommonRegister, &SIMR, 1);
    SIMR &= ~(1 << sockIndex);
    w5500write(SPI, pin_Ethernet_CS, w5500SIMR, w5500CommonRegister, &SIMR, 1); // Disable the socket interrupt
}

#endif  // COMPILE_ETHERNET
