/*
 * SPDX-FileCopyrightText: 2026 Haruna VLC port contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VLCENGINE_H
#define VLCENGINE_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <vlc/vlc.h>

struct VlcTrack {
    int id{-1};
    int type{-1}; // libvlc_track_audio / video / text
    QString title;
    QString language;
    QString codec;
};

struct VlcChapter {
    QString title;
    double start{0.0};
    double duration{0.0};
};

/**
 * Thin, Qt-friendly wrapper around a single libvlc media player.
 *
 * Video is decoded into memory through libvlc's vmem output. Frames are
 * delivered as BGRA and consumed by the QML renderer (VlcItem/VlcRenderer).
 * Subpictures, filters and image adjustments are rendered by VLC itself,
 * so the frames we get already contain subtitles.
 */
class VlcEngine : public QObject
{
    Q_OBJECT

public:
    explicit VlcEngine(QObject *parent = nullptr);
    ~VlcEngine() override;

    bool load(const QUrl &url, double startSeconds = 0.0, const QString &hardwareDecoding = QString());
    void setPreferredLanguages(const QString &audio, const QString &sub);
    void stop();
    void play();
    void setPaused(bool paused);
    bool isPaused() const;
    bool isPlaying() const;

    void setPosition(double seconds);
    double position() const;
    double duration() const;

    void setVolume(int volume);
    int volume() const;
    void setMuted(bool muted);
    bool isMuted() const;

    bool setRate(double rate);
    double rate() const;

    int audioTrack() const;
    void setAudioTrack(int id);
    int subtitleTrack() const;
    void setSubtitleTrack(int id);
    QList<VlcTrack> tracks() const;

    int chapter() const;
    void setChapter(int chapter);
    QList<VlcChapter> chapters() const;

    void addSubtitleFile(const QUrl &url);
    bool takeSnapshot(const QString &filePath);
    void frameStep();

    void setSubtitleDelay(double seconds);
    double subtitleDelay() const;

    bool deinterlaced() const;
    void setDeinterlaced(bool enabled);

    double imageAdjustment(const QString &name) const;
    void setImageAdjustment(const QString &name, double value);
    void resetImageAdjustments();

    void setAspectRatio(const QString &aspect);

    QString mediaTitle() const;

    /// Thread safe. Copies the latest frame for the render thread.
    bool takeFrame(QByteArray &data, int &bufferWidth, int &bufferHeight, int &displayWidth, int &displayHeight);

Q_SIGNALS:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void mediaTitleChanged(const QString &title);
    void playbackStateChanged(int state);
    void playing();
    void paused();
    void stopped();
    void endReached();
    void errorOccurred(const QString &message);
    void tracksChanged();
    void chaptersChanged();
    void videoSizeChanged(int width, int height);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void frameReady();

private:
    static void onVlcEvent(const libvlc_event_t *event, void *data);
    void processEvent(int type);

    static void *onVideoLock(void *opaque, void **planes);
    static void onVideoUnlock(void *opaque, void *picture, void *const *planes);
    static void onVideoDisplay(void *opaque, void *picture);
    static unsigned onVideoFormat(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines);
    static void onVideoCleanup(void *opaque);

    void refreshTracks();
    void refreshChapters();
    void refreshVideoSize();
    void applyImageAdjustments();

    libvlc_instance_t *m_instance{nullptr};
    libvlc_media_player_t *m_player{nullptr};
    libvlc_event_manager_t *m_eventManager{nullptr};

    QUrl m_url;
    QString m_mediaTitle;
    QString m_audioLanguage;
    QString m_subtitleLanguage;
    double m_startSeconds{0.0};
    int m_volume{100};
    bool m_muted{false};
    double m_rate{1.0};
    double m_subtitleDelay{0.0};
    bool m_stopping{false};
    bool m_endEmitted{false};
    bool m_loaded{false};

    // Video frame exchange with the VLC vout thread.
    QMutex m_frameMutex;
    QByteArray m_frontBuffer;
    QByteArray m_backBuffer;
    int m_bufferWidth{0};
    int m_bufferHeight{0};
    int m_displayWidth{0};
    int m_displayHeight{0};
    bool m_newFrame{false};

    // Image adjustment state on Haruna's scale (-100..100, 0 is neutral).
    // These are converted to libvlc's 1.0-neutral floats when applied.
    double m_brightness{0.0};
    double m_contrast{0.0};
    double m_saturation{0.0};
    double m_gamma{0.0};
    bool m_adjustmentsEnabled{false};
    bool m_deinterlace{false};
};

#endif // VLCENGINE_H
