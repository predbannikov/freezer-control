cmake_minimum_required(VERSION 3.16)

project(freezer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(freezer
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    hidworker.h
    hidworker.cpp
    temperaturelogger.h temperaturelogger.cpp
)

message(STATUS "_VCPKG_INSTALLED_DIR = ${_VCPKG_INSTALLED_DIR}")
message(STATUS "VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(unofficial-qwt CONFIG REQUIRED)

# # WORKAROUND for vcpkg Qwt: remove bad //include path
# get_target_property(_qwt_includes unofficial::qwt::qwt INTERFACE_INCLUDE_DIRECTORIES)
# list(REMOVE_ITEM _qwt_includes "//include")
# set_target_properties(unofficial::qwt::qwt PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_qwt_includes}")

find_package(hidapi REQUIRED)

target_compile_definitions(freezer PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(freezer
    PRIVATE
        unofficial::qwt::qwt
        hidapi::hidapi
        Qt6::Widgets
)

target_include_directories(freezer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
