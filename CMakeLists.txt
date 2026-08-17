cmake_minimum_required(VERSION 3.20)

project(
    DriverLevelInputSimulator
    VERSION 0.1.0
    DESCRIPTION "Controller for a Windows virtual HID input driver"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(app)