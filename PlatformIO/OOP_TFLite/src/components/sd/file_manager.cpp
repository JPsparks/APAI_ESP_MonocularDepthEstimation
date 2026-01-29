#include "file_manager.h"

FileManager::FileManager() {
}

bool FileManager::sdmmcInit(void){
    fs::SDMMCFS &fs = this->getFS();
    
    fs.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    if (!fs.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
        log("Card Mount Failed", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }
    
    uint8_t cardType = fs.cardType();
    
    if(cardType == CARD_NONE){
        log("No SD_MMC card attached", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }

    // Serial.print("SD_MMC Card Type: ");
    default_log("SD_MMC card type: ", FILEMAN_LOG_PERMISSION);
    if (cardType == CARD_MMC) {
        default_log("MMC", FILEMAN_LOG_PERMISSION);
    } else if(cardType == CARD_SD) {
        default_log("SDSC", FILEMAN_LOG_PERMISSION);
    } else if(cardType == CARD_SDHC){
        default_log("SDHC", FILEMAN_LOG_PERMISSION);
    } else {
        default_log("UNKNOWN", FILEMAN_LOG_PERMISSION);
    }
    
    uint64_t cardSize = fs.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);  
    Serial.printf("Total space: %lluMB\r\n", fs.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\r\n", fs.usedBytes() / (1024 * 1024));
    return true;
}

bool FileManager::createDir(const char * path){
    fs::FS &fs = this->getFS();

    default_log("Creating dir: " + String(path), FILEMAN_LOG_PERMISSION);
    bool success = fs.mkdir(path);
    if (success) { 
        default_log("Dir created", FILEMAN_LOG_PERMISSION);
    } else {
        log("mkdir failed", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}




bool FileManager::recursiveRemoveDir(const char * path){
    fs::FS &fs = this->getFS(); 
    default_log("Start recursive cleanup: " + String(path), FILEMAN_LOG_PERMISSION);

    File dir = fs.open(path);
    
    // Controlli di sicurezza
    if(!dir){
        log("Failed to open directory", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }
    if(!dir.isDirectory()){
        log("Path is not a directory", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }

    File file = dir.openNextFile();
    while(file){
        // Costruzione percorso sicuro
        String filePath = String(path);
        if (!filePath.endsWith("/")) filePath += "/";
        filePath += String(file.name());

        if(file.isDirectory()){
            // RICORSIONE: Svuota e rimuove la sottocartella
            if(!recursiveRemoveDir(filePath.c_str())){
                 log("Failed to remove subdir: " + filePath, WARNING, FILEMAN_LOG_PERMISSION);
            }
        } else {
            // FILE: Lo cancella direttamente
            if(!fs.remove(filePath.c_str())){
                 log("Failed to remove file: " + filePath, WARNING, FILEMAN_LOG_PERMISSION);
            }
        }
        file = dir.openNextFile();
    }
    
    // CRITICO: Chiudere la directory prima di provare a rimuoverla!
    // Senza questo, plainRemoveDir fallirà perché la risorsa è "busy".
    dir.close();

    // Ora che è vuota, usiamo la funzione plain per finire il lavoro
    return this->plainRemoveDir(path);
}

bool FileManager::plainRemoveDir(const char * path){
    fs::FS &fs = this->getFS(); 

    default_log("Removing dir (plain): " + String(path), FILEMAN_LOG_PERMISSION);
    
    // CORREZIONE: Usiamo rmdir, non mkdir!
    bool success = fs.rmdir(path);

    if(success){
        default_log("Dir removed", FILEMAN_LOG_PERMISSION);
    } else {
        log("rmdir failed (directory not empty?)", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}

bool FileManager::removeDir(const char * path, bool inRecursiveWay){
    if (inRecursiveWay){
        // delete dir and all its content
        return this->recursiveRemoveDir(path);
    } else {
        // delete only dir ! and only if it is empty
        return this->plainRemoveDir(path);
    }
}






void FileManager::readFile(const char * path){
    // fs::FS fs = this->disk_interface; 
    fs::FS &fs = this->getFS();

    default_log("Reading file: " + String(path), FILEMAN_LOG_PERMISSION);

    File file = fs.open(path);
    if(!file){
        log("Failed to open file for reading", ERROR, FILEMAN_LOG_PERMISSION);
        return;
    }

    default_log("Read from file: " + String(path), FILEMAN_LOG_PERMISSION);
    while(file.available()){
        Serial.write(file.read());
    }
}

bool FileManager::writeFile(const char * path, const char * message){
    fs::FS &fs = this->getFS();

    default_log("Writing file: " + String(path), FILEMAN_LOG_PERMISSION);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        log("Failed to open file for writing", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }

    bool success = file.print(message);
    
    if (success) {
        default_log("File written", FILEMAN_LOG_PERMISSION);
    } else {
        log("Write failed", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}

bool FileManager::appendFile(const char * path, const char * message){
    fs::FS &fs = this->getFS();
    default_log("Appending to file: " + String(path), FILEMAN_LOG_PERMISSION);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return false;
    }

    bool success = file.print(message);

    if(success){
        default_log("Message appended", FILEMAN_LOG_PERMISSION);

    } else {
        log("Append failed", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}

bool FileManager::renameFile(const char * path1, const char * path2){
    fs::FS &fs = this->getFS();

    default_log("Renaming file " + String(path1) + " to " + String(path2), FILEMAN_LOG_PERMISSION);

    bool success = fs.rename(path1, path2);
    if (success) {
        default_log("File renamed", FILEMAN_LOG_PERMISSION);
    } else {
        log("Rename failed", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}

bool FileManager::deleteFile(const char * path){
    fs::FS &fs = this->getFS();
    
    default_log("Deleting file " + String(path), FILEMAN_LOG_PERMISSION);
    
    bool success = fs.remove(path);

    if(fs.remove(path)){
        default_log("File deleted", FILEMAN_LOG_PERMISSION);
    } else {
        log("Delete failed", ERROR, FILEMAN_LOG_PERMISSION);
    }
    return success;
}

void FileManager::testFileIO(const char * path){
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


File FileManager::getRootPointer(const char * dirname){
    fs::FS &fs = this->getFS();

    File root = fs.open(dirname);
    if(!root) {
        log("Failed to open directory", ERROR, FILEMAN_LOG_PERMISSION);
        return File();
    }

    if(!root.isDirectory()){
        log("Not a directory", WARNING, FILEMAN_LOG_PERMISSION);
        return File();
    }

    return root;
}

void FileManager::listDir(const char * dirname, uint8_t levels){
    
    Serial.printf("Listing directory: %s\n", dirname);

    File root = this->getRootPointer(dirname);
    File file = root.openNextFile();

    while(file){
        if(file.isDirectory()){
            default_log("  DIR : " + String(file.name()), true);
            // default_log(, true);
            if(levels) {
                this->listDir(file.path(), levels -1);
            }
        } else {
            default_log("  FILE: " + String(file.name()), true);
            // default_log(, true);
            default_log("  SIZE: " + String(file.size()), true);
            default_log("", true);
            // default_log(, true);
        }
        file = root.openNextFile();
    }
}

int FileManager::readFileNum(const char * dirname){
    
    File root = this->getRootPointer(dirname);
    File file = root.openNextFile();
    int num=0;
    
    while (file) {
      file = root.openNextFile();
      num++;
    }
    return num;  
}


bool FileManager::writejpg(const char * path, const uint8_t *buf, size_t size){
    fs::FS &fs = this->getFS();

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      log("Failed to open file for writing", ERROR, FILEMAN_LOG_PERMISSION);
      return false;
    }
    file.write(buf, size);
    log("Saved file to path: " + String(path) + "\r", INFO, FILEMAN_LOG_PERMISSION);
    Serial.printf("\n", path);
    return true;
}


bool FileManager::writebmp(const char* path, uint8_t* rgb_data, size_t width, size_t height, int channels){
    fs::FS &fs = this->getFS();
    
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        log("Failed to open file for writing", ERROR, FILEMAN_LOG_PERMISSION);
        return false;
    }

    // Compute dims (BMP 24-bit standard)
    // Each row in BMP must be multiply of 4 byte (padding)
    // For compatibility, even if is 1 channel, store it as 24-bit
    const int bytesPerPixel = 3; 
    const uint32_t row_size_padded = ((width * bytesPerPixel + 3) / 4) * 4;
    const uint32_t pixel_data_size = row_size_padded * height;
    const uint32_t header_size = 54;
    const uint32_t file_size = header_size + pixel_data_size;
    const uint32_t padding = row_size_padded - (width * bytesPerPixel);

    uint8_t bmp_header[54] = {0};
    
    // BMP Signature
    bmp_header[0] = 'B'; 
    bmp_header[1] = 'M';
    
    // File Size
    memcpy(&bmp_header[2], &file_size, 4);
    
    // Offset to data
    uint32_t offset = 54;
    memcpy(&bmp_header[10], &offset, 4);
    
    // DIB Header size
    uint32_t dib_size = 40;
    memcpy(&bmp_header[14], &dib_size, 4);
    
    // Width & Height
    memcpy(&bmp_header[18], &width, 4);
    memcpy(&bmp_header[22], &height, 4);
    
    // Planes (1) & BPP (24)
    uint16_t planes = 1;
    uint16_t bpp = 24;
    memcpy(&bmp_header[26], &planes, 2);
    memcpy(&bmp_header[28], &bpp, 2);
    
    // Image Size (Raw)
    memcpy(&bmp_header[34], &pixel_data_size, 4);

    // header dump
    file.write(bmp_header, 54);

    // tmp buffer to allow sequential dump, row by tow (maybe not efficent but RAM-friendily)
    uint8_t* row_buffer = (uint8_t*)malloc(row_size_padded);
    if (!row_buffer) {
        Serial.println("Allocation of row failed");
        file.close();
        return false;
    }

    for (int y = height - 1; y >= 0; y--) {
        int buffer_idx = 0;
        
        for (int x = 0; x < width; x++) {
            int src_idx = (y * width + x) * channels;

            uint8_t r, g, b;

            if (channels == 1) {
                uint8_t val = rgb_data[src_idx];
                b = val; // Blue
                g = val; // Green
                r = val; // Red
            } else {
                r = rgb_data[src_idx];
                g = rgb_data[src_idx + 1];
                b = rgb_data[src_idx + 2];
            }

            // remind: BMP order is BGR
            row_buffer[buffer_idx++] = b;
            row_buffer[buffer_idx++] = g;
            row_buffer[buffer_idx++] = r;
        }

        // Add padding if needed
        for (uint32_t p = 0; p < padding; p++) {
            row_buffer[buffer_idx++] = 0;
        }

        file.write(row_buffer, row_size_padded);
    }

    free(row_buffer);
    file.close();
    String to_log = "BMP \"" + String(path) + "\" stored: " + String(width) + " x " + String(height) + " (" + String(channels) + " ch)";
    default_log(to_log, FILEMAN_LOG_PERMISSION);
    return true;
}