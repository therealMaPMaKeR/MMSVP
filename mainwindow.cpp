#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "videoplayer.h"
#include <QFileDialog>
#include <QDebug>

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
    
    // Connect signals
    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(ui->playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(ui->seekSlider, &QSlider::sliderPressed, this, &MainWindow::onSeekSliderPressed);
    connect(ui->seekSlider, &QSlider::sliderReleased, this, &MainWindow::onSeekSliderReleased);
    connect(ui->seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekSliderMoved);
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
    m_seeking = false;
    float position = ui->seekSlider->value() / 1000.0f;
    m_videoPlayer->setPosition(position);
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
