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
      sdFile = new SdFile;
#ifdef COMPILE_SD_MMC
    else
      file = new File;
#endif
  };
  
  ~FileSdFatMMC()
  {
    if (USE_SPI_MICROSD)
      if (sdFile)
        delete sdFile;
#ifdef COMPILE_SD_MMC
    else
      if (file)
        delete file;
#endif
  };

  size_t println(const char printMe[])
  {
    if (USE_SPI_MICROSD)
      return sdFile->println(printMe);
#ifdef COMPILE_SD_MMC
    else
      return file->println(printMe);
#endif
  };

  File open(const char *filepath, uint8_t mode)
  {
    if (USE_SPI_MICROSD)
      return (File)sdFile->open(filepath, mode);
#ifdef COMPILE_SD_MMC
    else
    {
      if (mode | O_APPEND)
        return file->open(filepath, FILE_APPEND);
      else if (mode | O_WRITE)
        return file->open(filepath, FILE_WRITE);
      else // if (mode | O_READ)
        return file->open(filepath, FILE_READ);
    }
#endif    
  };

protected:
  SdFile * sdFile;
#ifdef COMPILE_SD_MMC
  File * file;
#endif
};
