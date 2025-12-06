cmake_minimum_required(VERSION 3.16)

project(Amble VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick OpenGL QuickControls2 Widgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Amble
    src/main.cpp
)

set_target_properties(Amble PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

qt_add_qml_module(Amble
    URI Amble
    VERSION 1.0
    SOURCES
        src/CircuitViewport.cpp
        src/CircuitViewport.h
    QML_FILES
        qml/Main.qml
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Amble
)

# Add C++ sources separately
# target_sources(Amble PRIVATE
#     src/CircuitViewport.cpp
#     src/CircuitViewport.h
# )


# set_target_properties(Amble PROPERTIES
#     MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#     MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#     MACOSX_BUNDLE TRUE
#     WIN32_EXECUTABLE TRUE
# )

target_include_directories(Amble PRIVATE src)

target_link_libraries(Amble PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::OpenGL
        Qt6::Widgets
        Qt6::Qml
        Qt6::QuickControls2
)

# Installation configuration
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")

install(TARGETS Amble
    RUNTIME DESTINATION bin
)

qt_generate_deploy_qml_app_script(
    TARGET Amble
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)

install(SCRIPT ${deploy_script})


