#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "videoplayer.h"
#include <QFileDialog>
#include <QDebug>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_seeking(false)
{
    ui->setupUi(this);
    
    // Create video player widget
    m_videoPlayer = new VideoPlayer(this);
    ui->videoLayout->addWidget(m_videoPlayer);
    
    // Set initial volume
    ui->volumeSlider->setValue(50);
    m_videoPlayer->setVolume(50);
    
    // Make seek slider jump to click position instead of stepping
    ui->seekSlider->setPageStep(0);
    ui->seekSlider->installEventFilter(this);
    
    // Connect signals
    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(ui->playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(ui->seekSlider, &QSlider::sliderPressed, this, &MainWindow::onSeekSliderPressed);
    connect(ui->seekSlider, &QSlider::sliderReleased, this, &MainWindow::onSeekSliderReleased);
    connect(ui->seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekSliderMoved);
    connect(ui->seekSlider, &QSlider::valueChanged, this, [this](int value) {
        // Handle direct clicks on the slider bar (not dragging)
        if (!m_seeking) {
            float position = value / 1000.0f;
            m_videoPlayer->setPosition(position);
        }
    });
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    
    // Setup update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    m_updateTimer->start(100); // Update every 100ms
    
    setWindowTitle("Simple Video Player");
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        tr("Open Video File"), 
        QString(),
        tr("Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        m_videoPlayer->playMedia(fileName);
        ui->playPauseButton->setText("Pause");
    }
}

void MainWindow::onPlayPause()
{
    if (m_videoPlayer->isPlaying()) {
        m_videoPlayer->pause();
        ui->playPauseButton->setText("Play");
    } else {
        m_videoPlayer->play();
        ui->playPauseButton->setText("Pause");
    }
}

void MainWindow::onStop()
{
    m_videoPlayer->stop();
    ui->playPauseButton->setText("Play");
    ui->seekSlider->setValue(0);
    ui->timeLabel->setText("00:00 / 00:00");
}

void MainWindow::onSeekSliderPressed()
{
    m_seeking = true;
}

void MainWindow::onSeekSliderReleased()
{
    float position = ui->seekSlider->value() / 1000.0f;
    m_videoPlayer->setPosition(position);
    m_seeking = false;
}

void MainWindow::onSeekSliderMoved(int position)
{
    if (m_seeking) {
        qint64 length = m_videoPlayer->getLength();
        qint64 time = (length * position) / 1000;
        ui->timeLabel->setText(formatTime(time) + " / " + formatTime(length));
    }
}

void MainWindow::onVolumeChanged(int volume)
{
    m_videoPlayer->setVolume(volume);
}

void MainWindow::updateUI()
{
    if (!m_seeking) {
        float position = m_videoPlayer->getPosition();
        ui->seekSlider->setValue(static_cast<int>(position * 1000));
        
        qint64 time = m_videoPlayer->getTime();
        qint64 length = m_videoPlayer->getLength();
        ui->timeLabel->setText(formatTime(time) + " / " + formatTime(length));
    }
    
    // Update play/pause button text
    if (m_videoPlayer->isPlaying()) {
        ui->playPauseButton->setText("Pause");
    } else {
        ui->playPauseButton->setText("Play");
    }
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    int seconds = (milliseconds / 1000) % 60;
    int minutes = (milliseconds / 60000) % 60;
    int hours = (milliseconds / 3600000);
    
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

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->seekSlider && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        
        // Get the slider's style to calculate handle position
        QStyleOptionSlider opt;
        opt.initFrom(ui->seekSlider);
        opt.minimum = ui->seekSlider->minimum();
        opt.maximum = ui->seekSlider->maximum();
        opt.sliderPosition = ui->seekSlider->value();
        opt.sliderValue = ui->seekSlider->value();
        opt.orientation = ui->seekSlider->orientation();
        
        // Get the handle rectangle
        QRect handleRect = ui->seekSlider->style()->subControlRect(
            QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, ui->seekSlider);
        
        // If click is on the handle, let Qt handle it (for dragging)
        if (handleRect.contains(mouseEvent->pos())) {
            return QMainWindow::eventFilter(obj, event);
        }
        
        // Otherwise, calculate the clicked position and jump to it
        int sliderMin = ui->seekSlider->minimum();
        int sliderMax = ui->seekSlider->maximum();
        int sliderLength = ui->seekSlider->width();
        
        double clickPosition = mouseEvent->pos().x();
        int newValue = sliderMin + ((sliderMax - sliderMin) * clickPosition) / sliderLength;
        
        // Clamp the value to valid range
        if (newValue < sliderMin) newValue = sliderMin;
        if (newValue > sliderMax) newValue = sliderMax;
        
        // Set the slider value (this will trigger valueChanged)
        ui->seekSlider->setValue(newValue);
        
        // Create a new mouse event at the handle position to enable dragging
        // Calculate where the handle is now after moving
        opt.sliderPosition = newValue;
        opt.sliderValue = newValue;
        QRect newHandleRect = ui->seekSlider->style()->subControlRect(
            QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, ui->seekSlider);
        
        // Create a synthetic mouse press event on the handle center
        QPoint handleCenter = newHandleRect.center();
        QMouseEvent *syntheticEvent = new QMouseEvent(
            QEvent::MouseButtonPress,
            handleCenter,
            mouseEvent->globalPosition(),
            mouseEvent->button(),
            mouseEvent->buttons(),
            mouseEvent->modifiers()
        );
        
        // Send the synthetic event to the slider to start dragging
        QApplication::sendEvent(ui->seekSlider, syntheticEvent);
        delete syntheticEvent;
        
        // Return true to indicate we handled the original event
        return true;
    }
    
    // Pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}
