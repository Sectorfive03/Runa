#
# SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.16...3.30)

# RunaPlayer 1.0.0 - Hard fork of Haruna 1.9.0 with libmpv -> libvlc migration
# Built against libvlc/VLC 3.0.x (minimum 3.0.16)
project(runaplayer VERSION 1.0.0 LANGUAGES CXX)

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY} CACHE PATH "Extra QML import paths")

set(QT_MIN_VERSION 6.9.0)
set(KF6_MIN_VERSION 6.24.0)

include(FeatureSummary)

set(DBUS_PLATFORMS Linux FreeBSD GNU/kFreeBSD NetBSD OpenBSD)

# -------------------------------
# -- ECM
# -------------------------------
find_package(ECM ${KF6_MIN_VERSION} NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/ecm/"
    DESCRIPTION "Extra modules and scripts for CMake")

if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

    include(KDEInstallDirs)
    include(ECMInstallIcons)
    include(ECMAddAppIcon)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
    include(ECMQtDeclareLoggingCategory)

    include(KDEClangFormat)
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.hpp *.c)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

    include(KDEGitCommitHooks)
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

    include(ECMSetupVersion)
    ecm_setup_version(${PROJECT_VERSION}
        VARIABLE_PREFIX RUNAPLAYER
        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/runaplayer-version.h"
    )
endif()

# -------------------------------
#
# -- Qt6 dependencies
#
# -------------------------------
find_package(Qt6Core ${QT_MIN_VERSION})
set_package_properties(Qt6Core PROPERTIES TYPE REQUIRED)

find_package(Qt6Core5Compat ${QT_MIN_VERSION})
set_package_properties(Qt6Core5Compat PROPERTIES TYPE REQUIRED)

find_package(Qt6Gui ${QT_MIN_VERSION})
set_package_properties(Qt6Gui PROPERTIES TYPE REQUIRED)

find_package(Qt6Qml ${QT_MIN_VERSION})
set_package_properties(Qt6Qml PROPERTIES TYPE REQUIRED)

find_package(Qt6Quick ${QT_MIN_VERSION})
set_package_properties(Qt6Quick PROPERTIES TYPE REQUIRED)

find_package(Qt6QuickControls2 ${QT_MIN_VERSION})
set_package_properties(Qt6QuickControls2 PROPERTIES TYPE REQUIRED)

find_package(Qt6Sql ${QT_MIN_VERSION})
set_package_properties(Qt6Sql PROPERTIES TYPE REQUIRED)

find_package(Qt6ShaderTools ${QT_MIN_VERSION})
set_package_properties(Qt6ShaderTools PROPERTIES TYPE REQUIRED)

set(HAVE_DBUS FALSE)
if (CMAKE_SYSTEM_NAME IN_LIST DBUS_PLATFORMS)
    find_package(Qt6DBus ${QT_MIN_VERSION})
    set_package_properties(Qt6DBus PROPERTIES TYPE REQUIRED)
    set(HAVE_DBUS TRUE)
endif ()

# -------------------------------
#
# -- libvlc
#
# -------------------------------
# libvlc is located through pkg-config on every supported platform. On Windows
# the pkg-config file is provided by the MSYS2 UCRT64 VLC package.
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVLC REQUIRED IMPORTED_TARGET libvlc)
set_package_properties(LIBVLC PROPERTIES TYPE REQUIRED
    URL "https://www.videolan.org/vlc/libvlc.html"
    DESCRIPTION "VLC media player engine")

# -------------------------------
#
# -- KF6 dependencies
#
# -------------------------------
find_package(Breeze)
set_package_properties(Breeze PROPERTIES TYPE RECOMMENDED
    URL "https://api.kde.org/frameworks/breeze-icons/html/index.html")

find_package(KF6ColorScheme ${KF6_MIN_VERSION})
set_package_properties(KF6ColorScheme PROPERTIES TYPE REQUIRED
    URL "https://invent.kde.org/frameworks/kcolorscheme")

find_package(KF6Config ${KF6_MIN_VERSION})
set_package_properties(KF6Config PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfig/html/index.html")

find_package(KF6CoreAddons ${KF6_MIN_VERSION})
set_package_properties(KF6CoreAddons PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kcoreaddons/html/index.html")

find_package(KF6FileMetaData ${KF6_MIN_VERSION})
set_package_properties(KF6FileMetaData PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kfilemetadata/html/index.html")

find_package(KF6I18n ${KF6_MIN_VERSION})
set_package_properties(KF6I18n PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/ki18n/html/index.html")

find_package(KF6IconThemes ${KF6_MIN_VERSION})
set_package_properties(KF6IconThemes PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kiconthemes/html/index.html")

find_package(KF6KIO ${KF6_MIN_VERSION})
set_package_properties(KF6KIO PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kio/html/index.html")

find_package(KF6Kirigami ${KF6_MIN_VERSION})
set_package_properties(KF6Kirigami PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kirigami/html/index.html")

find_package(KF6WindowSystem ${KF6_MIN_VERSION})
set_package_properties(KF6WindowSystem PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kwindowsystem/html/index.html")

find_package(KF6Crash ${KF6_MIN_VERSION})
set_package_properties(KF6Crash PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kcrash/html/index.html")

find_package(KDSingleApplication-qt6)
set_package_properties(KDSingleApplication-qt6 PROPERTIES TYPE REQUIRED
    URL "https://github.com/KDAB/KDSingleApplication"
    DESCRIPTION "helper class for single-instance policy applications")

# -------------------------------
# -- FFmpeg
# -------------------------------
find_package(FFmpeg)
set_package_properties(FFmpeg PROPERTIES TYPE REQUIRED
    URL "https://ffmpeg.org"
    DESCRIPTION "needed for playlist and mpris thumbnails")

# -------------------------------
# -- Ytdlp
# -------------------------------
find_package(Ytdlp)
set_package_properties(Ytdlp PROPERTIES TYPE RUNTIME
    URL "https://github.com/yt-dlp/yt-dlp"
    DESCRIPTION "needed to play videos from youtube and other online sources")

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-runaplayer.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/src/config-runaplayer.h
)

ki18n_install(po)

add_subdirectory(data)
add_subdirectory(src)

# -------------------------------
# -- Packaging metadata
# -------------------------------
# The source project keeps the RunaPlayer name; the Linux package is "Runa".
set(CPACK_PACKAGE_NAME "Runa")
set(CPACK_PACKAGE_VENDOR "Sectorfive")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Media player built with Qt/QML and libvlc")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://runa.sectorfive.win")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Runa-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/GPL-3.0-or-later.txt")
include(CPack)
