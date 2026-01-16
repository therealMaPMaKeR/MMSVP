#ifndef LIGHTWEIGHTVIDEOPLAYER_H
#define LIGHTWEIGHTVIDEOPLAYER_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QPointer>
#include <memory>
#include "qspinbox.h"
#include "vp_vlcplayer.h"

/**
 * @class LightweightVideoPlayer
 * @brief Lightweight video player with essential playback features
 * 
 * Stripped-down version of BaseVideoPlayer without encryption,
 * fullscreen, or advanced features - just core playback functionality
 */
class LightweightVideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit LightweightVideoPlayer(QWidget *parent = nullptr, int initialVolume = 70);
    virtual ~LightweightVideoPlayer();

    // Core video control functions
    bool loadVideo(const QString& filePath);
    void play();
    void pause();
    void stop();
    void setVolume(int volume);
    void setPosition(qint64 position);
    void setPlaybackSpeed(qreal speed);
    
    // State query functions
    bool isPlaying() const;
    bool isPaused() const;
    qint64 duration() const;
    qint64 position() const;
    int volume() const;
    qreal playbackSpeed() const;
    QString currentVideoPath() const;

signals:
    void errorOccurred(const QString& error);
    void playbackStateChanged(VP_VLCPlayer::PlayerState state);
    void playbackStarted();
    void finished();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(int volume);
    void playbackSpeedChanged(qreal speed);

protected slots:
    // UI control slots
    void on_playButton_clicked();
    void on_positionSlider_sliderMoved(int position);
    void on_positionSlider_sliderPressed();
    void on_positionSlider_sliderReleased();
    void on_volumeSlider_sliderMoved(int position);
    void on_speedSpinBox_valueChanged(double value);
    
    // Media player slots
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void handleError(const QString &errorString);
    void handlePlaybackStateChanged(VP_VLCPlayer::PlayerState state);
    void handleVideoFinished();
    
protected:
    // Event handlers
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    // UI setup methods
    void setupUI();
    void createControls();
    void createLayouts();
    void connectSignals();
    
    // Helper methods
    QString formatTime(qint64 milliseconds) const;
    
    // Custom slider class for clickable seeking
    class ClickableSlider;
    ClickableSlider* createClickableSlider();
    
    // Core media components
    std::unique_ptr<VP_VLCPlayer> m_mediaPlayer;
    QPointer<QWidget> m_videoWidget;
    
    // Control widgets
    QPointer<QPushButton> m_playButton;
    QPointer<QPushButton> m_stopButton;
    QPointer<QSlider> m_positionSlider;
    QPointer<QSlider> m_volumeSlider;
    QPointer<QDoubleSpinBox> m_speedSpinBox;
    QPointer<QLabel> m_positionLabel;
    QPointer<QLabel> m_durationLabel;
    QPointer<QLabel> m_volumeLabel;
    QPointer<QLabel> m_speedLabel;
    QPointer<QWidget> m_controlsWidget;
    
    // Layout containers
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_controlLayout;
    QHBoxLayout* m_sliderLayout;
    
    // State tracking
    QString m_currentVideoPath;
    bool m_isSliderBeingMoved;
    bool m_isClosing;
    bool m_playbackStartedEmitted;
    
private:
    void initializePlayer();
};

#endif // LIGHTWEIGHTVIDEOPLAYER_H
