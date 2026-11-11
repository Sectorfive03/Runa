/*
 * SPDX-FileCopyrightText: 2026 Haruna VLC port contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "vlcengine.h"

#include <QFileInfo>
#include <QMetaObject>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "logging/_general.h"

using namespace Qt::StringLiterals;

namespace
{
QString fourccToString(uint32_t fourcc)
{
    if (fourcc == 0) {
        return {};
    }
    char text[5] = {char(fourcc & 0xff),
                    char((fourcc >> 8) & 0xff),
                    char((fourcc >> 16) & 0xff),
                    char((fourcc >> 24) & 0xff),
                    0};
    for (int i = 0; i < 4; ++i) {
        if (text[i] < 32 || text[i] > 126) {
            return {};
        }
    }
    return QString::fromLatin1(text);
}

bool isYouTubeUrl(const QUrl &url)
{
    const QString host = url.host().toLower();
    return host == u"youtu.be"_s || host.endsWith(u"youtube.com"_s) || host.endsWith(u"youtube-nocookie.com"_s);
}

// libvlc cannot open YouTube watch pages directly, so ask yt-dlp for the
// video title and a direct media URL. Playback falls back to the original URL
// if extraction fails; a failure is then reported through the normal libvlc
// error path.
struct YouTubeResolution {
    QUrl url;
    QString title;
};

YouTubeResolution resolveYouTubeUrl(const QUrl &url)
{
    YouTubeResolution result{url, {}};
    const QString ytDlp = QStandardPaths::findExecutable(u"yt-dlp"_s);
    if (ytDlp.isEmpty()) {
        return result;
    }

    QProcess process;
    process.setProgram(ytDlp);
    process.setArguments({
        u"--no-warnings"_s,
        u"--no-playlist"_s,
        u"-f"_s,
        u"18/b[acodec!=none][vcodec!=none]/best"_s,
        u"--print"_s,
        u"%(title)s"_s,
        u"-g"_s,
        u"--"_s,
        url.toString(),
    });
    process.start();
    if (!process.waitForFinished(20000)) {
        process.kill();
        process.waitForFinished();
        return result;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return result;
    }

    const QStringList lines = QString::fromUtf8(process.readAllStandardOutput()).split(u'\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString value = line.trimmed();
        if (value.startsWith(u"http"_s)) {
            result.url = QUrl(value);
            break;
        }
        if (result.title.isEmpty() && !value.isEmpty()) {
            result.title = value;
        }
    }
    return result;
}
} // namespace

VlcEngine::VlcEngine(QObject *parent)
    : QObject(parent)
{
    const char *args[] = {
        "--no-video-title-show",
        "--no-osd",
        "--no-snapshot-preview",
    };
    qCDebug(RunaplayerGeneral) << "Initializing libvlc" << libvlc_get_version();
    m_instance = libvlc_new(int(std::size(args)), args);
    if (!m_instance) {
        QMetaObject::invokeMethod(this, [this]() {
            Q_EMIT errorOccurred(QStringLiteral("Could not initialize libvlc"));
        }, Qt::QueuedConnection);
        return;
    }

    m_player = libvlc_media_player_new(m_instance);
    if (!m_player) {
        QMetaObject::invokeMethod(this, [this]() {
            Q_EMIT errorOccurred(QStringLiteral("Could not create a libvlc media player"));
        }, Qt::QueuedConnection);
        return;
    }

    libvlc_video_set_callbacks(m_player, &VlcEngine::onVideoLock, &VlcEngine::onVideoUnlock, &VlcEngine::onVideoDisplay, this);
    libvlc_video_set_format_callbacks(m_player, &VlcEngine::onVideoFormat, &VlcEngine::onVideoCleanup);

    m_eventManager = libvlc_media_player_event_manager(m_player);
    const int events[] = {
        libvlc_MediaPlayerMediaChanged,
        libvlc_MediaPlayerOpening,
        libvlc_MediaPlayerBuffering,
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerLengthChanged,
        libvlc_MediaPlayerSeekableChanged,
        libvlc_MediaPlayerVout,
        libvlc_MediaPlayerESAdded,
        libvlc_MediaPlayerESDeleted,
        libvlc_MediaPlayerESSelected,
        libvlc_MediaPlayerChapterChanged,
    };
    for (int event : events) {
        libvlc_event_attach(m_eventManager, event, &VlcEngine::onVlcEvent, this);
    }
}

VlcEngine::~VlcEngine()
{
    if (m_player) {
        libvlc_media_player_stop(m_player);
        libvlc_media_player_release(m_player);
        m_player = nullptr;
    }
    if (m_instance) {
        libvlc_release(m_instance);
        m_instance = nullptr;
    }
}

void VlcEngine::setPreferredLanguages(const QString &audio, const QString &sub)
{
    m_audioLanguage = audio;
    m_subtitleLanguage = sub;
}

bool VlcEngine::load(const QUrl &url, double startSeconds, const QString &hardwareDecoding)
{
    if (!m_player) {
        return false;
    }

    qCDebug(RunaplayerGeneral) << "Loading" << url << "start" << startSeconds << "hwdec" << hardwareDecoding;
    m_url = url;
    m_startSeconds = startSeconds;
    m_endEmitted = false;
    m_stopping = false;
    m_loaded = false;
    m_mediaTitle.clear();

    const bool isYouTube = isYouTubeUrl(url);
    const YouTubeResolution youtube = isYouTube ? resolveYouTubeUrl(url) : YouTubeResolution{url, QString()};
    const QUrl playbackUrl = youtube.url;

    libvlc_media_player_stop(m_player);

    libvlc_media_t *media = playbackUrl.isLocalFile()
        ? libvlc_media_new_path(m_instance, QFile::encodeName(playbackUrl.toLocalFile()).constData())
        : libvlc_media_new_location(m_instance, playbackUrl.toEncoded().constData());
    if (!media) {
        Q_EMIT errorOccurred(tr("Could not open %1").arg(url.toString()));
        return false;
    }

    libvlc_media_add_option_flag(media, ":no-video-title-show", libvlc_media_option_trusted);
    if (!youtube.title.isEmpty()) {
        libvlc_media_set_meta(media, libvlc_meta_Title, youtube.title.toUtf8().constData());
    }
    if (isYouTube) {
        libvlc_media_add_option_flag(media, ":http-referrer=https://www.youtube.com/", libvlc_media_option_trusted);
        libvlc_media_add_option_flag(media,
                                     ":http-user-agent=Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36",
                                     libvlc_media_option_trusted);
    }
    QString hwMode = hardwareDecoding;
    if (hwMode.isEmpty() || hwMode == u"auto"_s || hwMode == u"auto-safe"_s) {
        hwMode = u"any"_s;
    } else if (hwMode == u"disabled"_s || hwMode == u"no"_s) {
        hwMode = u"none"_s;
    } else if (hwMode == u"vdpau"_s) {
        hwMode = u"vdpau_avcodec"_s;
    }
    libvlc_media_add_option_flag(media,
                                 QStringLiteral(":avcodec-hw=%1").arg(hwMode).toUtf8().constData(),
                                 libvlc_media_option_trusted);
    if (startSeconds > 0.5) {
        libvlc_media_add_option_flag(media, QStringLiteral(":start-time=%1").arg(startSeconds, 0, 'f', 3).toUtf8().constData(),
                                     libvlc_media_option_trusted);
    }
    if (!m_audioLanguage.isEmpty()) {
        libvlc_media_add_option_flag(media, QStringLiteral(":audio-language=%1").arg(m_audioLanguage).toUtf8().constData(),
                                     libvlc_media_option_trusted);
    }
    if (!m_subtitleLanguage.isEmpty()) {
        libvlc_media_add_option_flag(media, QStringLiteral(":sub-language=%1").arg(m_subtitleLanguage).toUtf8().constData(),
                                     libvlc_media_option_trusted);
    }

    libvlc_media_player_set_media(m_player, media);
    libvlc_media_release(media);

    const int result = libvlc_media_player_play(m_player);
    if (result != 0) {
        Q_EMIT errorOccurred(tr("Could not play %1").arg(url.toString()));
        return false;
    }
    return true;
}

void VlcEngine::stop()
{
    if (!m_player) {
        return;
    }
    m_stopping = true;
    libvlc_media_player_stop(m_player);
    m_stopping = false;
}

void VlcEngine::play()
{
    if (!m_player) {
        return;
    }
    m_endEmitted = false;
    libvlc_media_player_play(m_player);
}

void VlcEngine::setPaused(bool paused)
{
    if (!m_player) {
        return;
    }
    libvlc_media_player_set_pause(m_player, paused ? 1 : 0);
}

bool VlcEngine::isPaused() const
{
    return m_player && libvlc_media_player_get_state(m_player) == libvlc_Paused;
}

bool VlcEngine::isPlaying() const
{
    return m_player && libvlc_media_player_is_playing(m_player);
}

void VlcEngine::setPosition(double seconds)
{
    if (!m_player) {
        return;
    }
    libvlc_media_player_set_time(m_player, libvlc_time_t(std::max(0.0, seconds) * 1000.0));
}

double VlcEngine::position() const
{
    if (!m_player) {
        return 0.0;
    }
    const libvlc_time_t time = libvlc_media_player_get_time(m_player);
    return time < 0 ? 0.0 : double(time) / 1000.0;
}

double VlcEngine::duration() const
{
    if (!m_player) {
        return 0.0;
    }
    const libvlc_time_t length = libvlc_media_player_get_length(m_player);
    return length < 0 ? 0.0 : double(length) / 1000.0;
}

void VlcEngine::setVolume(int volume)
{
    if (!m_player) {
        return;
    }
    m_volume = std::clamp(volume, 0, 200);
    libvlc_audio_set_volume(m_player, m_volume);
    Q_EMIT volumeChanged(m_volume);
}

int VlcEngine::volume() const
{
    return m_volume;
}

void VlcEngine::setMuted(bool muted)
{
    if (!m_player) {
        return;
    }
    m_muted = muted;
    libvlc_audio_set_mute(m_player, muted ? 1 : 0);
    Q_EMIT mutedChanged(muted);
}

bool VlcEngine::isMuted() const
{
    return m_muted;
}

bool VlcEngine::setRate(double rate)
{
    if (!m_player) {
        return false;
    }
    const bool ok = libvlc_media_player_set_rate(m_player, float(rate)) == 0;
    if (ok) {
        m_rate = rate;
    }
    return ok;
}

double VlcEngine::rate() const
{
    return m_rate;
}

QList<VlcTrack> VlcEngine::tracks() const
{
    QList<VlcTrack> result;
    if (!m_player) {
        return result;
    }

    libvlc_media_t *media = libvlc_media_player_get_media(m_player);
    if (media) {
        libvlc_media_track_t **infos = nullptr;
        const unsigned count = libvlc_media_tracks_get(media, &infos);
        if (count > 0 && infos) {
            result.reserve(int(count));
            for (unsigned i = 0; i < count; ++i) {
                const libvlc_media_track_t *info = infos[i];
                if (!info) {
                    continue;
                }
                VlcTrack track;
                track.id = info->i_id;
                track.type = info->i_type;
                track.language = QString::fromUtf8(info->psz_language ? info->psz_language : "");
                track.title = QString::fromUtf8(info->psz_description ? info->psz_description : "");
                track.codec = fourccToString(info->i_codec);
                result.append(track);
            }
            libvlc_media_tracks_release(infos, count);
        }
        libvlc_media_release(media);
    }

    // Fallback to the track descriptions exposed by the player. These are
    // available even when the media could not be parsed into track info.
    auto addFromDescriptions = [&result](libvlc_track_description_t *descriptions, int type, const QString &fallbackTitle) {
        for (libvlc_track_description_t *d = descriptions; d; d = d->p_next) {
            if (d->i_id < 0) {
                continue;
            }
            const bool exists = std::any_of(result.cbegin(), result.cend(), [d, type](const VlcTrack &t) {
                return t.id == d->i_id && t.type == type;
            });
            if (exists) {
                continue;
            }
            VlcTrack track;
            track.id = d->i_id;
            track.type = type;
            track.title = QString::fromUtf8(d->psz_name ? d->psz_name : "");
            if (track.title.isEmpty()) {
                track.title = fallbackTitle;
            }
            result.append(track);
        }
    };

    while (result.isEmpty()) {
        libvlc_track_description_t *audio = libvlc_audio_get_track_description(m_player);
        addFromDescriptions(audio, libvlc_track_audio, tr("Audio"));
        libvlc_track_description_list_release(audio);

        libvlc_track_description_t *spu = libvlc_video_get_spu_description(m_player);
        addFromDescriptions(spu, libvlc_track_text, tr("Subtitle"));
        libvlc_track_description_list_release(spu);

        libvlc_track_description_t *video = libvlc_video_get_track_description(m_player);
        addFromDescriptions(video, libvlc_track_video, tr("Video"));
        libvlc_track_description_list_release(video);
        break;
    }

    return result;
}

int VlcEngine::audioTrack() const
{
    return m_player ? libvlc_audio_get_track(m_player) : -1;
}

void VlcEngine::setAudioTrack(int id)
{
    if (!m_player) {
        return;
    }
    libvlc_audio_set_track(m_player, id);
}

int VlcEngine::subtitleTrack() const
{
    return m_player ? libvlc_video_get_spu(m_player) : -1;
}

void VlcEngine::setSubtitleTrack(int id)
{
    if (!m_player) {
        return;
    }
    libvlc_video_set_spu(m_player, id);
}

int VlcEngine::chapter() const
{
    return m_player ? libvlc_media_player_get_chapter(m_player) : 0;
}

void VlcEngine::setChapter(int chapter)
{
    if (!m_player) {
        return;
    }
    libvlc_media_player_set_chapter(m_player, chapter);
}

QList<VlcChapter> VlcEngine::chapters() const
{
    QList<VlcChapter> result;
    if (!m_player) {
        return result;
    }

    libvlc_chapter_description_t **chapters = nullptr;
    const int count = libvlc_media_player_get_full_chapter_descriptions(m_player, -1, &chapters);
    if (count > 0 && chapters) {
        result.reserve(count);
        for (int i = 0; i < count; ++i) {
            VlcChapter chapter;
            chapter.title = QString::fromUtf8(chapters[i]->psz_name ? chapters[i]->psz_name : "");
            if (chapter.title.isEmpty()) {
                chapter.title = tr("Chapter %1").arg(i + 1);
            }
            chapter.start = double(chapters[i]->i_time_offset) / 1000.0;
            chapter.duration = double(chapters[i]->i_duration) / 1000.0;
            result.append(chapter);
        }
        libvlc_chapter_descriptions_release(chapters, count);
    }
    return result;
}

void VlcEngine::addSubtitleFile(const QUrl &url)
{
    if (!m_player) {
        return;
    }
    const QByteArray uri = url.isLocalFile() ? QFile::encodeName(url.toLocalFile()) : url.toEncoded();
    libvlc_media_player_add_slave(m_player, libvlc_media_slave_type_subtitle, uri.constData(), true);
}

bool VlcEngine::takeSnapshot(const QString &filePath)
{
    if (!m_player) {
        return false;
    }
    return libvlc_video_take_snapshot(m_player, 0, QFile::encodeName(filePath).constData(), 0, 0) == 0;
}

void VlcEngine::frameStep()
{
    if (m_player) {
        libvlc_media_player_next_frame(m_player);
    }
}

void VlcEngine::setSubtitleDelay(double seconds)
{
    m_subtitleDelay = seconds;
    if (m_player) {
        libvlc_video_set_spu_delay(m_player, int64_t(seconds * 1000000.0));
    }
}

double VlcEngine::subtitleDelay() const
{
    return m_subtitleDelay;
}

void VlcEngine::setDeinterlaced(bool enabled)
{
    if (m_player) {
        libvlc_video_set_deinterlace(m_player, enabled ? "yadif" : nullptr);
    }
    m_deinterlace = enabled;
}

bool VlcEngine::deinterlaced() const
{
    return m_deinterlace;
}

double VlcEngine::imageAdjustment(const QString &name) const
{
    if (name == u"brightness") {
        return m_brightness;
    }
    if (name == u"contrast") {
        return m_contrast;
    }
    if (name == u"saturation") {
        return m_saturation;
    }
    if (name == u"gamma") {
        return m_gamma;
    }
    return 0.0;
}

void VlcEngine::setImageAdjustment(const QString &name, double value)
{
    double *target = nullptr;

    if (name == u"brightness") {
        target = &m_brightness;
    } else if (name == u"contrast") {
        target = &m_contrast;
    } else if (name == u"saturation") {
        target = &m_saturation;
    } else if (name == u"gamma") {
        target = &m_gamma;
    }

    if (!target) {
        return;
    }

    const double clampedValue = std::clamp(value, -100.0, 100.0);
    if (qFuzzyCompare(*target, clampedValue)) {
        return;
    }
    *target = clampedValue;
    applyImageAdjustments();
}

void VlcEngine::applyImageAdjustments()
{
    if (!m_player) {
        return;
    }

    const auto toVlc = [](const QString &name, double value) {
        // Haruna uses mpv's -100..100 scale with 0 as neutral. VLC uses
        // floats with 1.0 as neutral. Gamma is multiplicative, the others
        // are additive.
        if (name == u"gamma") {
            return float(std::pow(2.0, value / 100.0));
        }
        return float(1.0 + value / 100.0);
    };

    const bool enabled = m_brightness != 0.0 || m_contrast != 0.0
        || m_saturation != 0.0 || m_gamma != 0.0;
    m_adjustmentsEnabled = enabled;
    libvlc_video_set_adjust_int(m_player, libvlc_adjust_Enable, enabled ? 1 : 0);
    if (!enabled) {
        return;
    }
    libvlc_video_set_adjust_float(m_player, libvlc_adjust_Brightness, toVlc(u"brightness"_s, m_brightness));
    libvlc_video_set_adjust_float(m_player, libvlc_adjust_Contrast, toVlc(u"contrast"_s, m_contrast));
    libvlc_video_set_adjust_float(m_player, libvlc_adjust_Saturation, toVlc(u"saturation"_s, m_saturation));
    libvlc_video_set_adjust_float(m_player, libvlc_adjust_Gamma, toVlc(u"gamma"_s, m_gamma));
}

void VlcEngine::resetImageAdjustments()
{
    m_brightness = 0.0;
    m_contrast = 0.0;
    m_saturation = 0.0;
    m_gamma = 0.0;
    if (m_player) {
        libvlc_video_set_adjust_int(m_player, libvlc_adjust_Enable, 0);
    }
    m_adjustmentsEnabled = false;
}

void VlcEngine::setAspectRatio(const QString &aspect)
{
    if (m_player) {
        libvlc_video_set_aspect_ratio(m_player, aspect.isEmpty() ? nullptr : aspect.toUtf8().constData());
    }
}

QString VlcEngine::mediaTitle() const
{
    return m_mediaTitle;
}

bool VlcEngine::takeFrame(QByteArray &data, int &bufferWidth, int &bufferHeight, int &displayWidth, int &displayHeight)
{
    QMutexLocker locker(&m_frameMutex);
    if (!m_newFrame || m_frontBuffer.isEmpty()) {
        return false;
    }
    data = m_frontBuffer;
    bufferWidth = m_bufferWidth;
    bufferHeight = m_bufferHeight;
    displayWidth = m_displayWidth;
    displayHeight = m_displayHeight;
    m_newFrame = false;
    return true;
}

void VlcEngine::onVlcEvent(const libvlc_event_t *event, void *data)
{
    auto *engine = static_cast<VlcEngine *>(data);
    const int type = int(event->type);
    QMetaObject::invokeMethod(engine, [engine, type]() {
        engine->processEvent(type);
    }, Qt::QueuedConnection);
}

void VlcEngine::processEvent(int type)
{
    switch (type) {
    case libvlc_MediaPlayerMediaChanged: {
        libvlc_media_t *media = m_player ? libvlc_media_player_get_media(m_player) : nullptr;
        QString title;
        if (media) {
            char *meta = libvlc_media_get_meta(media, libvlc_meta_Title);
            if (meta) {
                title = QString::fromUtf8(meta);
                libvlc_free(meta);
            }
            libvlc_media_release(media);
        }
        if (title.isEmpty()) {
            title = m_url.isLocalFile() ? QFileInfo(m_url.toLocalFile()).fileName() : m_url.fileName();
        }
        if (m_mediaTitle != title) {
            m_mediaTitle = title;
            Q_EMIT mediaTitleChanged(title);
        }
        break;
    }
    case libvlc_MediaPlayerOpening:
    case libvlc_MediaPlayerBuffering:
        Q_EMIT playbackStateChanged(int(libvlc_media_player_get_state(m_player)));
        break;
    case libvlc_MediaPlayerPlaying: {
        m_loaded = true;
        m_endEmitted = false;
        refreshVideoSize();
        refreshTracks();
        refreshChapters();

        libvlc_media_t *media = libvlc_media_player_get_media(m_player);
        if (media) {
            char *meta = libvlc_media_get_meta(media, libvlc_meta_Title);
            if (meta) {
                const QString title = QString::fromUtf8(meta);
                libvlc_free(meta);
                if (!title.isEmpty() && title != m_mediaTitle) {
                    m_mediaTitle = title;
                    Q_EMIT mediaTitleChanged(title);
                }
            }
            libvlc_media_release(media);
        }

        Q_EMIT playbackStateChanged(int(libvlc_Playing));
        Q_EMIT playing();
        break;
    }
    case libvlc_MediaPlayerPaused:
        Q_EMIT playbackStateChanged(int(libvlc_Paused));
        Q_EMIT paused();
        break;
    case libvlc_MediaPlayerStopped:
        if (!m_stopping && !m_endEmitted) {
            Q_EMIT playbackStateChanged(int(libvlc_Stopped));
            Q_EMIT stopped();
        }
        break;
    case libvlc_MediaPlayerEndReached:
        if (!m_endEmitted) {
            m_endEmitted = true;
            Q_EMIT playbackStateChanged(int(libvlc_Ended));
            Q_EMIT endReached();
        }
        break;
    case libvlc_MediaPlayerEncounteredError:
        Q_EMIT playbackStateChanged(int(libvlc_Error));
        Q_EMIT errorOccurred(tr("Playback error: %1").arg(m_url.toString()));
        break;
    case libvlc_MediaPlayerTimeChanged:
        Q_EMIT positionChanged(position());
        break;
    case libvlc_MediaPlayerLengthChanged:
        Q_EMIT durationChanged(duration());
        refreshTracks();
        refreshChapters();
        break;
    case libvlc_MediaPlayerVout:
        refreshVideoSize();
        refreshTracks();
        break;
    case libvlc_MediaPlayerESAdded:
    case libvlc_MediaPlayerESDeleted:
    case libvlc_MediaPlayerESSelected:
        refreshTracks();
        break;
    case libvlc_MediaPlayerChapterChanged:
        refreshChapters();
        break;
    default:
        break;
    }
}

void VlcEngine::refreshTracks()
{
    Q_EMIT tracksChanged();
}

void VlcEngine::refreshChapters()
{
    Q_EMIT chaptersChanged();
}

void VlcEngine::refreshVideoSize()
{
    if (!m_player) {
        return;
    }
    unsigned width = 0;
    unsigned height = 0;
    if (libvlc_video_get_size(m_player, 0, &width, &height) == 0 && width > 0 && height > 0) {
        if (int(width) != m_displayWidth || int(height) != m_displayHeight) {
            m_displayWidth = int(width);
            m_displayHeight = int(height);
            Q_EMIT videoSizeChanged(m_displayWidth, m_displayHeight);
        }
    }
}

void *VlcEngine::onVideoLock(void *opaque, void **planes)
{
    auto *engine = static_cast<VlcEngine *>(opaque);
    planes[0] = engine->m_backBuffer.data();
    return nullptr;
}

void VlcEngine::onVideoUnlock(void *opaque, void *picture, void *const *planes)
{
    Q_UNUSED(opaque)
    Q_UNUSED(picture)
    Q_UNUSED(planes)
}

void VlcEngine::onVideoDisplay(void *opaque, void *picture)
{
    Q_UNUSED(picture)
    auto *engine = static_cast<VlcEngine *>(opaque);
    {
        QMutexLocker locker(&engine->m_frameMutex);
        std::swap(engine->m_frontBuffer, engine->m_backBuffer);
        engine->m_newFrame = true;
    }
    Q_EMIT engine->frameReady();
}

unsigned VlcEngine::onVideoFormat(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
    std::memcpy(chroma, "RV32", 4);
    const int w = int(*width);
    const int h = int(*height);

    auto *engine = static_cast<VlcEngine *>(opaque ? *opaque : nullptr);
    if (engine) {
        QMutexLocker locker(&engine->m_frameMutex);
        if (w != engine->m_bufferWidth || h != engine->m_bufferHeight || engine->m_backBuffer.isEmpty()) {
            engine->m_bufferWidth = w;
            engine->m_bufferHeight = h;
            engine->m_backBuffer = QByteArray(w * h * 4, Qt::Uninitialized);
            engine->m_frontBuffer = QByteArray(w * h * 4, Qt::Uninitialized);
            engine->m_newFrame = false;
        }
        if (engine->m_displayWidth <= 0 || engine->m_displayHeight <= 0) {
            engine->m_displayWidth = w;
            engine->m_displayHeight = h;
        }
    }

    qCDebug(RunaplayerGeneral) << "Video format" << chroma << w << "x" << h;
    pitches[0] = unsigned(w * 4);
    lines[0] = unsigned(h);
    return 1;
}

void VlcEngine::onVideoCleanup(void *opaque)
{
    Q_UNUSED(opaque)
}
