cmake_minimum_required(VERSION 3.25)
project(View360)
set(CMAKE_CXX_STANDARD 17)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 2)
configure_file(
        "${PROJECT_SOURCE_DIR}/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

if (MSVC)
    set(TarType)
    set(CMAKE_PREFIX_PATH "D:\\dev") # only useful on my computer
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /W4")
    set(app_icon "${PROJECT_SOURCE_DIR}/res/win/logo.ico")
elseif (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    set(TarType MACOSX_BUNDLE)
    set(app_icon "${PROJECT_SOURCE_DIR}/res/macos/logo.icns")
    set_source_files_properties(${app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif ()

file(GLOB_RECURSE srcs src/*)

if (MSVC)
    file(GLOB_RECURSE ress res/win/*)
elseif (APPLE)
    file(GLOB_RECURSE ress res/macos/*)
endif ()

add_executable(View360 ${TarType} ${srcs} ${ress} ${app_icon})

target_include_directories(View360 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(View360 PRIVATE PLATFORM_DESKTOP GRAPHICS_API_OPENGL_33)

find_package(raylib 4.0 REQUIRED)
target_include_directories(View360 PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(View360 PRIVATE ${raylib_LIBRARIES})

if (MSVC)
    target_link_libraries(View360 PRIVATE winmm)
else ()
    target_link_libraries(View360 PRIVATE "-framework IOKit")
    target_link_libraries(View360 PRIVATE "-framework Cocoa")
    target_link_libraries(View360 PRIVATE "-framework OpenGL")
    set_target_properties(View360 PROPERTIES
            BUNDLE True
            MACOSX_BUNDLE_GUI_IDENTIFIER dai.domain.style.identifier.View360
            MACOSX_BUNDLE_BUNDLE_NAME View360
            MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
            MACOSX_BUNDLE_ICON_FILE logo
            MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/res/macos/Info.plist
            )
endif ()
