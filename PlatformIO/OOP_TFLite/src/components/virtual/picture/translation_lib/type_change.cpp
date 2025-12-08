#include "type_change.h"

typedef struct {
    // Context for JPEG read
    uint8_t* jpeg_buffer;
    size_t jpeg_len;
    
    // Context for JPEG write
    uint8_t* rgb_buffer;
    size_t image_width;
    size_t image_height;
} jpg_decode_context_t;


#define LOG_BUFFER_SIZE 256 

char typechanger_to_log[LOG_BUFFER_SIZE];

static size_t jpg_read_cb(void* arg, size_t index, uint8_t* buf, size_t len) {
    
    if (arg == nullptr) {
        log("[READ] CRITICAL ERROR: arg is nullptr!", ERROR, TYPECHANGER_LOG_PERMISSION);
        return 0;
    }
    
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)arg;
    
    if (ctx->jpeg_buffer == nullptr) {
        log("[READ] ERROR: ctx->jpeg_buffer is nullptr!", ERROR, TYPECHANGER_LOG_PERMISSION);
        return 0;
    }
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[READ] index=%zu, len=%zu, buf=%p (ctx=%p) ", 
            index, len, buf, ctx);
    default_log(String(typechanger_to_log), TYPECHANGER_LOG_PERMISSION);

    // If buf is nullptr, the library is only asking if we can provide len bytes
    // This is normal behavior for some implementations (like "seek")
    if (buf == nullptr) {
        // Check if we can provide len bytes from the requested index
        if ((index + len) <= (ctx->jpeg_len)) {
            snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
                "[READ] Seek request: index=%zu len=%zu → OK ", 
                index, len);
            default_log(String(typechanger_to_log), TYPECHANGER_LOG_PERMISSION);

            return len;  // There is these bytes available
        } else {
            snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
                "[READ] Seek request: index=%zu len=%zu → FAIL (max=%zu) ", index, len, ctx->jpeg_len);
            log(String(typechanger_to_log), ERROR,  TYPECHANGER_LOG_PERMISSION);    
            
            return 0;  // There isn't enought bytes
        }
    }
    
    // Check bounds
    if (index >= ctx->jpeg_len) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[READ] ERROR: index %zu >= jpeg_len %zu ", index, ctx->jpeg_len);
        log(String(typechanger_to_log), ERROR,  TYPECHANGER_LOG_PERMISSION);   
        return 0;
    }
    
    if ((index + len) > (ctx->jpeg_len)) {
        size_t old_len = len;
        len = ctx->jpeg_len - index;
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[READ] Truncated from %zu to %zu bytes ", old_len, len);

        log(String(typechanger_to_log), WARNING, TYPECHANGER_LOG_PERMISSION); 
    }
    
    // Copy of data
    memcpy(buf, ctx->jpeg_buffer + index, len);
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[READ] Copied %zu bytes from offset %zu ", len, index);
            
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    return len;
}

static bool jpg_write_cb(void* arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data) {
    if (arg == nullptr) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
                "[WRITE] CRITICAL ERROR: arg is nullptr!");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        
        return false;
    }
    
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)arg;
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[WRITE] x=%d, y=%d, w=%d, h=%d (ctx=0x%p) ", x, y, w, h, ctx);
    log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 

    // Check context integrity
    if (ctx->rgb_buffer == nullptr) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[WRITE] ERROR: ctx->rgb_buffer is nullptr");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 

        return false;
    }
    
    if (data == nullptr) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "[WRITE] ERROR: data is nullptr");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        
        return false;
    }
    
    size_t width_total = ctx->image_width;
    
    // Copy row by row
    for (uint16_t row = 0; row < h; row++) {
        size_t y_pos = y + row;
        
        if (y_pos >= ctx->image_height) {
            snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
                "[WRITE] Warning: y_pos %zu >= height %zu, break ", y_pos, ctx->image_height);
            log(String(typechanger_to_log), WARNING, TYPECHANGER_LOG_PERMISSION); 
            break;
        }
        
        if (x + w > ctx->image_width) {
            snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
                "[WRITE] Warning: x=%d w=%d eccede width=%zu, skip ", x, w, ctx->image_width); 
            log(String(typechanger_to_log), WARNING, TYPECHANGER_LOG_PERMISSION); 
            continue;
        }
        
        size_t dst_offset = (y_pos * width_total + x) * 3;
        size_t src_offset = row * w * 3;
        
        memcpy(ctx->rgb_buffer + dst_offset, data + src_offset, w * 3);
    }
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "[WRITE] Block %dx%d completed ", w, h);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    return true;
}


Picture* change_byJSONtoRGB888(Picture* by) {
    if (by == nullptr) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "ERROR: Picture is nullptr");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        return nullptr;
    }
    
    if (by->get_format() != JPEG) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "ERROR: picture is not JPEG");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        return nullptr;
    }

    // ✅ Verifica magic bytes JPEG (FF D8 FF)
    uint8_t* jpeg_data = by->get_raw_data();
    if (jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "ERRORSE: JPEG's magic bytes invalid: 0x%02X 0x%02X (expected: 0xFF 0xD8) ", jpeg_data[0], jpeg_data[1]);
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        return nullptr;
    }
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "JPEG's magic bytes correct");
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Start decoding JPEG: %dx%d, %zu byte ", by->get_width(), by->get_height(), by->get_len());
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    // Check if the memory is enought free
    size_t rgb_len = by->get_width() * by->get_height() * 3;
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Heap libero: %d byte, richiesti: %zu byte ", ESP.getFreeHeap(), rgb_len);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    
    if (ESP.getFreeHeap() < rgb_len + 20000) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "ERROR: not enought memory!");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        return nullptr;
    }
    
    uint8_t* rgb_buf = (uint8_t*)malloc(rgb_len);
    
    if (!rgb_buf) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "Error in memory allocation: %zu byte ", rgb_len);
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        
        return nullptr;
    }
    
    // Start buffer with 0
    memset(rgb_buf, 0, rgb_len);
    
    // ALLOC CONTEXT in the Heap
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)malloc(sizeof(jpg_decode_context_t));
    
    if (!ctx) {
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "Errore allocazione contesto");
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        free(rgb_buf);
        return nullptr;
    }
    
    ctx->jpeg_buffer = by->get_raw_data();
    ctx->jpeg_len = by->get_len();
    ctx->rgb_buffer = rgb_buf;
    ctx->image_width = by->get_width();
    ctx->image_height = by->get_height();
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Context allocated at: 0x%p ", ctx);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "  jpeg_buffer: 0x%p, len: %zu ", ctx->jpeg_buffer, ctx->jpeg_len);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "  rgb_buffer: 0x%p ", ctx->rgb_buffer);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "  dimensions: %zu x %zu ", ctx->image_width, ctx->image_height);
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Starting esp_jpg_decode...");
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    
    // Decodifica
    esp_err_t err = esp_jpg_decode(
        by->get_len(),
        JPG_SCALE_NONE,
        jpg_read_cb,
        jpg_write_cb,
        ctx
    );
    
    if (err != ESP_OK) {    
        snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
            "Errore decodifica JPEG: 0x%x (%d) ", err, err);
        log(String(typechanger_to_log), ERROR, TYPECHANGER_LOG_PERMISSION); 
        
        free(ctx);
        free(rgb_buf);
        return nullptr;
    }
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Decodifica completata con successo!");
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    snprintf(typechanger_to_log, LOG_BUFFER_SIZE, 
        "Heap libero dopo decodifica: %d byte ", ESP.getFreeHeap());
    log(String(typechanger_to_log), INFO, TYPECHANGER_LOG_PERMISSION); 
    
    
    free(ctx);
    
    
    Picture* to_ret = new Picture(
        rgb_buf,
        rgb_len,
        by->get_width(),
        by->get_height(),
        RGB888,
        false
    );
    
    return to_ret;
}
