cmake_minimum_required(VERSION 3.10)
project(MeetAssistPart1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify vcpkg toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Find required packages
find_package(nlohmann_json CONFIG REQUIRED)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add source files
set(SOURCES 
    src/main.cpp
    src/auth/auth.cpp
    src/ui/signup_panel.cpp
    src/ui/login_panel.cpp
    src/services/email_service.cpp
    src/services/payment_service.cpp
    src/services/location_service.cpp
)

# Define header directories
set(INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/auth
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/services
)

# Add executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    user32
    gdi32
    d3d11
    dxgi
    dwmapi
    gdiplus
    shell32
    comctl32
    winhttp
    crypt32
    ws2_32
    iphlpapi
    nlohmann_json::nlohmann_json
)

# Add preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
)

# MSVC specific settings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4     # Warning level 4
        /MP     # Multi-processor compilation
        /EHsc   # Standard C++ exception handling
        /wd4100 # Unreferenced formal parameter
        /wd4244 # Conversion warnings
        /wd4312 # Type conversion warnings
    )
endif()

# Set startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Enable folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})