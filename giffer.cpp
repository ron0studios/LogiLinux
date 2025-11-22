#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <jpeglib.h>
#include <csetjmp>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class MXCreativeConsole {
private:
    int fd;
    static constexpr int LCD_SIZE = 118;
    static constexpr int TARGET_SIZE = 354;
    static constexpr int MAX_PACKET_SIZE = 4095;
    static constexpr int GRID_ROWS = 3;
    static constexpr int GRID_COLS = 3;

    // Init sequence for the device
    const std::vector<std::vector<uint8_t>> INIT_REPORTS = {
        {0x11, 0xff, 0x0b, 0x3b, 0x01, 0xa1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x11, 0xff, 0x0b, 0x3b, 0x01, 0xa2, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    };

    uint8_t generateWritePacketByte(int index, bool isFirst, bool isLast) {
        uint8_t value = index | 0b00100000;
        if (isFirst) value |= 0b10000000;
        if (isLast) value |= 0b01000000;
        return value;
    }

    std::vector<std::vector<uint8_t>> generateImagePackets(int keyIndex, const std::vector<uint8_t>& jpegData) {
        std::vector<std::vector<uint8_t>> result;
        const int PACKET1_HEADER = 20;

        // Calculate coordinates
        int row = keyIndex / 3;
        int col = keyIndex % 3;
        int x = 23 + col * (118 + 40);
        int y = 6 + row * (118 + 40);

        // First packet
        std::vector<uint8_t> packet1(MAX_PACKET_SIZE, 0);
        int byteCount1 = std::min((int)jpegData.size(), MAX_PACKET_SIZE - PACKET1_HEADER);
        
        std::memcpy(packet1.data() + PACKET1_HEADER, jpegData.data(), byteCount1);

        packet1[0] = 0x14; // Report ID
        packet1[1] = 0xff;
        packet1[2] = 0x02;
        packet1[3] = 0x2b;
        packet1[4] = generateWritePacketByte(1, true, byteCount1 >= (int)jpegData.size());
        
        // Big-endian 16-bit values
        packet1[5] = 0x01; packet1[6] = 0x00;
        packet1[7] = 0x01; packet1[8] = 0x00;
        packet1[9] = (x >> 8) & 0xFF; packet1[10] = x & 0xFF;
        packet1[11] = (y >> 8) & 0xFF; packet1[12] = y & 0xFF;
        packet1[13] = (LCD_SIZE >> 8) & 0xFF; packet1[14] = LCD_SIZE & 0xFF;
        packet1[15] = (LCD_SIZE >> 8) & 0xFF; packet1[16] = LCD_SIZE & 0xFF;
        
        int jpegLen = jpegData.size();
        packet1[18] = (jpegLen >> 8) & 0xFF; packet1[19] = jpegLen & 0xFF;

        result.push_back(packet1);

        // Subsequent packets
        int remainingBytes = jpegData.size() - byteCount1;
        int currentOffset = byteCount1;
        int part = 2;

        while (remainingBytes > 0) {
            const int headerSize = 5;
            int byteCount = std::min(remainingBytes, MAX_PACKET_SIZE - headerSize);
            std::vector<uint8_t> packet(MAX_PACKET_SIZE, 0);

            std::memcpy(packet.data() + headerSize, jpegData.data() + currentOffset, byteCount);

            packet[0] = 0x14;
            packet[1] = 0xff;
            packet[2] = 0x02;
            packet[3] = 0x2b;
            packet[4] = generateWritePacketByte(part, false, remainingBytes - byteCount == 0);

            result.push_back(packet);
            remainingBytes -= byteCount;
            currentOffset += byteCount;
            part++;
        }

        return result;
    }

public:
    bool connect(const char* device_path) {
        fd = open(device_path, O_RDWR);
        if (fd < 0) {
            std::cerr << "Failed to open device: " << device_path << std::endl;
            return false;
        }

        // Send init sequence
        for (const auto& report : INIT_REPORTS) {
            if (write(fd, report.data(), report.size()) < 0) {
                std::cerr << "Failed to send init report" << std::endl;
                close(fd);
                fd = -1;
                return false;
            }
        }

        std::cout << "Connected to MX Creative Console" << std::endl;
        return true;
    }

    void disconnect() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    bool setKeyImage(int keyIndex, const std::vector<uint8_t>& jpegData) {
        if (fd < 0 || keyIndex > 8) return false;

        auto packets = generateImagePackets(keyIndex, jpegData);
        
        for (const auto& packet : packets) {
            ssize_t written = write(fd, packet.data(), packet.size());
            if (written < 0) {
                std::cerr << "Failed to send image packet" << std::endl;
                return false;
            }
            usleep(100); // 0.1ms between packets
        }
        
        return true;
    }

    ~MXCreativeConsole() {
        disconnect();
    }
};

// JPEG encoder using libjpeg
std::vector<uint8_t> imageToJPEG(const uint8_t* data, int width, int height, int quality = 70) {
    std::vector<uint8_t> jpegData;
    
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    
    unsigned char* outbuffer = nullptr;
    unsigned long outsize = 0;
    jpeg_mem_dest(&cinfo, &outbuffer, &outsize);
    
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.optimize_coding = TRUE;
    
    jpeg_start_compress(&cinfo, TRUE);
    
    JSAMPROW row_pointer[1];
    int row_stride = width * 3;
    
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = (JSAMPROW)&data[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    
    jpeg_finish_compress(&cinfo);
    jpegData.assign(outbuffer, outbuffer + outsize);
    free(outbuffer);
    jpeg_destroy_compress(&cinfo);
    
    return jpegData;
}

struct GIFFrame {
    std::vector<uint8_t> data;
    int delay_ms;
    int width;
    int height;
};

std::vector<GIFFrame> loadGIF(const char* filename) {
    std::vector<GIFFrame> frames;
    
    FILE* f = fopen(filename, "rb");
    if (!f) {
        std::cerr << "Failed to open GIF file: " << filename << std::endl;
        return frames;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<uint8_t> buffer(fsize);
    fread(buffer.data(), 1, fsize, f);
    fclose(f);
    
    int* delays = nullptr;
    int width, height, frames_count;
    int channels = 0;
    
    stbi_uc* data = stbi_load_gif_from_memory(
        buffer.data(), buffer.size(), &delays, &width, &height, &frames_count, &channels, 3
    );
    
    if (!data) {
        std::cerr << "Failed to load GIF: " << filename << std::endl;
        return frames;
    }

    std::cout << "Loaded GIF: " << width << "x" << height 
              << ", " << frames_count << " frames" << std::endl;

    size_t frame_size = width * height * 3;
    for (int i = 0; i < frames_count; i++) {
        GIFFrame frame;
        frame.delay_ms = delays[i];
        frame.width = width;
        frame.height = height;
        
        uint8_t* frame_data = data + (i * frame_size);
        frame.data.assign(frame_data, frame_data + frame_size);
        
        frames.push_back(frame);
    }

    stbi_image_free(data);
    free(delays);

    return frames;
}

// Resize image to fit 118x118 (one cell)
std::vector<uint8_t> resizeToCell(const std::vector<uint8_t>& imgData, 
                                   int width, int height) {
    const int TARGET = 118;
    std::vector<uint8_t> result(TARGET * TARGET * 3);

    // Center crop to square first
    int cropSize = std::min(width, height);
    int sx = (width - cropSize) / 2;
    int sy = (height - cropSize) / 2;

    // Resize to 118x118
    for (int y = 0; y < TARGET; y++) {
        for (int x = 0; x < TARGET; x++) {
            int src_x = sx + (x * cropSize / TARGET);
            int src_y = sy + (y * cropSize / TARGET);
            
            int src_idx = (src_y * width + src_x) * 3;
            int dst_idx = (y * TARGET + x) * 3;
            
            result[dst_idx + 0] = imgData[src_idx + 0];
            result[dst_idx + 1] = imgData[src_idx + 1];
            result[dst_idx + 2] = imgData[src_idx + 2];
        }
    }

    return result;
}

// Composite a 118x118 image onto a 354x354 canvas at position (px, py)
// where px and py are in pixel coordinates (0-236 for smooth movement)
std::vector<uint8_t> compositeCellOnCanvas(const std::vector<uint8_t>& cellData,
                                            float px, float py) {
    const int CANVAS_SIZE = 354;
    const int CELL_SIZE = 118;
    std::vector<uint8_t> canvas(CANVAS_SIZE * CANVAS_SIZE * 3, 0);

    // Draw the cell at fractional position
    for (int cy = 0; cy < CELL_SIZE; cy++) {
        for (int cx = 0; cx < CELL_SIZE; cx++) {
            int canvas_x = (int)(px + cx);
            int canvas_y = (int)(py + cy);
            
            // Check bounds
            if (canvas_x >= 0 && canvas_x < CANVAS_SIZE && 
                canvas_y >= 0 && canvas_y < CANVAS_SIZE) {
                
                int cell_idx = (cy * CELL_SIZE + cx) * 3;
                int canvas_idx = (canvas_y * CANVAS_SIZE + canvas_x) * 3;
                
                canvas[canvas_idx + 0] = cellData[cell_idx + 0];
                canvas[canvas_idx + 1] = cellData[cell_idx + 1];
                canvas[canvas_idx + 2] = cellData[cell_idx + 2];
            }
        }
    }

    return canvas;
}

// Extract a 118x118 region from the 354x354 canvas for a specific cell
std::vector<uint8_t> extractCell(const std::vector<uint8_t>& canvas, int cellIdx) {
    const int CELL_SIZE = 118;
    const int CANVAS_SIZE = 354;
    std::vector<uint8_t> cellData(CELL_SIZE * CELL_SIZE * 3);
    
    int row = cellIdx / 3;
    int col = cellIdx % 3;
    
    for (int y = 0; y < CELL_SIZE; y++) {
        for (int x = 0; x < CELL_SIZE; x++) {
            int canvas_x = col * CELL_SIZE + x;
            int canvas_y = row * CELL_SIZE + y;
            int canvas_idx = (canvas_y * CANVAS_SIZE + canvas_x) * 3;
            int cell_idx = (y * CELL_SIZE + x) * 3;
            
            cellData[cell_idx + 0] = canvas[canvas_idx + 0];
            cellData[cell_idx + 1] = canvas[canvas_idx + 1];
            cellData[cell_idx + 2] = canvas[canvas_idx + 2];
        }
    }
    
    return cellData;
}

int main() {
    MXCreativeConsole mx;
    
    if (!mx.connect("/dev/hidraw1")) {
        return 1;
    }

    // Load GIF
    auto frames = loadGIF("earthrot.gif");
    if (frames.empty()) {
        std::cerr << "No frames loaded from GIF" << std::endl;
        return 1;
    }

    std::cout << "Resizing GIF to cell size (118x118)..." << std::endl;

    // Resize all GIF frames to 118x118
    std::vector<std::vector<uint8_t>> cellFrames;
    for (const auto& frame : frames) {
        auto resized = resizeToCell(frame.data, frame.width, frame.height);
        cellFrames.push_back(resized);
    }

    std::cout << "Processed " << cellFrames.size() << " frames" << std::endl;
    std::cout << "Playing GIF on center cell (cell 4)..." << std::endl;

    const int MIN_FRAME_TIME_MS = 50; // 20 fps
    const int CENTER_CELL = 4; // Middle cell (row 1, col 1)
    int frameIndex = 0;

    // Clear all cells initially
    std::vector<uint8_t> blackCell(118 * 118 * 3, 0);
    auto blackJpeg = imageToJPEG(blackCell.data(), 118, 118);
    for (int i = 0; i < 9; i++) {
        mx.setKeyImage(i, blackJpeg);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Animation loop - just play GIF on center cell
    while (true) {
        auto start = std::chrono::steady_clock::now();
        
        // Get current GIF frame
        const auto& currentFrame = cellFrames[frameIndex];
        
        // Convert to JPEG and send only to center cell
        auto jpeg = imageToJPEG(currentFrame.data(), 118, 118);
        mx.setKeyImage(CENTER_CELL, jpeg);
        
        // Advance GIF frame
        frameIndex = (frameIndex + 1) % cellFrames.size();
        
        // Frame timing
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        int remaining = MIN_FRAME_TIME_MS - elapsed;
        if (remaining > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(remaining));
        }
        
        // Log progress
        static int logCounter = 0;
        if (++logCounter % 20 == 0) {
            std::cout << "Frame: " << frameIndex << "/" << cellFrames.size() << std::endl;
        }
    }

    return 0;
}