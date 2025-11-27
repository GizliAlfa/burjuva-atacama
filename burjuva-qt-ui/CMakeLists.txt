cmake_minimum_required(VERSION 3.16)
project(burjuva-ui VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt packages
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    Charts
)

# Auto-generated files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/serialcontroller.cpp
    src/moduledetector.cpp
    src/io16widget.cpp
    src/aio20widget.cpp
    src/io16group.cpp
    src/aio20channel.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/serialcontroller.h
    src/moduledetector.h
    src/io16widget.h
    src/aio20widget.h
    src/io16group.h
    src/aio20channel.h
    src/moduletypes.h
)

# Executable
add_executable(burjuva-ui
    ${SOURCES}
    ${HEADERS}
)

# Link Qt libraries
target_link_libraries(burjuva-ui
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::Charts
)

# Install
install(TARGETS burjuva-ui
    RUNTIME DESTINATION bin
)

# Build info
message(STATUS "===========================================")
message(STATUS "Burjuva Atacama Control Panel")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "===========================================")
