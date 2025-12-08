#include "picture.h"

Picture::Picture(uint8_t* raw_data, size_t len, size_t width, size_t height, pic_format format, bool force_copy)
    : len(len), width(width), height(height), format(format) {
    
    if (force_copy) {
        this->raw_data = (uint8_t*)malloc(len*sizeof(uint8_t));
        memcpy(this->raw_data, raw_data, len);
    } else {
        this->raw_data = raw_data;
    }
    
}

Picture::~Picture() {
    if (raw_data != nullptr) {
        free(raw_data); //delete[] raw_data;  // TODO: check if it is better delete[] or free
        raw_data = nullptr; // a good practice
    }
}