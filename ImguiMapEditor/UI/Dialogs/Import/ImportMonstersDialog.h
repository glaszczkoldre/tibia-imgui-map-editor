#pragma once

#include <filesystem>
#include <string>
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * Dialog for importing spawns.xml (monsters/NPCs) into the current map.
 */
class ImportMonstersDialog {
public:
    enum class MergeMode {
        ReplaceAll,     // Clear existing spawns and import
        Merge,          // Add to existing spawns
        SkipDuplicates  // Only add spawns that don't conflict
    };
    
    struct ImportOptions {
        std::filesystem::path source_path;
        MergeMode merge_mode = MergeMode::Merge;
    };
    
    enum class Result {
        None,
        Confirmed,
        Cancelled
    };
    
    void show();
    Result render();
    
    const ImportOptions& getOptions() const { return options_; }
    bool isOpen() const { return is_open_; }
    
private:
    bool is_open_ = false;
    bool should_open_ = false;
    ImportOptions options_;
    char path_buffer_[512] = {};
};

} // namespace UI
} // namespace MapEditor
