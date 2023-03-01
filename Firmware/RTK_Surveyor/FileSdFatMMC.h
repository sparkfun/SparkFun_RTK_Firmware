// Define a hybrid class which can support both SdFat SdFile and SD_MMC File

#ifdef COMPILE_SD_MMC

#include "FS.h"
#include "SD_MMC.h"

class FileSdFatMMC : public SdFile, public File

#else

class FileSdFatMMC : public SdFile

#endif

{
  public:

    FileSdFatMMC()
    {
      if (USE_SPI_MICROSD)
        _sdFile = new SdFile;
#ifdef COMPILE_SD_MMC
      else
        _file = new File;
#endif
    };

    ~FileSdFatMMC()
    {
      if (USE_SPI_MICROSD)
      {
        ;
        //if (_sdFile) // operator bool
        //  delete _sdFile;
      }
#ifdef COMPILE_SD_MMC
      else
      {
        ;
        //if (_file) // operator bool
        //  delete _file;
      }
#endif
    };

    operator bool()
    {
      if (USE_SPI_MICROSD)
        return _sdFile;
#ifdef COMPILE_SD_MMC
      else
        return _file;
#endif
    };

    size_t println(const char printMe[])
    {
      if (USE_SPI_MICROSD)
        return _sdFile->println(printMe);
#ifdef COMPILE_SD_MMC
      else
        return _file->println(printMe);
#endif
    };

    bool open(const char *filepath, oflag_t mode)
    {
      if (USE_SPI_MICROSD)
      {
        if (_sdFile->open(filepath, mode) == true)
          return true;
        return false;
      }
#ifdef COMPILE_SD_MMC
      else
      {
        if (mode & O_APPEND)
          *_file = SD_MMC.open(filepath, FILE_APPEND);
        else if (mode & O_WRITE)
          *_file = SD_MMC.open(filepath, FILE_WRITE);
        else // if (mode & O_READ)
          *_file = SD_MMC.open(filepath, FILE_READ);
        if (_file) // operator bool
          return true;
        return false;
      }
#endif
    };

    uint32_t size()
    {
      if (USE_SPI_MICROSD)
        return _sdFile->size();
#ifdef COMPILE_SD_MMC
      else
        return _file->size();
#endif
    };

    uint32_t position()
    {
      if (USE_SPI_MICROSD)
        return _sdFile->position();
#ifdef COMPILE_SD_MMC
      else
        return _file->position();
#endif
    };

    int available()
    {
      if (USE_SPI_MICROSD)
        return _sdFile->available();
#ifdef COMPILE_SD_MMC
      else
        return _file->available();
#endif
    };

    int read(uint8_t *buf, uint16_t nbyte)
    {
      if (USE_SPI_MICROSD)
        return _sdFile->read(buf, nbyte);
#ifdef COMPILE_SD_MMC
      else
        return _file->read(buf, nbyte);
#endif
    };

    size_t write(const uint8_t *buf, size_t size)
    {
      if (USE_SPI_MICROSD)
        return _sdFile->write(buf, size);
#ifdef COMPILE_SD_MMC
      else
        return _file->write(buf, size);
#endif
    };

    void close()
    {
      if (USE_SPI_MICROSD)
        _sdFile->close();
#ifdef COMPILE_SD_MMC
      else
        _file->close();
#endif
    };

    void updateFileAccessTimestamp()
    {
      if (USE_SPI_MICROSD)
      {
        if (online.rtc == true)
        {
          //ESP32Time returns month:0-11
          _sdFile->timestamp(T_ACCESS, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
          _sdFile->timestamp(T_WRITE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
        }
      }
    };

    void updateFileCreateTimestamp()
    {
      if (USE_SPI_MICROSD)
      {
        if (online.rtc == true)
        {
          _sdFile->timestamp(T_CREATE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond()); //ESP32Time returns month:0-11
        }
      }
    };

    void sync()
    {
      if (USE_SPI_MICROSD)
        _sdFile->sync();
    };

    int fileSize()
    {
      if (USE_SPI_MICROSD)
        return _sdFile->fileSize();
#ifdef COMPILE_SD_MMC
      else
        return _file->size();
#endif
    };

  protected:
    SdFile * _sdFile;
#ifdef COMPILE_SD_MMC
    File * _file;
#endif
};

//Update the file access and write time with date and time obtained from GNSS
// These are SdFile-specific. SD_MMC does this automatically
void updateDataFileAccess(SdFile *dataFile)
{
  if (online.rtc == true)
  {
    //ESP32Time returns month:0-11
    dataFile->timestamp(T_ACCESS, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
    dataFile->timestamp(T_WRITE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
  }
}

//Update the file create time with date and time obtained from GNSS
void updateDataFileCreate(SdFile *dataFile)
{
  if (online.rtc == true)
    dataFile->timestamp(T_CREATE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond()); //ESP32Time returns month:0-11
}
