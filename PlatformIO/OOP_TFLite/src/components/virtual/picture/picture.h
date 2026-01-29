#ifndef __PW_APAI_PIC_H__
#define __PW_APAI_PIC_H__

#include <Arduino.h>
#include "../../../utility/logger.h"
#include "../../config.h"


//note: this is a copy paste of the enum pixformat_t in esp_camera
typedef enum {
    RGB565,    //PIXFORMAT_RGB565,    // 2BPP/RGB565
    YUV422,    //PIXFORMAT_YUV422,    // 2BPP/YUV422
    YUV420,    //PIXFORMAT_YUV420,    // 1.5BPP/YUV420
    GRAYSCALE, //PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
    JPEG,      //PIXFORMAT_JPEG,      // JPEG/COMPRESSED
    RGB888,    //PIXFORMAT_           // 3BPP/RGB888
    RAW,       //PIXFORMAT_           // RAW
    RGB444,    //PIXFORMAT_RGB444,    // 3BP2P/RGB444
    RGB555,    //PIXFORMAT_RGB555,    // 3BP2P/RGB555
} pic_format;



class Picture
{
public:
    Picture(uint8_t* raw_data, 
            size_t len, 
            size_t width, 
            size_t height, 
            pic_format format = JPEG,
            bool force_copy = false);
    ~Picture();

    Picture(const Picture&) = delete;
    Picture& operator=(const Picture&) = delete;

    uint8_t* get_raw_data() const { return raw_data; }
    size_t get_len() const { return len; }
    size_t get_width() const { return width; }
    size_t get_height() const { return height; }
    pic_format get_format() const { return format; }

    const uint8_t* get_pixel(size_t x, size_t y) const {
        if (x >= width || y >= height) {
            return nullptr;
        }
        
        if (!is_pixel_addressable()) {
            return nullptr;
        }
        
        return &raw_data[(y * width + x) * bytes_per_pixel()];
    }
    
    size_t bytes_per_pixel() const {
        switch(format) {
            case RGB888:     return 3;
            case RGB565:     return 2;
            case RGB555:     return 2;
            case RGB444:     return 2;
            case GRAYSCALE:  return 1;
            case YUV422:     return 2; // Average: 4 byte per 2 pixel
            
            // Format non supported for direct access
            case JPEG:       return 0;
            case YUV420:     return 0;
            case RAW:        return 0;
            
            default:         return 0;
        }
    }

private:
    // note: this is a kind of clone of esp_camera/camera_fb_t
    // taken only the most meaningfull informations
    uint8_t* raw_data;               /* <-- Pointer to the pixel data */
    size_t len;                      /* <-- Length of the buffer in bytes */
    size_t width;                    /* <-- Width of the buffer in pixels */
    size_t height;                   /* <-- Height of the buffer in pixels */
    pic_format format;               /* <-- Format of the pixel data */
    // struct timeval timestamp;

    bool is_pixel_addressable() const {
        switch(format) {
            case RGB565:
            case RGB888:
            case RGB555:
            case RGB444:
            case GRAYSCALE:
            case YUV422:
                return true;
                
            case JPEG:      
            case YUV420:    
            case RAW:       
                return false;
                
            default:
                return false;
        }
    }

};

#endif 