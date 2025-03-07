# вова лох
# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Name of the project
project(MySFMLApp)

# ---------------------------------------
# modify if necessary
#   for Linux:
# set(SFML_INCLUDE_DIR /usr/local/include) # Path to SFML headers
# message(STATUS "SFML_DIR: ${SFML_DIR}")
# message(STATUS "SFML_INCLUDE_DIR: ${SFML_INCLUDE_DIR}")
# message(STATUS "SFML_LIBRARIES: ${SFML_LIBRARIES}")
#   for MacOS:
# include_directories(/opt/homebrew/include)
# ---------------------------------------


# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set( CLIENT_FILES
    ./network_example/client_tcp.cpp
)

set( SERVER_FILES
    ./network_example/server_tcp.cpp
)

# Find SFML library
find_package(SFML COMPONENTS Graphics Window System Network REQUIRED)

# Add executable
add_executable(client ./network_example/client_tcp.cpp)
add_executable(server ./network_example/server_tcp.cpp)

add_library(lib_server STATIC ${SERVER_FILES})
add_library(lib_client STATIC ${CLIENT_FILES})

# Link SFML libraries
target_link_libraries(client lib_client sfml-graphics sfml-window sfml-system sfml-network)
target_link_libraries(server lib_server sfml-graphics sfml-window sfml-system sfml-network)

#cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
