//
// Created by daiyan on 2023/5/19.
//

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <shlobj.h>
#include <windows.h>
#pragma comment(lib, "shell32.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static bool create_directory_if_not_exist(const std::string &path) {
    try {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir) && !std::filesystem::create_directories(dir)) {
            std::cerr << "Failed to create directory: " << path << std::endl;
            return false;
        }
    } catch (std::exception &e) {
        std::cerr << "Create directory: ";
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

extern std::string get_config_path(const std::string &appName) {
    std::string configDir;

#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        configDir = std::string(path) + "\\" + appName + "\\";
    }

#elif defined(__linux__)
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) home = pwd->pw_dir;
    }

    if (home) {
        configDir = std::string(home) + "/.config/" + appName + "/";
    }

#elif defined(__APPLE__)
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) home = pwd->pw_dir;
    }

    if (home) {
        configDir = std::string(home) + "/Library/Application Support/" + appName + "/";
    }
#endif

    create_directory_if_not_exist(configDir);

    return configDir;
}
