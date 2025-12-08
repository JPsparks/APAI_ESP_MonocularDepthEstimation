#ifndef __PW_APAI_F_MAN_H
#define __PW_APAI_F_MAN_H

#include "../../utility/logger.h"
#include "../../config.h"

#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"

#define SD_MMC_CMD  38 //Please do not modify it.
#define SD_MMC_CLK  39 //Please do not modify it. 
#define SD_MMC_D0   40 //Please do not modify it.

#define FILEMAN_LOG_PERMISSION FILEMAN_LOG_CONFIG_PERMISSION

// Note: this class is at the moment a copy-paste of the FreeNove tutorial

class FileManager {
  public:
    FileManager();
    bool sdmmcInit(void); 

    void listDir(const char * dirname, uint8_t levels); //mainly a dev function

    bool createDir(const char * path);
    bool removeDir(const char * path);

    void readFile(const char * path); //for now this function is simply copied by provided tutorial (see also Freenove tutorials)
    
    bool writeFile(const char * path, const char * message);
    bool appendFile(const char * path, const char * message);
    bool renameFile(const char * path1, const char * path2);
    bool deleteFile(const char * path);
    
    void testFileIO(const char * path); //another dev function

    bool writejpg(const char * path, const uint8_t *buf, size_t size);
    int readFileNum(const char * dirname);
    bool writebmp(const char* path, uint8_t* rgb_data, size_t width, size_t height, int channels);

  private:
    fs::SDMMCFS &getFS() {
        return SD_MMC; 
    }
};


#endif 