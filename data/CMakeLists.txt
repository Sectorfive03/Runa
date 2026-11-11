#
# SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(ICONS_FOLDER ${PROJECT_SOURCE_DIR}/data/icons)
set(ICON_FILES
    ${ICONS_FOLDER}/16-apps-runa.png
    ${ICONS_FOLDER}/22-apps-runa.png
    ${ICONS_FOLDER}/32-apps-runa.png
    ${ICONS_FOLDER}/44-apps-runa.png
    ${ICONS_FOLDER}/48-apps-runa.png
    ${ICONS_FOLDER}/64-apps-runa.png
    ${ICONS_FOLDER}/96-apps-runa.png
    ${ICONS_FOLDER}/128-apps-runa.png
    ${ICONS_FOLDER}/150-apps-runa.png
    ${ICONS_FOLDER}/256-apps-runa.png
    ${ICONS_FOLDER}/310-apps-runa.png
    ${ICONS_FOLDER}/512-apps-runa.png
)

# used in src/CMakeLists.txt
set(DATA_ICONS ${ICON_FILES} PARENT_SCOPE)

ecm_install_icons(
    ICONS ${ICON_FILES}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

install(FILES win.sectorfive.runaplayer.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES win.sectorfive.runaplayer.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})