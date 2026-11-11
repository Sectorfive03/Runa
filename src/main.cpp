/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QPalette>
#include <QStandardPaths>
#include <QStyleHints>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>

#include <KColorScheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KSharedConfig>

#include <KDSingleApplication>

#include "application.h"
#include "generalsettings.h"
#include "thumbnailimageprovider.h"

#ifdef QT_DEBUG
#include <QElapsedTimer>
#endif

using namespace Qt::StringLiterals;

static const auto INITIAL_STYLE = QQuickStyle::name();

namespace
{
// RunaPlayer ships a light and a dark Breeze-based color scheme, both using
// the VLC safety-orange accent. The one matching the current desktop
// appearance is copied out of the resource bundle and used for this process,
// so the player follows the system light/dark preference without touching the
// user's desktop settings or leaving any blue accents behind.
QString runaplayerSchemePath(bool dark)
{
    const QString name = dark ? u"RunaPlayerDark"_s : u"RunaPlayerLight"_s;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    const QString path = dir + u'/' + name + u".colors"_s;

    QFile source(u":/color-schemes/%1.colors"_s.arg(name));
    if (source.open(QIODevice::ReadOnly)) {
        QFile target(path);
        if (target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            target.write(source.readAll());
        }
    }
    return path;
}

void applyRunaplayerScheme(QApplication &app)
{
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    const bool dark = scheme == Qt::ColorScheme::Dark
        || (scheme == Qt::ColorScheme::Unknown
            && app.palette().color(QPalette::Window).lightness() < 128);

    const QString path = runaplayerSchemePath(dark);
    app.setProperty("KDE_COLOR_SCHEME_PATH", path);
    app.setPalette(KColorScheme::createApplicationPalette(KSharedConfig::openConfig(path)));
}

// The platform theme can deliver palette changes after startup. Re-apply the
// RunaPlayer scheme so the application never falls back to the desktop accent.
class RunaplayerPaletteGuard : public QObject
{
public:
    explicit RunaplayerPaletteGuard(QApplication *app)
        : QObject(app)
        , m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::ApplicationPaletteChange && !m_applying) {
            m_applying = true;
            applyRunaplayerScheme(*m_app);
            m_applying = false;
        }
        return QObject::eventFilter(object, event);
    }

private:
    QApplication *m_app;
    bool m_applying{false};
};
} // namespace


int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    qSetMessagePattern(u"%{function}() : %{message}\n"_s);

    QApplication::setOrganizationName(u"Sectorfive"_s);
    QApplication::setApplicationName(u"runaplayer"_s);
    QApplication::setOrganizationDomain(u"sectorfive.win"_s);
    QApplication::setApplicationDisplayName(u"RunaPlayer"_s);
    QApplication::setDesktopFileName(u"win.sectorfive.runaplayer"_s);
    QApplication::setApplicationVersion(Application::version());

    // required by the libvlc video renderer (QQuickFramebufferObject)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QIcon::setThemeName(u"breeze"_s);
    QIcon::setFallbackThemeName(u"hicolor"_s);

    // On platforms without a system Breeze icon theme (e.g. Windows), fall back
    // to hicolor so the player still resolves its symbolic and app icons. If
    // neither theme provides an icon, the multi-resolution window icon loaded
    // from resources below is used instead.

    QApplication qApplication(argc, argv);

#ifdef Q_OS_WIN
    // Make bundled tools (yt-dlp) and libraries discoverable when the
    // application is started from Explorer or another working directory.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QByteArray pathVariable = QFile::encodeName(appDir) + ';' + qgetenv("PATH");
    qputenv("PATH", pathVariable);

    // libvlc searches for its modules in <app>/lib/vlc/plugins.
    const QString vlcPlugins = appDir + u"/lib/vlc/plugins"_s;
    if (qEnvironmentVariableIsEmpty("VLC_PLUGIN_PATH") && QDir(vlcPlugins).exists()) {
        qputenv("VLC_PLUGIN_PATH", QFile::encodeName(vlcPlugins));
    }
#endif

    // Build a multi-resolution icon so the title bar and task manager always
    // pick a crisp square pixmap instead of scaling a single large one.
    QIcon appIcon;
    const int iconSizes[] = {16, 22, 32, 44, 48, 64, 96, 128, 150, 256, 310, 512};
    for (int size : iconSizes) {
        appIcon.addFile(u":/icons/%1-apps-runa.png"_s.arg(size));
    }
    QApplication::setWindowIcon(appIcon);

    bool handledByQPT = INITIAL_STYLE != QQuickStyle::name();
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE") && !handledByQPT) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
        QQuickStyle::setFallbackStyle(u"Fusion"_s);
    }

    auto *application = Application::instance();

    applyRunaplayerScheme(qApplication);
    auto *paletteGuard = new RunaplayerPaletteGuard(&qApplication);
    qApplication.installEventFilter(paletteGuard);
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &qApplication, [&qApplication]() {
        applyRunaplayerScheme(qApplication);
    });

    KDSingleApplication kdsApp;
    if (kdsApp.isPrimaryInstance()) {
        QObject::connect(&kdsApp, &KDSingleApplication::messageReceived, &qApplication, [=](const QByteArray &message) {
            QString file;
            QString token;

            QByteArray byteArray{message};
            QDataStream dataStream(&byteArray, QIODevice::ReadOnly);
            dataStream >> file >> token;

            application->handleSecondayInstanceMessage(file.toUtf8(), token);
        });
    } else {
        if (GeneralSettings::useSingleInstance()) {
            QCommandLineParser clParser;
            clParser.process(qApplication);
            if (!clParser.positionalArguments().isEmpty()) {
                const QString file = clParser.positionalArguments().constFirst();
                const QString token = qEnvironmentVariable("XDG_ACTIVATION_TOKEN");

                QByteArray byteArray;
                QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
                dataStream << file << token;

                kdsApp.sendMessage(byteArray);
            }
            exit(EXIT_SUCCESS);
        }
    }

    QQmlApplicationEngine engine(&qApplication);

    KLocalization::setupLocalizedContext(&engine);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &error : warnings) {
            qWarning() << "QML Warning:" << error.toString();
        }
    });

    engine.addImageProvider(u"thumbnail"_s, new ThumbnailImageProvider());
    engine.loadFromModule("win.sectorfive.runaplayer", "Main");

    application->setQmlEngine(&engine);

#ifdef QT_DEBUG
    qDebug() << timer.elapsed() << "ms elapsed";
#endif
    return QApplication::exec();
}
