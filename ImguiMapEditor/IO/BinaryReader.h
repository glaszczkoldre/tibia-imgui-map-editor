#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace MapEditor {
namespace IO {

/**
 * Binary file reader with Tibia-specific data type support
 * Handles little-endian data typical in Tibia files
 */
class BinaryReader {
public:
    BinaryReader() = default;
    explicit BinaryReader(const std::filesystem::path& path);
    ~BinaryReader();
    
    // No copy
    BinaryReader(const BinaryReader&) = delete;
    BinaryReader& operator=(const BinaryReader&) = delete;
    
    // Move
    BinaryReader(BinaryReader&& other) noexcept;
    BinaryReader& operator=(BinaryReader&& other) noexcept;
    
    // Open/close file
    bool open(const std::filesystem::path& path);
    void close();
    bool isOpen() const { return file_.is_open(); }
    
    // Reading primitives
    uint8_t readU8();
    uint16_t readU16();
    uint32_t readU32();
    uint64_t readU64();
    
    int8_t readS8();
    int16_t readS16();
    int32_t readS32();
    
    float readFloat();
    double readDouble();
    
    // Read string (length-prefixed with 16-bit length)
    std::string readString();
    
    // Read fixed-length string
    std::string readString(size_t length);
    
    // Read raw bytes
    std::vector<uint8_t> readBytes(size_t count);
    bool readBytes(uint8_t* buffer, size_t count);
    
    // Position control
    size_t tell() const;
    bool seek(size_t position);
    bool skip(size_t bytes);
    bool seekRelative(int64_t offset);
    
    // File info
    size_t size() const { return file_size_; }
    size_t remaining() const;
    bool eof() const;
    
    // Error state
    bool good() const { return !error_ && file_.good(); }
    bool hasError() const { return error_; }
    const std::string& getError() const { return error_message_; }
    void clearError();

private:
    void setError(const std::string& message);
    
    mutable std::ifstream file_;
    size_t file_size_ = 0;
    bool error_ = false;
    std::string error_message_;
};

} // namespace IO
} // namespace MapEditor
