#include "pickers.h"

Picture* byXtoY(Picture* by, int scale_factor, int new_rows, int new_cols, bool haveToCenter) {
    // Initial check
    if (!by || !by->get_raw_data() || by->bytes_per_pixel() == 0 ||
        (int)by->get_width() < new_cols * scale_factor ||
        (int)by->get_height() < new_rows * scale_factor) {
        // Here will be added more error handling
        return nullptr;
    }

    // For simplicity, we only handle addressable and uncompressed formats
    if (by->get_format() == JPEG || by->get_format() == YUV420 || by->get_format() == RAW) {
        // Error handling for unsupported formats goes here
        return nullptr;
    }

    // Get bytes per pixel
    size_t bpp = by->bytes_per_pixel();

    // Memory Allocation for the new image (PSRAM/HEAP)
    // ! malloc, might allocate on PSRAM if available and configured.
    // The resized image will have 'new_rows * new_cols' pixels.
    uint8_t* res_raw_data = (uint8_t*)malloc(new_rows * new_cols * bpp * sizeof(uint8_t));
    if (res_raw_data == nullptr) {
        // other window for err handling
        return nullptr;
    }

    // Calculation of Start Offsets for Sampling
    // start_x and start_y are the coordinates (relative to 0,0 of the block) of the pixel
    // that is the center of the first scale_factor x scale_factor square
    int start_x = scale_factor / 2;
    int start_y = scale_factor / 2;
    
    // Calculate the cropping offset if haveToCenter is true.
    // This shifts the sampling area if the original image is larger than necessary.
    int offset_x = 0;
    int offset_y = 0;
    
    if (haveToCenter) {
        // Calculate the size of the area of interest (the area to be sampled)
        size_t required_width = new_cols * scale_factor;
        size_t required_height = new_rows * scale_factor;
        
        // Calculate the offset to center the area of interest within the original image
        offset_x = ((int)by->get_width() - (int)required_width) / 2;
        offset_y = ((int)by->get_height() - (int)required_height) / 2;
        
        // Ensure offsets are not negative (should be guaranteed by initial checks, but this is a safeguard)
        if (offset_x < 0) offset_x = 0;
        if (offset_y < 0) offset_y = 0;
    }


    // Resizing Loop (Subsampling)
    // i and j are the coordinates of the pixel in the output image (result).
    for (int i = 0; i < new_rows; i++){
        for (int j = 0; j < new_cols; j++){

            // Calculate the coordinates of the pixel to be sampled in the original image 'by'.
            // This pixel is the "center" of the 'scale_factor x scale_factor' block 
            // corresponding to the output pixel [i, j].
            int original_y = offset_y + (i * scale_factor) + start_y;
            int original_x = offset_x + (j * scale_factor) + start_x;
            
            // Get the pointer to the pixel in the original image
            const uint8_t* original_pixel = by->get_pixel(original_x, original_y);

            if (original_pixel != nullptr) {
                // Calculate the destination position in the output buffer
                // The index in the 1D buffer is: (row_index * width + column_index) * bytes_per_pixel
                size_t dest_index = (i * new_cols + j) * bpp;
                
                // Copia i byte del pixel
                for (size_t k = 0; k < bpp; k++) {
                    res_raw_data[dest_index + k] = original_pixel[k];
                }
            } else {
                // Error handling (the pixel shouldn't be nullptr if initial checks pass,
                // but in case of special formats or issues with get_pixel, fill with black or other value)
                for (size_t k = 0; k < bpp; k++) {
                    res_raw_data[(i * new_cols + j) * bpp + k] = 0; // Black or 0 depending on the format
                }
            }
        }
    }

    
    Picture* result = new Picture(
        res_raw_data, 
        new_rows * new_cols * bpp, 
        new_cols, 
        new_rows, 
        by->get_format(), 
        false
    );
    
    return result;
}

// Example function by 240x240 to 48x48
Picture* pick_by240to48(Picture* by, bool haveToCenter) {

    return byXtoY(by, 5, 48, 48, haveToCenter);
}