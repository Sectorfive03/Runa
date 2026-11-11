#
# SPDX-FileCopyrightText: 2026 George Florea Bănuș <georgefb899@gmail.com>
# SPDX-FileCopyrightText: 2026 Muhammet Sadık Uğursoy <sadikugursoy@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

qt_add_library(transcript STATIC)
add_library(Runaplayer::Transcript ALIAS transcript)
qt_add_qml_module(transcript
    URI win.sectorfive.runaplayer.transcript
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/win/sectorfive/runaplayer/transcript
    IMPORT_PATH ${CMAKE_BINARY_DIR}
    QML_FILES
        Transcript.qml
        TranscriptItemDelegate.qml
    SOURCES
        subtitleline.h
        subtitleparser.h
        subtitleparser.cpp
        transcriptmodel.h
        transcriptmodel.cpp
        transcriptproxymodel.h
        transcriptproxymodel.cpp
)

target_include_directories(transcript
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(transcript PRIVATE
    Qt6::Core
    Qt6::Gui

    KF6::FileMetaData
    KF6::I18n

    FFmpeg::AVCODEC
    FFmpeg::AVFORMAT

    utilitiesplugin
)
