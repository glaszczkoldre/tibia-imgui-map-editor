#pragma once

#include <pugixml.hpp>
#include <filesystem>
#include <string>

namespace MapEditor::IO {

class XmlUtils {
public:
    /**
     * Loads an XML file, checks for parse errors, and retrieves the root node.
     *
     * @param path The path to the XML file.
     * @param rootNodeName The expected name of the root node.
     * @param doc The pugi::xml_document object to load into.
     * @param errorOut Output string for error messages.
     * @return The root node if successful, or an empty node if failed.
     */
    static pugi::xml_node loadXmlFile(
        const std::filesystem::path& path,
        const char* rootNodeName,
        pugi::xml_document& doc,
        std::string& errorOut);
};

} // namespace MapEditor::IO
