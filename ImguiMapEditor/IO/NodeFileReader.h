#pragma once

#include <filesystem>
#include <cstring>
#include <string>
#include <cstdint>
#include <memory>
#include <stack>
#include <vector>
#include <iterator>
#include "Core/Config.h"

namespace MapEditor {
namespace IO {

/**
 * Node markers for binary node file format
 */
enum class NodeMarker : uint8_t {
    Start = 0xFE,
    End = 0xFF,
    Escape = 0xFD
};

/**
 * Error codes for node file operations
 */
enum class NodeFileError {
    None,
    CouldNotOpen,
    InvalidIdentifier,
    StringTooLong,
    ReadError,
    WriteError,
    SyntaxError,
    PrematureEnd,
    OutOfMemory
};

// Forward declarations
class NodeFileReadHandle;

/**
 * Represents a node in a binary tree structure
 * Provides methods to read data and navigate to children/siblings
 */
class BinaryNode {
public:
    BinaryNode(NodeFileReadHandle* file, BinaryNode* parent);
    ~BinaryNode();
    
    // Non-copyable
    BinaryNode(const BinaryNode&) = delete;
    BinaryNode& operator=(const BinaryNode&) = delete;
    
    // Data reading methods
    bool getU8(uint8_t& value);
    bool getU16(uint16_t& value);
    bool getU32(uint32_t& value);
    bool getU64(uint64_t& value);
    bool peekU8(uint8_t& value) const;
    bool getString(std::string& str);
    bool getLongString(std::string& str);
    bool getRAW(uint8_t* ptr, size_t size);
    bool getRAW(std::string& str, size_t size);
    bool skip(size_t size);
    
    /**
     * Get first child node
     * @return Child node or nullptr if no child
     */
    BinaryNode* getChild();
    
    /**
     * Advance to next sibling node
     * @return This node if successful, nullptr if no more siblings
     */
    BinaryNode* advance();
    
    /**
     * Get remaining bytes in this node
     */
    size_t bytesRemaining() const { 
        return data_.size() > read_offset_ ? data_.size() - read_offset_ : 0; 
    }

    // Iterator support for range-based for loops
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = BinaryNode;
        using difference_type = std::ptrdiff_t;
        using pointer = BinaryNode*;
        using reference = BinaryNode&;

        Iterator() = default;
        explicit Iterator(BinaryNode* node) : current_(node) {}

        reference operator*() const { return *current_; }
        pointer operator->() const { return current_; }

        Iterator& operator++() {
            if (current_) current_ = current_->advance();
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const { return current_ == other.current_; }
        bool operator!=(const Iterator& other) const { return current_ != other.current_; }

    private:
        BinaryNode* current_ = nullptr;
    };

    struct ChildrenView {
        BinaryNode* parent_;
        Iterator begin() { return Iterator(parent_->getChild()); }
        Iterator end() { return Iterator(nullptr); }
    };

    [[nodiscard]] ChildrenView children() { return ChildrenView{this}; }

private:
    template<typename T>
    bool getType(T& ref) {
        // Prevent integer overflow during bounds check
        if (read_offset_ >= data_.size() || sizeof(T) > data_.size() - read_offset_) {
            read_offset_ = data_.size();
            return false;
        }
        // Fix UB: unaligned access via memcpy
        std::memcpy(&ref, data_.data() + read_offset_, sizeof(T));
        read_offset_ += sizeof(T);
        return true;
    }
    
    void load();
    
    std::string data_;
    size_t read_offset_ = 0;
    NodeFileReadHandle* file_;
    BinaryNode* parent_;
    BinaryNode* child_ = nullptr;
    
    friend class NodeFileReadHandle;
    friend class DiskNodeFileReadHandle;
    friend class MemoryNodeFileReadHandle;
};

/**
 * Abstract base for node-based file reading
 * Implements buffered reading with escape sequence handling
 */
class NodeFileReadHandle {
public:
    NodeFileReadHandle();
    virtual ~NodeFileReadHandle();
    
    // Non-copyable
    NodeFileReadHandle(const NodeFileReadHandle&) = delete;
    NodeFileReadHandle& operator=(const NodeFileReadHandle&) = delete;
    
    /**
     * Get the root node of the file
     * @return Root node or nullptr on error
     */
    virtual BinaryNode* getRootNode() = 0;
    
    /**
     * Get file size
     */
    virtual size_t size() const = 0;
    
    /**
     * Get current read position
     */
    virtual size_t tell() const = 0;
    
    /**
     * Check if file is valid
     */
    virtual bool isOk() const { return error_code_ == NodeFileError::None; }
    
    /**
     * Get error code
     */
    NodeFileError getErrorCode() const { return error_code_; }
    
    /**
     * Get error message
     */
    std::string getErrorMessage() const;

protected:
    BinaryNode* getNode(BinaryNode* parent);
    BinaryNode* getAndLoadNode(BinaryNode* parent);
    void freeNode(BinaryNode* node);
    virtual bool renewCache() = 0;
    
    bool last_was_start_ = false;
    uint8_t* cache_ = nullptr;
    size_t cache_size_ = Config::Data::FILE_BUFFER_SIZE;
    size_t cache_length_ = 0;
    size_t local_read_index_ = 0;
    
    BinaryNode* root_node_ = nullptr;
    NodeFileError error_code_ = NodeFileError::None;
    
    // Node memory pool for performance
    std::stack<void*> unused_;
    
    friend class BinaryNode;
};

/**
 * Disk-based node file reader
 * Reads node files from disk with buffered I/O
 */
class DiskNodeFileReadHandle : public NodeFileReadHandle {
public:
    /**
     * Open a node file
     * @param path Path to the file
     * @param acceptable_identifiers List of valid 4-byte file identifiers
     */
    DiskNodeFileReadHandle(const std::filesystem::path& path,
                           const std::vector<std::string>& acceptable_identifiers = {});
    ~DiskNodeFileReadHandle() override;
    
    void close();
    BinaryNode* getRootNode() override;
    
    size_t size() const override { return file_size_; }
    size_t tell() const override;
    bool isOk() const override;

protected:
    bool renewCache() override;
    
    FILE* file_ = nullptr;
    size_t file_size_ = 0;
};

/**
 * Memory-based node file reader
 * Reads node data from a memory buffer
 */
class MemoryNodeFileReadHandle : public NodeFileReadHandle {
public:
    /**
     * Create reader from memory buffer
     * Does NOT take ownership of the memory
     */
    MemoryNodeFileReadHandle(const uint8_t* data, size_t size);
    ~MemoryNodeFileReadHandle() override;
    
    void assign(const uint8_t* data, size_t size);
    void close();
    
    BinaryNode* getRootNode() override;
    size_t size() const override { return cache_size_; }
    size_t tell() const override { return local_read_index_; }
    bool isOk() const override { return true; }

protected:
    bool renewCache() override;
};

} // namespace IO
} // namespace MapEditor
