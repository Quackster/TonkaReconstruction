#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>

// Lightweight module data loader that parses the JSON exported by the Python tools.
// Avoids external JSON library dependency by doing simple manual parsing.

struct OverlayEntry {
    int index = 0;
    int type = 0;
    int width = 0;
    int height = 0;
    int cx = 0;
    int cy = 0;
    int flag = 0;
    int frames = 0;
    int navModule = 0;
};

struct ModuleInfo {
    int moduleId = 0;
    int entryCount = 0;
    int bgWidth = 640;
    int bgHeight = 480;
    std::vector<OverlayEntry> entries;
};

inline int jsonInt(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return 0;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    return std::atoi(json.c_str() + pos);
}

inline ModuleInfo loadModuleInfo(const std::string& path) {
    ModuleInfo info;

    std::ifstream file(path);
    if (!file.is_open()) return info;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    info.moduleId = jsonInt(json, "module_id");
    info.entryCount = jsonInt(json, "entry_count");
    info.bgWidth = jsonInt(json, "bg_width");
    info.bgHeight = jsonInt(json, "bg_height");

    // Parse entries array
    auto entriesPos = json.find("\"entries\"");
    if (entriesPos == std::string::npos) return info;

    // Find each { } block within entries
    size_t pos = json.find('[', entriesPos);
    if (pos == std::string::npos) return info;

    while (true) {
        auto start = json.find('{', pos);
        if (start == std::string::npos) break;
        auto end = json.find('}', start);
        if (end == std::string::npos) break;

        std::string block = json.substr(start, end - start + 1);

        OverlayEntry e;
        e.index = jsonInt(block, "index");
        e.type = jsonInt(block, "type");
        e.width = jsonInt(block, "width");
        e.height = jsonInt(block, "height");
        e.cx = jsonInt(block, "cx");
        e.cy = jsonInt(block, "cy");
        e.flag = jsonInt(block, "flag");
        e.frames = jsonInt(block, "frames");
        e.navModule = jsonInt(block, "nav_module");
        info.entries.push_back(e);

        pos = end + 1;
    }

    return info;
}
