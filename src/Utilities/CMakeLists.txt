#
# SPDX-FileCopyrightText: 2025 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

qt_add_library(utilities STATIC)
add_library(Runaplayer::Utilities ALIAS utilities)
qt_add_qml_module(utilities
    URI win.sectorfive.runaplayer.utilities
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/win/sectorfive/runaplayer/utilities
    IMPORT_PATH ${CMAKE_BINARY_DIR}
    SOURCES
        miscutils.h
        miscutils.cpp
        pathutils.h
        pathutils.cpp
        systemutils.h
        systemutils.cpp
        commandlineoptions.h
        commandlineoptions.cpp
        osdutils.h
        osdutils.cpp
)

target_link_libraries(utilities PRIVATE
    Qt6::Core
    Qt6::Gui

    KF6::ConfigCore
    KF6::ConfigGui
    KF6::FileMetaData
    KF6::I18n
    KF6::KIOWidgets
    KF6::WindowSystem

    Runaplayer::Settings
)