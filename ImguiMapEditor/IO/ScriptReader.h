#pragma once

#include <string>
#include <fstream>
#include <cstdint>
#include <vector>

namespace MapEditor {
namespace IO {

/**
 * Token types for script parsing
 */
enum class TokenType {
    EndOfFile = 0,
    Identifier,
    Number,
    String,
    Special,
    Bytes       // Byte sequence like "0-4" for SEC coordinates
};

/**
 * Script tokenizer for items.srv format
 * Adapted from RME's script.h for reading ancient Tibia item definitions
 */
class ScriptReader {
public:
    ScriptReader() = default;
    ~ScriptReader();

    // Prevent copying
    ScriptReader(const ScriptReader&) = delete;
    ScriptReader& operator=(const ScriptReader&) = delete;

    /**
     * Open a script file for reading
     * @param filename Path to the script file
     * @return true if successful
     */
    bool open(const std::string& filename);

    /**
     * Close the current file
     */
    void close();

    /**
     * Advance to the next token
     */
    void nextToken();

    /**
     * Read and expect an identifier, returning it
     */
    std::string readIdentifier();

    /**
     * Read and expect a number, returning it
     */
    int readNumber();

    /**
     * Read and expect a quoted string, returning it
     */
    std::string readString();

    /**
     * Read and expect a specific symbol character
     */
    void readSymbol(char expected);

    /**
     * Get current identifier (assumes token is Identifier)
     */
    std::string getIdentifier() const;

    /**
     * Get current number (assumes token is Number)
     */
    int getNumber() const;

    /**
     * Get current string (assumes token is String)
     */
    std::string getString() const;

    /**
     * Get current special character (assumes token is Special)
     */
    char getSpecial() const;

    /**
     * Get byte sequence (assumes token is Bytes)
     * For SEC coordinates like "0-4", returns [0, 4]
     */
    const std::vector<uint8_t>& getBytes() const;

    /**
     * Report a parsing error
     */
    void error(const std::string& message);

    // Current token state
    TokenType token = TokenType::EndOfFile;

private:
    void skipWhitespaceAndComments();
    int peekChar();
    int getChar();

    std::ifstream file_;
    std::string filename_;
    int line_ = 1;

    // Token value storage
    std::string string_value_;
    int number_value_ = 0;
    char special_value_ = 0;
    std::vector<uint8_t> bytes_;
};

} // namespace IO
} // namespace MapEditor
