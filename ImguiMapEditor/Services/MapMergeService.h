#pragma once
#include "Domain/Position.h"
#include "Domain/ChunkedMap.h"
#include <string>

namespace MapEditor {
namespace AppLogic {

class EditorSession;

/**
 * Service for merging maps.
 * 
 * Handles importing one map into another with offset and merge options.
 * Keeps IO parsing (OtbmReader) separate from merge logic.
 */
class MapMergeService {
public:
    struct MergeOptions {
        Domain::Position offset{0, 0, 7};
        bool overwrite_existing = false;  // If true, replace tiles; if false, merge items
    };
    
    struct MergeResult {
        bool success = false;
        std::string error;
        int tiles_merged = 0;
        int tiles_skipped = 0;
    };
    
    /**
     * Merge source map into target session at the specified offset.
     * 
     * This operation is undoable - creates appropriate undo actions.
     * 
     * @param target The editor session to merge into
     * @param source The map to merge from
     * @param options Merge options (offset, overwrite mode)
     * @return Result with success status and statistics
     */
    MergeResult merge(
        EditorSession& target,
        const Domain::ChunkedMap& source,
        const MergeOptions& options
    );
};

} // namespace AppLogic
} // namespace MapEditor
