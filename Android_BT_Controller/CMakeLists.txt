cmake_minimum_required(VERSION 3.20)
project(AndroidBTController LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Bluetooth)

add_executable(AndroidBTController
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/bluetoothclient.h
    src/bluetoothclient.cpp
    src/gamepadwidget.h
    src/gamepadwidget.cpp
)

target_link_libraries(AndroidBTController PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Bluetooth
)

