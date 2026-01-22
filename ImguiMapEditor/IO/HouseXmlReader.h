#pragma once
#include "Domain/House.h"
#include "Domain/House.h"
#include "Domain/ChunkedMap.h"
#include <filesystem>
#include <string>

namespace MapEditor {
namespace IO {

class HouseXmlReader {
public:
    struct Result {
        bool success = false;
        std::string error;
        int houses_loaded = 0;
    };

    static Result read(const std::filesystem::path& path, Domain::ChunkedMap& map);
};

} // namespace IO
} // namespace MapEditor
