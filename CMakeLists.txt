cmake_minimum_required(VERSION 3.16)

project(Amble VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Amble
    src/Main.cpp
)

qt_add_qml_module(Amble
    URI Amble
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

set_target_properties(Amble PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(Amble
    PRIVATE Qt6::Quick
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


