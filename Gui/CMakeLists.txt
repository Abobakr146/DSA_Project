cmake_minimum_required(VERSION 3.16)
project(MineProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt
find_package(Qt6 6.10.1 REQUIRED COMPONENTS Widgets)

# Enable Qt specific features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add source files WITH WIN32 FLAG
add_executable(MineProject WIN32
    main.cpp
    mainwindow.cpp
    functions.cpp
    utils.cpp
)

# Link against Qt libraries
target_link_libraries(MineProject PRIVATE Qt6::Widgets)