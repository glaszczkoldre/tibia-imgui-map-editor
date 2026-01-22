#include "XmlUtils.h"
#include <string>

namespace MapEditor::IO {

pugi::xml_node XmlUtils::loadXmlFile(
    const std::filesystem::path& path,
    const char* rootNodeName,
    pugi::xml_document& doc,
    std::string& errorOut) {

    if (!std::filesystem::exists(path)) {
        errorOut = "File not found: " + path.string();
        return pugi::xml_node();
    }

    // Use c_str() which handles both char* (POSIX) and wchar_t* (Windows) correctly for pugi::load_file
    pugi::xml_parse_result parseResult = doc.load_file(path.c_str());

    if (!parseResult) {
        errorOut = "XML parse error at offset " +
                   std::to_string(parseResult.offset) + ": " +
                   parseResult.description();
        return pugi::xml_node();
    }

    pugi::xml_node rootNode = doc.child(rootNodeName);
    if (!rootNode) {
        errorOut = "Invalid root node: expected <" + std::string(rootNodeName) + ">";
        return pugi::xml_node();
    }

    return rootNode;
}

} // namespace MapEditor::IO
