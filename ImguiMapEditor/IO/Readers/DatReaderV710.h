#pragma once
#include "DatReaderBase.h"
namespace MapEditor {
namespace IO {

class DatReaderV710 : public DatReaderBase {
public:
    std::pair<uint32_t, uint32_t> getVersionRange() const override {
        return {710, 730};
    }
    
    const char* getName() const override {
        return "DAT V710 (7.10-7.30)";
    }

protected:
    uint8_t transformFlag(uint8_t raw) override;
    bool handleSpecificFlag(uint8_t flag, ClientItem& item, BinaryReader& reader) override;
    
    bool shouldReadPatternZ() const override {
        return false;
    }
};

} // namespace IO
} // namespace MapEditor
