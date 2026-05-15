cmake_minimum_required(VERSION 3.21)
project(PrimordialParticleSystem LANGUAGES CXX)

# ── Language standard ─────────────────────────────────────────────────────────
# Tell CMake to use C++23. MSVC will receive /std:c++latest automatically.
# We set EXTENSIONS OFF to avoid compiler-specific GNU extensions.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generates compile_commands.json — useful for clangd / IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies (FetchContent) ───────────────────────────────────────────────
# FetchContent downloads and builds dependencies at configure time.
# Order matters: ImGui must be available before ImGui-SFML and ImPlot,
# because both need to find imgui headers.

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui)
FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_DIR)
set(IMGUI_SFML_FIND_SFML OFF)

FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG v3.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui-SFML)


FetchContent_Declare(ImPlot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        v0.17
)
FetchContent_MakeAvailable(ImPlot)


# Disable SFML fetching its own system deps (freetype etc.) — we want static
set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE)

# Suppress a CMake policy warning from older dependency CMakeLists files
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# ImGui-SFML needs to know where imgui source lives before it configures
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)


# ── Executable sources ────────────────────────────────────────────────────────
# Listed explicitly (no glob) so CMake always detects added/removed files.
add_executable(PrimordialParticleSystem
    # ── Entry point
    src/main.cpp
  "src/particle_system/particle_system.cpp" "src/simulation/simulation.cpp" "src/particle_system/state.h" 
  "src/simulation/events.cpp" "src/utils/stop_watch.h"

  # ── ImPlot (header-only library — sources must be compiled as part of our target)
   ${implot_SOURCE_DIR}/implot.cpp
   ${implot_SOURCE_DIR}/implot_items.cpp
 "src/particle_system/heatmap.h")


# ── Include directories ───────────────────────────────────────────────────────
# PRIVATE means these paths are only used when compiling PrimordialParticleSystem itself,
# not exposed to anything that links against it.
target_include_directories(PrimordialParticleSystem PRIVATE
    src                          # allows #include "settings.h" etc. from anywhere
    ${implot_SOURCE_DIR}         # allows #include <implot.h>
)

# ── Linked libraries ──────────────────────────────────────────────────────────
target_link_libraries(PrimordialParticleSystem PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    ImGui-SFML::ImGui-SFML
    $<$<CXX_COMPILER_ID:MSVC>:dbghelp>   # MSVC only — stack trace support
)

# ── Preprocessor definitions ──────────────────────────────────────────────────
target_compile_definitions(PrimordialParticleSystem PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"   # lets code reference assets by absolute path
    IMGUI_DEFINE_MATH_OPERATORS          # required by ImPlot: enables ImVec2 +/-/* operators
    $<$<BOOL:${SFML_STATIC_LIBRARIES}>:SFML_STATIC>
)

set_source_files_properties(
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# ── MSVC-specific compiler options ───────────────────────────────────────────
if(MSVC)
    # Remove the /EHsc that CMake injects by default before we add our own,
    # otherwise both /EHsc and /EHa end up on the command line causing D9025.
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    target_compile_options(PrimordialParticleSystem PRIVATE
        /Zc:__cplusplus      # makes MSVC report the real __cplusplus value, not 199711L
        /EHa                 # enable SEH exceptions (needed for some SFML/Windows interop)
        /FS                  # force synchronous PDB writes (required for parallel builds)
        /W3                  # warning level 3 (reasonable default)
        $<$<CONFIG:Debug>:/Zi>           # full debug info in Debug builds
        $<$<CONFIG:RelWithDebInfo>:/Zi>  # also in RelWithDebInfo
        $<$<CONFIG:Release>:/Z7>         # embed debug info in .obj for Release (no .pdb)
    )
endif()