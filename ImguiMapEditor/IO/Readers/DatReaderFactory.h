#pragma once
#include "DatReaderBase.h"
#include "DatReaderV710.h"
#include "DatReaderV740.h"
#include "DatReaderV755.h"
#include "DatReaderV780.h"
#include "DatReaderV860.h"
#include "DatReaderV1010.h"
#include <memory>
#include <stdexcept>

namespace MapEditor {
namespace IO {

/**
 * Factory for creating the appropriate DAT reader based on client version
 */
class DatReaderFactory {
public:
    /**
     * Create appropriate reader for the given client version
     * @param version Client version (e.g., 860 for 8.60, 1010 for 10.10)
     * @return DatReaderBase pointer
     */
    static std::unique_ptr<DatReaderBase> create(uint32_t version) {
        // 7.10 - 7.30
        if (version >= 710 && version < 740) {
            return std::make_unique<DatReaderV710>();
        }
        // 7.40 - 7.54
        else if (version >= 740 && version < 755) {
            return std::make_unique<DatReaderV740>();
        }
        // 7.55 - 7.72
        else if (version >= 755 && version < 780) {
            return std::make_unique<DatReaderV755>();
        }
        // 7.80 - 8.54
        else if (version >= 780 && version < 860) {
            return std::make_unique<DatReaderV780>();
        }
        // 8.60 - 9.86
        else if (version >= 860 && version < 1010) {
            return std::make_unique<DatReaderV860>();
        }
        // 10.10+
        else if (version >= 1010) {
            return std::make_unique<DatReaderV1010>(version);
        }
        
        throw std::runtime_error("Unsupported client version: " + std::to_string(version));
    }
    
    /**
     * Read DAT file with automatic version detection
     */
    static DatResult read(const std::filesystem::path& path,
                          uint32_t version,
                          uint32_t expected_signature = 0) {
        auto reader = create(version);
        return reader->read(path, expected_signature);
    }
};

} // namespace IO
} // namespace MapEditor
