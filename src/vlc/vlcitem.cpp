/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 * SPDX-FileCopyrightText: 2025 Muhammet Sadık Uğursoy <sadikugursoy@gmail.com>
 * SPDX-FileCopyrightText: 2026 Haruna VLC port contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "vlcitem.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <KLocalizedString>

#include "audiosettings.h"
#include "chaptersmodel.h"
#include "database.h"
#include "generalsettings.h"
#include "lockmanager.h"
#include "logging/_general.h"
#include "miscutils.h"
#include "osdutils.h"
#include "playbacksettings.h"
#include "playlistsettings.h"
#include "playlistsmanager.h"
#include "seekablerangesmodel.h"
#include "subtitlessettings.h"
#include "tracksmodel.h"
#include "videosettings.h"
#include "vlcengine.h"
#include "worker.h"

#if HAVE_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif

using namespace Qt::StringLiterals;

namespace
{
bool isVideoMimeType(const QUrl &url)
{
    const QString mimeType = MiscUtils::instance()->mimeType(url);
    return mimeType.startsWith(u"video/"_s);
}
} // namespace

class VlcRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
public:
    explicit VlcRenderer(VlcItem *item)
        : m_item(item)
    {
    }

    ~VlcRenderer() override
    {
        cleanup();
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        auto *vlcItem = static_cast<VlcItem *>(item);
        m_itemWidth = float(vlcItem->width());
        m_itemHeight = float(vlcItem->height());

        QByteArray frame;
        int bufferWidth = 0;
        int bufferHeight = 0;
        int displayWidth = 0;
        int displayHeight = 0;
        if (vlcItem->m_engine->takeFrame(frame, bufferWidth, bufferHeight, displayWidth, displayHeight)) {
            m_frame = std::move(frame);
            m_bufferWidth = bufferWidth;
            m_bufferHeight = bufferHeight;
            m_displayWidth = displayWidth;
            m_displayHeight = displayHeight;
            m_uploadNeeded = true;
        }
    }

    void render() override
    {
        initialize();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_frame.isEmpty() || m_bufferWidth <= 0 || m_bufferHeight <= 0) {
            return;
        }

        if (m_uploadNeeded) {
            uploadFrame();
            m_uploadNeeded = false;
        }

        const int displayWidth = m_displayWidth > 0 ? m_displayWidth : m_bufferWidth;
        const int displayHeight = m_displayHeight > 0 ? m_displayHeight : m_bufferHeight;
        if (displayWidth <= 0 || displayHeight <= 0 || m_itemHeight <= 0.0f) {
            return;
        }

        const float videoAspect = float(displayWidth) / float(displayHeight);
        const float itemAspect = m_itemWidth > 0.0f ? m_itemWidth / m_itemHeight : videoAspect;
        float x = 1.0f;
        float y = 1.0f;
        if (videoAspect > itemAspect) {
            y = itemAspect / videoAspect;
        } else {
            x = videoAspect / itemAspect;
        }

        // VLC may pad the memory buffer (for example 640x386 for a 640x360
        // video) and stores the picture bottom-up. Sample only the real
        // picture rows [0, displayHeight) and flip them vertically so the
        // image is displayed the right way up.
        const float uMax = std::min(1.0f, float(displayWidth) / float(m_bufferWidth));
        const float vMax = std::min(1.0f, float(displayHeight) / float(m_bufferHeight));

        const GLfloat vertices[] = {
            -x, -y, 0.0f, 0.0f,
            x,  -y, uMax, 0.0f,
            -x, y,  0.0f, vMax,
            x,  y,  uMax, vMax,
        };

        m_program.bind();
        m_program.setUniformValue(m_textureUniform, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        if (m_vao.isCreated()) {
            m_vao.bind();
        }

        m_vbo.bind();
        m_vbo.write(0, vertices, sizeof(vertices));

        m_program.enableAttributeArray(0);
        m_program.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
        m_program.enableAttributeArray(1);
        m_program.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_program.disableAttributeArray(0);
        m_program.disableAttributeArray(1);
        m_vbo.release();
        if (m_vao.isCreated()) {
            m_vao.release();
        }
        m_program.release();
    }

private:
    void initialize()
    {
        if (m_initialized) {
            return;
        }
        initializeOpenGLFunctions();

        QOpenGLContext *context = QOpenGLContext::currentContext();
        const bool coreProfile = context && context->format().profile() == QSurfaceFormat::CoreProfile;
        const bool gles = context && context->isOpenGLES();

        QString vertexSource;
        QString fragmentSource;
        if (gles) {
            vertexSource = QStringLiteral(
                "attribute vec2 position;\n"
                "attribute vec2 texCoord;\n"
                "varying vec2 vTexCoord;\n"
                "void main() { vTexCoord = texCoord; gl_Position = vec4(position, 0.0, 1.0); }\n");
            fragmentSource = QStringLiteral(
                "precision mediump float;\n"
                "varying vec2 vTexCoord;\n"
                "uniform sampler2D frameTexture;\n"
                "void main() { gl_FragColor = texture2D(frameTexture, vTexCoord); }\n");
        } else if (coreProfile) {
            vertexSource = QStringLiteral(
                "#version 150 core\n"
                "in vec2 position;\n"
                "in vec2 texCoord;\n"
                "out vec2 vTexCoord;\n"
                "void main() { vTexCoord = texCoord; gl_Position = vec4(position, 0.0, 1.0); }\n");
            fragmentSource = QStringLiteral(
                "#version 150 core\n"
                "in vec2 vTexCoord;\n"
                "out vec4 fragColor;\n"
                "uniform sampler2D frameTexture;\n"
                "void main() { fragColor = texture(frameTexture, vTexCoord); }\n");
        } else {
            vertexSource = QStringLiteral(
                "#version 120\n"
                "attribute vec2 position;\n"
                "attribute vec2 texCoord;\n"
                "varying vec2 vTexCoord;\n"
                "void main() { vTexCoord = texCoord; gl_Position = vec4(position, 0.0, 1.0); }\n");
            fragmentSource = QStringLiteral(
                "#version 120\n"
                "varying vec2 vTexCoord;\n"
                "uniform sampler2D frameTexture;\n"
                "void main() { gl_FragColor = texture2D(frameTexture, vTexCoord); }\n");
        }

        m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource);
        m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource);
        m_program.bindAttributeLocation("position", 0);
        m_program.bindAttributeLocation("texCoord", 1);
        m_program.link();
        m_textureUniform = m_program.uniformLocation("frameTexture");

        if (coreProfile) {
            m_vao.create();
            if (m_vao.isCreated()) {
                m_vao.bind();
            }
        }
        m_vbo.create();
        m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        m_vbo.bind();
        m_vbo.allocate(16 * sizeof(GLfloat));
        m_vbo.release();
        if (m_vao.isCreated()) {
            m_vao.release();
        }

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_initialized = true;
    }

    void uploadFrame()
    {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        if (m_textureWidth != m_bufferWidth || m_textureHeight != m_bufferHeight) {
            m_textureWidth = m_bufferWidth;
            m_textureHeight = m_bufferHeight;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_textureWidth, m_textureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, m_frame.constData());
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_textureWidth, m_textureHeight, GL_BGRA, GL_UNSIGNED_BYTE, m_frame.constData());
        }
    }

    void cleanup()
    {
        if (!m_initialized) {
            return;
        }
        if (m_texture) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
        if (m_vbo.isCreated()) {
            m_vbo.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        m_initialized = false;
    }

    VlcItem *m_item{nullptr};
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject m_vao;
    GLuint m_texture{0};
    int m_textureUniform{-1};
    int m_textureWidth{0};
    int m_textureHeight{0};
    QByteArray m_frame;
    int m_bufferWidth{0};
    int m_bufferHeight{0};
    int m_displayWidth{0};
    int m_displayHeight{0};
    float m_itemWidth{0.0f};
    float m_itemHeight{0.0f};
    bool m_uploadNeeded{false};
    bool m_initialized{false};
};

VlcItem::VlcItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_engine{std::make_unique<VlcEngine>()}
    , m_audioTracksModel{std::make_unique<TracksModel>()}
    , m_subtitleTracksModel{std::make_unique<TracksModel>()}
    , m_chaptersModel{std::make_unique<ChaptersModel>()}
    , m_seekableRangesModel{std::make_unique<SeekableRangesModel>()}
    , m_saveTimePositionTimer{std::make_unique<QTimer>()}
{
    m_volumeMax = 100;
    m_volume = AudioSettings::volume();
    m_engine->setVolume(m_volume);

    setupConnections();

    // The whole picture is sampled from VLC, no need for a transparent item.
    setFlag(QQuickItem::ItemHasContents, true);

    m_saveTimePositionTimer->setInterval(PlaybackSettings::savePositionInterval() * 1000);
    m_saveTimePositionTimer->start();

    connect(m_saveTimePositionTimer.get(), &QTimer::timeout, this, [this]() {
        if (!finishedLoading() || duration() < 1 || pause()) {
            return;
        }
        if (position() < duration() - 10) {
            saveTimePosition();
        } else {
            resetTimePosition();
        }
    });

    connect(QApplication::instance(), &QApplication::aboutToQuit, this, [this]() {
        if (playbackState() == PlaybackState::Stopped) {
            return;
        }
        if (position() < duration() - 10) {
            saveTimePosition();
        } else {
            resetTimePosition();
        }
    });

    setIsReady(true);
}

VlcItem::~VlcItem() = default;

QQuickFramebufferObject::Renderer *VlcItem::createRenderer() const
{
    return new VlcRenderer(const_cast<VlcItem *>(this));
}

void VlcItem::setupConnections()
{
    connect(m_engine.get(), &VlcEngine::playing, this, &VlcItem::onFileLoaded, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::playbackStateChanged, this, &VlcItem::onPlaybackStateChanged, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::positionChanged, this, [this](double seconds) {
        m_position = seconds;
        m_remaining = std::max(0.0, m_duration - m_position);
        updateTaskbarPlaybackProgress();

        if (m_abLoopA >= 0.0 && m_abLoopB > m_abLoopA && m_position >= m_abLoopB) {
            m_engine->setPosition(m_abLoopA);
        }

        if (m_duration > 0) {
            const auto pos = static_cast<int>(m_position);
            const auto oldSize = m_secondsWatched.size();
            m_secondsWatched.insert(pos);
            if (m_secondsWatched.size() > oldSize) {
                const int percentage = (m_secondsWatched.size() * 100) / int(m_duration);
                setWatchPercentage(std::min(percentage, 100));
            }
        }

        Q_EMIT positionChanged();
        Q_EMIT remainingChanged();
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::durationChanged, this, [this](double seconds) {
        m_duration = seconds;
        m_remaining = std::max(0.0, m_duration - m_position);
        updateTaskbarPlaybackProgress();
        Q_EMIT durationChanged();
        Q_EMIT remainingChanged();
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::mediaTitleChanged, this, [this](const QString &title) {
        m_mediaTitle = title;
        Q_EMIT mediaTitleChanged();
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::tracksChanged, this, &VlcItem::loadTracks, Qt::QueuedConnection);
    connect(m_engine.get(), &VlcEngine::chaptersChanged, this, &VlcItem::loadChapters, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::videoSizeChanged, this, [this](int width, int height) {
        if (m_videoWidth != width || m_videoHeight != height) {
            m_videoWidth = width;
            m_videoHeight = height;
            Q_EMIT videoWidthChanged();
            Q_EMIT videoHeightChanged();
            Q_EMIT videoReconfig();
        }
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::volumeChanged, this, [this](int volume) {
        m_volume = volume;
        AudioSettings::setVolume(m_volume);
        AudioSettings::self()->save();
        Q_EMIT volumeChanged();
        OsdUtils::instance().message(i18nc("@info:tooltip", "Volume: %1", m_volume), OsdUtils::Context::ShowVolume);
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::mutedChanged, this, [this](bool muted) {
        m_mute = muted;
        Q_EMIT muteChanged();
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::endReached, this, &VlcItem::onEndOfFileReached, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::errorOccurred, this, [this](const QString &message) {
        Q_EMIT MiscUtils::instance()->error(message);
    }, Qt::QueuedConnection);

    connect(m_engine.get(), &VlcEngine::frameReady, this, [this]() {
        update();
    }, Qt::QueuedConnection);

    connect(this, &VlcItem::currentUrlChanged, this, [this]() {
        setFinishedLoading(false);
    });

    connect(Worker::instance(), &Worker::subtitlesFound, this, [this](const QStringList &subs) {
        for (const auto &sub : subs) {
            addSubtitles(sub);
        }
        loadTracks();
    });

    connect(this, &VlcItem::chapterChanged, this, &VlcItem::onChapterChanged);

    connect(this, &VlcItem::pauseChanged, this, [this]() {
        static LockManager lockManager;
        if (pause()) {
            lockManager.setInhibitionOff();
        } else if (!m_currentUrl.isEmpty()) {
            lockManager.setInhibitionOn();
        }
    });

    connect(this, &VlcItem::savePositionToDB, Worker::instance(), &Worker::savePositionToDB, Qt::QueuedConnection);

    connect(this, &VlcItem::playNext, this, []() {});
    connect(this, &VlcItem::playPrevious, this, []() {});
}

void VlcItem::onFileLoaded()
{
    if (m_finishedLoading) {
        return;
    }

    loadTracks();
    loadChapters();

    qCDebug(RunaplayerGeneral) << "File loaded:" << m_currentUrl << "video" << m_videoWidth << "x" << m_videoHeight;

    const bool resumePaused = PlaybackSettings::restoreFilePosition()
        ? !PlaybackSettings::playOnResume() && watchLaterPosition() > 1
        : false;

    setIsVideo(isVideoMimeType(m_currentUrl));
    setIsLocalFile(m_currentUrl.isLocalFile());
    setFinishedLoading(true);

    if (!SubtitlesSettings::autoSelectSubtitles()) {
        setSubtitlesVisible(false);
    } else if (SubtitlesSettings::preferredTrack() > 0) {
        setSubtitleId(SubtitlesSettings::preferredTrack());
    }

    if (AudioSettings::preferredTrack() > 0) {
        setAudioId(AudioSettings::preferredTrack());
    }

    setPause(resumePaused);
    setPlaybackState(resumePaused ? PlaybackState::Paused : PlaybackState::Playing);

    Q_EMIT fileStarted();
}

void VlcItem::onPlaybackStateChanged(int state)
{
    switch (state) {
    case libvlc_Opening:
    case libvlc_Buffering:
        if (m_playbackState == PlaybackState::Stopped || m_playbackState == PlaybackState::Loading) {
            setPlaybackState(PlaybackState::Loading);
        }
        break;
    case libvlc_Playing:
        if (m_pause) {
            m_pause = false;
            Q_EMIT pauseChanged();
        }
        if (!m_finishedLoading) {
            onFileLoaded();
        } else {
            setPlaybackState(PlaybackState::Playing);
        }
        break;
    case libvlc_Paused:
        if (!m_pause) {
            m_pause = true;
            Q_EMIT pauseChanged();
        }
        setPlaybackState(PlaybackState::Paused);
        break;
    case libvlc_Stopped:
    case libvlc_Ended:
    case libvlc_Error:
        if (!m_pause) {
            m_pause = true;
            Q_EMIT pauseChanged();
        }
        setPlaybackState(PlaybackState::Stopped);
        break;
    default:
        break;
    }
}

void VlcItem::onEndOfFileReached()
{
    Q_EMIT eofReached();

    switch (endOfFileAction()) {
    case EndOfFileAction::PlayNext:
        playlistsManager()->playNext();
        Q_EMIT eofPlayNext();
        break;
    case EndOfFileAction::Replay: {
        setFinishedLoading(false);
        m_secondsWatched.clear();
        setWatchPercentage(0);
        m_engine->load(m_currentUrl, 0.0, PlaybackSettings::hWDecoding());
        break;
    }
    case EndOfFileAction::Stop:
    default:
        stop();
        break;
    }
}

VlcItem::EndOfFileAction VlcItem::endOfFileAction() const
{
    auto *const playlistModel = playlistsManager()->activePlaylist();
    const auto behavior = PlaylistSettings::playbackBehavior();
    if (behavior == u"RepeatItem"_s) {
        return EndOfFileAction::Replay;
    }
    if (behavior == u"StopAfterItem"_s) {
        return EndOfFileAction::Stop;
    }
    if (behavior == u"StopAfterLast"_s) {
        if (playlistModel->isLastItem(playlistModel->getPlayingItem())) {
            return EndOfFileAction::Stop;
        }
        return EndOfFileAction::PlayNext;
    }
    if (behavior == u"RepeatPlaylist"_s) {
        if (playlistModel->rowCount() == 1) {
            return EndOfFileAction::Replay;
        }
        return EndOfFileAction::PlayNext;
    }
    return EndOfFileAction::Stop;
}

void VlcItem::loadFile(const QString &file)
{
    auto url = QUrl::fromUserInput(file);
    if (m_currentUrl != url) {
        m_currentUrl = url;
        Q_EMIT currentUrlChanged();
    }

    QFileInfo fi{url.toString(QUrl::PreferLocalFile)};
    if (url.isLocalFile() && !fi.exists()) {
        m_currentUrl = {};
        Q_EMIT currentUrlChanged();
        Q_EMIT MiscUtils::instance()->error(i18nc("@info:tooltip; %1 is an absolute path", "File doesn't exist: %1", file));
        return;
    }

    setPlaybackState(PlaybackState::Loading);
    setFinishedLoading(false);
    m_secondsWatched.clear();
    setWatchPercentage(0);

    setWatchLaterPosition(loadTimePosition());

    const QString audioLanguage = AudioSettings::preferredLanguage().remove(u" "_s);
    const QString subtitleLanguage = SubtitlesSettings::preferredLanguage().remove(u" "_s);
    m_engine->setPreferredLanguages(audioLanguage, subtitleLanguage);

    m_engine->setVolume(m_volume);
    m_engine->setMuted(m_mute);

    m_engine->load(m_currentUrl,
                   PlaybackSettings::restoreFilePosition() ? m_watchLaterPosition : 0.0,
                   PlaybackSettings::hWDecoding());

    const auto *playlistModel = playlistsManager()->activePlaylist()->playlistModel();
    const auto playingRow = playlistModel->playingItem();
    const auto playingIndex = playlistModel->index(playingRow, 0);
    const auto title = playlistModel->data(playingIndex, PlaylistModel::Roles::TitleRole).toString();
    const auto filename = playlistModel->data(playingIndex, PlaylistModel::Roles::NameRole).toString();
    const auto itemName = PlaylistSettings::showMediaTitle() ? title : filename;
    const auto msg = i18nc("%1 current item number, %2 total items count, %3 item name",
                           "[%1/%2] %3", playingRow + 1, playlistModel->rowCount(), itemName);
    OsdUtils::instance().message(msg, OsdUtils::Context::ShowLoadFile);
}

void VlcItem::loadTracks()
{
    m_subtitleTracksModel->clear();
    m_audioTracksModel->clear();

    const Track noneTrack = {
        0,
        QString{},
        i18nc("@action The \"None\" subtitle track is used to clear/unset selected track", "None"),
        QString{},
        std::nullopt,
        0,
    };
    m_subtitleTracksModel->addTrack(noneTrack);

    int audioRow = 0;
    int subtitleRow = 0;
    const int currentAudio = m_engine->audioTrack();
    const int currentSubtitle = m_engine->subtitleTrack();

    const auto tracks = m_engine->tracks();
    for (const auto &item : tracks) {
        if (item.type == libvlc_track_audio) {
            Track track;
            track.trackid = item.id;
            track.lang = item.language;
            track.title = item.title;
            track.codec = item.codec;
            track.streamIndex = -1;
            track.externalPath = std::nullopt;
            m_audioTracksModel->addTrack(track);
            if (track.trackid == currentAudio) {
                m_audioTracksModel->setActiveRow(audioRow);
            }
            ++audioRow;
        } else if (item.type == libvlc_track_text) {
            Track track;
            track.trackid = item.id;
            track.lang = item.language;
            track.title = item.title;
            track.codec = item.codec;
            track.streamIndex = -1;
            track.externalPath = std::nullopt;
            m_subtitleTracksModel->addTrack(track);
            if (track.trackid == currentSubtitle) {
                m_subtitleTracksModel->setActiveRow(subtitleRow);
            }
            ++subtitleRow;
        }
    }

    const int audioId = currentAudio > 0 ? currentAudio : 0;
    if (audioId != m_audioId) {
        m_audioId = audioId;
        Q_EMIT audioIdChanged();
    }

    const int subtitleId = currentSubtitle > 0 ? currentSubtitle : 0;
    if (subtitleId != m_subtitleId) {
        m_subtitleId = subtitleId;
        Q_EMIT subtitleIdChanged();
    }

    Q_EMIT audioTracksModelChanged();
    Q_EMIT subtitleTracksModelChanged();
}

void VlcItem::loadChapters()
{
    QList<Chapter> chaptersList;
    QMap<double, QString> startTimeToTitleMap;

    const auto chapters = m_engine->chapters();
    for (const auto &src : chapters) {
        Chapter chapter;
        chapter.title = src.title;
        chapter.startTime = src.start;
        chaptersList.append(chapter);
        startTimeToTitleMap.insert(chapter.startTime, chapter.title);
    }

    if (!chaptersList.isEmpty()) {
        m_chaptersModel->setChapters(chaptersList);
        m_chaptersModel->setStartTimeToTitleMap(startTimeToTitleMap);
    }

    const int currentChapter = m_engine->chapter();
    if (currentChapter >= 0 && currentChapter != m_chapter) {
        m_chapter = currentChapter;
        Q_EMIT chapterChanged();
    }
    Q_EMIT chapterListChanged();
}

void VlcItem::onChapterChanged()
{
    if (!finishedLoading() || !PlaybackSettings::skipChapters()) {
        return;
    }

    const QString chaptersToSkip = PlaybackSettings::chaptersToSkip();
    if (m_chaptersModel->rowCount() == 0 || chaptersToSkip == QString()) {
        return;
    }

    const QStringList words = chaptersToSkip.split(u","_s);
    if (words.isEmpty()) {
        return;
    }

    const auto modelIndex = m_chaptersModel->index(chapter(), 0);
    const auto title = m_chaptersModel->data(modelIndex, ChaptersModel::Roles::TitleRole).toString();
    for (const auto &word : words) {
        const auto trimmed = word.simplified();
        if (title.contains(trimmed, Qt::CaseInsensitive)) {
            nextChapter();
            const auto msg = i18nc("@info:tooltip osd", "Skipped chapter: %1", title);
            OsdUtils::instance().message(msg, OsdUtils::Context::ShowChapter);
            return;
        }
    }
}

void VlcItem::updateTaskbarPlaybackProgress()
{
#if HAVE_DBUS
    if (!GeneralSettings::showTaskbarProgress()) {
        return;
    }
    if (duration() <= 0) {
        return;
    }

    const double currentPosition = position();
    const double currentDuration = duration();
    const auto state = playbackState();

    QThreadPool::globalInstance()->start([currentPosition, currentDuration, state]() {
        static QList<PlaybackState> activeStates = {PlaybackState::Playing, PlaybackState::Paused};
        const bool isProgressVisible = activeStates.contains(state);

        QVariantMap map;
        map.insert(u"progress-visible"_s, isProgressVisible);
        map.insert(u"progress"_s, currentPosition / currentDuration);

        QDBusMessage msg = QDBusMessage::createSignal(u"/win/sectorfive/runaplayer"_s, u"com.canonical.Unity.LauncherEntry"_s, u"Update"_s);
        msg << u"application://win.sectorfive.runaplayer.desktop"_s << map;

        QDBusConnection::sessionBus().send(msg);
    });
#endif
}

void VlcItem::saveTimePosition()
{
    if (duration() < PlaybackSettings::minDurationToSavePosition() * 60) {
        return;
    }
    const auto hash = MiscUtils::md5(currentUrl().toString());
    Q_EMIT savePositionToDB(hash, currentUrl().toString(), position());
}

double VlcItem::loadTimePosition()
{
    const auto hash = MiscUtils::md5(currentUrl().toString());
    return Database::instance()->playbackPosition(hash);
}

void VlcItem::resetTimePosition()
{
    const auto hash = MiscUtils::md5(currentUrl().toString());
    Database::instance()->deletePlaybackPosition(hash);
}

void VlcItem::selectSubtitleTrack()
{
    if (!SubtitlesSettings::autoSelectSubtitles()) {
        setSubtitlesVisible(false);
    }
}

void VlcItem::addSubtitles(const QString &subtitlesFile)
{
    m_engine->addSubtitleFile(QUrl::fromUserInput(subtitlesFile));
    loadTracks();
}

void VlcItem::stop()
{
    m_engine->stop();
    setPlaybackState(PlaybackState::Stopped);
    setPause(true);
    update();

    auto *const playlistModel = playlistsManager()->activePlaylist();
    playlistModel->setIsPlaying(false);
}

void VlcItem::setTrack(int row, TrackType type)
{
    switch (type) {
    case TrackType::Audio: {
        const auto track = m_audioTracksModel->track(row);
        setAudioId(track.trackid);
        m_audioTracksModel->setActiveRow(row);
        const auto msg = i18nc(
            "@info:tooltip; %1 is the track number, "
            "%2 is a string in the form: `track.title track.language track.codec`",
            "Audio: %1 %2",
            row + 1,
            track.display());
        OsdUtils::instance().message(msg, OsdUtils::Context::ShowTrack);
        break;
    }
    case TrackType::Subtitle: {
        const auto track = m_subtitleTracksModel->track(row);
        setSubtitleId(track.trackid);
        m_subtitleTracksModel->setActiveRow(row);
        if (row == 0) {
            OsdUtils::instance().message(i18nc("@info:tooltip; osd message when no sub is selected", "Subtitle: none"),
                                         OsdUtils::Context::ShowTrack);
        } else {
            const auto msg = i18nc(
                "@info:tooltip; %1 is the track number, "
                "%2 is a string in the form: `track.title track.language track.codec`",
                "Subtitle: %1 %2",
                row,
                track.display());
            OsdUtils::instance().message(msg, OsdUtils::Context::ShowTrack);
        }
        break;
    }
    case TrackType::Video:
    default:
        break;
    }
}

void VlcItem::setNextTrack(TrackType type)
{
    if (type == TrackType::Audio) {
        setTrack(m_audioTracksModel->nextRow(), type);
    } else if (type == TrackType::Subtitle) {
        setTrack(m_subtitleTracksModel->nextRow(), type);
    }
}

void VlcItem::setPreviousTrack(TrackType type)
{
    if (type == TrackType::Audio) {
        setTrack(m_audioTracksModel->previousRow(), type);
    } else if (type == TrackType::Subtitle) {
        setTrack(m_subtitleTracksModel->previousRow(), type);
    }
}

ChaptersModel *VlcItem::chaptersModel() const
{
    return m_chaptersModel.get();
}

void VlcItem::setChaptersModel(ChaptersModel *_chaptersModel)
{
    if (m_chaptersModel.get() == _chaptersModel) {
        return;
    }
    m_chaptersModel.reset(_chaptersModel);
    Q_EMIT chaptersModelChanged();
}

SeekableRangesModel *VlcItem::seekableRangesModel() const
{
    return m_seekableRangesModel.get();
}

TracksModel *VlcItem::subtitleTracksModel() const
{
    return m_subtitleTracksModel.get();
}

TracksModel *VlcItem::audioTracksModel() const
{
    return m_audioTracksModel.get();
}

PlaylistsManager *VlcItem::playlistsManager() const
{
    return m_playlistsManager;
}

void VlcItem::setPlaylistsManager(PlaylistsManager *newPlaylistsManager)
{
    if (m_playlistsManager != nullptr) {
        return;
    }
    m_playlistsManager = newPlaylistsManager;

    connect(playlistsManager(), &PlaylistsManager::playingItemChanged, this, [this]() {
        loadFile(playlistsManager()->activeItemPath());
    });

    Q_EMIT playlistsManagerChanged();
}

QString VlcItem::mediaTitle()
{
    return m_mediaTitle;
}

double VlcItem::position()
{
    return m_position;
}

void VlcItem::setPosition(double value)
{
    if (qFuzzyCompare(value + 1.0, position() + 1.0)) {
        return;
    }
    m_engine->setPosition(value);
    m_position = value;
    m_remaining = std::max(0.0, m_duration - m_position);
    Q_EMIT positionChanged();
    Q_EMIT remainingChanged();
}

void VlcItem::seekAbsolute(double seconds)
{
    setPosition(std::max(0.0, seconds));
}

void VlcItem::seekRelative(double seconds)
{
    setPosition(std::clamp(m_position + seconds, 0.0, m_duration > 0.0 ? m_duration : m_position + seconds));
}

double VlcItem::remaining()
{
    return m_remaining;
}

double VlcItem::duration()
{
    return m_duration;
}

bool VlcItem::pause()
{
    return m_pause;
}

void VlcItem::setPause(bool value)
{
    if (value == m_pause) {
        m_engine->setPaused(value);
        return;
    }
    m_pause = value;
    m_engine->setPaused(value);
    Q_EMIT pauseChanged();
}

int VlcItem::volume()
{
    return m_volume;
}

void VlcItem::setVolume(int value)
{
    const int clamped = std::clamp(value, 0, m_volumeMax);
    if (clamped == m_volume) {
        return;
    }
    m_volume = clamped;
    m_engine->setVolume(m_volume);
    Q_EMIT volumeChanged();
}

int VlcItem::volumeMax()
{
    return m_volumeMax;
}

void VlcItem::setVolumeMax(int value)
{
    if (m_volumeMax == value) {
        return;
    }
    m_volumeMax = value;
    Q_EMIT volumeMaxChanged();
}

bool VlcItem::mute()
{
    return m_mute;
}

void VlcItem::setMute(bool value)
{
    if (value == m_mute) {
        return;
    }
    m_mute = value;
    m_engine->setMuted(value);
    Q_EMIT muteChanged();
}

int VlcItem::chapter()
{
    return m_chapter;
}

void VlcItem::setChapter(int value)
{
    if (value == m_chapter) {
        return;
    }
    m_engine->setChapter(value);
    m_chapter = value;
    Q_EMIT chapterChanged();
}

void VlcItem::nextChapter()
{
    const int count = int(m_engine->chapters().size());
    if (m_chapter + 1 < count) {
        setChapter(m_chapter + 1);
    }
}

void VlcItem::previousChapter()
{
    if (m_chapter > 0) {
        setChapter(m_chapter - 1);
    }
}

int VlcItem::audioId()
{
    return m_audioId;
}

void VlcItem::setAudioId(int value)
{
    if (value == m_audioId) {
        return;
    }
    if (value <= 0) {
        m_engine->setAudioTrack(-1);
        m_audioId = 0;
        Q_EMIT audioIdChanged();
        return;
    }
    if (!m_audioTracksModel->hasTrackId(value)) {
        return;
    }
    m_engine->setAudioTrack(value);
    m_audioId = value;
    Q_EMIT audioIdChanged();
}

int VlcItem::subtitleId()
{
    return m_subtitleId;
}

void VlcItem::setSubtitleId(int value)
{
    if (value <= 0) {
        const int current = m_engine->subtitleTrack();
        if (current > 0) {
            m_lastSubtitleTrack = current;
        }
        m_engine->setSubtitleTrack(-1);
        m_subtitleId = 0;
        setSubtitlesVisible(false);
        Q_EMIT subtitleIdChanged();
        return;
    }
    if (!m_subtitleTracksModel->hasTrackId(value)) {
        return;
    }
    m_engine->setSubtitleTrack(value);
    m_subtitleId = value;
    m_lastSubtitleTrack = value;
    setSubtitlesVisible(true);
    Q_EMIT subtitleIdChanged();
}

bool VlcItem::subtitlesVisible() const
{
    return m_subtitlesVisible;
}

void VlcItem::setSubtitlesVisible(bool visible)
{
    if (visible) {
        if (m_lastSubtitleTrack > 0) {
            m_engine->setSubtitleTrack(m_lastSubtitleTrack);
            m_subtitleId = m_lastSubtitleTrack;
            Q_EMIT subtitleIdChanged();
        }
    } else {
        const int current = m_engine->subtitleTrack();
        if (current > 0) {
            m_lastSubtitleTrack = current;
        }
        m_engine->setSubtitleTrack(-1);
        m_subtitleId = 0;
        Q_EMIT subtitleIdChanged();
    }

    if (m_subtitlesVisible != visible) {
        m_subtitlesVisible = visible;
        Q_EMIT subtitlesVisibleChanged();
    }
}

double VlcItem::subtitleDelay() const
{
    return m_subtitleDelay;
}

void VlcItem::setSubtitleDelay(double seconds)
{
    if (qFuzzyCompare(m_subtitleDelay + 1.0, seconds + 1.0)) {
        return;
    }
    m_subtitleDelay = seconds;
    m_engine->setSubtitleDelay(seconds);
    Q_EMIT subtitleDelayChanged();
}

bool VlcItem::deinterlaced() const
{
    return m_deinterlaced;
}

void VlcItem::setDeinterlaced(bool enabled)
{
    if (m_deinterlaced == enabled) {
        return;
    }
    m_deinterlaced = enabled;
    m_engine->setDeinterlaced(enabled);
    Q_EMIT deinterlacedChanged();
}

QString VlcItem::formattedDuration() const
{
    return MiscUtils::formatTime(m_duration);
}

QString VlcItem::formattedRemaining() const
{
    return MiscUtils::formatTime(m_remaining);
}

QString VlcItem::formattedPosition() const
{
    return MiscUtils::formatTime(m_position);
}

QUrl VlcItem::currentUrl() const
{
    return m_currentUrl;
}

bool VlcItem::isReady() const
{
    return m_isReady;
}

void VlcItem::setIsReady(bool ready)
{
    if (m_isReady == ready) {
        return;
    }
    m_isReady = ready;
    Q_EMIT isReadyChanged();
}

VlcItem::PlaybackState VlcItem::playbackState() const
{
    return m_playbackState;
}

void VlcItem::setPlaybackState(PlaybackState newPlaybackState)
{
    if (m_playbackState == newPlaybackState) {
        return;
    }
    m_playbackState = newPlaybackState;
    Q_EMIT playbackStateChanged();
}

double VlcItem::watchLaterPosition() const
{
    return m_watchLaterPosition;
}

void VlcItem::setWatchLaterPosition(double position)
{
    if (qFuzzyCompare(m_watchLaterPosition + 1, position + 1)) {
        return;
    }
    m_watchLaterPosition = position;
    Q_EMIT watchLaterPositionChanged();
}

bool VlcItem::finishedLoading() const
{
    return m_finishedLoading;
}

void VlcItem::setFinishedLoading(bool finishedLoading)
{
    if (m_finishedLoading == finishedLoading) {
        return;
    }
    m_finishedLoading = finishedLoading;
    Q_EMIT finishedLoadingChanged();
}

double VlcItem::watchPercentage()
{
    return m_watchPercentage;
}

void VlcItem::setWatchPercentage(double value)
{
    if (qFuzzyCompare(m_watchPercentage + 1.0, value + 1.0)) {
        return;
    }
    m_watchPercentage = value;
    Q_EMIT watchPercentageChanged();
}

int VlcItem::videoWidth()
{
    return m_videoWidth;
}

int VlcItem::videoHeight()
{
    return m_videoHeight;
}

double VlcItem::playbackSpeed() const
{
    return m_playbackSpeed;
}

void VlcItem::setPlaybackSpeed(double newSpeed)
{
    if (qFuzzyCompare(newSpeed + 1.0, m_playbackSpeed + 1.0)) {
        return;
    }
    newSpeed = std::clamp(newSpeed, 0.25, 4.0);
    if (m_engine->setRate(newSpeed)) {
        m_playbackSpeed = newSpeed;
        Q_EMIT playbackSpeedChanged();
    }
}

bool VlcItem::isLocalFile() const
{
    return m_isLocalFile;
}

void VlcItem::setIsLocalFile(bool newIsLocalFile)
{
    if (m_isLocalFile == newIsLocalFile) {
        return;
    }
    m_isLocalFile = newIsLocalFile;
    Q_EMIT isLocalFileChanged();
}

bool VlcItem::isVideo() const
{
    return m_isVideo;
}

void VlcItem::setIsVideo(bool newIsVideo)
{
    if (m_isVideo == newIsVideo) {
        return;
    }
    m_isVideo = newIsVideo;
    Q_EMIT isVideoChanged();
}

double VlcItem::abLoopA() const
{
    return m_abLoopA;
}

void VlcItem::setABLoopA(double seconds)
{
    if (qFuzzyCompare(m_abLoopA + 1.0, seconds + 1.0)) {
        return;
    }
    m_abLoopA = seconds;
    Q_EMIT abLoopChanged();
}

double VlcItem::abLoopB() const
{
    return m_abLoopB;
}

void VlcItem::setABLoopB(double seconds)
{
    if (qFuzzyCompare(m_abLoopB + 1.0, seconds + 1.0)) {
        return;
    }
    m_abLoopB = seconds;
    Q_EMIT abLoopChanged();
}

void VlcItem::clearABLoop()
{
    m_abLoopA = -1.0;
    m_abLoopB = -1.0;
    Q_EMIT abLoopChanged();
}

void VlcItem::frameStepForward()
{
    m_engine->frameStep();
}

void VlcItem::takeScreenshot()
{
    const QString path = buildScreenshotPath();
    if (path.isEmpty()) {
        OsdUtils::instance().message(i18nc("@info:tooltip osd", "Screenshot failed"), OsdUtils::Context::ShowScreenshot);
        return;
    }
    if (m_engine->takeSnapshot(path)) {
        OsdUtils::instance().message(i18nc("@info:tooltip osd; %1 is a filename with path", "Screenshot: %1", path),
                                     OsdUtils::Context::ShowScreenshot);
    } else {
        OsdUtils::instance().message(i18nc("@info:tooltip osd", "Screenshot failed"), OsdUtils::Context::ShowScreenshot);
    }
}

QString VlcItem::buildScreenshotPath() const
{
    QString path = VideoSettings::screenshotTemplate();
    if (path.isEmpty()) {
        return {};
    }

    const QString baseName = m_currentUrl.isLocalFile() ? QFileInfo(m_currentUrl.toLocalFile()).completeBaseName()
                                                       : m_currentUrl.fileName();

    QString directory = QFileInfo(path).absolutePath();
    if (directory.isEmpty() || directory == u"."_s) {
        directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + u"/Runaplayer screenshots"_s;
    }
    QDir().mkpath(directory);

    QString name = QFileInfo(path).fileName();
    name.replace(u"${filename}"_s, baseName);
    name.replace(u"%F"_s, baseName);

    const QString format = VideoSettings::screenshotFormat().isEmpty() ? u"png"_s : VideoSettings::screenshotFormat();
    if (!name.contains(u'.')) {
        name += u'.' + format;
    }

    // Replace the numbering token with the first free index.
    int index = 1;
    QString candidate;
    do {
        candidate = name;
        candidate.replace(u"%n"_s, QString::number(index));
        candidate.replace(u"${#}"_s, QString::number(index));
        candidate = directory + u'/' + candidate;
        ++index;
    } while (QFileInfo::exists(candidate) && index < 10000);

    return candidate;
}

double VlcItem::imageAdjustment(const QString &name) const
{
    return m_engine->imageAdjustment(name);
}

void VlcItem::setImageAdjustment(const QString &name, double value)
{
    m_engine->setImageAdjustment(name, value);
}

void VlcItem::resetImageAdjustments()
{
    m_engine->resetImageAdjustments();
}

void VlcItem::setAspectRatio(const QString &aspect)
{
    m_engine->setAspectRatio(aspect);
}

#include "moc_vlcitem.cpp"
