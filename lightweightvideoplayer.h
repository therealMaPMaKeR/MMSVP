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
#include <QMargins>
#include <QTimer>
#include <QPointer>
#include <memory>
#include "qspinbox.h"
#include "vp_vlcplayer.h"
#include "keybindmanager.h"

// Forward declaration
class TemporaryMessageLabel;

/**
 * @class LightweightVideoPlayer
 * @brief Lightweight video player with essential playback features including fullscreen
 * 
 * Simple video player with core playback functionality and fullscreen support
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
    void setVolume(int volume, bool showMessage = false);
    void setPosition(qint64 position);
    void setPlaybackSpeed(qreal speed, bool showMessage = false);
    
    // Fullscreen management
    void toggleFullScreen();
    void enterFullScreen();
    void exitFullScreen();
    
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
    void fullScreenChanged(bool isFullScreen);

protected slots:
    // UI control slots
    void on_playButton_clicked();
    void on_positionSlider_sliderMoved(int position);
    void on_positionSlider_sliderPressed();
    void on_positionSlider_sliderReleased();
    void on_volumeSlider_sliderMoved(int position);
    void on_speedSpinBox_valueChanged(double value);
    void on_fullScreenButton_clicked();
    
    // Media player slots
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void handleError(const QString &errorString);
    void handlePlaybackStateChanged(VP_VLCPlayer::PlayerState state);
    void handleVideoFinished();
    
    // Cursor management
    void hideCursor();
    void showCursor();
    void checkMouseMovement();
    
protected:
    // Event handlers
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    // UI setup methods
    void setupUI();
    void createControls();
    void createLayouts();
    void connectSignals();
    
    // Helper methods
    QString formatTime(qint64 milliseconds) const;
    void startCursorTimer();
    void stopCursorTimer();
    
    // Custom slider class for clickable seeking
    class ClickableSlider;
    ClickableSlider* createClickableSlider();
    
    // Core media components
    std::unique_ptr<VP_VLCPlayer> m_mediaPlayer;
    QPointer<QWidget> m_videoWidget;
    
    // Control widgets
    QPointer<QPushButton> m_playButton;
    QPointer<QPushButton> m_stopButton;
    QPointer<QPushButton> m_fullScreenButton;
    QPointer<QPushButton> m_keybindsButton;
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
    bool m_isFullScreen;
    QRect m_normalGeometry;
    QMargins m_normalMargins;
    bool m_isClosing;
    bool m_playbackStartedEmitted;
    
    // Mouse cursor auto-hide
    QTimer* m_cursorTimer;
    QTimer* m_mouseCheckTimer;
    QPoint m_lastMousePos;
    
    // Keybind manager
    std::unique_ptr<KeybindManager> m_keybindManager;
    
    // Temporary message display
    TemporaryMessageLabel* m_messageLabel;
    
    // Loop mode enumeration
    enum class LoopMode {
        NoLoop,
        LoopSingle,
        LoopAll
    };
    
    // Playback state system
    struct PlaybackState {
        qint64 startPosition;
        qint64 endPosition;
        qreal playbackSpeed;
        bool isValid;
        bool hasEndPosition;
        
        PlaybackState() : startPosition(0), endPosition(0), playbackSpeed(1.0), isValid(false), hasEndPosition(false) {}
        PlaybackState(qint64 start, qreal speed) : startPosition(start), endPosition(0), playbackSpeed(speed), isValid(true), hasEndPosition(false) {}
    };
    
    PlaybackState m_playbackStates[4][12];  // 4 groups x 12 states for keys 1,2,3,4,5,6,7,8,9,0,-,=
    int m_currentStateGroup;  // Current active state group (0-3)
    LoopMode m_loopMode;
    bool m_loadPlaybackSpeed;
    int m_currentLoopStateIndex;  // Track which state is currently looping
    qint64 m_lastClickedPosition;  // Last position clicked on slider
    
private:
    void initializePlayer();
    void openKeybindEditor();
    void savePlaybackState(int stateIndex);
    void setLoopEndPosition(int stateIndex);
    void loadPlaybackState(int stateIndex);
    void deletePlaybackState(int stateIndex);
    void toggleLoadPlaybackSpeed();
    void cycleLoopMode();
    void returnToLastPosition();
    void checkLoopPoint();
    int getStateIndexFromKey(Qt::Key key) const;
    int getStateIndexFromKeySequence(const QKeySequence& keySeq) const;
    QString getLoopModeString() const;
    void showTemporaryMessage(const QString& message);
    
    // State group management
    void switchStateGroup(int groupIndex);
    void saveStatesToFile();
    void loadStatesFromFile();
    QString getStatesFilePath(int groupIndex) const;
    int findFirstValidLoop() const;
};

// Temporary message label overlay
class TemporaryMessageLabel : public QLabel
{
    Q_OBJECT
public:
    explicit TemporaryMessageLabel(QWidget* parent = nullptr);
    void showMessage(const QString& message, int durationMs = 2000);

private:
    QTimer* m_fadeTimer;
};

#endif // LIGHTWEIGHTVIDEOPLAYER_H
