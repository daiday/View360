//
// Created by daiyan on 2023/5/17.
//

#ifndef VIEW360_CONFIG_H
#define VIEW360_CONFIG_H

struct Config {
    float ratioScale = 50;
    bool gammaCorrect = true;
    bool flipImage = true;
    bool needGenCubeMap = true;
    bool showInfo = true;
    bool showGrid = true;
    float minFov = 1.0f;
    float maxFov = 120.0f;
    float stepFov = 1.0f;
    float defaultFov = 45.0f;
    bool inverseWheel = true;
    int fontSize = 10;
};

extern Config *getConfig();
extern void loadConfig();
extern void saveConfig();

#endif//VIEW360_CONFIG_H
