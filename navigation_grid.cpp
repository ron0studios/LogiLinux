#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

constexpr size_t MAX_PACKET_SIZE = 4095;
constexpr size_t LCD_SIZE = 118;

class MXCreativeConsole {
private:
    int fd;
    std::atomic<bool> running{true};
    std::thread monitor_thread;
    
    const std::vector<std::vector<uint8_t>> INIT_REPORTS = {
        {0x11, 0xff, 0x0b, 0x3b, 0x01, 0xa1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x11, 0xff, 0x0b, 0x3b, 0x01, 0xa2, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    };
    
public:
    MXCreativeConsole(const char* device_path) : fd(-1) {
        fd = open(device_path, O_RDWR);
        if (fd < 0) {
            throw std::runtime_error(std::string("Error opening device: ") + strerror(errno));
        }
        
        struct hidraw_devinfo info;
        if (ioctl(fd, HIDIOCGRAWINFO, &info) >= 0) {
            char name[256] = {0};
            ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name);
            std::cout << "Connected to: " << name << std::endl;
        }
        
        for (const auto& report : INIT_REPORTS) {
            write(fd, report.data(), report.size());
            usleep(10000);
        }
    }
    
    ~MXCreativeConsole() {
        running = false;
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        if (fd >= 0) close(fd);
    }
    
    void setKeyImage(int keyIndex, const std::vector<uint8_t>& jpegData) {
        if (keyIndex < 0 || keyIndex > 8) return;
        
        auto packets = generateImagePackets(keyIndex, jpegData);
        
        for (const auto& packet : packets) {
            write(fd, packet.data(), packet.size());
            usleep(2000);
        }
    }
    
    void startMonitoring(std::function<void(const std::vector<uint8_t>&)> callback) {
        monitor_thread = std::thread([this, callback]() {
            std::vector<uint8_t> report(256);
            
            while (running) {
                int bytes_read = read(fd, report.data(), report.size());
                
                if (bytes_read < 0) {
                    if (errno == EINTR || errno == EAGAIN) {
                        usleep(10000);
                        continue;
                    }
                    break;
                }
                
                if (bytes_read > 0) {
                    std::vector<uint8_t> event_data(report.begin(), report.begin() + bytes_read);
                    callback(event_data);
                }
            }
        });
    }
    
private:
    std::vector<std::vector<uint8_t>> generateImagePackets(int keyIndex, const std::vector<uint8_t>& jpegData) {
        std::vector<std::vector<uint8_t>> result;
        
        int row = keyIndex / 3;
        int col = keyIndex % 3;
        uint16_t x = 23 + col * (118 + 40);
        uint16_t y = 6 + row * (118 + 40);
        
        const size_t PACKET1_HEADER = 20;
        size_t byteCount1 = std::min(jpegData.size(), MAX_PACKET_SIZE - PACKET1_HEADER);
        
        std::vector<uint8_t> packet1(MAX_PACKET_SIZE, 0);
        std::copy(jpegData.begin(), jpegData.begin() + byteCount1, packet1.begin() + PACKET1_HEADER);
        
        packet1[0] = 0x14;
        packet1[1] = 0xff;
        packet1[2] = 0x02;
        packet1[3] = 0x2b;
        packet1[4] = generateWritePacketByte(1, true, byteCount1 >= jpegData.size());
        packet1[5] = 0x01; packet1[6] = 0x00;
        packet1[7] = 0x01; packet1[8] = 0x00;
        packet1[9] = (x >> 8) & 0xff; packet1[10] = x & 0xff;
        packet1[11] = (y >> 8) & 0xff; packet1[12] = y & 0xff;
        packet1[13] = (LCD_SIZE >> 8) & 0xff; packet1[14] = LCD_SIZE & 0xff;
        packet1[15] = (LCD_SIZE >> 8) & 0xff; packet1[16] = LCD_SIZE & 0xff;
        packet1[18] = (jpegData.size() >> 8) & 0xff; packet1[19] = jpegData.size() & 0xff;
        
        result.push_back(packet1);
        
        size_t remainingBytes = jpegData.size() - byteCount1;
        size_t currentOffset = byteCount1;
        int part = 2;
        
        while (remainingBytes > 0) {
            const size_t headerSize = 5;
            size_t byteCount = std::min(remainingBytes, MAX_PACKET_SIZE - headerSize);
            
            std::vector<uint8_t> packet(MAX_PACKET_SIZE, 0);
            std::copy(jpegData.begin() + currentOffset, jpegData.begin() + currentOffset + byteCount,
                     packet.begin() + headerSize);
            
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
    
    uint8_t generateWritePacketByte(int index, bool isFirst, bool isLast) {
        uint8_t value = index | 0b00100000;
        if (isFirst) value |= 0b10000000;
        if (isLast) value |= 0b01000000;
        return value;
    }
};

// Generate a solid color JPEG image
std::vector<uint8_t> generateColorJPEG(uint8_t r, uint8_t g, uint8_t b) {
    char tmpfile[256];
    snprintf(tmpfile, sizeof(tmpfile), "/tmp/color_%02x%02x%02x.jpg", r, g, b);
    
    // Create a solid color image using ImageMagick
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "magick -size 118x118 xc:'srgb(%d,%d,%d)' -type TrueColor -quality 95 %s 2>/dev/null",
             r, g, b, tmpfile);
    
    if (system(cmd) != 0) {
        std::cerr << "Failed to generate image with command: " << cmd << std::endl;
        return std::vector<uint8_t>();
    }
    
    // Read the JPEG file
    FILE* f = fopen(tmpfile, "rb");
    if (!f) {
        std::cerr << "Failed to open generated image: " << tmpfile << std::endl;
        return std::vector<uint8_t>();
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<uint8_t> data(size);
    fread(data.data(), 1, size, f);
    fclose(f);
    
    unlink(tmpfile);
    
    std::cout << "Generated " << (int)r << "," << (int)g << "," << (int)b 
              << " image: " << size << " bytes" << std::endl;
    
    return data;
}

int main() {
    std::cout << "Navigation Grid Example" << std::endl;
    std::cout << "=======================" << std::endl;
    
    const char* device_path = "/dev/hidraw1";
    
    try {
        MXCreativeConsole console(device_path);
        
        // Generate white and black images
        auto white_img = generateColorJPEG(255, 255, 255);
        auto black_img = generateColorJPEG(0, 0, 0);
        
        if (white_img.empty() || black_img.empty()) {
            std::cerr << "Failed to generate images (ImageMagick required)" << std::endl;
            return 1;
        }
        
        std::cout << "Generated images successfully" << std::endl;
        
        // Current position (row-major order: 0-8)
        std::atomic<int> current_pos{0};
        
        // Function to update display
        auto updateDisplay = [&](int pos) {
            std::cout << "Updating display - white at position " << pos << std::endl;
            for (int i = 0; i < 9; i++) {
                if (i == pos) {
                    console.setKeyImage(i, white_img);
                } else {
                    console.setKeyImage(i, black_img);
                }
            }
            std::cout << "Display update complete" << std::endl;
        };
        
        // Set initial display
        std::cout << "Setting initial position (top-left)" << std::endl;
        updateDisplay(current_pos);
        
        // Start monitoring for button events
        console.startMonitoring([&](const std::vector<uint8_t>& data) {
            // Button events: 11 ff 0b 00 01 a1/a2 (press) or 11 ff 0b 00 00 00 (release)
            // data[0] = 0x11, data[1] = 0xff, data[2] = 0x0b, data[3] = 0x00
            // data[4] = 0x01 (press) or 0x00 (release)
            // data[5] = 0xa1 (P1/left) or 0xa2 (P2/right)
            if (data.size() >= 6 && data[0] == 0x11 && data[1] == 0xff && data[2] == 0x0b && data[4] == 0x01) {
                int old_pos = current_pos.load();
                int new_pos = old_pos;
                
                if (data[5] == 0xa1) {  // P1 - Left button
                    new_pos = (old_pos - 1 + 9) % 9;  // Move left with wraparound
                    std::cout << "Left button (P1) pressed - Position: " << old_pos << " -> " << new_pos << std::endl;
                } else if (data[5] == 0xa2) {  // P2 - Right button
                    new_pos = (old_pos + 1) % 9;  // Move right with wraparound
                    std::cout << "Right button (P2) pressed - Position: " << old_pos << " -> " << new_pos << std::endl;
                }
                
                if (new_pos != old_pos) {
                    current_pos = new_pos;
                    updateDisplay(new_pos);
                }
            }
        });
        
        std::cout << "Ready! Use left/right buttons (P1/P2) to navigate." << std::endl;
        std::cout << "Press Ctrl+C to exit." << std::endl;
        
        // Keep running
        while (true) {
            sleep(1);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
