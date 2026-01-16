#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class VideoPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onOpenFile();
    void onPlayPause();
    void onStop();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onSeekSliderMoved(int position);
    void onVolumeChanged(int volume);
    void updateUI();

private:
    Ui::MainWindow *ui;
    VideoPlayer *m_videoPlayer;
    QTimer *m_updateTimer;
    bool m_seeking;

    QString formatTime(qint64 milliseconds);
};

#endif // MAINWINDOW_H
