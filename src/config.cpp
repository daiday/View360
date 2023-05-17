//
// Created by daiyan on 2023/5/17.
//

#include "config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config,
                                   ratioScale,
                                   gammaCorrect,
                                   flipImage,
                                   needGenCubeMap,
                                   showInfo,
                                   showGrid,
                                   minFov,
                                   maxFov,
                                   stepFov,
                                   defaultFov,
                                   inverseWheel,
                                   fontSize)

static const char *gConfigFile = "config.json";

extern Config *getConfig() {
    static Config config;
    return &config;
}

extern void loadConfig() {
    try {
        nlohmann::json json;
        std::ifstream in(gConfigFile);
        if (in.is_open()) {
            in >> json;
            in.close();

            auto *cfg = getConfig();
            from_json(json, *cfg);
        }

    } catch (std::exception &e) {
        std::cerr << "Load Config: ";
        std::cerr << e.what() << std::endl;
    }
}

extern void saveConfig() {
    try {
        nlohmann::json json;
        auto *cfg = getConfig();
        to_json(json, *cfg);

        std::ofstream out(gConfigFile);
        if (out.is_open()) {
            out << json.dump(2);
            out.close();
        }
    } catch (std::exception &e) {
        std::cerr << "Save Config: ";
        std::cerr << e.what() << std::endl;
    }
}
