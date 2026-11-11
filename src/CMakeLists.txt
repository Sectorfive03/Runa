#
# SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

# DATA_ICONS is defined in data/CMakeLists.txt
ecm_add_app_icon(ICONS_SRCS ICONS ${DATA_ICONS})

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(runaplayer)

# The visible product name is "Runa"; the CMake target keeps the project name.
set_target_properties(runaplayer PROPERTIES OUTPUT_NAME runa)

# Build a GUI executable on Windows (no console window).
if (WIN32)
    set_target_properties(runaplayer PROPERTIES WIN32_EXECUTABLE TRUE)
endif ()

target_sources(runaplayer
    PRIVATE
    application.h application.cpp
    lockmanager.h lockmanager.cpp
    main.cpp
    models/actionsmodel.h models/actionsmodel.cpp
    models/chaptersmodel.h models/chaptersmodel.cpp
    models/fontsmodel.h models/fontsmodel.cpp
    models/mouseactionsmodel.h models/mouseactionsmodel.cpp
    models/recentfilesmodel.h models/recentfilesmodel.cpp
    models/subtitlesfoldersmodel.h models/subtitlesfoldersmodel.cpp
    models/tracksmodel.h models/tracksmodel.cpp
    vlc/vlcengine.h vlc/vlcengine.cpp
    vlc/vlcitem.h vlc/vlcitem.cpp
    vlc/seekablerangesmodel.h vlc/seekablerangesmodel.cpp
    mprisitem.h mprisitem.cpp
    thumbnailimageprovider.h thumbnailimageprovider.cpp
    worker.h worker.cpp
    ${ICONS_SRCS}
)

ecm_qt_declare_logging_category(runaplayer
    HEADER "logging/_general.h"
    IDENTIFIER "RunaplayerGeneral"
    CATEGORY_NAME "win.sectorfive.runaplayer.general"
    DEFAULT_SEVERITY Info
)

qt_add_resources(runaplayer color_schemes
    PREFIX "/color-schemes"
    BASE "../data/color-schemes"
    FILES
        ../data/color-schemes/RunaPlayerLight.colors
        ../data/color-schemes/RunaPlayerDark.colors
)

qt_add_resources(runaplayer embedded_icons
    PREFIX "/icons"
    BASE "../data/icons"
    FILES
        ../data/icons/16-apps-runa.png
        ../data/icons/22-apps-runa.png
        ../data/icons/32-apps-runa.png
        ../data/icons/44-apps-runa.png
        ../data/icons/48-apps-runa.png
        ../data/icons/64-apps-runa.png
        ../data/icons/96-apps-runa.png
        ../data/icons/128-apps-runa.png
        ../data/icons/150-apps-runa.png
        ../data/icons/256-apps-runa.png
        ../data/icons/310-apps-runa.png
        ../data/icons/512-apps-runa.png
)

qt6_add_shaders(runaplayer
    URI win.sectorfive.runaplayer
    VERSION 1.0
    PREFIX
        "/"
    FILES
        shader/colorfill.frag
)

if (CMAKE_SYSTEM_NAME IN_LIST DBUS_PLATFORMS)
    qt_add_dbus_interface(runaplayer_screensaver_inhibit_SRCS ../org.freedesktop.ScreenSaver.xml screensaverdbusinterface)
    target_sources(runaplayer
        PRIVATE
        ${runaplayer_screensaver_inhibit_SRCS}
    )
endif()

add_subdirectory(Database)
add_subdirectory(Playlist)
add_subdirectory(Settings)
add_subdirectory(Transcript)
add_subdirectory(Utilities)
add_subdirectory(YouTube)
add_subdirectory(framedecoder)

set_source_files_properties(qml/Models.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(runaplayer
    URI win.sectorfive.runaplayer
    VERSION 1.0
    QML_FILES
        qml/Actions.qml
        qml/Footer.qml
        qml/HProgressBar.qml
        qml/MenuBarLoader.qml
        qml/VlcVideo.qml
        qml/Osd.qml
        qml/VolumeSlider.qml
        qml/Main.qml
        qml/Models.qml

        qml/Runaplayer/Components/AudioTracksMenu.qml
        qml/Runaplayer/Components/ColorPickerButton.qml
        qml/Runaplayer/Components/HamburgerMenu.qml
        qml/Runaplayer/Components/ImageAdjustmentSlider.qml
        qml/Runaplayer/Components/InputPopup.qml
        qml/Runaplayer/Components/LabelWithTooltip.qml
        qml/Runaplayer/Components/ResizeablePage.qml
        qml/Runaplayer/Components/ResizeHandler.qml
        qml/Runaplayer/Components/SelectActionPopup.qml
        qml/Runaplayer/Components/SubtitlesFolders.qml
        qml/Runaplayer/Components/SubtitleTracksMenu.qml
        qml/Runaplayer/Components/ToolTipButton.qml

        qml/Menus/AudioMenu.qml
        qml/Menus/PlayerContextMenu.qml
        qml/Menus/FileMenu.qml
        qml/Menus/HelpMenu.qml
        qml/Menus/PlaybackMenu.qml
        qml/Menus/SubtitlesMenu.qml
        qml/Menus/VideoMenu.qml
        qml/Menus/ViewMenu.qml
        qml/Menus/Global/GlobalAudioMenu.qml
        qml/Menus/Global/GlobalFileMenu.qml
        qml/Menus/Global/GlobalHelpMenu.qml
        qml/Menus/Global/GlobalPlaybackMenu.qml
        qml/Menus/Global/GlobalSubtitlesMenu.qml
        qml/Menus/Global/GlobalVideoMenu.qml
        qml/Menus/Global/GlobalViewMenu.qml


    DEPENDENCIES
        win.sectorfive.runaplayer.playlist

    RESOURCES
        ${COMPILED_SHADERS}
)

target_include_directories(runaplayer
    PRIVATE
        models
        vlc
        # generated config-runaplayer.h (HAVE_DBUS)
        ${CMAKE_CURRENT_BINARY_DIR}
        ${FFMPEG_INCLUDE_DIRS}
)

target_compile_definitions(runaplayer PRIVATE $<$<CONFIG:Debug>:QT_QML_DEBUG>)

target_link_libraries(runaplayer PRIVATE
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::ShaderTools

    KF6::ColorScheme
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::FileMetaData
    KF6::I18n
    KF6::I18nQml
    KF6::IconThemes
    KF6::KIOWidgets
    KF6::Kirigami
    KF6::WindowSystem
    KF6::Crash

    KDAB::kdsingleapplication

    PkgConfig::LIBVLC

    Runaplayer::Database
    Runaplayer::FrameDecoder
    Runaplayer::Playlist
    Runaplayer::Settings
    Runaplayer::Transcript
    Runaplayer::Utilities
    Runaplayer::YouTube
)

if (CMAKE_SYSTEM_NAME IN_LIST DBUS_PLATFORMS)
    add_subdirectory(mpris)
    target_link_libraries(runaplayer
        PRIVATE
            Runaplayer::Mpris
    )
endif()
install(TARGETS runaplayer ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
