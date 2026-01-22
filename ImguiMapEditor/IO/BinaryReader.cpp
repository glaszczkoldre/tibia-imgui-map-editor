#include "BinaryReader.h"
#include <cstring>
#include <format>
#include <limits>

namespace MapEditor {
namespace IO {

BinaryReader::BinaryReader(const std::filesystem::path& path) {
    open(path);
}

BinaryReader::~BinaryReader() {
    close();
}

BinaryReader::BinaryReader(BinaryReader&& other) noexcept
    : file_(std::move(other.file_))
    , file_size_(other.file_size_)
    , error_(other.error_)
    , error_message_(std::move(other.error_message_))
{
    other.file_size_ = 0;
    other.error_ = false;
}

BinaryReader& BinaryReader::operator=(BinaryReader&& other) noexcept {
    if (this != &other) {
        close();
        file_ = std::move(other.file_);
        file_size_ = other.file_size_;
        error_ = other.error_;
        error_message_ = std::move(other.error_message_);
        other.file_size_ = 0;
        other.error_ = false;
    }
    return *this;
}

bool BinaryReader::open(const std::filesystem::path& path) {
    close();
    
    file_.open(path, std::ios::binary);
    if (!file_) {
        setError("Failed to open file: " + path.string());
        return false;
    }
    
    // Get file size
    file_.seekg(0, std::ios::end);
    file_size_ = static_cast<size_t>(file_.tellg());
    file_.seekg(0, std::ios::beg);
    
    return true;
}

void BinaryReader::close() {
    if (file_.is_open()) {
        file_.close();
    }
    file_size_ = 0;
    clearError();
}

uint8_t BinaryReader::readU8() {
    uint8_t value = 0;
    file_.read(reinterpret_cast<char*>(&value), 1);
    if (!file_) setError("Failed to read U8");
    return value;
}

uint16_t BinaryReader::readU16() {
    uint16_t value = 0;
    file_.read(reinterpret_cast<char*>(&value), 2);
    if (!file_) setError("Failed to read U16");
    return value;
}

uint32_t BinaryReader::readU32() {
    uint32_t value = 0;
    file_.read(reinterpret_cast<char*>(&value), 4);
    if (!file_) setError("Failed to read U32");
    return value;
}

uint64_t BinaryReader::readU64() {
    uint64_t value = 0;
    file_.read(reinterpret_cast<char*>(&value), 8);
    if (!file_) setError("Failed to read U64");
    return value;
}

int8_t BinaryReader::readS8() {
    return static_cast<int8_t>(readU8());
}

int16_t BinaryReader::readS16() {
    return static_cast<int16_t>(readU16());
}

int32_t BinaryReader::readS32() {
    return static_cast<int32_t>(readU32());
}

float BinaryReader::readFloat() {
    float value = 0;
    file_.read(reinterpret_cast<char*>(&value), 4);
    if (!file_) setError("Failed to read float");
    return value;
}

double BinaryReader::readDouble() {
    double value = 0;
    file_.read(reinterpret_cast<char*>(&value), 8);
    if (!file_) setError("Failed to read double");
    return value;
}

std::string BinaryReader::readString() {
    uint16_t length = readU16();
    if (error_) return "";
    return readString(length);
}

std::string BinaryReader::readString(size_t length) {
    if (length == 0) return "";
    
    // Safety check: Don't attempt to allocate more memory than file size
    // This prevents DoS attacks via malicious length fields
    size_t rem = remaining();
    if (length > rem) {
        setError(std::format("String length {} exceeds remaining file size {}", length, rem));
        return "";
    }

    // Safety check: Prevent overflow when casting to streamsize (signed)
    if (length > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
        setError(std::format("String length {} exceeds maximum stream size", length));
        return "";
    }

    std::string result(length, '\0');
    file_.read(result.data(), static_cast<std::streamsize>(length));
    if (!file_) {
        setError("Failed to read string");
        return "";
    }
    return result;
}

std::vector<uint8_t> BinaryReader::readBytes(size_t count) {
    // Safety check: Don't attempt to allocate more memory than file size
    size_t rem = remaining();
    if (count > rem) {
        setError(std::format("Byte count {} exceeds remaining file size {}", count, rem));
        return {};
    }

    std::vector<uint8_t> result(count);
    if (count > 0) {
        file_.read(reinterpret_cast<char*>(result.data()), 
                   static_cast<std::streamsize>(count));
        if (!file_) {
            setError("Failed to read bytes");
            result.clear();
        }
    }
    return result;
}

bool BinaryReader::readBytes(uint8_t* buffer, size_t count) {
    if (count == 0) return true;
    file_.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(count));
    if (!file_) {
        setError("Failed to read bytes");
        return false;
    }
    return true;
}

size_t BinaryReader::tell() const {
    // tellg() can return -1 on failure
    auto pos = file_.tellg();
    if (pos == -1) return static_cast<size_t>(-1);
    return static_cast<size_t>(pos);
}

bool BinaryReader::seek(size_t position) {
    file_.seekg(static_cast<std::streamoff>(position), std::ios::beg);
    return file_.good();
}

bool BinaryReader::skip(size_t bytes) {
    file_.seekg(static_cast<std::streamoff>(bytes), std::ios::cur);
    return file_.good();
}

bool BinaryReader::seekRelative(int64_t offset) {
    file_.seekg(static_cast<std::streamoff>(offset), std::ios::cur);
    return file_.good();
}

size_t BinaryReader::remaining() const {
    auto pos = tell();
    // Check for error return from tell() or position past EOF
    if (pos == static_cast<size_t>(-1) || pos > file_size_) return 0;
    return file_size_ - pos;
}

bool BinaryReader::eof() const {
    return file_.eof() || remaining() == 0;
}

void BinaryReader::setError(const std::string& message) {
    error_ = true;
    error_message_ = message;
}

void BinaryReader::clearError() {
    error_ = false;
    error_message_.clear();
    if (file_.is_open()) {
        file_.clear();
    }
}

} // namespace IO
} // namespace MapEditor
