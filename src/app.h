//
// Created by daiyan on 2023/5/17.
//

#ifndef VIEW360_APP_H
#define VIEW360_APP_H

#include <array>
#include <raylib.h>
#include <string>
#include <vector>

class App {
public:
    App();

    ~App();

    void run();

private:
    void init();

    void initScene();

    void update();

    void handleEvent();

    void handleKeyEvent();

    void handleMouseEvent();

    void handleDropEvent();

    void draw();

    void drawHelp();

    void drawInfo();

    void drawHelpTips();

    void loadCubemap();

    Camera _camera{};
    Model _skybox{};
    Shader _renderCubeMapShader{};

    int _ratioIndex{0};
    int _textureSize{1024};
    float _currentFovy{45.0f};
    std::vector<std::string> _fileList{};
    int _currentFileIndex{-1};
    bool _reload{false};
    bool _showHelp{false};
};

#endif//VIEW360_APP_H
