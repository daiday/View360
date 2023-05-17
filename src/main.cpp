#include <fstream>
#include <locale>

#include <array>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "shader_source.h"
#include "version.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else
#define GLSL_VERSION 100
#endif

#if !defined(_DEBUG) && defined(WIN32)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#ifdef _DEBUG
const char *compile_mode = "Debug";
#else
const char *compile_mode = "Release";
#endif

static void setSkyboxTexture(const char *filePath, Model &skybox, const Shader &shader, bool genMap);

static TextureCubemap genTextureCubemap(const Shader &shader, Texture2D &panorama, int size, int format);

static void regenCubeMap(Model &skybox, const char *filePath, const Shader &shader, bool genMap);

static void drawInformation(const char *filename);

static void drawHelpIcon();

static void drawHelp();

std::vector<std::string> fileList;

std::vector<std::array<float, 2>> ratioList{
        {16, 9},
        {17, 9},
        {18, 9},
        {18.5, 9},
        {19, 9},
        {19.5, 9},
        {19, 10},
};
float currentFovy = 45.0f;

// config
int ratioIndex = 0;
float ratioScale = 50;
bool gammaCorrect = true;
bool flipImage = true;
bool needGenCubeMap = true;
bool showInfo = true;
bool showGrid = true;
int texture_size = 1024;
float minFov = 1.0f;
float maxFov = 120.0f;
float stepFov = 1.0f;
float defaultFov = 45.0f;
bool inverseWheel = true;
int fontSize = 10;

int main(int /*argc*/, char ** /*argv*/) {
    // set locale for chinese
    std::locale::global(std::locale("zh_CN.UTF-8"));

#ifdef _DEBUG
    SetTraceLogLevel(LOG_INFO);
#else
    SetTraceLogLevel(LOG_WARNING);
#endif

    try {
        nlohmann::json json;
        std::ifstream in("config.json");
        if (in.is_open()) {
            in >> json;
            in.close();
            ratioIndex = json["config"]["ratioIndex"];
            ratioScale = json["config"]["ratioScale"];
            gammaCorrect = json["config"]["gammaCorrect"];
            flipImage = json["config"]["flipImage"];
            needGenCubeMap = json["config"]["needGenCubeMap"];
            showInfo = json["config"]["showInfo"];
            showGrid = json["config"]["showGrid"];
            minFov = json["config"]["minFov"];
            maxFov = json["config"]["maxFov"];
            stepFov = json["config"]["stepFov"];
            defaultFov = json["config"]["defaultFov"];
            inverseWheel = json["config"]["inverseWheel"];
            fontSize = json["config"]["fontSize"];
        }
    } catch (...) {
        TraceLog(LOG_WARNING, "config load failed");
    }

    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "全景图观察者");

    const int uniEnvMap = MATERIAL_MAP_CUBEMAP;
    const int uniEquirect = MATERIAL_MAP_ALBEDO;

    int uniDoGamma = gammaCorrect ? 1 : 0;
    int uniFlipped = flipImage ? 1 : 0;

    Camera camera = {0};
    camera.position = {1.0f, 1.0f, 1.0f};
    camera.target = {4.0f, 1.0f, 4.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = defaultFov;
    camera.projection = CAMERA_PERSPECTIVE;

    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    Model skybox = LoadModelFromMesh(cube);

    Shader shaderSkybox = LoadShaderFromMemory(skybox_vs, skybox_fs);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "environmentMap"), &uniEnvMap, SHADER_UNIFORM_INT);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "doGamma"), &uniDoGamma, SHADER_UNIFORM_INT);
    SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "vflipped"), &uniFlipped, SHADER_UNIFORM_INT);
    skybox.materials[0].shader = shaderSkybox;

    Shader shaderGenCubemap = LoadShaderFromMemory(cubemap_vs, cubemap_fs);
    SetShaderValue(shaderGenCubemap, GetShaderLocation(shaderGenCubemap, "equirectangularMap"), &uniEquirect, SHADER_UNIFORM_INT);

    SetTargetFPS(60);

    bool regen;
    bool showHelp;
    int currentFileIndex = -1;

    while (!WindowShouldClose()) {
        showHelp = false;
        regen = false;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        }

        auto delta = GetMouseWheelMove();
        if (fabsf(delta) > 0.1f) {
            if (inverseWheel) {
                delta = -delta;
            }
            currentFovy += delta * stepFov;
            currentFovy = Clamp(currentFovy, minFov, maxFov);
        }

        if (IsKeyPressed('1')) {
            if (texture_size != 1024) {
                texture_size = 1024;
                regen = true;
            }
        }
        if (IsKeyPressed('2')) {
            if (texture_size != 2048) {
                texture_size = 2048;
                regen = true;
            }
        }
        if (IsKeyPressed('3')) {
            if (texture_size != 4096) {
                texture_size = 4096;
                regen = true;
            }
        }
        if (IsKeyPressed('4')) {
            if (texture_size != 8192) {
                texture_size = 8192;
                regen = true;
            }
        }

        if (IsKeyPressed(KEY_C)) {
            gammaCorrect = !gammaCorrect;
            uniDoGamma = gammaCorrect ? 1 : 0;
            SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "doGamma"), &uniDoGamma, SHADER_UNIFORM_INT);
        }

        if (IsKeyPressed(KEY_L)) {
            flipImage = !flipImage;
            uniFlipped = flipImage ? 1 : 0;
            SetShaderValue(shaderSkybox, GetShaderLocation(shaderSkybox, "vflipped"), &uniFlipped, SHADER_UNIFORM_INT);
        }

        if (IsKeyPressed(KEY_P)) {
            needGenCubeMap = !needGenCubeMap;
            if (currentFileIndex >= 0) {
                regen = true;
            }
        }

        if (IsKeyPressed(KEY_F)) {
            if (IsWindowMaximized())
                RestoreWindow();
            else
                MaximizeWindow();
        }

        if (IsKeyPressed(KEY_LEFT)) {
            --ratioIndex;
            if (ratioIndex < 0) {
                ratioIndex = (int) ratioList.size() - 1;
            }
            float w = ratioScale * ratioList[ratioIndex][0];
            float h = ratioScale * ratioList[ratioIndex][1];
            SetWindowSize((int) w, (int) h);
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            ++ratioIndex;
            if (ratioIndex == ratioList.size()) {
                ratioIndex = 0;
            }
            float w = ratioScale * ratioList[ratioIndex][0];
            float h = ratioScale * ratioList[ratioIndex][1];
            SetWindowSize((int) w, (int) h);
        }

        if (!fileList.empty()) {
            if (IsKeyPressed(KEY_UP)) {
                if (currentFileIndex > 0) {
                    --currentFileIndex;
                    regen = true;
                }
            }

            if (IsKeyPressed(KEY_DOWN)) {
                if (currentFileIndex < (int) fileList.size() - 1) {
                    ++currentFileIndex;
                    regen = true;
                }
            }
        }

        if (IsKeyPressed(KEY_I)) {
            showInfo = !showInfo;
        }

        if (IsKeyPressed(KEY_G)) {
            showGrid = !showGrid;
        }

        if (IsKeyDown(KEY_F1) || IsKeyDown(KEY_H)) {
            showHelp = true;
        }

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count == 1) {
                if (IsFileExtension(droppedFiles.paths[0], ".png;.jpg;.hdr;.bmp;.tga")) {
                    fileList.emplace_back(droppedFiles.paths[0]);
                    currentFileIndex = (int) fileList.size() - 1;
                    regen = true;
                }
            } else if (droppedFiles.count > 1) {
                fileList.clear();
                currentFileIndex = -1;
                for (unsigned int i = 0; i < droppedFiles.count; ++i) {
                    if (IsFileExtension(droppedFiles.paths[i], ".png;.jpg;.hdr;.bmp;.tga")) {
                        fileList.emplace_back(droppedFiles.paths[i]);
                    }
                }
                if (!fileList.empty()) {
                    currentFileIndex = 0;
                }
                if (currentFileIndex >= 0) {
                    regen = true;
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (regen) {
            regenCubeMap(skybox, fileList[currentFileIndex].c_str(), shaderGenCubemap, needGenCubeMap);
            regen = false;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        camera.fovy = currentFovy;
        BeginMode3D(camera);

        if (currentFileIndex >= 0) {
            rlDisableBackfaceCulling();
            rlDisableDepthMask();
            DrawModel(skybox, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            rlEnableBackfaceCulling();
            rlEnableDepthMask();
        }

        if (showGrid) {
            DrawGrid(10, 1.0f);
        }
        EndMode3D();

        drawHelpIcon();

        if (showInfo) {
            const char *filename = nullptr;
            if (currentFileIndex >= 0) {
                filename = fileList[currentFileIndex].c_str();
            }
            drawInformation(filename);
        }

        if (showHelp) {
            drawHelp();
        }

        EndDrawing();
    }

    UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    UnloadShader(shaderSkybox);
    UnloadModel(skybox);

    CloseWindow();

    try {
        nlohmann::json json;
        json["config"]["ratioIndex"] = ratioIndex;
        json["config"]["ratioScale"] = ratioScale;
        json["config"]["gammaCorrect"] = gammaCorrect;
        json["config"]["flipImage"] = flipImage;
        json["config"]["needGenCubeMap"] = needGenCubeMap;
        json["config"]["showInfo"] = showInfo;
        json["config"]["showGrid"] = showGrid;
        json["config"]["minFov"] = minFov;
        json["config"]["maxFov"] = maxFov;
        json["config"]["stepFov"] = stepFov;
        json["config"]["currentFov"] = currentFovy;
        json["config"]["inverseWheel"] = inverseWheel;
        json["config"]["fontSize"] = fontSize;

        std::ofstream out("config.json");
        out << json.dump(2);
        out.close();
    } catch (...) {
        TraceLog(LOG_WARNING, "save config failed.");
    }

    return 0;
}

void setSkyboxTexture(const char *filePath, Model &skybox, const Shader &shader, bool genMap) {
    Image img = LoadImage(filePath);
    if (genMap) {
        Texture2D panorama = LoadTextureFromImage(img);
        skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = genTextureCubemap(shader, panorama, texture_size, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        UnloadTexture(panorama);
    } else {
        skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(img, CUBEMAP_LAYOUT_AUTO_DETECT);
    }
    UnloadImage(img);
}

TextureCubemap genTextureCubemap(const Shader &shader, Texture2D &panorama, int size, int format) {
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

void regenCubeMap(Model &skybox, const char *filePath, const Shader &shader, bool genMap) {
    if (IsTextureReady(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture)) {
        UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    }
    setSkyboxTexture(filePath, skybox, shader, genMap);
}

static void drawInformation(const char *filename) {
    int posX1 = 10;
    int posX2 = 100;
    int posY = 10;
    int posYOffset = 15;

    auto textColor = BLACK;
    auto textColorHighlight = BLUE;

    if (filename) {
        DrawText(filename, posX1, posY, fontSize, textColor);
        posY += posYOffset;
    }

    DrawText("[C]orrect Gamma:", posX1, posY, fontSize, textColor);
    DrawText(gammaCorrect ? "On" : "Off", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("F[l]ip Image:", posX1, posY, fontSize, textColor);
    DrawText(flipImage ? "On" : "Off", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("[P]anorama:", posX1, posY, fontSize, textColor);
    DrawText(needGenCubeMap ? "Yes" : "No", posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("Aspect Ratio:", posX1, posY, fontSize, textColor);
    DrawText(TextFormat("%g : %g", ratioList[ratioIndex][0], ratioList[ratioIndex][1]), posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;

    DrawText("Camera Fovy:", posX1, posY, fontSize, textColor);
    DrawText(TextFormat("%g", currentFovy), posX2, posY, fontSize, textColorHighlight);
    posY += posYOffset;
}

static void drawHelpIcon() {
    int posX = 10;
    int posY = GetScreenHeight() - 10 - fontSize;
    int size = 15;

    auto textColor = BLACK;
    auto textColorHighlight = RED;

    DrawRectangle(posX, posY, size, size, {0, 0, 0, 0});

    if (CheckCollisionPointRec(GetMousePosition(), {(float) posX, (float) posY, (float) size, (float) size})) {
        DrawText("Hold F1 or 'h' to show help.", posX, posY, fontSize, RED);
    } else {
        DrawText(" ? ", posX, posY, fontSize, RED);
    }
}

static void drawHelp() {
    int margin = 5;
    int spacing = 5;
    auto textColor = BLACK;
    auto textColorHighlight = RED;
    auto textColorHighlight2 = BLUE;
    Color backgroundColor = {255, 255, 255, 255};
    int w = GetScreenWidth();
    int h = GetScreenHeight();

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
    contents.emplace_back("Use Arrow Up/Down to view in history.");
    contents.emplace_back("Drop multiple files, will clear history.");
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
