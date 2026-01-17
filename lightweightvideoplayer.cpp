#include "lightweightvideoplayer.h"
#include "keybindeditordialog.h"
#include "stateseditordialog.h"
#include <QGuiApplication>
#include <QDebug>
#include <QFileInfo>
#include <QStyle>
#include <QTime>
#include <QCloseEvent>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QScreen>
#include <QCursor>
#include <QBuffer>
#include <QEventLoop>
#include <QTimer>

// Custom clickable slider class for seeking in video
class LightweightVideoPlayer::ClickableSlider : public QSlider
{
public:
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSlider(orientation, parent), m_isPressed(false) {}

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            m_isPressed = true;
            
            // Calculate position based on click
            qint64 value = 0;
            
            if (orientation() == Qt::Horizontal) {
                qreal clickPos = event->position().x();
                qreal widgetWidth = width();
                
                qint64 range = static_cast<qint64>(maximum()) - static_cast<qint64>(minimum());
                qint64 widgetSize = static_cast<qint64>(widgetWidth);
                
                if (widgetSize > 0) {
                    value = minimum() + (range * clickPos) / widgetSize;
                } else {
                    value = minimum();
                }
            } else {
                qreal clickPos = height() - event->position().y();
                qreal widgetHeight = height();
                
                qint64 range = static_cast<qint64>(maximum()) - static_cast<qint64>(minimum());
                qint64 widgetSize = static_cast<qint64>(widgetHeight);
                
                if (widgetSize > 0) {
                    value = minimum() + (range * clickPos) / widgetSize;
                } else {
                    value = minimum();
                }
            }
            
            value = qBound(static_cast<qint64>(minimum()), value, static_cast<qint64>(maximum()));
            
            setValue(static_cast<int>(value));
            emit sliderMoved(static_cast<int>(value));
            emit sliderPressed();
            
            QSlider::mousePressEvent(event);
        }
        else {
            QSlider::mousePressEvent(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (m_isPressed) {
            m_isPressed = false;
            emit sliderReleased();
        }
        QSlider::mouseReleaseEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        if (m_isPressed) {
            m_isPressed = false;
            emit sliderReleased();
        }
        QSlider::focusOutEvent(event);
    }

private:
    bool m_isPressed;
};

LightweightVideoPlayer::LightweightVideoPlayer(QWidget *parent, int initialVolume)
    : QWidget(parent)
    , m_videoWidget(nullptr)
    , m_playButton(nullptr)
    , m_stopButton(nullptr)
    , m_fullScreenButton(nullptr)
    , m_keybindsButton(nullptr)
    , m_editStatesButton(nullptr)
    , m_positionSlider(nullptr)
    , m_volumeSlider(nullptr)
    , m_speedSpinBox(nullptr)
    , m_positionLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_volumeLabel(nullptr)
    , m_speedLabel(nullptr)
    , m_controlsWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_sliderLayout(nullptr)
    , m_isSliderBeingMoved(false)
    , m_isFullScreen(false)
    , m_isClosing(false)
    , m_playbackStartedEmitted(false)
    , m_cursorTimer(nullptr)
    , m_mouseCheckTimer(nullptr)
    , m_lastMousePos(QPoint(-1, -1))
    , m_messageLabel(nullptr)
    , m_currentStateGroup(0)
    , m_loopMode(LoopMode::NoLoop)
    , m_loadPlaybackSpeed(true)
    , m_currentLoopStateIndex(-1)
    , m_lastClickedPosition(-1)
{
    qDebug() << "LightweightVideoPlayer: Constructor called";
    
    // Enable mouse tracking for auto-hide cursor functionality
    setMouseTracking(true);
    
    // Set window properties
    setWindowTitle(tr("Lightweight Video Player"));
    resize(800, 600);
    
    // Center window on screen
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - rect().center());
    }
    
    // Initialize the player
    initializePlayer();
    
    // Set initial volume
    if (m_mediaPlayer) {
        m_mediaPlayer->setVolume(initialVolume);
    }
}

LightweightVideoPlayer::~LightweightVideoPlayer()
{
    qDebug() << "LightweightVideoPlayer: Destructor called";
    
    // Stop timers
    if (m_cursorTimer) {
        m_cursorTimer->stop();
        delete m_cursorTimer;
    }
    
    if (m_mouseCheckTimer) {
        m_mouseCheckTimer->stop();
        delete m_mouseCheckTimer;
    }
    
    // Stop media player
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}

void LightweightVideoPlayer::initializePlayer()
{
    qDebug() << "LightweightVideoPlayer: Initializing player";

    // Initialize keybind manager
    m_keybindManager = std::make_unique<KeybindManager>(this);
    if (!m_keybindManager->initialize()) {
        qDebug() << "LightweightVideoPlayer: Failed to initialize keybind manager";
        QMessageBox::warning(this, tr("Warning"), 
                           tr("Failed to initialize keybind system. Using defaults."));
    }

    // Create VLC player instance
    m_mediaPlayer = std::make_unique<VP_VLCPlayer>(this);

    if (!m_mediaPlayer->initialize()) {
        qDebug() << "LightweightVideoPlayer: Failed to initialize VLC player";
        emit errorOccurred(tr("Failed to initialize video player"));
        return;
    }

    // Setup UI
    setupUI();

    // Connect signals
    connectSignals();
    
    // Initialize cursor timers for fullscreen auto-hide
    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setSingleShot(true);
    connect(m_cursorTimer, &QTimer::timeout, this, &LightweightVideoPlayer::hideCursor);

    m_mouseCheckTimer = new QTimer(this);
    m_mouseCheckTimer->setInterval(100);
    connect(m_mouseCheckTimer, &QTimer::timeout, this, &LightweightVideoPlayer::checkMouseMovement);
    
    qDebug() << "LightweightVideoPlayer: Initialization complete";
}

void LightweightVideoPlayer::setupUI()
{
    qDebug() << "LightweightVideoPlayer: Setting up UI";
    
    // Create video widget
    m_videoWidget = new QWidget(this);
    m_videoWidget->setMinimumSize(400, 300);
    m_videoWidget->setStyleSheet("background-color: black;");
    m_videoWidget->setAutoFillBackground(true);
    
    // Set VLC video output widget
    m_mediaPlayer->setVideoWidget(m_videoWidget);
    
    m_videoWidget->show();
    m_videoWidget->installEventFilter(this);
    
    // Enable mouse tracking on video widget too
    m_videoWidget->setMouseTracking(true);
    
    // Set focus policy
    setFocusPolicy(Qt::StrongFocus);
    m_videoWidget->setFocusPolicy(Qt::StrongFocus);
    
    // Create temporary message label
    m_messageLabel = new TemporaryMessageLabel(this);
    m_messageLabel->setVisible(false);
    m_messageLabel->raise();  // Ensure it's on top
    
    // Create controls
    createControls();
    
    // Create layouts
    createLayouts();
}

LightweightVideoPlayer::ClickableSlider* LightweightVideoPlayer::createClickableSlider()
{
    return new ClickableSlider(Qt::Horizontal, this);
}

void LightweightVideoPlayer::createControls()
{
    qDebug() << "LightweightVideoPlayer: Creating controls";
    
    // Play/Pause button
    m_playButton = new QPushButton(this);
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playButton->setToolTip(tr("Play"));
    m_playButton->setFocusPolicy(Qt::NoFocus);
    
    // Stop button
    m_stopButton = new QPushButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setToolTip(tr("Stop"));
    m_stopButton->setFocusPolicy(Qt::NoFocus);
    
    // Fullscreen button
    m_fullScreenButton = new QPushButton(this);
    m_fullScreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    m_fullScreenButton->setToolTip(tr("Full Screen (F11)"));
    m_fullScreenButton->setFocusPolicy(Qt::NoFocus);
    
    // Keybinds button
    m_keybindsButton = new QPushButton(tr("Keybinds"), this);
    m_keybindsButton->setToolTip(tr("Edit Keybinds"));
    m_keybindsButton->setFocusPolicy(Qt::NoFocus);
    
    // Edit States button
    m_editStatesButton = new QPushButton(tr("Edit States"), this);
    m_editStatesButton->setToolTip(tr("Edit Playback States"));
    m_editStatesButton->setFocusPolicy(Qt::NoFocus);
    
    // Position slider
    m_positionSlider = createClickableSlider();
    m_positionSlider->setRange(0, 0);
    m_positionSlider->setToolTip(tr("Click to seek\nLeft/Right: Seek 10s"));
    m_positionSlider->setFocusPolicy(Qt::ClickFocus);
    
    // Volume slider
    m_volumeSlider = createClickableSlider();
    m_volumeSlider->setRange(0, 200);
    m_volumeSlider->setValue(70);
    m_volumeSlider->setMaximumWidth(100);
    m_volumeSlider->setToolTip(tr("Volume (up to 200%)\nUp/Down: Adjust volume\nMouse Wheel: Adjust volume"));
    m_volumeSlider->setFocusPolicy(Qt::ClickFocus);
    
    // Speed spin box
    m_speedSpinBox = new QDoubleSpinBox(this);
    m_speedSpinBox->setRange(0.1, 5.0);
    m_speedSpinBox->setSingleStep(0.1);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSuffix("x");
    m_speedSpinBox->setDecimals(1);
    m_speedSpinBox->setMaximumWidth(80);
    m_speedSpinBox->setToolTip(tr("Playback Speed"));
    m_speedSpinBox->setFocusPolicy(Qt::NoFocus);
    
    // Labels
    m_positionLabel = new QLabel("00:00", this);
    m_positionLabel->setMinimumWidth(50);
    
    m_durationLabel = new QLabel("00:00", this);
    m_durationLabel->setMinimumWidth(50);
    
    m_volumeLabel = new QLabel(tr("Vol (70%):"), this);
    m_speedLabel = new QLabel(tr("Speed:"), this);
}

void LightweightVideoPlayer::createLayouts()
{
    qDebug() << "LightweightVideoPlayer: Creating layouts";
    
    // Control layout (buttons)
    m_controlLayout = new QHBoxLayout();
    m_controlLayout->addWidget(m_playButton);
    m_controlLayout->addWidget(m_stopButton);
    m_controlLayout->addWidget(m_fullScreenButton);
    m_controlLayout->addWidget(m_keybindsButton);
    m_controlLayout->addWidget(m_editStatesButton);
    m_controlLayout->addStretch();
    
    // Slider layout (position, volume, and speed)
    m_sliderLayout = new QHBoxLayout();
    m_sliderLayout->addWidget(m_positionLabel);
    m_sliderLayout->addWidget(m_positionSlider, 1);
    m_sliderLayout->addWidget(m_durationLabel);
    m_sliderLayout->addSpacing(20);
    m_sliderLayout->addWidget(m_volumeLabel);
    m_sliderLayout->addWidget(m_volumeSlider);
    m_sliderLayout->addSpacing(20);
    m_sliderLayout->addWidget(m_speedLabel);
    m_sliderLayout->addWidget(m_speedSpinBox);
    
    // Controls widget layout
    m_controlsWidget = new QWidget(this);
    m_controlsWidget->setMouseTracking(true);
    m_controlsWidget->installEventFilter(this);
    
    QVBoxLayout* controlsLayout = new QVBoxLayout(m_controlsWidget);
    controlsLayout->addLayout(m_controlLayout);
    controlsLayout->addLayout(m_sliderLayout);
    controlsLayout->setContentsMargins(5, 5, 5, 5);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->addWidget(m_videoWidget, 1);
    m_mainLayout->addWidget(m_controlsWidget);
    
    // Store normal margins for restoration later
    m_normalMargins = m_mainLayout->contentsMargins();
    
    setLayout(m_mainLayout);
}

void LightweightVideoPlayer::connectSignals()
{
    qDebug() << "LightweightVideoPlayer: Connecting signals";
    
    // Button signals
    if (m_playButton) {
        connect(m_playButton, &QPushButton::clicked,
                this, &LightweightVideoPlayer::on_playButton_clicked);
    }
    
    if (m_stopButton) {
        connect(m_stopButton, &QPushButton::clicked,
                this, &LightweightVideoPlayer::stop);
    }
    
    if (m_fullScreenButton) {
        connect(m_fullScreenButton, &QPushButton::clicked,
                this, &LightweightVideoPlayer::on_fullScreenButton_clicked);
    }
    
    if (m_keybindsButton) {
        connect(m_keybindsButton, &QPushButton::clicked,
                this, &LightweightVideoPlayer::openKeybindEditor);
    }
    
    if (m_editStatesButton) {
        connect(m_editStatesButton, &QPushButton::clicked,
                this, &LightweightVideoPlayer::openStatesEditor);
    }
    
    // Slider signals
    if (m_positionSlider) {
        connect(m_positionSlider, &QSlider::sliderMoved,
                this, &LightweightVideoPlayer::on_positionSlider_sliderMoved);
        
        connect(m_positionSlider, &QSlider::sliderPressed,
                this, &LightweightVideoPlayer::on_positionSlider_sliderPressed);
        
        connect(m_positionSlider, &QSlider::sliderReleased,
                this, &LightweightVideoPlayer::on_positionSlider_sliderReleased);
    }
    
    if (m_volumeSlider) {
        connect(m_volumeSlider, &QSlider::sliderMoved,
                this, &LightweightVideoPlayer::on_volumeSlider_sliderMoved);
    }
    
    if (m_speedSpinBox) {
        connect(m_speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &LightweightVideoPlayer::on_speedSpinBox_valueChanged);
    }
    
    // Media player signals
    connect(m_mediaPlayer.get(), &VP_VLCPlayer::positionChanged,
            this, &LightweightVideoPlayer::updatePosition);
    
    connect(m_mediaPlayer.get(), &VP_VLCPlayer::durationChanged,
            this, &LightweightVideoPlayer::updateDuration);
    
    connect(m_mediaPlayer.get(), &VP_VLCPlayer::stateChanged,
            this, &LightweightVideoPlayer::handlePlaybackStateChanged);
    
    connect(m_mediaPlayer.get(), &VP_VLCPlayer::errorOccurred,
            this, &LightweightVideoPlayer::handleError);
    
    connect(m_mediaPlayer.get(), &VP_VLCPlayer::finished,
            this, &LightweightVideoPlayer::handleVideoFinished);
}

bool LightweightVideoPlayer::loadVideo(const QString& filePath)
{
    qDebug() << "LightweightVideoPlayer: Loading video:" << filePath;
    
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        qDebug() << "LightweightVideoPlayer: File does not exist:" << filePath;
        emit errorOccurred(tr("File not found: %1").arg(filePath));
        return false;
    }
    
    // Stop current playback if any
    if (m_mediaPlayer->isPlaying()) {
        m_mediaPlayer->stop();
    }
    
    // Load the media with VLC
    if (!m_mediaPlayer->loadMedia(filePath)) {
        qDebug() << "LightweightVideoPlayer: Failed to load media with VLC";
        emit errorOccurred(tr("Failed to load video: %1").arg(m_mediaPlayer->lastError()));
        return false;
    }
    
    // Store the media path
    m_currentVideoPath = filePath;
    
    // Load saved states for current group from file
    loadStateGroupFromFile(m_currentStateGroup);
    
    // Force video widget to update
    m_videoWidget->update();
    m_videoWidget->show();
    
    // Process events to ensure rendering
    QApplication::processEvents();
    
    // Update window title with filename
    setWindowTitle(tr("%1").arg(fileInfo.fileName()));
    
    // Ensure the widget has focus for keyboard input
    setFocus();
    
    qDebug() << "LightweightVideoPlayer: Video loaded successfully";
    return true;
}

void LightweightVideoPlayer::play()
{
    qDebug() << "LightweightVideoPlayer: Play requested";
    
    if (m_currentVideoPath.isEmpty()) {
        qDebug() << "LightweightVideoPlayer: No video loaded";
        emit errorOccurred(tr("No video loaded"));
        return;
    }
    
    m_mediaPlayer->play();
    setFocus();
}

void LightweightVideoPlayer::pause()
{
    qDebug() << "LightweightVideoPlayer: Pause requested";
    m_mediaPlayer->pause();
}

void LightweightVideoPlayer::stop()
{
    qDebug() << "LightweightVideoPlayer: Stop requested";
    
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
    if (m_positionSlider) {
        m_positionSlider->setValue(0);
    }
    if (m_positionLabel) {
        m_positionLabel->setText("00:00");
    }
}

void LightweightVideoPlayer::setVolume(int volume, bool showMessage)
{
    qDebug() << "LightweightVideoPlayer: Setting volume to" << volume << "%";

    volume = qBound(0, volume, 200);

    if (m_mediaPlayer) {
        m_mediaPlayer->setVolume(volume);
    }

    if (m_volumeLabel) {
        m_volumeLabel->setText(tr("Vol (%1%):").arg(volume));
    }

    if (m_volumeSlider && m_volumeSlider->value() != volume && !m_volumeSlider->isSliderDown()) {
        m_volumeSlider->setValue(volume);
    }

    // Show temporary message if requested (keybind actions only)
    if (showMessage) {
        showTemporaryMessage(tr("Volume: %1%").arg(volume));
    }

    emit volumeChanged(volume);
}

void LightweightVideoPlayer::setPosition(qint64 position)
{
    qDebug() << "LightweightVideoPlayer: Setting position to" << position << "ms";
    
    if (!m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        qDebug() << "LightweightVideoPlayer: No media loaded, cannot set position";
        return;
    }
    
    qint64 duration = m_mediaPlayer->duration();
    if (duration > 0) {
        position = qBound(static_cast<qint64>(0), position, duration);
    }
    
    m_mediaPlayer->setPosition(position);
    
    if (!m_isSliderBeingMoved && m_positionSlider) {
        m_positionSlider->setValue(static_cast<int>(position));
    }
    if (m_positionLabel) {
        m_positionLabel->setText(formatTime(position));
    }
}

void LightweightVideoPlayer::setPlaybackSpeed(qreal speed, bool showMessage)
{
    qDebug() << "LightweightVideoPlayer: Setting playback speed to" << speed;
    
    speed = qBound(0.1, speed, 5.0);
    
    if (m_mediaPlayer) {
        m_mediaPlayer->setPlaybackRate(static_cast<float>(speed));
    }
    
    if (m_speedSpinBox && !qFuzzyCompare(m_speedSpinBox->value(), speed)) {
        m_speedSpinBox->blockSignals(true);
        m_speedSpinBox->setValue(speed);
        m_speedSpinBox->blockSignals(false);
    }
    
    // Show temporary message if requested (keybind actions only)
    if (showMessage) {
        showTemporaryMessage(tr("Speed: %1x").arg(speed, 0, 'f', 1));
    }
    
    emit playbackSpeedChanged(speed);
}

void LightweightVideoPlayer::toggleFullScreen()
{
    if (m_isFullScreen) {
        exitFullScreen();
    } else {
        enterFullScreen();
    }
}

void LightweightVideoPlayer::enterFullScreen()
{
    if (!m_isFullScreen) {
        qDebug() << "LightweightVideoPlayer: Entering fullscreen mode";
        
        // Store normal geometry before going fullscreen
        m_normalGeometry = geometry();
        
        // Show fullscreen
        showFullScreen();
        
        // Remove margins in fullscreen
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        
        // Set fullscreen flag BEFORE starting timers
        m_isFullScreen = true;
        
        // Initialize mouse position to current cursor position
        m_lastMousePos = QCursor::pos();
        qDebug() << "LightweightVideoPlayer: Initialized mouse position to" << m_lastMousePos;
        
        // Start timer to auto-hide controls
        startCursorTimer();
        m_mouseCheckTimer->start();
        
        // Update button icon
        if (m_fullScreenButton) {
            m_fullScreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
            m_fullScreenButton->setToolTip(tr("Exit Full Screen (F11/Esc)"));
        }
        
        emit fullScreenChanged(true);
    }
}

void LightweightVideoPlayer::exitFullScreen()
{
    if (m_isFullScreen) {
        qDebug() << "LightweightVideoPlayer: Exiting fullscreen mode";
        
        // Stop cursor hide timers
        stopCursorTimer();
        m_mouseCheckTimer->stop();
        
        // Reset mouse position tracking
        m_lastMousePos = QPoint(-1, -1);
        
        // Show cursor and controls
        showCursor();
        m_controlsWidget->setVisible(true);
        
        // Restore margins
        m_mainLayout->setContentsMargins(m_normalMargins);
        
        // Exit fullscreen mode
        showNormal();
        
        // Restore geometry
        if (!m_normalGeometry.isEmpty()) {
            setGeometry(m_normalGeometry);
        }
        
        // Ensure window is raised and active
        raise();
        activateWindow();
        
        m_isFullScreen = false;
        
        // Update button icon
        if (m_fullScreenButton) {
            m_fullScreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
            m_fullScreenButton->setToolTip(tr("Full Screen (F11)"));
        }
        
        emit fullScreenChanged(false);
    }
}

// State query functions
bool LightweightVideoPlayer::isPlaying() const
{
    return m_mediaPlayer && m_mediaPlayer->isPlaying();
}

bool LightweightVideoPlayer::isPaused() const
{
    return m_mediaPlayer && m_mediaPlayer->isPaused();
}

qint64 LightweightVideoPlayer::duration() const
{
    return m_mediaPlayer ? m_mediaPlayer->duration() : 0;
}

qint64 LightweightVideoPlayer::position() const
{
    return m_mediaPlayer ? m_mediaPlayer->position() : 0;
}

int LightweightVideoPlayer::volume() const
{
    return m_mediaPlayer ? m_mediaPlayer->volume() : 0;
}

qreal LightweightVideoPlayer::playbackSpeed() const
{
    return m_mediaPlayer ? static_cast<qreal>(m_mediaPlayer->playbackRate()) : 1.0;
}

QString LightweightVideoPlayer::currentVideoPath() const
{
    return m_currentVideoPath;
}

// Slot implementations
void LightweightVideoPlayer::on_playButton_clicked()
{
    qDebug() << "LightweightVideoPlayer: Play button clicked";
    
    if (m_mediaPlayer->isPlaying()) {
        pause();
    } else {
        play();
    }
}

void LightweightVideoPlayer::on_fullScreenButton_clicked()
{
    qDebug() << "LightweightVideoPlayer: Fullscreen button clicked";
    toggleFullScreen();
}

void LightweightVideoPlayer::on_positionSlider_sliderMoved(int position)
{
    qDebug() << "LightweightVideoPlayer: Position slider moved to" << position;
    
    // Save the clicked position for "Return to Last Position" feature
    m_lastClickedPosition = position;
    qDebug() << "LightweightVideoPlayer: Saved last clicked position:" << m_lastClickedPosition;
    
    setPosition(position);
}

void LightweightVideoPlayer::on_positionSlider_sliderPressed()
{
    qDebug() << "LightweightVideoPlayer: Position slider pressed";
    m_isSliderBeingMoved = true;
}

void LightweightVideoPlayer::on_positionSlider_sliderReleased()
{
    qDebug() << "LightweightVideoPlayer: Position slider released";
    m_isSliderBeingMoved = false;
}

void LightweightVideoPlayer::on_volumeSlider_sliderMoved(int position)
{
    qDebug() << "LightweightVideoPlayer: Volume slider moved to" << position << "%";
    setVolume(position);
}

void LightweightVideoPlayer::on_speedSpinBox_valueChanged(double value)
{
    qDebug() << "LightweightVideoPlayer: Speed spin box changed to" << value;
    setPlaybackSpeed(value);
}

void LightweightVideoPlayer::updatePosition(qint64 position)
{
    if (!m_isSliderBeingMoved && m_positionSlider) {
        m_positionSlider->setValue(static_cast<int>(position));
    }
    if (m_positionLabel) {
        m_positionLabel->setText(formatTime(position));
    }
    
    // Check for loop points
    checkLoopPoint();
    
    emit positionChanged(position);
}

void LightweightVideoPlayer::updateDuration(qint64 duration)
{
    qDebug() << "LightweightVideoPlayer: Duration updated to" << duration << "ms";
    
    if (m_positionSlider) {
        m_positionSlider->setMaximum(static_cast<int>(duration));
    }
    if (m_durationLabel) {
        m_durationLabel->setText(formatTime(duration));
    }
    
    emit durationChanged(duration);
}

void LightweightVideoPlayer::handleError(const QString &errorString)
{
    qDebug() << "LightweightVideoPlayer: Error occurred:" << errorString;
    emit errorOccurred(errorString);
}

void LightweightVideoPlayer::handlePlaybackStateChanged(VP_VLCPlayer::PlayerState state)
{
    qDebug() << "LightweightVideoPlayer: Playback state changed to" << static_cast<int>(state);
    
    if (!m_playButton) {
        emit playbackStateChanged(state);
        return;
    }
    
    switch (state) {
        case VP_VLCPlayer::PlayerState::Playing:
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            m_playButton->setToolTip(tr("Pause"));
            
            if (!m_playbackStartedEmitted) {
                m_playbackStartedEmitted = true;
                emit playbackStarted();
            }
            break;
            
        case VP_VLCPlayer::PlayerState::Paused:
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            m_playButton->setToolTip(tr("Play"));
            break;
            
        case VP_VLCPlayer::PlayerState::Stopped:
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            m_playButton->setToolTip(tr("Play"));
            m_playbackStartedEmitted = false;
            break;
            
        default:
            break;
    }
    
    emit playbackStateChanged(state);
}

void LightweightVideoPlayer::handleVideoFinished()
{
    qDebug() << "LightweightVideoPlayer: Video finished";
    
    // Update UI to reflect that we're at the beginning and paused
    if (m_positionSlider) {
        m_positionSlider->setValue(0);
    }
    if (m_positionLabel) {
        m_positionLabel->setText("00:00");
    }
    
    emit finished();
}

// Cursor management
void LightweightVideoPlayer::hideCursor()
{
    if (m_isFullScreen) {
        setCursor(Qt::BlankCursor);
        m_videoWidget->setCursor(Qt::BlankCursor);
        m_controlsWidget->setVisible(false);
        qDebug() << "LightweightVideoPlayer: Cursor and controls hidden";
    }
}

void LightweightVideoPlayer::showCursor()
{
    setCursor(Qt::ArrowCursor);
    m_videoWidget->setCursor(Qt::ArrowCursor);
    qDebug() << "LightweightVideoPlayer: Cursor shown";
}

void LightweightVideoPlayer::checkMouseMovement()
{
    if (!m_isFullScreen) {
        return;
    }
    
    QPoint currentPos = QCursor::pos();
    
    // Check if this is the first check (initialization)
    if (m_lastMousePos == QPoint(-1, -1)) {
        // First time checking - initialize position but don't show controls
        m_lastMousePos = currentPos;
        qDebug() << "LightweightVideoPlayer: Initial mouse position set to" << currentPos;
        return;
    }
    
    // Check if mouse has moved
    if (m_lastMousePos != currentPos) {
        qDebug() << "LightweightVideoPlayer: Mouse movement detected from"
                  << m_lastMousePos << "to" << currentPos;
        
        // Show cursor and controls
        showCursor();
        
        if (!m_controlsWidget->isVisible()) {
            m_controlsWidget->setVisible(true);
        }
        
        // Restart the hide timer
        startCursorTimer();
    }
    
    m_lastMousePos = currentPos;
}

void LightweightVideoPlayer::startCursorTimer()
{
    if (m_isFullScreen && m_cursorTimer) {
        m_cursorTimer->stop();
        m_cursorTimer->start(3000);  // Hide after 3 seconds
    }
}

void LightweightVideoPlayer::stopCursorTimer()
{
    if (m_cursorTimer) {
        m_cursorTimer->stop();
    }
}

// Event handlers
void LightweightVideoPlayer::closeEvent(QCloseEvent *event)
{
    qDebug() << "LightweightVideoPlayer: Close event received";
    
    if (!m_isClosing) {
        m_isClosing = true;
        
        // Stop playback
        if (m_mediaPlayer) {
            m_mediaPlayer->stop();
        }
    }
    
    event->accept();
}

void LightweightVideoPlayer::keyPressEvent(QKeyEvent *event)
{
    // Special keys that are not bindable
    if (event->key() == Qt::Key_F11) {
        toggleFullScreen();
        event->accept();
        return;
    }
    
    if (event->key() == Qt::Key_Escape) {
        if (m_isFullScreen) {
            exitFullScreen();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;
    }
    
    Qt::Key key = static_cast<Qt::Key>(event->key());
    Qt::KeyboardModifiers modifiers = event->modifiers();
    
    // Normalize shifted keys back to their base keys for state detection
    // When Shift is pressed with number keys, Qt reports the shifted character
    Qt::Key normalizedKey = key;
    if (modifiers & Qt::ShiftModifier) {
        switch (key) {
            case Qt::Key_Exclam:       normalizedKey = Qt::Key_1; break;  // !
            case Qt::Key_At:           normalizedKey = Qt::Key_2; break;  // @
            case Qt::Key_NumberSign:   normalizedKey = Qt::Key_3; break;  // #
            case Qt::Key_Dollar:       normalizedKey = Qt::Key_4; break;  // $
            case Qt::Key_Percent:      normalizedKey = Qt::Key_5; break;  // %
            case Qt::Key_AsciiCircum:  normalizedKey = Qt::Key_6; break;  // ^
            case Qt::Key_Ampersand:    normalizedKey = Qt::Key_7; break;  // &
            case Qt::Key_Asterisk:     normalizedKey = Qt::Key_8; break;  // *
            case Qt::Key_ParenLeft:    normalizedKey = Qt::Key_9; break;  // (
            case Qt::Key_ParenRight:   normalizedKey = Qt::Key_0; break;  // )
            case Qt::Key_Underscore:   normalizedKey = Qt::Key_Minus; break;  // _
            case Qt::Key_Plus:         normalizedKey = Qt::Key_Equal; break;  // +
            default: break;
        }
    }
    
    // Handle Ctrl+F1-F4 for saving state groups
    if (modifiers == Qt::ControlModifier) {
        if (key == Qt::Key_F1) {
            saveStateGroup(0);
            event->accept();
            return;
        } else if (key == Qt::Key_F2) {
            saveStateGroup(1);
            event->accept();
            return;
        } else if (key == Qt::Key_F3) {
            saveStateGroup(2);
            event->accept();
            return;
        } else if (key == Qt::Key_F4) {
            saveStateGroup(3);
            event->accept();
            return;
        }
    }
    
    // Handle Alt+F1-F4 for deleting state groups
    if (modifiers == Qt::AltModifier) {
        if (key == Qt::Key_F1) {
            deleteStateGroup(0);
            event->accept();
            return;
        } else if (key == Qt::Key_F2) {
            deleteStateGroup(1);
            event->accept();
            return;
        } else if (key == Qt::Key_F3) {
            deleteStateGroup(2);
            event->accept();
            return;
        } else if (key == Qt::Key_F4) {
            deleteStateGroup(3);
            event->accept();
            return;
        }
    }
    
    // Create QKeySequence from the event
    QKeyCombination combination(modifiers, key);
    QKeySequence keySeq(combination);
    
    // Check if this is a state key without modifiers (from the customizable StateKeys list)
    QKeySequence baseKeySeq(normalizedKey);
    int baseStateIndex = getStateIndexFromKeySequence(baseKeySeq);
    
    // Also check using the direct key mapping for modifier combinations
    int directStateIndex = getStateIndexFromKey(normalizedKey);
    
    // Handle state-related keybinds
    if (directStateIndex >= 0 && modifiers != Qt::NoModifier) {
        // Use direct state index for modifier combinations
        if (modifiers == Qt::ControlModifier) {
            // Ctrl + state key: Save state
            savePlaybackState(directStateIndex);
            event->accept();
            return;
        }
        else if (modifiers == Qt::AltModifier) {
            // Alt + state key: Set loop end
            setLoopEndPosition(directStateIndex);
            event->accept();
            return;
        }
        else if (modifiers == Qt::ShiftModifier) {
            // Shift + state key: Delete state
            deletePlaybackState(directStateIndex);
            event->accept();
            return;
        }
    }
    else if (baseStateIndex >= 0 && modifiers == Qt::NoModifier) {
        // Plain state key without modifiers: Load state
        loadPlaybackState(baseStateIndex);
        event->accept();
        return;
    }
    
    qDebug() << "LightweightVideoPlayer: Key press event - KeySequence:" << keySeq.toString();
    
    // Find if this key is bound to an action
    bool handled = false;
    
    if (m_keybindManager) {
        // Check all actions
        QList<KeybindManager::Action> actions = {
            KeybindManager::Action::PlayPause,
            KeybindManager::Action::Stop,
            KeybindManager::Action::SeekForward,
            KeybindManager::Action::SeekBackward,
            KeybindManager::Action::VolumeUp,
            KeybindManager::Action::VolumeDown,
            KeybindManager::Action::SpeedUp,
            KeybindManager::Action::SpeedDown,
            KeybindManager::Action::ToggleLoadSpeed,
            KeybindManager::Action::CycleLoopMode,
            KeybindManager::Action::ReturnToLastPosition,
            KeybindManager::Action::StateGroup1,
            KeybindManager::Action::StateGroup2,
            KeybindManager::Action::StateGroup3,
            KeybindManager::Action::StateGroup4
        };
        
        for (const auto& action : actions) {
            QList<QKeySequence> keybinds = m_keybindManager->getKeybinds(action);
            
            if (keybinds.contains(keySeq)) {
                // Execute the action
                switch (action) {
                    case KeybindManager::Action::PlayPause:
                        on_playButton_clicked();
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::Stop:
                        stop();
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::SeekForward:
                        if (m_mediaPlayer && m_mediaPlayer->hasMedia()) {
                            qint64 newPos = m_mediaPlayer->position() + 10000;
                            setPosition(newPos);
                            handled = true;
                        }
                        break;
                        
                    case KeybindManager::Action::SeekBackward:
                        if (m_mediaPlayer && m_mediaPlayer->hasMedia()) {
                            qint64 newPos = m_mediaPlayer->position() - 10000;
                            setPosition(newPos);
                            handled = true;
                        }
                        break;
                        
                    case KeybindManager::Action::VolumeUp:
                        setVolume(volume() + 5, true);  // Show message for keybind action
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::VolumeDown:
                        setVolume(volume() - 5, true);  // Show message for keybind action
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::SpeedUp:
                        setPlaybackSpeed(playbackSpeed() + 0.1, true);  // Show message for keybind action
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::SpeedDown:
                        setPlaybackSpeed(playbackSpeed() - 0.1, true);  // Show message for keybind action
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::ToggleLoadSpeed:
                        toggleLoadPlaybackSpeed();
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::CycleLoopMode:
                        cycleLoopMode();
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::ReturnToLastPosition:
                        returnToLastPosition();
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::StateGroup1:
                        switchStateGroup(0);
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::StateGroup2:
                        switchStateGroup(1);
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::StateGroup3:
                        switchStateGroup(2);
                        handled = true;
                        break;
                        
                    case KeybindManager::Action::StateGroup4:
                        switchStateGroup(3);
                        handled = true;
                        break;
                }
                
                break; // Found a matching keybind, stop checking
            }
        }
    }
    
    if (handled) {
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void LightweightVideoPlayer::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    
    if (delta > 0) {
        setVolume(volume() + 5);
    } else if (delta < 0) {
        setVolume(volume() - 5);
    }
    
    event->accept();
}

void LightweightVideoPlayer::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    
    if (m_isFullScreen) {
        // Show cursor and controls
        showCursor();
        
        if (!m_controlsWidget->isVisible()) {
            m_controlsWidget->setVisible(true);
        }
        
        // Restart the hide timer
        startCursorTimer();
    }
}

bool LightweightVideoPlayer::eventFilter(QObject *watched, QEvent *event)
{
    // Handle double-click on video widget for fullscreen toggle
    if (watched == m_videoWidget && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            toggleFullScreen();
            return true;
        }
    }
    
    // Handle mouse movement on controls to show them in fullscreen
    if (m_isFullScreen && (watched == m_controlsWidget || watched == m_videoWidget)) {
        if (event->type() == QEvent::MouseMove) {
            showCursor();
            if (!m_controlsWidget->isVisible()) {
                m_controlsWidget->setVisible(true);
            }
            startCursorTimer();
        }
    }
    
    return QWidget::eventFilter(watched, event);
}

void LightweightVideoPlayer::openKeybindEditor()
{
    qDebug() << "LightweightVideoPlayer: Opening keybind editor";
    
    if (!m_keybindManager) {
        QMessageBox::warning(this, tr("Error"), 
                           tr("Keybind manager is not initialized."));
        return;
    }
    
    KeybindEditorDialog dialog(m_keybindManager.get(), this);
    dialog.exec();
}

void LightweightVideoPlayer::openStatesEditor()
{
    qDebug() << "LightweightVideoPlayer: Opening states editor";
    
    if (m_currentVideoPath.isEmpty()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
                           tr("Please load a video before editing states."));
        return;
    }
    
    // Auto-pause playback when opening the editor
    if (m_mediaPlayer && m_mediaPlayer->isPlaying()) {
        pause();
    }
    
    // NOTE: States are already in RAM (m_playbackStates) - no need to save
    // The current group in RAM is always up-to-date with any Ctrl+1-9 changes
    // The dialog will load directly from this RAM state
    
    StatesEditorDialog dialog(this, this);
    dialog.exec();
}

// State access methods for StatesEditorDialog  
const LightweightVideoPlayer::PlaybackState& LightweightVideoPlayer::getPlaybackState(int stateIndex) const
{
    static PlaybackState emptyState;
    
    if (stateIndex < 0 || stateIndex >= 12) {
        return emptyState;
    }
    
    return m_playbackStates[stateIndex];
}

void LightweightVideoPlayer::setPlaybackState(int stateIndex, const PlaybackState& state)
{
    if (stateIndex < 0 || stateIndex >= 12) {
        return;
    }
    
    m_playbackStates[stateIndex] = state;
}

QPixmap LightweightVideoPlayer::captureFrameAtPosition(qint64 position)
{
    if (!m_mediaPlayer) {
        return QPixmap();
    }
    
    return m_mediaPlayer->captureFrameAtPosition(position);
}

// Helper methods
QString LightweightVideoPlayer::formatTime(qint64 milliseconds) const
{
    if (milliseconds < 0) {
        return "00:00";
    }
    
    int hours = milliseconds / 3600000;
    int minutes = (milliseconds % 3600000) / 60000;
    int seconds = (milliseconds % 60000) / 1000;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

void LightweightVideoPlayer::savePlaybackState(int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= 12) {
        qDebug() << "LightweightVideoPlayer: Invalid state index" << stateIndex;
        return;
    }
    
    if (!m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        qDebug() << "LightweightVideoPlayer: No media loaded, cannot save state";
        return;
    }
    
    qint64 currentPosition = m_mediaPlayer->position();
    qreal currentSpeed = m_mediaPlayer->playbackRate();
    
    // Pause video if playing to ensure clean frame capture
    bool wasPlaying = m_mediaPlayer->isPlaying();
    if (wasPlaying) {
        m_mediaPlayer->pause();
        // Wait for VLC to settle before capturing
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(200);  // 200ms wait
        loop.exec();
    }
    
    // Capture preview image at current position
    QPixmap preview = m_mediaPlayer->captureFrameAtPosition(currentPosition);
    
    // Don't resume playback - let it stay paused
    
    // Save the state with start position and clear any existing end position
    m_playbackStates[stateIndex].startPosition = currentPosition;
    m_playbackStates[stateIndex].playbackSpeed = currentSpeed;
    m_playbackStates[stateIndex].isValid = true;
    m_playbackStates[stateIndex].endPosition = 0;
    m_playbackStates[stateIndex].hasEndPosition = false;
    m_playbackStates[stateIndex].previewImage = preview;
    
    qDebug() << "LightweightVideoPlayer: Saved state" << (stateIndex + 1) 
             << "in group" << (m_currentStateGroup + 1)
             << "- Start Position:" << currentPosition << "ms, Speed:" << currentSpeed << "x"
             << "Preview size:" << preview.size();
    
    // Note: File saving is now manual via Ctrl+F1-F4
    
    showTemporaryMessage(tr("G%1 State %2 Saved").arg(m_currentStateGroup + 1).arg(stateIndex + 1));
}

void LightweightVideoPlayer::loadPlaybackState(int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= 12) {
        qDebug() << "LightweightVideoPlayer: Invalid state index" << stateIndex;
        return;
    }
    
    if (!m_playbackStates[stateIndex].isValid) {
        qDebug() << "LightweightVideoPlayer: State" << (stateIndex + 1) << "does not exist, ignoring";
        return;
    }
    
    if (!m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        qDebug() << "LightweightVideoPlayer: No media loaded, cannot load state";
        return;
    }
    
    const PlaybackState& state = m_playbackStates[stateIndex];
    
    setPosition(state.startPosition);
    
    // Only change playback speed if the option is enabled
    if (m_loadPlaybackSpeed) {
        setPlaybackSpeed(state.playbackSpeed);
    }
    
    // Track current state for looping
    m_currentLoopStateIndex = stateIndex;
    
    qDebug() << "LightweightVideoPlayer: Loaded state" << (stateIndex + 1) 
             << "from group" << (m_currentStateGroup + 1)
             << "- Start Position:" << state.startPosition << "ms";
    if (m_loadPlaybackSpeed) {
        qDebug() << "  Speed:" << state.playbackSpeed << "x";
    }
    
    // No message shown when loading state
    // showTemporaryMessage(tr("State %1 Loaded").arg(stateIndex + 1));
}

void LightweightVideoPlayer::setLoopEndPosition(int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= 12) {
        qDebug() << "LightweightVideoPlayer: Invalid state index" << stateIndex;
        return;
    }
    
    if (!m_playbackStates[stateIndex].isValid) {
        qDebug() << "LightweightVideoPlayer: State" << (stateIndex + 1) << "does not exist, cannot set loop end";
        showTemporaryMessage(tr("State %1 does not exist").arg(stateIndex + 1));
        return;
    }
    
    if (!m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        qDebug() << "LightweightVideoPlayer: No media loaded, cannot set loop end";
        return;
    }
    
    qint64 currentPosition = m_mediaPlayer->position();
    
    // Make sure end position is after start position
    if (currentPosition <= m_playbackStates[stateIndex].startPosition) {
        qDebug() << "LightweightVideoPlayer: End position must be after start position";
        showTemporaryMessage(tr("Loop end must be after start"));
        return;
    }
    
    m_playbackStates[stateIndex].endPosition = currentPosition;
    m_playbackStates[stateIndex].hasEndPosition = true;
    
    qDebug() << "LightweightVideoPlayer: Set loop end for state" << (stateIndex + 1)
             << "in group" << (m_currentStateGroup + 1)
             << "- End Position:" << currentPosition << "ms";
    
    // Note: File saving is now manual via Ctrl+F1-F4
    
    showTemporaryMessage(tr("G%1 State %2 Loop End Set").arg(m_currentStateGroup + 1).arg(stateIndex + 1));
}

void LightweightVideoPlayer::deletePlaybackState(int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= 12) {
        qDebug() << "LightweightVideoPlayer: Invalid state index" << stateIndex;
        return;
    }
    
    // Clear the state completely
    m_playbackStates[stateIndex] = PlaybackState();
    
    qDebug() << "LightweightVideoPlayer: Deleted state" << (stateIndex + 1)
             << "from group" << (m_currentStateGroup + 1);
    
    // Note: File saving is now manual via Ctrl+F1-F4
    
    showTemporaryMessage(tr("G%1 State %2 Deleted").arg(m_currentStateGroup + 1).arg(stateIndex + 1));
}

void LightweightVideoPlayer::toggleLoadPlaybackSpeed()
{
    m_loadPlaybackSpeed = !m_loadPlaybackSpeed;
    
    QString status = m_loadPlaybackSpeed ? tr("ON") : tr("OFF");
    qDebug() << "LightweightVideoPlayer: Load Playback Speed toggled to" << status;
    
    showTemporaryMessage(tr("Load Speed: %1").arg(status));
}

void LightweightVideoPlayer::cycleLoopMode()
{
    switch (m_loopMode) {
        case LoopMode::NoLoop:
            m_loopMode = LoopMode::LoopSingle;
            break;
        case LoopMode::LoopSingle:
            // When entering LoopAll mode, check for valid loops first
            {
                int firstLoopIndex = findFirstValidLoop();
                if (firstLoopIndex >= 0) {
                    // Found at least one valid loop, enter LoopAll mode
                    m_loopMode = LoopMode::LoopAll;
                    qDebug() << "LightweightVideoPlayer: Found valid loop at index" << firstLoopIndex << ", entering Loop All mode";
                    
                    // Only jump to the loop if media is loaded and playing
                    if (m_mediaPlayer && m_mediaPlayer->hasMedia()) {
                        // Temporarily disable loop checking to prevent recursion
                        LoopMode savedMode = m_loopMode;
                        m_loopMode = LoopMode::NoLoop;
                        loadPlaybackState(firstLoopIndex);
                        m_loopMode = savedMode;
                    }
                } else {
                    // No valid loops found, skip to NoLoop instead
                    m_loopMode = LoopMode::NoLoop;
                    m_currentLoopStateIndex = -1;
                    qDebug() << "LightweightVideoPlayer: No valid loops found, skipping to No Loop";
                    showTemporaryMessage(tr("No valid loops - No Loop"));
                }
            }
            break;
        case LoopMode::LoopAll:
            m_loopMode = LoopMode::NoLoop;
            m_currentLoopStateIndex = -1;  // Reset tracking
            break;
    }
    
    QString modeStr = getLoopModeString();
    qDebug() << "LightweightVideoPlayer: Loop mode changed to" << modeStr;
    
    // Only show the mode message if we're not showing the "no valid loops" message
    if (m_loopMode != LoopMode::NoLoop || m_currentLoopStateIndex != -1) {
        showTemporaryMessage(tr("Loop Mode: %1").arg(modeStr));
    }
}

void LightweightVideoPlayer::returnToLastPosition()
{
    // Check if there's a saved position
    if (m_lastClickedPosition < 0) {
        qDebug() << "LightweightVideoPlayer: No last clicked position saved, doing nothing";
        return;
    }
    
    // Check if media is loaded
    if (!m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        qDebug() << "LightweightVideoPlayer: No media loaded, cannot return to last position";
        return;
    }
    
    qDebug() << "LightweightVideoPlayer: Returning to last clicked position:" << m_lastClickedPosition << "ms";
    setPosition(m_lastClickedPosition);
}

void LightweightVideoPlayer::checkLoopPoint()
{
    if (m_loopMode == LoopMode::NoLoop || !m_mediaPlayer || !m_mediaPlayer->hasMedia()) {
        return;
    }
    
    qint64 currentPosition = m_mediaPlayer->position();
    const int tolerance = 200;  // 200ms tolerance
    
    if (m_loopMode == LoopMode::LoopSingle) {
        // Check if current loaded state has reached its end point
        if (m_currentLoopStateIndex >= 0 && m_currentLoopStateIndex < 12) {
            const PlaybackState& state = m_playbackStates[m_currentLoopStateIndex];
            
            if (state.isValid && state.hasEndPosition) {
                // Check if we've reached or passed the end position
                if (currentPosition >= state.endPosition - tolerance) {
                    qDebug() << "LightweightVideoPlayer: Loop point reached for state" << (m_currentLoopStateIndex + 1);
                    setPosition(state.startPosition);
                }
            }
        }
    }
    else if (m_loopMode == LoopMode::LoopAll) {
        // If no state is currently active, find the first loopable state
        if (m_currentLoopStateIndex < 0 || m_currentLoopStateIndex >= 12) {
            for (int i = 0; i < 12; i++) {
                if (m_playbackStates[i].isValid && m_playbackStates[i].hasEndPosition) {
                    qDebug() << "LightweightVideoPlayer: Starting LoopAll with state" << (i + 1);
                    loadPlaybackState(i);
                    return;
                }
            }
            // No loopable states found, disable loop all
            qDebug() << "LightweightVideoPlayer: No loopable states found, disabling LoopAll";
            m_loopMode = LoopMode::NoLoop;
            return;
        }
        
        // Check if current state has reached its end, then move to next loopable state
        const PlaybackState& currentState = m_playbackStates[m_currentLoopStateIndex];
        
        if (currentState.isValid && currentState.hasEndPosition) {
            if (currentPosition >= currentState.endPosition - tolerance) {
                // Find next valid loopable state
                int nextStateIndex = (m_currentLoopStateIndex + 1) % 12;
                int searchCount = 0;
                
                while (searchCount < 12) {
                    if (m_playbackStates[nextStateIndex].isValid && 
                        m_playbackStates[nextStateIndex].hasEndPosition) {
                        // Found next loopable state
                        qDebug() << "LightweightVideoPlayer: Moving to next loop state" << (nextStateIndex + 1);
                        
                        // Temporarily disable loop checking to prevent recursion
                        LoopMode savedMode = m_loopMode;
                        m_loopMode = LoopMode::NoLoop;
                        loadPlaybackState(nextStateIndex);
                        m_loopMode = savedMode;
                        return;
                    }
                    
                    nextStateIndex = (nextStateIndex + 1) % 12;
                    searchCount++;
                }
                
                // No more loopable states found, go back to first loopable state
                for (int i = 0; i < 12; i++) {
                    if (m_playbackStates[i].isValid && m_playbackStates[i].hasEndPosition) {
                        qDebug() << "LightweightVideoPlayer: Looping back to first state" << (i + 1);
                        
                        // Temporarily disable loop checking to prevent recursion
                        LoopMode savedMode = m_loopMode;
                        m_loopMode = LoopMode::NoLoop;
                        loadPlaybackState(i);
                        m_loopMode = savedMode;
                        return;
                    }
                }
            }
        }
    }
}

int LightweightVideoPlayer::getStateIndexFromKey(Qt::Key key) const
{
    // Map keys to state indices (1-9, 0, -, =)
    switch (key) {
        case Qt::Key_1: return 0;
        case Qt::Key_2: return 1;
        case Qt::Key_3: return 2;
        case Qt::Key_4: return 3;
        case Qt::Key_5: return 4;
        case Qt::Key_6: return 5;
        case Qt::Key_7: return 6;
        case Qt::Key_8: return 7;
        case Qt::Key_9: return 8;
        case Qt::Key_0: return 9;
        case Qt::Key_Minus: return 10;
        case Qt::Key_Equal: return 11;
        default: return -1;  // Not a state key
    }
}

int LightweightVideoPlayer::getStateIndexFromKeySequence(const QKeySequence& keySeq) const
{
    if (!m_keybindManager || keySeq.isEmpty()) {
        return -1;
    }
    
    // Get the state keys from the keybind manager
    QList<QKeySequence> stateKeys = m_keybindManager->getKeybinds(KeybindManager::Action::StateKeys);
    
    // Find which index this key sequence corresponds to
    for (int i = 0; i < stateKeys.size() && i < 12; i++) {
        if (stateKeys[i] == keySeq) {
            return i;
        }
    }
    
    return -1;  // Not a state key
}

QString LightweightVideoPlayer::getLoopModeString() const
{
    switch (m_loopMode) {
        case LoopMode::NoLoop:
            return tr("No Loop");
        case LoopMode::LoopSingle:
            return tr("Loop Single");
        case LoopMode::LoopAll:
            return tr("Loop All");
        default:
            return tr("Unknown");
    }
}

void LightweightVideoPlayer::showTemporaryMessage(const QString& message)
{
    if (m_messageLabel) {
        m_messageLabel->showMessage(message, 2000);
    }
}

void LightweightVideoPlayer::switchStateGroup(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4) {
        qDebug() << "LightweightVideoPlayer: Invalid state group index" << groupIndex;
        return;
    }
    
    m_currentStateGroup = groupIndex;
    m_currentLoopStateIndex = -1;  // Reset loop tracking when switching groups
    m_loopMode = LoopMode::NoLoop;  // Disable looping when switching groups
    
    // Load the group from file
    loadStateGroupFromFile(groupIndex);
    
    qDebug() << "LightweightVideoPlayer: Switched to state group" << (groupIndex + 1) << "- Looping disabled";
    showTemporaryMessage(tr("State Group %1").arg(groupIndex + 1));
}

// Load a specific state group from its file
bool LightweightVideoPlayer::loadStateGroupFromFile(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4) {
        qDebug() << "LightweightVideoPlayer: Invalid group index" << groupIndex;
        return false;
    }
    
    if (m_currentVideoPath.isEmpty()) {
        qDebug() << "LightweightVideoPlayer: No video loaded, cannot load states";
        return false;
    }
    
    // Clear current states first
    for (int i = 0; i < 12; i++) {
        m_playbackStates[i] = PlaybackState();
    }
    
    QString filePath = getStatesFilePath(groupIndex);
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "LightweightVideoPlayer: No state file exists for group" << (groupIndex + 1) << "- group is empty";
        return false;  // No file means empty group
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "LightweightVideoPlayer: Failed to open states file for reading:" << filePath;
        return false;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    int statesLoaded = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        // Parse line: StateIndex,StartPos,EndPos,Speed,Valid,HasEnd,ImageData
        QStringList parts = line.split(",");
        if (parts.size() < 6) {
            qDebug() << "LightweightVideoPlayer: Invalid line format in states file:" << line;
            continue;
        }
        
        bool ok = true;
        int stateIndex = parts[0].toInt(&ok);
        if (!ok || stateIndex < 0 || stateIndex >= 12) {
            qDebug() << "LightweightVideoPlayer: Invalid state index:" << parts[0];
            continue;
        }
        
        qint64 startPos = parts[1].toLongLong(&ok);
        if (!ok) {
            qDebug() << "LightweightVideoPlayer: Invalid start position:" << parts[1];
            continue;
        }
        
        qint64 endPos = parts[2].toLongLong(&ok);
        if (!ok) {
            qDebug() << "LightweightVideoPlayer: Invalid end position:" << parts[2];
            continue;
        }
        
        qreal speed = parts[3].toDouble(&ok);
        if (!ok) {
            qDebug() << "LightweightVideoPlayer: Invalid speed:" << parts[3];
            continue;
        }
        
        bool isValid = (parts[4] == "1");
        bool hasEnd = (parts[5] == "1");
        
        // Load preview image if available (v2.0 format)
        QPixmap previewImage;
        if (parts.size() > 6 && !parts[6].isEmpty()) {
            QByteArray imageData = QByteArray::fromBase64(parts[6].toUtf8());
            previewImage.loadFromData(imageData, "PNG");
        }
        
        // Load the state into current group's memory
        m_playbackStates[stateIndex].startPosition = startPos;
        m_playbackStates[stateIndex].endPosition = endPos;
        m_playbackStates[stateIndex].playbackSpeed = speed;
        m_playbackStates[stateIndex].isValid = isValid;
        m_playbackStates[stateIndex].hasEndPosition = hasEnd;
        m_playbackStates[stateIndex].previewImage = previewImage;
        
        statesLoaded++;
    }
    
    file.close();
    qDebug() << "LightweightVideoPlayer: Loaded" << statesLoaded << "states from group" << (groupIndex + 1);
    return true;
}



QString LightweightVideoPlayer::getStatesFilePath(int groupIndex) const
{
    if (m_currentVideoPath.isEmpty()) {
        return QString();
    }
    
    QFileInfo fileInfo(m_currentVideoPath);
    QString basePath = fileInfo.absolutePath() + "/0state_" + fileInfo.completeBaseName();
    
    return basePath + QString(".statesG%1").arg(groupIndex + 1);
}

int LightweightVideoPlayer::findFirstValidLoop() const
{
    // Search for the first state that has both isValid and hasEndPosition set
    for (int i = 0; i < 12; i++) {
        const PlaybackState& state = m_playbackStates[i];
        if (state.isValid && state.hasEndPosition) {
            return i;
        }
    }
    
    // No valid loop found
    return -1;
}

void LightweightVideoPlayer::saveStateGroup(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4) {
        qDebug() << "LightweightVideoPlayer: Invalid state group index" << groupIndex;
        return;
    }
    
    if (m_currentVideoPath.isEmpty()) {
        qDebug() << "LightweightVideoPlayer: No video loaded, cannot save state group";
        return;
    }
    
    // Only save if this is the current group (since we only have current group in memory)
    if (groupIndex != m_currentStateGroup) {
        qDebug() << "LightweightVideoPlayer: Can only save current group (" << (m_currentStateGroup + 1) << "), not group" << (groupIndex + 1);
        showTemporaryMessage(tr("Switch to Group %1 first to save it").arg(groupIndex + 1));
        return;
    }
    
    // Save the current group to file
    QString filePath = getStatesFilePath(groupIndex);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "LightweightVideoPlayer: Failed to open states file for writing:" << filePath;
        showTemporaryMessage(tr("Failed to save Group %1").arg(groupIndex + 1));
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // Write header
    out << "# Video Player State Group File v2.0\n";
    out << "# Format: StateIndex,StartPos,EndPos,Speed,Valid,HasEnd,ImageData\n";
    out << "\n";
    
    // Write each state for current group
    for (int i = 0; i < 12; i++) {
        const PlaybackState& state = m_playbackStates[i];
        
        out << i << ","
            << state.startPosition << ","
            << state.endPosition << ","
            << state.playbackSpeed << ","
            << (state.isValid ? "1" : "0") << ","
            << (state.hasEndPosition ? "1" : "0") << ",";
        
        // Encode preview image as Base64 if available
        if (!state.previewImage.isNull()) {
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            state.previewImage.save(&buffer, "PNG");
            
            QString base64 = imageData.toBase64();
            out << base64;
        }
        
        out << "\n";
    }
    
    file.close();
    qDebug() << "LightweightVideoPlayer: Saved state group" << (groupIndex + 1) << "to" << filePath;
    showTemporaryMessage(tr("Group %1 Saved").arg(groupIndex + 1));
}

void LightweightVideoPlayer::deleteStateGroup(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4) {
        qDebug() << "LightweightVideoPlayer: Invalid state group index" << groupIndex;
        return;
    }
    
    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Delete State Group"),
        tr("Are you sure you want to delete all states in Group %1?").arg(groupIndex + 1),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        qDebug() << "LightweightVideoPlayer: User cancelled deletion of group" << (groupIndex + 1);
        return;
    }
    
    // Only clear if this is the current group (since we only have current group in memory)
    if (groupIndex == m_currentStateGroup) {
        // Clear all states in memory for current group
        for (int i = 0; i < 12; i++) {
            m_playbackStates[i] = PlaybackState();
        }
    }
    
    // Delete the file from disk if it exists
    if (!m_currentVideoPath.isEmpty()) {
        QString filePath = getStatesFilePath(groupIndex);
        QFile file(filePath);
        
        if (file.exists()) {
            if (file.remove()) {
                qDebug() << "LightweightVideoPlayer: Deleted state group file:" << filePath;
            } else {
                qDebug() << "LightweightVideoPlayer: Failed to delete state group file:" << filePath;
            }
        }
    }
    
    qDebug() << "LightweightVideoPlayer: Deleted state group" << (groupIndex + 1);
    showTemporaryMessage(tr("Group %1 Deleted").arg(groupIndex + 1));
}

// TemporaryMessageLabel implementation
TemporaryMessageLabel::TemporaryMessageLabel(QWidget* parent)
    : QLabel(parent)
    , m_fadeTimer(new QTimer(this))
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet(
        "QLabel {"
        "    background-color: rgba(0, 0, 0, 180);"
        "    color: white;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    padding: 20px;"
        "    border-radius: 10px;"
        "}"
    );
    
    m_fadeTimer->setSingleShot(true);
    connect(m_fadeTimer, &QTimer::timeout, this, &TemporaryMessageLabel::hide);
}

void TemporaryMessageLabel::showMessage(const QString& message, int durationMs)
{
    setText(message);
    adjustSize();
    
    // Center the label in parent
    if (parentWidget()) {
        QRect parentRect = parentWidget()->rect();
        move((parentRect.width() - width()) / 2, (parentRect.height() - height()) / 2);
    }
    
    setVisible(true);
    raise();  // Bring to front
    
    m_fadeTimer->start(durationMs);
}
