cmake_minimum_required(VERSION 3.16)

project(InertialNavigation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Звук дополняет демонстрацию, но не мешает сборке с одним Qt Widgets.
option(INS_ENABLE_SOUND "Enable correction sound when Qt Multimedia is available" ON)
if(INS_ENABLE_SOUND)
    find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Multimedia)
endif()

add_executable(InertialNavigation
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    simulationmodel.cpp
    simulationmodel.h
    navigationview.cpp
    navigationview.h
    errorchart.cpp
    errorchart.h
    resources.qrc
)

target_link_libraries(InertialNavigation PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(INS_ENABLE_SOUND AND TARGET Qt${QT_VERSION_MAJOR}::Multimedia)
    target_link_libraries(InertialNavigation PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
    target_compile_definitions(InertialNavigation PRIVATE INS_WITH_SOUND)
else()
    message(STATUS "Correction sound disabled; chart and animation remain available")
endif()

set_target_properties(InertialNavigation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(MSVC)
    target_compile_options(InertialNavigation PRIVATE /utf-8)
endif()
