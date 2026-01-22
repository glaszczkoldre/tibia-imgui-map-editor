#pragma once
#include "Domain/CopyBuffer.h"
namespace MapEditor::AppLogic {

class EditorSession;  // Forward declaration

/**
 * Handles copy/cut/paste operations between editor sessions.
 * Operates on the shared CopyBuffer.
 */
class ClipboardService {
public:
    explicit ClipboardService(Domain::CopyBuffer& buffer);
    
    /**
     * Copy selected tiles from session to buffer.
     * @param session Source session with selection
     * @return Number of tiles copied
     */
    size_t copy(const EditorSession& session);
    
    /**
     * Cut selected tiles (copy + delete from map).
     * @param session Source session (will be modified)
     * @return Number of tiles cut
     */
    size_t cut(EditorSession& session);
    
    /**
     * Paste buffer contents to session at position.
     * @param session Target session
     * @param target_pos World position for paste origin
     * @return Number of tiles pasted
     */
    size_t paste(EditorSession& session, const Domain::Position& target_pos);
    
    /**
     * Check if paste is possible.
     */
    bool canPaste() const;
    
    /**
     * Get width/height of clipboard contents (for preview).
     */
    int32_t getClipboardWidth() const;
    int32_t getClipboardHeight() const;

    /**
     * Get the number of items/tiles in the clipboard.
     */
    size_t getItemCount() const { return buffer_.size(); }

    /**
     * Get access to the underlying buffer (for direct tile access).
     */
    const Domain::CopyBuffer& getBuffer() const { return buffer_; }
    
private:
    Domain::CopyBuffer& buffer_;
};

} // namespace MapEditor::AppLogic
