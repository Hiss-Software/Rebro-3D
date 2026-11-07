cmake_minimum_required(VERSION 3.20)
project(Rebro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(assimp REQUIRED)

# ── Sources ──
add_executable(Rebro WIN32
    src/main.cpp
    src/SceneObject.h
    src/MeshData.h
    src/MeshData.cpp
    src/MeshImporter.h
    src/MeshImporter.cpp
    src/MeshExporter.h
    src/MeshExporter.cpp
    src/SceneDocument.h
    src/SceneDocument.cpp
    src/UndoStack.h
    src/MainWindow.h
    src/MainWindow.cpp
    src/ViewportWidget.h
    src/ViewportWidget.cpp
    src/MeshRay.h
    src/MeshRay.cpp
    src/ViewportShaders.h
    src/ViewportHelpers.h
    src/ViewportCommands.cpp
    src/ViewportPicking.cpp
    src/MainWindowHelpers.h
    src/MainWindowActions.cpp
    src/MainWindowPanels.cpp
    src/SettingsDialog.cpp
)

# Windows resource file (icon, version info)
if(WIN32)
    target_sources(Rebro PRIVATE src/app.rc)
endif()

target_include_directories(Rebro PRIVATE src)

# ── Dependencies ──
# GLM (assume it's in third_party/glm as a header-only lib)
add_library(glm_headers INTERFACE)
target_include_directories(glm_headers INTERFACE third_party/glm)

target_link_libraries(Rebro PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    assimp::assimp
    glm_headers
)

# ── Compiler flags ──
if(MSVC)
    target_compile_options(Rebro PRIVATE /W4 /O2 /utf-8)
else()
    target_compile_options(Rebro PRIVATE -O2 -Wall -Wextra)
endif()

if(MINGW)
    target_compile_options(Rebro PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# ── Dist: exe + ALL DLLs (Windows only) ──
if(WIN32)
    set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)
    set_target_properties(Rebro PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})

    # Auto-detect MinGW prefix (MSYS2 env var) or fail with a clear message
    if(NOT DEFINED MINGW_PREFIX OR MINGW_PREFIX STREQUAL "")
        if(DEFINED ENV{MINGW_PREFIX})
            set(MINGW_PREFIX "$ENV{MINGW_PREFIX}")
        elseif(MINGW)
            get_filename_component(_epi_cxx_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            get_filename_component(_epi_maybe_prefix "${_epi_cxx_dir}" DIRECTORY)
            if(EXISTS "${_epi_maybe_prefix}/bin")
                set(MINGW_PREFIX "${_epi_maybe_prefix}")
            endif()
        endif()
    endif()

    if(NOT DEFINED MINGW_PREFIX OR MINGW_PREFIX STREQUAL "")
        message(FATAL_ERROR
            "Rebro: MINGW_PREFIX is not set.\n"
            "Run from MSYS2 MinGW shell (which sets MINGW_PREFIX),\n"
            "or pass -DMINGW_PREFIX=C:/path/to/msys64/mingw64")
    endif()

    set(MINGW_BIN "${MINGW_PREFIX}/bin")

    set(REBRO_DLLS
        libassimp-6.dll
        libminizip-1.dll
        zlib1.dll
        libgcc_s_seh-1.dll
        libstdc++-6.dll
        libwinpthread-1.dll
        libfreetype-6.dll
        libmd4c.dll
        libpng16-16.dll
        libharfbuzz-0.dll
        libb2-1.dll
        libdouble-conversion.dll
        libicuin78.dll
        libicuuc78.dll
        libicudt78.dll
        libpcre2-16-0.dll
        libpcre2-8-0.dll
        libzstd.dll
        libbz2-1.dll
        libbrotlidec.dll
        libbrotlicommon.dll
        libglib-2.0-0.dll
        libgraphite2.dll
        libintl-8.dll
        libiconv-2.dll
    )

    add_custom_command(TARGET Rebro POST_BUILD
        COMMAND Qt6::windeployqt --dir ${DIST_DIR}
                                 --compiler-runtime
                                 --no-translations
                                 $<TARGET_FILE:Rebro>
        COMMENT "Deploying dist: exe + DLLs + Qt plugins"
    )

    foreach(DLL ${REBRO_DLLS})
        add_custom_command(TARGET Rebro POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${MINGW_BIN}/${DLL} ${DIST_DIR})
    endforeach()
endif()