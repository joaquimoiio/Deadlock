cmake_minimum_required(VERSION 3.20)
project(DeadUnlock VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DWINDOWS)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/nlohmann
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/memory.cpp
    src/core/offsets.cpp
    src/features/aimbot.cpp
    src/features/esp.cpp
    src/features/radar.cpp
    src/math/geometry.cpp
    src/render/overlay.cpp
    src/render/imgui_menu.cpp
)

# ImGui sources
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/backends/imgui_impl_dx11.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/backends/imgui_impl_win32.cpp
)

# Executable
add_executable(DeadUnlock WIN32 ${SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(DeadUnlock
    d3d11
    d3dcompiler
    dxgi
    winmm
    psapi
    user32
    gdi32
)

# Windows subsystem
set_target_properties(DeadUnlock PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)