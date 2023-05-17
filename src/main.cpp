#include <fstream>
#include <locale>

#include "app.h"
#include "config.h"

#if !defined(_DEBUG) && defined(WIN32)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int /*argc*/, char ** /*argv*/) {
    // set locale for chinese
    std::locale::global(std::locale("zh_CN.UTF-8"));

#ifdef _DEBUG
    SetTraceLogLevel(LOG_INFO);
#else
    SetTraceLogLevel(LOG_WARNING);
#endif

    {
        loadConfig();
        App app;
        app.run();
        saveConfig();
    }


    return 0;
}
