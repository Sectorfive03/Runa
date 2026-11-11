/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 * SPDX-FileCopyrightText: 2025 Muhammet Sadık Uğursoy <sadikugursoy@gmail.com>
 * SPDX-FileCopyrightText: 2026 Haruna VLC port contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VLCITEM_H
#define VLCITEM_H

#include <QQuickFramebufferObject>
#include <QUrl>

#include <memory>

class SeekableRangesModel;
class ChaptersModel;
class TracksModel;
class PlaylistsManager;
class VlcEngine;
class VlcRenderer;

class VlcItem : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit VlcItem(QQuickItem *parent = nullptr);
    ~VlcItem() override;

    enum class PlaybackState {
        Stopped,
        Loading,
        Playing,
        Paused,
    };
    Q_ENUM(PlaybackState)

    enum class TrackType {
        Video,
        Audio,
        Subtitle,
    };
    Q_ENUM(TrackType)

    enum class EndOfFileAction {
        PlayNext,
        Replay,
        Stop,
    };

    Renderer *createRenderer() const override;

    Q_PROPERTY(SeekableRangesModel *seekableRangesModel READ seekableRangesModel NOTIFY seekableRangesModelChanged FINAL)
    SeekableRangesModel *seekableRangesModel() const;

    Q_PROPERTY(TracksModel *audioTracksModel READ audioTracksModel NOTIFY audioTracksModelChanged)
    TracksModel *audioTracksModel() const;

    Q_PROPERTY(TracksModel *subtitleTracksModel READ subtitleTracksModel NOTIFY subtitleTracksModelChanged)
    TracksModel *subtitleTracksModel() const;

    Q_PROPERTY(ChaptersModel *chaptersModel READ chaptersModel WRITE setChaptersModel NOTIFY chaptersModelChanged)
    ChaptersModel *chaptersModel() const;
    void setChaptersModel(ChaptersModel *_chaptersModel);

    Q_PROPERTY(PlaylistsManager *playlistsManager READ playlistsManager WRITE setPlaylistsManager NOTIFY playlistsManagerChanged FINAL REQUIRED)
    PlaylistsManager *playlistsManager() const;
    void setPlaylistsManager(PlaylistsManager *newPlaylistsManager);

    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged)
    QString mediaTitle();

    Q_PROPERTY(double position READ position WRITE setPosition NOTIFY positionChanged)
    double position();
    void setPosition(double value);

    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    double duration();

    Q_PROPERTY(double remaining READ remaining NOTIFY remainingChanged)
    double remaining();

    Q_PROPERTY(QString formattedPosition READ formattedPosition NOTIFY positionChanged)
    QString formattedPosition() const;

    Q_PROPERTY(QString formattedRemaining READ formattedRemaining NOTIFY remainingChanged)
    QString formattedRemaining() const;

    Q_PROPERTY(QString formattedDuration READ formattedDuration NOTIFY durationChanged)
    QString formattedDuration() const;

    Q_PROPERTY(double watchLaterPosition READ watchLaterPosition WRITE setWatchLaterPosition NOTIFY watchLaterPositionChanged)
    double watchLaterPosition() const;
    void setWatchLaterPosition(double _watchLaterPosition);

    Q_PROPERTY(double watchPercentage READ watchPercentage WRITE setWatchPercentage NOTIFY watchPercentageChanged)
    double watchPercentage();
    void setWatchPercentage(double value);

    Q_PROPERTY(bool pause READ pause WRITE setPause NOTIFY pauseChanged)
    bool pause();
    void setPause(bool value);

    Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
    bool mute();
    void setMute(bool value);

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    int volume();
    void setVolume(int value);

    Q_PROPERTY(int volumeMax READ volumeMax WRITE setVolumeMax NOTIFY volumeMaxChanged)
    int volumeMax();
    void setVolumeMax(int value);

    Q_PROPERTY(int chapter READ chapter WRITE setChapter NOTIFY chapterChanged)
    int chapter();
    void setChapter(int value);

    Q_PROPERTY(int audioId READ audioId WRITE setAudioId NOTIFY audioIdChanged)
    int audioId();
    void setAudioId(int value);

    Q_PROPERTY(int subtitleId READ subtitleId WRITE setSubtitleId NOTIFY subtitleIdChanged)
    int subtitleId();
    void setSubtitleId(int value);

    Q_PROPERTY(bool subtitlesVisible READ subtitlesVisible WRITE setSubtitlesVisible NOTIFY subtitlesVisibleChanged)
    bool subtitlesVisible() const;
    void setSubtitlesVisible(bool visible);

    Q_PROPERTY(double subtitleDelay READ subtitleDelay WRITE setSubtitleDelay NOTIFY subtitleDelayChanged)
    double subtitleDelay() const;
    void setSubtitleDelay(double seconds);

    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoWidthChanged)
    int videoWidth();

    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoHeightChanged)
    int videoHeight();

    Q_PROPERTY(double playbackSpeed READ playbackSpeed WRITE setPlaybackSpeed NOTIFY playbackSpeedChanged FINAL)
    double playbackSpeed() const;
    void setPlaybackSpeed(double newSpeed);

    Q_PROPERTY(bool finishedLoading READ finishedLoading WRITE setFinishedLoading NOTIFY finishedLoadingChanged)
    bool finishedLoading() const;
    void setFinishedLoading(bool _finishedLoading);

    Q_PROPERTY(QUrl currentUrl READ currentUrl NOTIFY currentUrlChanged)
    QUrl currentUrl() const;

    Q_PROPERTY(bool isReady READ isReady WRITE setIsReady NOTIFY isReadyChanged)
    bool isReady() const;
    void setIsReady(bool _isReady);

    Q_PROPERTY(PlaybackState playbackState READ playbackState WRITE setPlaybackState NOTIFY playbackStateChanged FINAL)
    PlaybackState playbackState() const;
    void setPlaybackState(PlaybackState newPlaybackState);

    Q_PROPERTY(bool isLocalFile READ isLocalFile WRITE setIsLocalFile NOTIFY isLocalFileChanged)
    bool isLocalFile() const;
    void setIsLocalFile(bool newIsLocalFile);

    Q_PROPERTY(bool isVideo READ isVideo WRITE setIsVideo NOTIFY isVideoChanged)
    bool isVideo() const;
    void setIsVideo(bool newIsVideo);

    Q_PROPERTY(double abLoopA READ abLoopA WRITE setABLoopA NOTIFY abLoopChanged)
    double abLoopA() const;
    void setABLoopA(double seconds);

    Q_PROPERTY(double abLoopB READ abLoopB WRITE setABLoopB NOTIFY abLoopChanged)
    double abLoopB() const;
    void setABLoopB(double seconds);

    Q_PROPERTY(bool deinterlaced READ deinterlaced WRITE setDeinterlaced NOTIFY deinterlacedChanged)
    bool deinterlaced() const;
    void setDeinterlaced(bool enabled);

    Q_INVOKABLE void loadFile(const QString &file);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void selectSubtitleTrack();
    Q_INVOKABLE void addSubtitles(const QString &subtitlesFile);

    Q_INVOKABLE void setTrack(int row, TrackType type);
    Q_INVOKABLE void setNextTrack(TrackType type);
    Q_INVOKABLE void setPreviousTrack(TrackType type);

    Q_INVOKABLE void seekAbsolute(double seconds);
    Q_INVOKABLE void seekRelative(double seconds);
    Q_INVOKABLE void nextChapter();
    Q_INVOKABLE void previousChapter();
    Q_INVOKABLE void frameStepForward();
    Q_INVOKABLE void takeScreenshot();

    Q_INVOKABLE double imageAdjustment(const QString &name) const;
    Q_INVOKABLE void setImageAdjustment(const QString &name, double value);
    Q_INVOKABLE void resetImageAdjustments();

    Q_INVOKABLE void setAspectRatio(const QString &aspect);
    Q_INVOKABLE void clearABLoop();

Q_SIGNALS:
    void seekableRangesModelChanged();
    void audioTracksModelChanged();
    void subtitleTracksModelChanged();
    void chaptersModelChanged();
    void playlistsManagerChanged();
    void finishedLoadingChanged();
    void mediaTitleChanged();
    void currentUrlChanged();
    void watchPercentageChanged();
    void positionChanged();
    void watchLaterPositionChanged();
    void durationChanged();
    void remainingChanged();
    void pauseChanged();
    void muteChanged();
    void volumeChanged();
    void volumeMaxChanged();
    void chapterChanged();
    void chapterListChanged();
    void audioIdChanged();
    void subtitleIdChanged();
    void subtitlesVisibleChanged();
    void subtitleDelayChanged();
    void isReadyChanged();
    void playbackStateChanged();
    void fileStarted();
    void videoReconfig();
    void videoWidthChanged();
    void videoHeightChanged();
    void eofReached();
    void playbackSpeedChanged();
    void savePositionToDB(const QString &md5Hash, const QString &path, double position);
    void eofPlayNext();
    void isLocalFileChanged();
    void isVideoChanged();
    void abLoopChanged();
    void deinterlacedChanged();

    // signals used for mpris
    void playNext();
    void playPrevious();
    void openUri(const QString &uri);

private:
    friend class VlcRenderer;

    void setupConnections();
    void onFileLoaded();
    void onPlaybackStateChanged(int state);
    void onEndOfFileReached();
    VlcItem::EndOfFileAction endOfFileAction() const;
    void saveTimePosition();
    double loadTimePosition();
    void resetTimePosition();
    void loadTracks();
    void loadChapters();
    void onChapterChanged();
    void updateTaskbarPlaybackProgress();
    QString buildScreenshotPath() const;

    std::unique_ptr<VlcEngine> m_engine;
    std::unique_ptr<TracksModel> m_audioTracksModel;
    std::unique_ptr<TracksModel> m_subtitleTracksModel;
    std::unique_ptr<ChaptersModel> m_chaptersModel;
    std::unique_ptr<SeekableRangesModel> m_seekableRangesModel;
    std::unique_ptr<QTimer> m_saveTimePositionTimer;

    QSet<int> m_secondsWatched;
    double m_watchPercentage{0.0};

    double m_position{0.0};
    double m_remaining{0.0};
    double m_duration{0.0};
    QString m_mediaTitle;
    bool m_pause{true};
    bool m_mute{false};
    int m_chapter{0};
    int m_volume{100};
    int m_volumeMax{100};
    int m_audioId{0};
    int m_subtitleId{0};
    int m_videoWidth{0};
    int m_videoHeight{0};
    double m_playbackSpeed{1.0};
    double m_subtitleDelay{0.0};
    bool m_subtitlesVisible{true};
    int m_lastSubtitleTrack{-1};
    double m_abLoopA{-1.0};
    double m_abLoopB{-1.0};
    bool m_deinterlaced{false};

    double m_watchLaterPosition{0.0};
    bool m_isReady{false};
    QUrl m_currentUrl;
    bool m_finishedLoading{false};
    PlaybackState m_playbackState{PlaybackState::Stopped};
    PlaylistsManager *m_playlistsManager = nullptr;
    bool m_isLocalFile{false};
    bool m_isVideo{false};
    bool m_pausedByUs{false};
};

#endif // VLCITEM_H
