cmake_minimum_required(VERSION 3.16)

project(q VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Find qtermwidget (Qt6 version preferred)
find_package(qtermwidget6 QUIET)
if(NOT qtermwidget6_FOUND)
    message(STATUS "qtermwidget6 not found, trying qtermwidget5")
    find_package(qtermwidget5 QUIET)
    if(NOT qtermwidget5_FOUND)
        message(FATAL_ERROR "qtermwidget not found. Please install qtermwidget5 or qtermwidget6")
    endif()
endif()

# Find R
find_program(R_EXECUTABLE R)
if(NOT R_EXECUTABLE)
    message(FATAL_ERROR "R not found. Please install R.")
endif()

execute_process(
    COMMAND ${R_EXECUTABLE} CMD config --ldflags
    OUTPUT_VARIABLE R_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${R_EXECUTABLE} CMD config --cppflags
    OUTPUT_VARIABLE R_CPPFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/codeeditor.cpp
    src/codeeditor.h
    src/rsyntaxhighlighter.cpp
    src/rsyntaxhighlighter.h
    src/filebrowser.cpp
    src/filebrowser.h
    src/thememanager.cpp
    src/thememanager.h
    src/terminalwidget.cpp
    src/terminalwidget.h
    src/environmentpane.cpp
    src/environmentpane.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Qt libraries and qtermwidget
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# Link qtermwidget
if(qtermwidget6_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE qtermwidget6)
    target_include_directories(${PROJECT_NAME} PRIVATE ${qtermwidget6_INCLUDE_DIRS})
elseif(qtermwidget5_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE qtermwidget5)
    target_include_directories(${PROJECT_NAME} PRIVATE ${qtermwidget5_INCLUDE_DIRS})
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install themes directory
install(DIRECTORY themes
    DESTINATION share/${PROJECT_NAME}
)
