#pragma once

namespace MapEditor::Utils {

/**
 * RAII wrapper to automatically toggle a boolean flag.
 * Sets flag to true on construction, resets to false on destruction.
 */
class ScopedFlag {
public:
    explicit ScopedFlag(bool& flag) : flag_(flag) {
        flag_ = true;
    }

    ~ScopedFlag() {
        flag_ = false;
    }

    // Non-copyable
    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;

private:
    bool& flag_;
};

} // namespace MapEditor::Utils
