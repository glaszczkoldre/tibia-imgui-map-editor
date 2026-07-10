#pragma once
#include "Brushes/Core/IBrush.h"
#include <string>

namespace MapEditor::Brushes {

    class PlaceholderBrush : public IBrush {
    public:
        explicit PlaceholderBrush(const std::string& name) : name_(name) {}
        ~PlaceholderBrush() override = default;

        const std::string& getName() const override { return name_; }
        BrushType getType() const override { return BrushType::Placeholder; }
        uint32_t getLookId() const override { return 0; } // No ID
        
        bool isDraggable() const override { return false; }
        
        void draw(Domain::ChunkedMap&, Domain::Tile*, const DrawContext&) override {}
        void undraw(Domain::ChunkedMap&, Domain::Tile*) override {}

    private:
        std::string name_;
    };

} // namespace MapEditor::Brushes
