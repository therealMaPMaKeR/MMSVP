#include "videoplayer.h"
#include <QDebug>
#include <QResizeEvent>
#include <QUrl>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
    , m_vlcInstance(nullptr)
    , m_mediaPlayer(nullptr)
    , m_media(nullptr)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setMinimumSize(320, 240);
    
    initializeVLC();
}

VideoPlayer::~VideoPlayer()
{
    cleanupVLC();
}

void VideoPlayer::initializeVLC()
{
    // Create VLC instance
    const char *vlc_args[] = {
        "--no-xlib" // Don't use Xlib for Linux compatibility
    };
    
    m_vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    
    if (!m_vlcInstance) {
        qCritical() << "Failed to create VLC instance";
        return;
    }
    
    // Create media player
    m_mediaPlayer = libvlc_media_player_new(m_vlcInstance);
    
    if (!m_mediaPlayer) {
        qCritical() << "Failed to create VLC media player";
        return;
    }
    
    // Set the window handle for video output
#if defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(m_mediaPlayer, reinterpret_cast<void*>(winId()));
#elif defined(Q_OS_MACOS)
    libvlc_media_player_set_nsobject(m_mediaPlayer, reinterpret_cast<void*>(winId()));
#else
    libvlc_media_player_set_xwindow(m_mediaPlayer, winId());
#endif
    
    qDebug() << "VLC initialized successfully";
}

void VideoPlayer::cleanupVLC()
{
    if (m_media) {
        libvlc_media_release(m_media);
        m_media = nullptr;
    }
    
    if (m_mediaPlayer) {
        libvlc_media_player_stop(m_mediaPlayer);
        libvlc_media_player_release(m_mediaPlayer);
        m_mediaPlayer = nullptr;
    }
    
    if (m_vlcInstance) {
        libvlc_release(m_vlcInstance);
        m_vlcInstance = nullptr;
    }
}

void VideoPlayer::playMedia(const QString &filePath)
{
    if (!m_vlcInstance || !m_mediaPlayer) {
        qWarning() << "VLC not initialized";
        return;
    }
    
    // Release previous media if any
    if (m_media) {
        libvlc_media_release(m_media);
        m_media = nullptr;
    }
    
    // Convert to proper file URI for better compatibility
    QString fileUri = QUrl::fromLocalFile(filePath).toString();
    qDebug() << "Loading media:" << filePath;
    qDebug() << "URI:" << fileUri;
    
    // Create new media using location (URI) for better compatibility
    m_media = libvlc_media_new_location(m_vlcInstance, fileUri.toUtf8().constData());
    
    if (!m_media) {
        qCritical() << "Failed to create media from path:" << filePath;
        return;
    }
    
    // Set media to player
    libvlc_media_player_set_media(m_mediaPlayer, m_media);
    
    // Start playback
    libvlc_media_player_play(m_mediaPlayer);
    
    qDebug() << "Playing media:" << filePath;
}

void VideoPlayer::play()
{
    if (m_mediaPlayer) {
        libvlc_media_player_play(m_mediaPlayer);
    }
}

void VideoPlayer::pause()
{
    if (m_mediaPlayer) {
        libvlc_media_player_pause(m_mediaPlayer);
    }
}

void VideoPlayer::stop()
{
    if (m_mediaPlayer) {
        libvlc_media_player_stop(m_mediaPlayer);
    }
}

void VideoPlayer::setPosition(float position)
{
    if (m_mediaPlayer) {
        libvlc_media_player_set_position(m_mediaPlayer, position);
    }
}

void VideoPlayer::setVolume(int volume)
{
    if (m_mediaPlayer) {
        libvlc_audio_set_volume(m_mediaPlayer, volume);
    }
}

float VideoPlayer::getPosition() const
{
    if (m_mediaPlayer) {
        return libvlc_media_player_get_position(m_mediaPlayer);
    }
    return 0.0f;
}

qint64 VideoPlayer::getLength() const
{
    if (m_mediaPlayer) {
        return libvlc_media_player_get_length(m_mediaPlayer);
    }
    return 0;
}

qint64 VideoPlayer::getTime() const
{
    if (m_mediaPlayer) {
        return libvlc_media_player_get_time(m_mediaPlayer);
    }
    return 0;
}

bool VideoPlayer::isPlaying() const
{
    if (m_mediaPlayer) {
        return libvlc_media_player_is_playing(m_mediaPlayer) == 1;
    }
    return false;
}

void VideoPlayer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}
