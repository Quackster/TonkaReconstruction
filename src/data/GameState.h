#pragma once

#include <array>
#include <string>
#include <fstream>

// Tracks player progression across all areas.
// Mirrors the original game's "time card" system.
struct GameState {
    // Area completion flags (index = module ID)
    // 0 = not started, 1 = in progress, 2 = completed
    std::array<int, 20> areaProgress = {};

    // Player name (from sign-in screen)
    std::string playerName = "Builder";

    bool isAreaComplete(int moduleId) const {
        if (moduleId < 0 || moduleId >= 20) return false;
        return areaProgress[moduleId] == 2;
    }

    void completeArea(int moduleId) {
        if (moduleId >= 0 && moduleId < 20) {
            areaProgress[moduleId] = 2;
        }
    }

    void startArea(int moduleId) {
        if (moduleId >= 0 && moduleId < 20 && areaProgress[moduleId] == 0) {
            areaProgress[moduleId] = 1;
        }
    }

    bool isMasterBuilder() const {
        // All main areas completed: Quarry(1), Avalanche(2), Desert(3),
        // City Park(4), Castle(5), Hi-Rise(6)
        return isAreaComplete(1) && isAreaComplete(2) && isAreaComplete(3) &&
               isAreaComplete(4) && isAreaComplete(5) && isAreaComplete(6);
    }

    int certificateModule(int areaModuleId) const {
        switch (areaModuleId) {
            case 1: return 61;  // Quarry
            case 2: return 62;  // Avalanche
            case 3: return 63;  // Desert
            case 4: return 64;  // City Park
            case 5: return 65;  // Castle
            case 6: return 66;  // Hi-Rise
            case 12: return 67; // Garage
            default: return 0;
        }
    }

    bool save(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(areaProgress.data()),
                areaProgress.size() * sizeof(int));
        uint32_t nameLen = static_cast<uint32_t>(playerName.size());
        f.write(reinterpret_cast<const char*>(&nameLen), 4);
        f.write(playerName.data(), nameLen);
        return true;
    }

    bool load(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.read(reinterpret_cast<char*>(areaProgress.data()),
               areaProgress.size() * sizeof(int));
        uint32_t nameLen = 0;
        f.read(reinterpret_cast<char*>(&nameLen), 4);
        if (nameLen > 0 && nameLen < 256) {
            playerName.resize(nameLen);
            f.read(playerName.data(), nameLen);
        }
        return true;
    }
};
