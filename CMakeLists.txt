cmake_minimum_required(VERSION 3.16)
project(WorkforceMonitoringAgent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

# Optional BCC support for eBPF
find_package(PkgConfig QUIET)
pkg_check_modules(BCC QUIET bcc)
if(BCC_FOUND)
    add_definitions(-DUSE_BCC)
    include_directories(${BCC_INCLUDE_DIRS})
    link_directories(${BCC_LIBRARY_DIRS})
endif()

# Find cURL
find_package(CURL REQUIRED)
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# Check for nlohmann/json (header-only library)
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
if(NLOHMANN_JSON_INCLUDE_DIR)
    include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    add_definitions(-DHAS_NLOHMANN_JSON)
endif()

# Include directories
include_directories(include)
include_directories(${LIBEVDEV_INCLUDE_DIRS})
include_directories(${WAYLAND_CLIENT_INCLUDE_DIRS})
include_directories(${WAYLAND_PROTOCOLS_INCLUDE_DIRS})
include_directories(/usr/include/libevdev-1.0)

# Source files
set(SOURCES
    src/agent/main.cpp
    src/agent/activity_monitor.cpp
    src/agent/dlp_monitor.cpp
    src/agent/behavior_analyzer.cpp
    src/agent/llm_behavior_analyzer.cpp
    src/agent/time_tracker.cpp
    src/agent/upgrade_manager.cpp
)

# Create executable
add_executable(wm-agent ${SOURCES})

# Link libraries
target_link_libraries(wm-agent
    ${LIBEVDEV_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_PROTOCOLS_LIBRARIES}
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
    ssl
    crypto
)

# Install target
install(TARGETS wm-agent DESTINATION bin)
