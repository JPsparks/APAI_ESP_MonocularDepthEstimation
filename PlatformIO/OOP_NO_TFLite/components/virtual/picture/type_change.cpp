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

static size_t jpg_read_cb(void* arg, size_t index, uint8_t* buf, size_t len) {
    if (arg == nullptr) {
        Serial.println("[READ] CRITICAL ERROR: arg is nullptr!");
        return 0;
    }
    
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)arg;
    
    if (ctx->jpeg_buffer == nullptr) {
        Serial.println("[READ] ERROR: ctx->jpeg_buffer is nullptr");
        return 0;
    }
    
    Serial.printf("[READ] index=%zu, len=%zu, buf=%p (ctx=%p)\n", index, len, buf, ctx);
    
    // If buf is nullptr, the library is only asking if we can provide len bytes
    // This is normal behavior for some implementations (like "seek")
    if (buf == nullptr) {
        // Check if we can provide len bytes from the requested index
        if ((index + len) <= (ctx->jpeg_len)) {
            Serial.printf("[READ] Seek request: index=%zu len=%zu → OK\n", index, len);
            return len;  // There is these bytes available
        } else {
            Serial.printf("[READ] Seek request: index=%zu len=%zu → FAIL (max=%zu)\n", 
                         index, len, ctx->jpeg_len);
            return 0;  // There isn't enought bytes
        }
    }
    
    // Check bounds
    if (index >= ctx->jpeg_len) {
        Serial.printf("[READ] ERROR: index %zu >= jpeg_len %zu\n", index, ctx->jpeg_len);
        return 0;
    }
    
    if ((index + len) > (ctx->jpeg_len)) {
        size_t old_len = len;
        len = ctx->jpeg_len - index;
        Serial.printf("[READ] Truncated from %zu to %zu bytes\n", old_len, len);
    }
    
    // Copy of data
    memcpy(buf, ctx->jpeg_buffer + index, len);
    Serial.printf("[READ] Copied %zu bytes from offset %zu\n", len, index);
    
    return len;
}

static bool jpg_write_cb(void* arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data) {
    if (arg == nullptr) {
        Serial.println("[WRITE] CRITICAL ERROR: arg is nullptr!");
        return false;
    }
    
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)arg;
    
    Serial.printf("[WRITE] x=%d, y=%d, w=%d, h=%d (ctx=0x%p)\n", x, y, w, h, ctx);
    
    // Check context integrity
    if (ctx->rgb_buffer == nullptr) {
        Serial.println("[WRITE] ERROR: ctx->rgb_buffer is nullptr");
        return false;
    }
    
    if (data == nullptr) {
        Serial.println("[WRITE] ERROR: data is nullptr");
        return false;
    }
    
    size_t width_total = ctx->image_width;
    
    // Copy row by row
    for (uint16_t row = 0; row < h; row++) {
        size_t y_pos = y + row;
        
        if (y_pos >= ctx->image_height) {
            Serial.printf("[WRITE] Warning: y_pos %zu >= height %zu, break\n", 
                         y_pos, ctx->image_height);
            break;
        }
        
        if (x + w > ctx->image_width) {
            Serial.printf("[WRITE] Warning: x=%d w=%d eccede width=%zu, skip\n", 
                         x, w, ctx->image_width);
            continue;
        }
        
        size_t dst_offset = (y_pos * width_total + x) * 3;
        size_t src_offset = row * w * 3;
        
        memcpy(ctx->rgb_buffer + dst_offset, data + src_offset, w * 3);
    }
    
    Serial.printf("[WRITE] Block %dx%d completed\n", w, h);
    
    return true;
}


Picture* change_byJSONtoRGB888(Picture* by) {
    if (by == nullptr) {
        Serial.println("ERROR: Picture is nullptr");
        return nullptr;
    }
    
    if (by->get_format() != JPEG) {
        Serial.println("ERROR: picture is not JPEG");
        return nullptr;
    }

    // ✅ Verifica magic bytes JPEG (FF D8 FF)
    uint8_t* jpeg_data = by->get_raw_data();
    if (jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
        Serial.printf("ERRORSE: JPEG's magic bytes invalid: 0x%02X 0x%02X (expected: 0xFF 0xD8)\n",
                     jpeg_data[0], jpeg_data[1]);
        return nullptr;
    }
    
    Serial.println("JPEG's magic bytes correct");
    
    Serial.printf("Start decoding JPEG: %dx%d, %zu byte\n", 
                  by->get_width(), by->get_height(), by->get_len());
    
    // Check if the memory is enought free
    size_t rgb_len = by->get_width() * by->get_height() * 3;
    Serial.printf("Heap libero: %d byte, richiesti: %zu byte\n", 
                  ESP.getFreeHeap(), rgb_len);
    
    if (ESP.getFreeHeap() < rgb_len + 20000) {
        Serial.println("ERROR: not enought memory!");
        return nullptr;
    }
    
    uint8_t* rgb_buf = (uint8_t*)malloc(rgb_len);
    
    if (!rgb_buf) {
        Serial.printf("Error in memory allocation: %zu byte\n", rgb_len);
        return nullptr;
    }
    
    // Start buffer with 0
    memset(rgb_buf, 0, rgb_len);
    
    // ALLOC CONTEXT in the Heap
    jpg_decode_context_t* ctx = (jpg_decode_context_t*)malloc(sizeof(jpg_decode_context_t));
    
    if (!ctx) {
        Serial.println("Errore allocazione contesto");
        free(rgb_buf);
        return nullptr;
    }
    
    ctx->jpeg_buffer = by->get_raw_data();
    ctx->jpeg_len = by->get_len();
    ctx->rgb_buffer = rgb_buf;
    ctx->image_width = by->get_width();
    ctx->image_height = by->get_height();
    
    Serial.printf("Context allocated at: 0x%p\n", ctx);
    Serial.printf("  jpeg_buffer: 0x%p, len: %zu\n", ctx->jpeg_buffer, ctx->jpeg_len);
    Serial.printf("  rgb_buffer: 0x%p\n", ctx->rgb_buffer);
    Serial.printf("  dimensions: %zu x %zu\n", ctx->image_width, ctx->image_height);
    
    Serial.println("Starting esp_jpg_decode...");
    
    // Decodifica
    esp_err_t err = esp_jpg_decode(
        by->get_len(),
        JPG_SCALE_NONE,
        jpg_read_cb,
        jpg_write_cb,
        ctx
    );
    
    if (err != ESP_OK) {
        Serial.printf("Errore decodifica JPEG: 0x%x (%d)\n", err, err);
        free(ctx);
        free(rgb_buf);
        return nullptr;
    }
    
    Serial.println("Decodifica completata con successo!");
    Serial.printf("Heap libero dopo decodifica: %d byte\n", ESP.getFreeHeap());
    
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
