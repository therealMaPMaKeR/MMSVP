#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <vlc/vlc.h>

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

    void playMedia(const QString &filePath);
    void play();
    void pause();
    void stop();
    void setPosition(float position); // 0.0 to 1.0
    void setVolume(int volume); // 0 to 100
    float getPosition() const;
    qint64 getLength() const;
    qint64 getTime() const;
    bool isPlaying() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    libvlc_instance_t *m_vlcInstance;
    libvlc_media_player_t *m_mediaPlayer;
    libvlc_media_t *m_media;

    void initializeVLC();
    void cleanupVLC();
};

#endif // VIDEOPLAYER_H
