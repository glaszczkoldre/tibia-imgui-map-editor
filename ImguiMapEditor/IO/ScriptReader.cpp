#include "ScriptReader.h"
#include <spdlog/spdlog.h>
#include <cctype>
#include <algorithm>

namespace MapEditor {
namespace IO {

ScriptReader::~ScriptReader() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool ScriptReader::open(const std::string& filename) {
    if (file_.is_open()) {
        file_.close();
    }
    
    file_.open(filename, std::ios::binary);
    if (!file_.is_open()) {
        spdlog::error("ScriptReader: Cannot open file '{}'", filename);
        return false;
    }
    
    filename_ = filename;
    line_ = 1;
    token = TokenType::EndOfFile;
    return true;
}

void ScriptReader::close() {
    if (file_.is_open()) {
        file_.close();
    }
}

int ScriptReader::peekChar() {
    return file_.peek();
}

int ScriptReader::getChar() {
    int c = file_.get();
    if (c == '\n') {
        line_++;
    }
    return c;
}

void ScriptReader::skipWhitespaceAndComments() {
    while (true) {
        int c = peekChar();
        
        // Skip whitespace
        if (std::isspace(c)) {
            getChar();
            continue;
        }
        
        // Skip // comments
        if (c == '/') {
            getChar();
            int next = peekChar();
            if (next == '/') {
                // Skip until end of line
                while ((c = getChar()) != EOF && c != '\n') {}
                continue;
            } else {
                // Not a comment, push back would be needed but we'll treat / as special
                special_value_ = '/';
                token = TokenType::Special;
                return;
            }
        }
        
        // Skip # comments (common in old scripts)
        if (c == '#') {
            while ((c = getChar()) != EOF && c != '\n') {}
            continue;
        }
        
        break;
    }
}

void ScriptReader::nextToken() {
    skipWhitespaceAndComments();
    
    // Check if we already set a special token during comment skip
    if (token == TokenType::Special && special_value_ == '/') {
        return;
    }
    
    int c = peekChar();
    
    if (c == EOF) {
        token = TokenType::EndOfFile;
        return;
    }
    
    // String literal
    if (c == '"') {
        getChar(); // consume opening quote
        string_value_.clear();
        
        while ((c = getChar()) != EOF && c != '"') {
            if (c == '\\') {
                int escaped = getChar();
                switch (escaped) {
                    case 'n': string_value_ += '\n'; break;
                    case 't': string_value_ += '\t'; break;
                    case '\\': string_value_ += '\\'; break;
                    case '"': string_value_ += '"'; break;
                    default: string_value_ += static_cast<char>(escaped); break;
                }
            } else {
                string_value_ += static_cast<char>(c);
            }
        }
        
        token = TokenType::String;
        return;
    }
    
    // Number or Byte sequence (like 0-4 for SEC coordinates)
    // Reference: tibia-game script.cc lines 449-500
    if (std::isdigit(c)) {
        bytes_.clear();
        int current_number = getChar() - '0';
        
        // Keep reading digits
        while (std::isdigit(peekChar())) {
            current_number = current_number * 10 + (getChar() - '0');
        }
        
        // Check if this is a byte sequence (digit followed by - followed by digit)
        if (peekChar() == '-') {
            getChar(); // consume the '-'
            
            // Check if there's another digit after the -
            if (!std::isdigit(peekChar())) {
                // Not a valid byte sequence - put the '-' back so it can be 
                // parsed as a separate Special token on the next call
                file_.unget();
                number_value_ = current_number;
                token = TokenType::Number;
                return;
            }
            
            // This is a byte sequence like "0-4" or "0-4-7"
            bytes_.push_back(static_cast<uint8_t>(current_number));
            
            while (true) {
                current_number = getChar() - '0';
                while (std::isdigit(peekChar())) {
                    current_number = current_number * 10 + (getChar() - '0');
                }
                bytes_.push_back(static_cast<uint8_t>(current_number));
                
                // Check for more bytes
                if (peekChar() != '-') {
                    break;
                }
                getChar(); // consume the '-'
                
                if (!std::isdigit(peekChar())) {
                    // Trailing '-' at end of byte sequence - put it back
                    file_.unget();
                    break;
                }
            }
            
            token = TokenType::Bytes;
            return;
        }
        
        // Just a regular number
        number_value_ = current_number;
        token = TokenType::Number;
        return;
    }
    
    // Identifier
    if (std::isalpha(c) || c == '_') {
        string_value_.clear();
        while ((c = peekChar()) != EOF && (std::isalnum(c) || c == '_')) {
            string_value_ += static_cast<char>(getChar());
        }
        
        // Convert to lowercase
        std::transform(string_value_.begin(), string_value_.end(), 
                       string_value_.begin(), [](unsigned char c){ return std::tolower(c); });
        
        token = TokenType::Identifier;
        return;
    }
    
    // Special characters
    special_value_ = static_cast<char>(getChar());
    token = TokenType::Special;
}

std::string ScriptReader::readIdentifier() {
    nextToken();
    if (token != TokenType::Identifier) {
        error("Identifier expected");
        return "";
    }
    return string_value_;
}

int ScriptReader::readNumber() {
    nextToken();
    
    // Handle negative sign as special
    int sign = 1;
    if (token == TokenType::Special && special_value_ == '-') {
        sign = -1;
        nextToken();
    }
    
    if (token != TokenType::Number) {
        error("Number expected");
        return 0;
    }
    return number_value_ * sign;
}

std::string ScriptReader::readString() {
    nextToken();
    if (token != TokenType::String) {
        error("String expected");
        return "";
    }
    return string_value_;
}

void ScriptReader::readSymbol(char expected) {
    nextToken();
    if (token != TokenType::Special || special_value_ != expected) {
        error(std::string("Expected '") + expected + "'");
    }
}

std::string ScriptReader::getIdentifier() const {
    return string_value_;
}

int ScriptReader::getNumber() const {
    return number_value_;
}

std::string ScriptReader::getString() const {
    return string_value_;
}

char ScriptReader::getSpecial() const {
    return special_value_;
}

const std::vector<uint8_t>& ScriptReader::getBytes() const {
    return bytes_;
}

void ScriptReader::error(const std::string& message) {
    spdlog::error("ScriptReader error in '{}' line {}: {}", filename_, line_, message);
}

} // namespace IO
} // namespace MapEditor
