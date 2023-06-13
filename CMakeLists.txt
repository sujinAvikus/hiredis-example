cmake_minimum_required(VERSION 3.16)
project(HiredisProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
message("==> Using FetchContent")

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

# Find the hiredis package using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

# Include hiredis headers
include_directories(${HIREDIS_INCLUDE_DIRS})

# Add your main.cpp file
add_executable(MyProject main.cpp)

target_compile_options(MyProject PUBLIC ${HIREDIS_CFLAGS})

# Link against the hiredis library
target_link_libraries(MyProject ${HIREDIS_LIBRARIES} nlohmann_json::nlohmann_json)
