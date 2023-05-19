//
// Created by daiyan on 2023/5/17.
//

#include "app.h"
#include "config.h"
#include "shader_source.h"
#include "version.h"
#include <raymath.h>
#include <rlgl.h>

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else
#define GLSL_VERSION 100
#endif

static TextureCubemap genTextureCubemap(const Shader &shader, Texture2D &panorama, int size, int format);

std::vector<std::array<float, 2>> gRatioList{
        {16, 9},
        {17, 9},
        {18, 9},
        {18.5, 9},
        {19, 9},
        {19.5, 9},
        {19, 10},
};

App::App() {
    init();
}

App::~App() {
    UnloadTexture(_skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    UnloadShader(_skybox.materials[0].shader);
    UnloadShader(_renderCubeMapShader);
    UnloadModel(_skybox);
    CloseWindow();
}

void App::run() {
    while (!WindowShouldClose()) {
        update();
    }
}

void App::init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "全景图观察者");

    SetTargetFPS(60);

    initScene();
}

void App::initScene() {
    const int uniEnvMap = MATERIAL_MAP_CUBEMAP;
    const int uniEquirect = MATERIAL_MAP_ALBEDO;

    int uniDoGamma = getConfig()->gammaCorrect ? 1 : 0;
    int uniFlipped = getConfig()->flipImage ? 1 : 0;

    _camera.position = {1.0f, 1.0f, 1.0f};
    _camera.target = {4.0f, 1.0f, 4.0f};
    _camera.up = {0.0f, 1.0f, 0.0f};
    _camera.fovy = getConfig()->defaultFov;
    _camera.projection = CAMERA_PERSPECTIVE;

    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    _skybox = LoadModelFromMesh(cube);

    Shader shaderSkybox = LoadShaderFromMemory(skybox_vs, skybox_fs);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "environmentMap"), &uniEnvMap, SHADER_UNIFORM_INT);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "doGamma"), &uniDoGamma, SHADER_UNIFORM_INT);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "vflipped"), &uniFlipped, SHADER_UNIFORM_INT);
    _skybox.materials[0].shader = shaderSkybox;

    _renderCubeMapShader = LoadShaderFromMemory(cubemap_vs, cubemap_fs);
    SetShaderValue(_renderCubeMapShader, GetShaderLocation(_renderCubeMapShader, "equirectangularMap"), &uniEquirect, SHADER_UNIFORM_INT);
}

void App::update() {
    handleEvent();
    draw();
}

void App::handleEvent() {
    _reload = false;
    _showHelp = false;

    handleKeyEvent();

    handleMouseEvent();

    if (IsFileDropped()) {
        handleDropEvent();
    }

    if (_reload) {
        loadCubemap();
        _reload = false;
    }
}

void App::handleKeyEvent() {
    if (IsKeyPressed('1')) {
        if (_textureSize != 1024) {
            _textureSize = 1024;
            _reload = true;
        }
    }

    if (IsKeyPressed('2')) {
        if (_textureSize != 2048) {
            _textureSize = 2048;
            _reload = true;
        }
    }

    if (IsKeyPressed('3')) {
        if (_textureSize != 4096) {
            _textureSize = 4096;
            _reload = true;
        }
    }

    if (IsKeyPressed('4')) {
        if (_textureSize != 8192) {
            _textureSize = 8192;
            _reload = true;
        }
    }

    if (IsKeyPressed(KEY_C)) {
        Shader &shaderSkybox = _skybox.materials[0].shader;
        getConfig()->gammaCorrect = !getConfig()->gammaCorrect;
        int uniDoGamma = getConfig()->gammaCorrect ? 1 : 0;
        SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "doGamma"), &uniDoGamma, SHADER_UNIFORM_INT);
    }

    if (IsKeyPressed(KEY_L)) {
        Shader &shaderSkybox = _skybox.materials[0].shader;
        getConfig()->flipImage = !getConfig()->flipImage;
        int uniFlipped = getConfig()->flipImage ? 1 : 0;
        SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "vflipped"), &uniFlipped, SHADER_UNIFORM_INT);
    }

    if (IsKeyPressed(KEY_P)) {
        getConfig()->needGenCubeMap = !getConfig()->needGenCubeMap;
        if (_currentFileIndex >= 0) {
            _reload = true;
        }
    }

    if (IsKeyPressed(KEY_F)) {
        if (IsWindowMaximized())
            RestoreWindow();
        else
            MaximizeWindow();
    }

    if (IsKeyPressed(KEY_I)) {
        getConfig()->showInfo = !getConfig()->showInfo;
    }

    if (IsKeyPressed(KEY_G)) {
        getConfig()->showGrid = !getConfig()->showGrid;
    }

    if (IsKeyDown(KEY_F1) || IsKeyDown(KEY_H)) {
        _showHelp = true;
    }

    if (IsKeyPressed(KEY_LEFT)) {
        --_ratioIndex;
        if (_ratioIndex < 0) {
            _ratioIndex = (int) gRatioList.size() - 1;
        }
        float w = getConfig()->ratioScale * gRatioList[_ratioIndex][0];
        float h = getConfig()->ratioScale * gRatioList[_ratioIndex][1];
        SetWindowSize((int) w, (int) h);
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        ++_ratioIndex;
        if (_ratioIndex == gRatioList.size()) {
            _ratioIndex = 0;
        }
        float w = getConfig()->ratioScale * gRatioList[_ratioIndex][0];
        float h = getConfig()->ratioScale * gRatioList[_ratioIndex][1];
        SetWindowSize((int) w, (int) h);
    }

    if (IsKeyPressed(KEY_UP)) {
        if (!_fileList.empty() && _currentFileIndex > 0) {
            --_currentFileIndex;
            _reload = true;
        }
    }

    if (IsKeyPressed(KEY_DOWN)) {
        if (!_fileList.empty() && _currentFileIndex < (int) _fileList.size() - 1) {
            ++_currentFileIndex;
            _reload = true;
        }
    }
}

void App::handleMouseEvent() {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        UpdateCamera(&_camera, CAMERA_FIRST_PERSON);
    }

    auto delta = GetMouseWheelMove();
    if (fabsf(delta) > 0.1f) {
        if (getConfig()->inverseWheel) {
            delta = -delta;
        }
        _currentFovy += delta * getConfig()->stepFov;
        _currentFovy = Clamp(_currentFovy, getConfig()->minFov, getConfig()->maxFov);
    }
}

void App::handleDropEvent() {
    _fileList.clear();
    _currentFileIndex = -1;
    const char *filters = ".png;.jpg;.hdr;.bmp;.tga";
    FilePathList droppedFiles = LoadDroppedFiles();
    if (droppedFiles.count == 1) {
        const char *path = droppedFiles.paths[0];
        if (IsPathFile(path)) {
            if (IsFileExtension(path, filters)) {
                _fileList.emplace_back(path);
            }
        } else {
            auto subFiles = LoadDirectoryFilesEx(path, filters, false);
            for (unsigned int i = 0; i < subFiles.count; ++i) {
                _fileList.emplace_back(subFiles.paths[i]);
            }
            UnloadDirectoryFiles(subFiles);
        }

    } else if (droppedFiles.count > 1) {
        for (unsigned int i = 0; i < droppedFiles.count; ++i) {
            if (IsFileExtension(droppedFiles.paths[i], filters)) {
                _fileList.emplace_back(droppedFiles.paths[i]);
            }
        }
    }
    if (!_fileList.empty()) {
        _currentFileIndex = 0;
        _reload = true;
    }
    UnloadDroppedFiles(droppedFiles);
}

void App::draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    _camera.fovy = _currentFovy;
    BeginMode3D(_camera);

    if (_currentFileIndex >= 0) {
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        DrawModel(_skybox, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
    }

    if (getConfig()->showGrid) {
        DrawGrid(10, 1.0f);
    }
    EndMode3D();

    drawHelpTips();

    if (getConfig()->showInfo) {
        drawInfo();
    }

    if (_showHelp) {
        drawHelp();
    }

    EndDrawing();
}

void App::drawHelp() {
    int fontSize = getConfig()->fontSize;
    int margin = 5;
    int spacing = 5;
    auto textColor = BLACK;
    auto textColorHighlight = RED;
    auto textColorHighlight2 = BLUE;
    Color backgroundColor = {255, 255, 255, 255};
    int w = GetScreenWidth();
    int h = GetScreenHeight();

#ifdef _DEBUG
    const char *compile_mode = "DEBUG";
#else
    const char *compile_mode = "RELEASE";
#endif

    std::vector<std::string> contents;
    contents.emplace_back("Press 'Esc' to EXIT!");
    contents.emplace_back("Press 'f' to toggle maximize window.");
    contents.emplace_back("Press 'c' to toggle gamma correct.");
    contents.emplace_back("Press 'l' to toggle flip image.");
    contents.emplace_back("Press 'p' to toggle generate panorama.");
    contents.emplace_back("Press 'i' to toggle display information.");
    contents.emplace_back("Press 'g' to toggle display grid.");
    contents.emplace_back("Use Mouse Whell to change camera fovy.");
    contents.emplace_back("Use Arrow Left/Right to change window ratio.");
    contents.emplace_back("DROP FILE TO OPEN!");
    contents.emplace_back("Drop file will clear history.");
    contents.emplace_back("You can drop multiple files, or a single directory.");
    contents.emplace_back("Use Arrow Up/Down to view in history.");
    contents.emplace_back(TextFormat("[%s] [%s] [%s %s]", VERSION_STRING, compile_mode, __DATE__, __TIME__));

    int maxWidth = 0;
    for (const auto &content: contents) {
        maxWidth = std::max(maxWidth, MeasureText(content.c_str(), fontSize));
    }

    int rectWidth = maxWidth + 2 * margin;
    int rectHeight = (int) contents.size() * (fontSize + spacing) + 2 * margin - spacing;

    int posX = (w - rectWidth) / 2;
    int posY = (h - rectHeight) / 2;

    DrawRectangle(posX, posY, rectWidth, rectHeight, backgroundColor);

    posX += margin;
    posY += margin;
    for (const auto &content: contents) {
        Color color = textColor;
        if (content.back() == '!') {
            color = textColorHighlight;
        } else if (content.back() == ']') {
            color = textColorHighlight2;
        }
        DrawText(content.c_str(), posX, posY, fontSize, color);
        posY += fontSize + spacing;
    }
}

void App::drawInfo() {
    int fontSize = getConfig()->fontSize;
    int posX1 = 10;
    int posX2 = 100;
    int posY = 10;
    int posYOffset = 15;

    auto textColor = BLACK;
    auto textColorHighlight = BLUE;

    if (_currentFileIndex >= 0) {
        const char *filename = _fileList[_currentFileIndex].c_str();
        DrawText(filename, posX1, posY, fontSize, textColor);
        posY += posYOffset;
    }

    DrawText("[C]orrect Gamma:", posX1, posY, fontSize, textColor);
    DrawText(getConfig()->gammaCorrect ? "On" : "Off", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("F[l]ip Image:", posX1, posY, fontSize, textColor);
    DrawText(getConfig()->flipImage ? "On" : "Off", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("[P]anorama:", posX1, posY, fontSize, textColor);
    DrawText(getConfig()->needGenCubeMap ? "Yes" : "No", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("Aspect Ratio:", posX1, posY, fontSize, textColor);
    DrawText(TextFormat("%g : %g", gRatioList[_ratioIndex][0], gRatioList[_ratioIndex][1]), posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("Camera Fovy:", posX1, posY, fontSize, textColor);
    DrawText(TextFormat("%g", _currentFovy), posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;
}

void App::drawHelpTips() {
    int fontSize = getConfig()->fontSize;
    int posX = 10;
    int posY = GetScreenHeight() - 10 - fontSize;
    int size = 15;

    auto textColor = BLACK;
    auto textColorHighlight = RED;

    DrawRectangle(posX, posY, size, size, {0, 0, 0, 0});

    if (CheckCollisionPointRec(GetMousePosition(), {(float) posX, (float) posY, (float) size, (float) size})) {
        DrawText("Hold F1 or 'h' to show help.", posX, posY, fontSize, textColor);
    } else {
        DrawText(" ? ", posX, posY, fontSize, textColorHighlight);
    }
}

void App::loadCubemap() {
    if (IsTextureReady(_skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture)) {
        UnloadTexture(_skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    }
    const char *filePath = _fileList[_currentFileIndex].c_str();
    if (filePath) {
        Image img = LoadImage(filePath);
        if (getConfig()->needGenCubeMap) {
            Texture2D panorama = LoadTextureFromImage(img);
            _skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = genTextureCubemap(_renderCubeMapShader, panorama, _textureSize, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            UnloadTexture(panorama);
        } else {
            _skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(img, CUBEMAP_LAYOUT_AUTO_DETECT);
        }
        UnloadImage(img);
    }
}

static TextureCubemap genTextureCubemap(const Shader &shader, Texture2D &panorama, int size, int format) {
    TextureCubemap cubemap = {0};

    rlDisableBackfaceCulling();

    // step 1: setup framebuffer
    //---------------------------------------------------------------------------------
    unsigned int rbo = rlLoadTextureDepth(size, size, true);
    cubemap.id = rlLoadTextureCubemap(nullptr, size, format);

    unsigned int fbo = rlLoadFramebuffer(size, size);
    rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

    if (!rlFramebufferComplete(fbo)) {
        TraceLog(LOG_WARNING, "Cubemap framebuffer generated failed!");
    }

    // step 2: draw to framebuffer
    //---------------------------------------------------------------------------------
    rlEnableShader(shader.id);

    Matrix matFboProjection = MatrixPerspective(90.0f * DEG2RAD, 1.0f, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

    Matrix fboViews[6] = {
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
            MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f})};

    rlViewport(0, 0, size, size);

    rlActiveTextureSlot(0);
    rlEnableTexture(panorama.id);

    for (int i = 0; i < 6; i++) {
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
        rlEnableFramebuffer(fbo);

        rlClearScreenBuffers();
        rlLoadDrawCube();
    }

    // step 3: cleanup
    //---------------------------------------------------------------------------------
    rlDisableShader();
    rlDisableTexture();
    rlDisableFramebuffer();
    rlUnloadFramebuffer(fbo);

    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
    rlEnableBackfaceCulling();

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = 1;
    cubemap.format = format;

    return cubemap;
}
