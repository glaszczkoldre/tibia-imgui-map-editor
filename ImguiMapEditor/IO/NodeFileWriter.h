#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "Core/Config.h"

namespace MapEditor::IO {

/**
 * Node markers for binary node file format (same as reader)
 */
constexpr uint8_t NODE_START = 0xFE;
constexpr uint8_t NODE_END = 0xFF;
constexpr uint8_t NODE_ESCAPE = 0xFD;

/**
 * Binary node file writer.
 * Writes OTBM-style node tree files with escape encoding.
 */
class NodeFileWriteHandle {
public:
    NodeFileWriteHandle(const std::filesystem::path& path, const std::string& identifier);
    ~NodeFileWriteHandle();
    
    // Non-copyable
    NodeFileWriteHandle(const NodeFileWriteHandle&) = delete;
    NodeFileWriteHandle& operator=(const NodeFileWriteHandle&) = delete;
    
    bool isOk() const { return !error_ && (file_ != nullptr || closed_successfully_); }
    
    /**
     * Start a new node with the given type.
     */
    bool startNode(uint8_t type);
    
    /**
     * End the current node.
     */
    bool endNode();
    
    // Write methods (with escape encoding)
    bool writeU8(uint8_t value);
    bool writeU16(uint16_t value);
    bool writeU32(uint32_t value);
    bool writeU64(uint64_t value);
    bool writeString(const std::string& str);
    bool writeLongString(const std::string& str);
    bool writeRAW(const uint8_t* data, size_t size);
    bool writeRAW(const std::string& data);
    
    /**
     * Close and flush the file.
     */
    void close();
    
private:
    bool writeRawByte(uint8_t byte);
    bool writeEscaped(uint8_t byte);
    bool flush();
    
    FILE* file_ = nullptr;
    bool error_ = false;
    bool closed_successfully_ = false;
    int node_depth_ = 0;
    
    // Write buffer for performance
    std::vector<uint8_t> buffer_;
    static constexpr size_t BUFFER_SIZE = Config::Data::FILE_BUFFER_SIZE;
};

} // namespace MapEditor::IO
