#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: 2026 Sectorfive
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assemble a self-contained Windows distribution from a Runa build.
# Must be run from an MSYS2 UCRT64 shell (or a shell with MINGW_PREFIX set).

set -euo pipefail

EXE="${1:?usage: bundle.sh <path-to-runa.exe> <output-dir>}"
DIST="${2:?usage: bundle.sh <path-to-runa.exe> <output-dir>}"
MINGW_PREFIX="${MINGW_PREFIX:-/ucrt64}"
SRC_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

if [[ ! -f "$EXE" ]]; then
    echo "error: executable not found: $EXE" >&2
    exit 1
fi

rm -rf "$DIST"
mkdir -p "$DIST"
cp "$EXE" "$DIST/runa.exe"

# Qt libraries, platform plugins and the QML modules imported by the sources.
windeployqt --release \
    --no-translations \
    --no-system-d3d-compiler \
    --qmldir "$SRC_DIR/src" \
    "$DIST/runa.exe"

# windeployqt writes plugins either next to the executable or into a
# "plugins" subdirectory depending on the Qt version; follow its layout.
if [[ -d "$DIST/plugins/platforms" ]]; then
    PLUGIN_ROOT="$DIST/plugins"
    PLUGIN_REL="plugins"
else
    PLUGIN_ROOT="$DIST"
    PLUGIN_REL="."
fi

if [[ -d "$DIST/qml/QtQuick" ]]; then
    QML_REL="qml"
elif [[ -d "$DIST/share/qt6/qml/QtQuick" ]]; then
    QML_REL="share/qt6/qml"
else
    QML_REL="qml"
fi

# KDE QML modules (Kirigami, the desktop style, KItemModels, ...).
mkdir -p "$DIST/$QML_REL/org"
cp -a "$MINGW_PREFIX/share/qt6/qml/org/kde" "$DIST/$QML_REL/org/"

# Plugins used indirectly through KF6/KIO rather than by the executable
# itself, so windeployqt does not pick them up: the SVG image/icon engines
# (Breeze icons are SVG), the Windows TLS backend and the Qt widget styles.
for plugin_dir in imageformats iconengines tls styles; do
    if [[ -d "$MINGW_PREFIX/share/qt6/plugins/$plugin_dir" ]]; then
        mkdir -p "$PLUGIN_ROOT/$plugin_dir"
        cp -a "$MINGW_PREFIX/share/qt6/plugins/$plugin_dir/." "$PLUGIN_ROOT/$plugin_dir/"
    fi
done

# Only SQLite is used (playlist database); the other drivers need external
# client libraries that are not part of the bundle. windeployqt copies the
# whole directory, so replace it.
rm -rf "$PLUGIN_ROOT/sqldrivers"
mkdir -p "$PLUGIN_ROOT/sqldrivers"
cp "$MINGW_PREFIX/share/qt6/plugins/sqldrivers/qsqlite.dll" "$PLUGIN_ROOT/sqldrivers/"

# KDE plugins: KFileMetaData extractors, KIO workers and the KIconEngine
# plugin. Sonnet's optional spell-check backends are left out: they link
# against aspell, hunspell and voikko, which are not bundled.
for plugin_dir in kf6 kiconthemes6; do
    if [[ -d "$MINGW_PREFIX/share/qt6/plugins/$plugin_dir" ]]; then
        mkdir -p "$PLUGIN_ROOT"
        cp -a "$MINGW_PREFIX/share/qt6/plugins/$plugin_dir" "$PLUGIN_ROOT/"
    fi
done
rm -rf "$PLUGIN_ROOT/kf6/sonnet"

# License texts for the application itself.
mkdir -p "$DIST/LICENSES"
cp -a "$SRC_DIR/LICENSES/." "$DIST/LICENSES/"

# libvlc and its plugin modules.
cp "$MINGW_PREFIX/bin/libvlc.dll" "$MINGW_PREFIX/bin/libvlccore.dll" "$DIST/"
mkdir -p "$DIST/lib/vlc"
cp -a "$MINGW_PREFIX/lib/vlc/plugins" "$DIST/lib/vlc/"

# The SDL image decoder links against the SDL1.2 compatibility layer, which in
# turn needs SDL2 at runtime and shows an error dialog while libvlc probes its
# modules. The remaining decoders cover the formats the player uses.
find "$DIST/lib/vlc/plugins" -iname '*sdl*' -delete

# Software OpenGL fallback: Qt's llvmpipe build runs on machines without a
# usable OpenGL driver (virtual machines, very old GPUs). The application
# restarts itself with QT_OPENGL=software when no 2.0 context is available.
# The DLL is taken from the Qt build shipped in the PySide6 wheel, which is
# self-contained and only needs system libraries.
SW_OPENGL_URL="${SW_OPENGL_URL:-https://files.pythonhosted.org/packages/03/61/9a7e32a37b224f68506785bfb0f288d516098ec71681782d72b695a6c95d/pyside6_essentials-6.12.0-cp310-abi3-win_amd64.whl}"
if [[ -n "${SW_OPENGL_DLL:-}" ]]; then
    cp "$SW_OPENGL_DLL" "$DIST/opengl32sw.dll"
else
    sw_wheel="${TMPDIR:-/tmp}/runa-opengl32sw.whl"
    curl -fL --retry 3 -o "$sw_wheel" "$SW_OPENGL_URL"
    unzip -o -j "$sw_wheel" "PySide6/opengl32sw.dll" -d "$DIST" > /dev/null
    rm -f "$sw_wheel"
fi
if [[ ! -f "$DIST/opengl32sw.dll" ]]; then
    echo "error: could not bundle opengl32sw.dll" >&2
    exit 1
fi

# yt-dlp is used by the YouTube integration. The MSYS2 package is only a
# launcher for the MSYS2 Python installation, so bundle the self-contained
# upstream build instead.
YTDLP_URL="${YTDLP_URL:-https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe}"
if [[ -n "${YTDLP_EXE:-}" ]]; then
    cp "$YTDLP_EXE" "$DIST/yt-dlp.exe"
else
    curl -fL --retry 3 -o "$DIST/yt-dlp.exe" "$YTDLP_URL"
fi
if [[ "$(stat -c %s "$DIST/yt-dlp.exe")" -lt 1000000 ]]; then
    echo "error: bundled yt-dlp.exe looks invalid" >&2
    exit 1
fi

cat > "$DIST/qt.conf" <<EOF
[Paths]
Prefix=.
Plugins=$PLUGIN_REL
Qml2Imports=$QML_REL
EOF

# Copy every non-system DLL the bundle depends on (MSYS2 runtime, KF6,
# FFmpeg, VLC and QML plugin dependencies). The PE import tables are read
# with objdump and resolved breadth-first: much faster than resolving the
# whole dependency graph recursively with ntldd for every file.
declare -A dll_paths=()
while IFS= read -r -d '' dll; do
    base="${dll##*/}"
    key="${base,,}"
    if [[ -z "${dll_paths[$key]:-}" ]]; then
        dll_paths[$key]="$dll"
    fi
done < <(find "$MINGW_PREFIX/bin" "$MINGW_PREFIX/lib" "$MINGW_PREFIX/share/qt6" \
    -type f -iname '*.dll' -print0 2>/dev/null)

queue=()
while IFS= read -r -d '' file; do
    queue+=("$file")
done < <(find "$DIST" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)

index=0
while (( index < ${#queue[@]} )); do
    file="${queue[$index]}"
    index=$((index + 1))
    while IFS= read -r name; do
        key="${name,,}"
        dep="${dll_paths[$key]:-}"
        [[ -n "$dep" ]] || continue
        base="${dep##*/}"
        if [[ ! -f "$DIST/$base" ]]; then
            cp "$dep" "$DIST/$base"
            queue+=("$DIST/$base")
        fi
    done < <(objdump -p "$file" 2>/dev/null | sed -n 's/^[[:space:]]*DLL Name: //p')
done

echo "Windows distribution ready in $DIST"
