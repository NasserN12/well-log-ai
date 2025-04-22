cmake_minimum_required(VERSION 3.10)
project(WellLogAnalyzer VERSION 0.1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Include directories
include_directories(include ${CURL_INCLUDE_DIRS})

# Add source files
set(SOURCES 
    src/main.cpp
    src/well_log.cpp
    src/ai_integration.cpp
)

# Create executable
add_executable(well_log_analyzer ${SOURCES})

# Link libraries
target_link_libraries(well_log_analyzer PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)