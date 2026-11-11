/*
 * SPDX-FileCopyrightText: 2026 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TRANSLATION_DOMAIN "runaplayer"

#include "osdutils.h"

#include <QQmlEngine>

#include <KLocalizedString>

#include "osdsettings.h"

using namespace Qt::StringLiterals;

using Function = bool (*)();

// clang-format off
QHash<OsdUtils::Context, Function> functions()
{
    static const QHash<OsdUtils::Context, Function> _functions = {
        { OsdUtils::Context::ShowABLoop,           &OsdSettings::showABLoop },
        { OsdUtils::Context::ShowChapter,          &OsdSettings::showChapter },
        { OsdUtils::Context::ShowDeinterlace,      &OsdSettings::showDeinterlace },
        { OsdUtils::Context::ShowImageAdjustments, &OsdSettings::showImageAdjustments },
        { OsdUtils::Context::ShowLoadFile,         &OsdSettings::showLoadFile },
        { OsdUtils::Context::ShowPlaybackSpeed,    &OsdSettings::showPlaybackSpeed },
        { OsdUtils::Context::ShowScreenshot,       &OsdSettings::showScreenshot },
        { OsdUtils::Context::ShowSubtitleDelay,    &OsdSettings::showSubtitleDelay },
        { OsdUtils::Context::ShowSubtitleToggle,   &OsdSettings::showSubtitleToggle },
        { OsdUtils::Context::ShowTrack,            &OsdSettings::showTrack },
        { OsdUtils::Context::ShowVolume,           &OsdSettings::showVolume },
    };

    return _functions;
}

OsdUtils &OsdUtils::instance()
{
    static OsdUtils i;
    return i;
}

OsdUtils *OsdUtils::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

OsdUtils::OsdUtils(QObject *parent)
    : QObject{parent}
{
}

void OsdUtils::message(const QString &message, Context context)
{
    if (context == OsdUtils::Context::None) {
        Q_EMIT messageReceived(message, context);
        return;
    }

    const auto f = functions();
    if (auto it = f.constFind(context); it != f.cend()) {
        if (it.value()()) {
            Q_EMIT messageReceived(message, context);
        }
    }
}

QList<OsdOption> OsdUtils::options()
{
    static const QList<OsdOption> options = {
        { u"showABLoop"_s,           i18nc("@option:check", "AB loop") },
        { u"showImageAdjustments"_s, i18nc("@option:check", "Image adjustments (contrast, brightness, gamma and saturation)") },
        { u"showPlaybackSpeed"_s,    i18nc("@option:check", "Playback speed change") },
        { u"showLoadFile"_s,         i18nc("@option:check", "Playing new file") },
        { u"showChapter"_s,          i18nc("@option:check", "Skip chapter") },
        { u"showSubtitleDelay"_s,    i18nc("@option:check", "Subtitle timing change") },
        { u"showScreenshot"_s,       i18nc("@option:check", "Taking screenshot") },
        { u"showDeinterlace"_s,      i18nc("@option:check", "Toggle deinterlace") },
        { u"showSubtitleToggle"_s,   i18nc("@option:check", "Toggle subtitle") },
        { u"showTrack"_s,            i18nc("@option:check", "Track change") },
        { u"showVolume"_s,           i18nc("@option:check", "Volume change") },
    };

    return options;
}

#include "moc_osdutils.cpp"
