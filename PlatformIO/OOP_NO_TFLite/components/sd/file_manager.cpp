#include "file_manager.h"

FileManager::FileManager(){

}

bool FileManager::sdmmcInit(void){
    fs::SDMMCFS &fs = this->getFS();
    
    fs.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    if (!fs.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
        Serial.println("Card Mount Failed");
        return false;
    }
    
    uint8_t cardType = fs.cardType();
    
    if(cardType == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return false;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = fs.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);  
    Serial.printf("Total space: %lluMB\r\n", fs.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\r\n", fs.usedBytes() / (1024 * 1024));
    return true;
}


void FileManager::listDir(const char * dirname, uint8_t levels){
    
    fs::FS &fs = this->getFS();

    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels) {
                this->listDir(file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool FileManager::createDir(const char * path){
    // fs::FS fs = this->disk_interface; 
    fs::FS &fs = this->getFS();

    Serial.printf("Creating Dir: %s\n", path);
    bool success = fs.mkdir(path);
    if(success){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
    return success;
}

bool FileManager::removeDir(const char * path){
    // fs::FS fs = this->disk_interface;
    fs::FS &fs = this->getFS(); 

    Serial.printf("Removing Dir: %s\n", path);
    bool success = fs.mkdir(path);

    if(success){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
    return success;
}

void FileManager::readFile(const char * path){
    // fs::FS fs = this->disk_interface; 
    fs::FS &fs = this->getFS();

    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
}

bool FileManager::writeFile(const char * path, const char * message){
    fs::FS &fs = this->getFS();

    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return false;
    }
    bool success = file.print(message);
    if(success){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    return success;
}

bool FileManager::appendFile(const char * path, const char * message){
    fs::FS &fs = this->getFS();
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    bool success = file.print(message);
    if(success){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    return success;
}

bool FileManager::renameFile(const char * path1, const char * path2){
    fs::FS &fs = this->getFS();

    Serial.printf("Renaming file %s to %s\n", path1, path2);
    bool success = fs.rename(path1, path2);
    if (success) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
    return success;
}

bool FileManager::deleteFile(const char * path){
    fs::FS &fs = this->getFS();

    Serial.printf("Deleting file: %s\n", path);
    
    bool success = fs.remove(path);

    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void FileManager::testFileIO(const char * path){
    // fs::FS fs = this->disk_interface; 
    fs::FS &fs = this->getFS();

    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

bool FileManager::writejpg(const char * path, const uint8_t *buf, size_t size){
    fs::FS &fs = this->getFS();

    File file = fs.open(path, FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file for writing");
      return false;
    }
    file.write(buf, size);
    Serial.printf("Saved file to path: %s\r\n", path);
    return true;
}

int FileManager::readFileNum(const char * dirname){
    fs::FS &fs = this->getFS();

    File root = fs.open(dirname);
    if(!root) {
        Serial.println("Failed to open directory");
        return -1;
    }

    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return -1;
    }

    File file = root.openNextFile();
    int num=0;
    
    while(file){
      file = root.openNextFile();
      num++;
    }
    return num;  
}


bool FileManager::writebmp(const char* path, uint8_t* rgb_data, size_t width, size_t height){

    fs::FS &fs = this->getFS();

    File file = fs.open(path, FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file for writing");
      return false;
    }

    // Header BMP (14 byte) + Info header (40 byte)
    const uint32_t header_size = 54;
    const uint32_t row_size = ((width * 3 + 3) / 4) * 4; // Padding a 4 byte
    const uint32_t pixel_data_size = row_size * height;
    const uint32_t file_size = header_size + pixel_data_size;
    
    Serial.printf("BMP size: %u bytes (header=%u, pixels=%u)\n", 
                  file_size, header_size, pixel_data_size);
    
    // Verifica memoria disponibile
    if (ESP.getFreeHeap() < file_size + 10000) {
        Serial.printf("Memoria insufficiente! Disponibile: %d, richiesto: %u\n",
                     ESP.getFreeHeap(), file_size);
        file.close();
        return false;
    }
    
    uint8_t* bmp_buf = (uint8_t*)malloc(file_size);
    if (!bmp_buf) {
        Serial.println("Errore allocazione buffer BMP");
        file.close();
        return false;
    }
    
    memset(bmp_buf, 0, file_size);
    
    // BMP File Header (14 byte)
    bmp_buf[0] = 'B';
    bmp_buf[1] = 'M';
    *(uint32_t*)&bmp_buf[2] = file_size;        // File size
    *(uint32_t*)&bmp_buf[10] = header_size;     // Offset to pixel data
    
    // BMP Info Header (40 byte)
    *(uint32_t*)&bmp_buf[14] = 40;              // Info header size
    *(uint32_t*)&bmp_buf[18] = width;           // Width
    *(uint32_t*)&bmp_buf[22] = height;          // Height
    *(uint16_t*)&bmp_buf[26] = 1;               // Planes
    *(uint16_t*)&bmp_buf[28] = 24;              // Bits per pixel (RGB888)
    *(uint32_t*)&bmp_buf[30] = 0;               // Compression (0 = none)
    *(uint32_t*)&bmp_buf[34] = pixel_data_size; // Image size
    
    // Copia pixel data (BMP è BGR, bottom-to-top)
    for (size_t y = 0; y < height; y++) {
        size_t src_row = height - 1 - y;  // BMP è bottom-to-top
        size_t src_offset = src_row * width * 3;
        size_t dst_offset = header_size + y * row_size;
        
        for (size_t x = 0; x < width; x++) {
            // RGB → BGR
            bmp_buf[dst_offset + x * 3 + 0] = rgb_data[src_offset + x * 3 + 2]; // B
            bmp_buf[dst_offset + x * 3 + 1] = rgb_data[src_offset + x * 3 + 1]; // G
            bmp_buf[dst_offset + x * 3 + 2] = rgb_data[src_offset + x * 3 + 0]; // R
        }
    }
    
    // // Salva file
    // FILE* f = fopen(path, "wb");
    // if (!f) {
    //     Serial.printf("Errore apertura file: %s\n", path);
    //     free(bmp_buf);
    //     return false;
    // }
    
    // ✅ Scrivi dati binari con write(), NON print()
    size_t written = file.write(bmp_buf, file_size);
    
    file.close();
    
    bool success = (written == file_size);
    
    if (success) {
        Serial.printf("✓ BMP scritto: %zu byte\n", written);
    } else {
        Serial.printf("✗ Errore scrittura: scritti %zu byte su %u\n", written, file_size);
    }
    
    free(bmp_buf);
    
    return success;
}